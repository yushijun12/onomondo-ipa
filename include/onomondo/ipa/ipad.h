/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file ipad.h
 * @brief IPAd (IoT Profile Assistant) 主 API 接口
 * 
 * 本文件定义了 IPAd 的核心接口，包括上下文管理、配置结构和主要操作流程。
 * IPAd 是 GSMA SGP.32 兼容的 IoT eSIM 配置助手，用于管理 eUICC 上的配置文件。
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/** @defgroup IPAd IPAd 核心模块
 *  @{
 */

/*! @brief FQDN 最大长度（字节） */
#define IPA_LEN_FQDN 255
/*! @brief TAC（终端类型分配码）长度（字节） */
#define IPA_LEN_TAC 4
/*! @brief 允许的 CA 证书数量 */
#define IPA_LEN_ALLOWED_CA 20
/*! @brief EIM ID 最大长度（字节） */
#define IPE_LEN_EIM_ID 256

struct ipa_context;
struct ipa_buf;

/**
 * @brief 配置文件安装同意回调函数（已弃用）
 * 
 * @deprecated 参见 GitHub issue #5，此回调已不再推荐使用
 * @param sm_dp_plus_address SM-DP+ 服务器地址
 * @param ac_token 认证令牌
 * @return true 同意安装，false 拒绝安装
 */
/* (deprecated, see github issue #5) */
typedef bool (*ipa_prfle_inst_consent_cb)(char *sm_dp_plus_address, char *ac_token);

/**
 * @brief IPAd 轮询函数返回值
 * 
 * 定义 ipa_poll() 函数的可能返回值，指示调用者下一步操作。
 */
enum ipa_poll_rc {
	/*! 立即再次调用 ipa_poll()（可能还有 eIM 包待执行） */
	IPA_POLL_AGAIN = 0,

	/*! 可以稍后调用 ipa_poll()（没有更多 eIM 包） */
	IPA_POLL_AGAIN_LATER = 1,

	/*! eIM 包包含可能导致 IP 连接暂时丢失的操作（如切换 eUICC 配置文件），
	 *  在 IP 连接恢复后立即调用 ipa_poll() */
	IPA_POLL_AGAIN_WHEN_ONLINE = 2,

	/*! 无法与 eUICC 通信，在 eUICC 连接恢复后调用 ipa_poll() */
	IPA_POLL_CHECK_SCARD = -1000,

	/*! 无法与 eIM 通信，在 eIM 连接恢复后调用 ipa_poll() */
	IPA_POLL_CHECK_HTTP = -2000,
};

/**
 * @brief IPAd 配置结构
 * 
 * 包含初始化 IPAd 上下文所需的所有配置参数。
 * 注意：某些成员（如 tac）可在运行时更新。
 */
struct ipa_config {
	/*! 首选 eIM ID（可选）。设为 NULL 时使用 EimConfigurationData 列表中的第一个 eIM 配置项 */
	char *preferred_eim_id;

	/*! 当前 TAC（终端类型分配码）。此成员可在上下文创建后随时更新 */
	uint8_t tac[IPA_LEN_TAC];

	/*! CA 证书束文件路径。该字符串在初始化时传递给 ipa_http_init() */
	const char *eim_cabundle;

	/*! 在测试环境中禁用 SSL 以简化调试 */
	bool eim_disable_ssl;

	/*! 在测试环境中禁用 SSL 证书验证以简化调试 */
	bool eim_disable_ssl_verif;

	/*! eIM 请求失败时的重试次数 */
	unsigned int esipa_req_retries;

	/*! 配置文件回滚时的刷新标志。参见 SGP.32 第 5.9.16 节。
	 *  启用 IoT eUICC 仿真时，此标志也用于配置文件的启用/禁用操作。
	 *  参见 SGP.22 第 5.7.16 和 5.7.17 节 */
	bool refresh_flag;

	/*! 接口 eUICC 的读卡器 ID 号 */
	unsigned int reader_num;

	/*! 用于与 ISD-R 通信的逻辑通道号 */
	uint8_t euicc_channel;

	/*! 启用 IoT eUICC 仿真模式。
	 *  IPAd 支持使用消费级 eUICC，其接口略有不同。启用此模式后，IPAd 将在 ES10x 函数级别
	 *  适配接口，使消费级 eUICC 在流程级别表现为 IoT eUICC */
	bool iot_euicc_emu_enabled;

	/*! 配置文件安装同意回调（已弃用）。
	 *  @deprecated 参见 GitHub issue #5
	 *  SGP.32 要求提示用户同意配置文件安装。API 用户可在此传递回调函数处理同意请求。
	 *  如不提供回调函数，onomondo-eim 将自动同意所有配置文件安装 */
	ipa_prfle_inst_consent_cb prfle_inst_consent_cb;
};

/**
 * @brief 创建新的 IPAd 上下文
 * 
 * 分配并初始化一个新的 IPAd 上下文结构。如果提供了非易失状态数据，
 * 则从中恢复之前的状态。
 * 
 * @param cfg 配置结构指针（不能为 NULL）
 * @param nvstate 非易失状态缓冲区（可选，可为 NULL）
 * @return 新创建的上下文指针，失败时返回 NULL
 */
struct ipa_context *ipa_new_ctx(struct ipa_config *cfg, struct ipa_buf *nvstate);

/**
 * @brief 初始化 IPAd 上下文
 * 
 * 执行上下文初始化，包括读取 eUICC 信息（EID 等）和建立初始连接。
 * 必须在调用 ipa_poll() 之前调用此函数。
 * 
 * @param ctx IPAd 上下文指针（由 ipa_new_ctx() 创建）
 * @return 0 成功，负值表示错误代码
 */
int ipa_init(struct ipa_context *ctx);

/**
 * @brief 初始化 eIM 连接
 * 
 * 建立与 eIM（eSIM IoT Manager）的连接并获取配置信息。
 * 
 * @param ctx IPAd 上下文指针
 * @return 0 成功，负值表示错误代码
 */
int eim_init(struct ipa_context *ctx);

/**
 * @brief 添加初始 eIM 配置
 * 
 * 将 eIM 配置数据添加到上下文中。
 * 
 * @param ctx IPAd 上下文指针
 * @param cfg 包含 eIM 配置数据的缓冲区
 * @return 0 成功，负值表示错误代码
 */
int ipa_add_init_eim_cfg(struct ipa_context *ctx, struct ipa_buf *cfg);

/**
 * @brief 执行 eUICC 内存重置
 * 
 * 重置 eUICC 的内存内容，可选择性地保留或删除特定类型的数据。
 * 
 * @param ctx IPAd 上下文指针
 * @param operatnl_profiles 是否重置运营配置文件
 * @param test_profiles 是否重置测试配置文件
 * @param default_smdp_addr 是否重置默认 SM-DP+ 地址
 * @param eim_cfg_data 是否重置 eIM 配置数据
 * @param auto_enable_cfg 是否重置自动启用配置
 * @return 0 成功，负值表示错误代码
 */
int ipa_euicc_mem_rst(struct ipa_context *ctx, bool operatnl_profiles, bool test_profiles, bool default_smdp_addr,
		      bool eim_cfg_data, bool auto_enable_cfg);

/**
 * @brief 主轮询函数
 * 
 * 处理待处理的 eIM 包和执行流程。调用者应定期调用此函数以维持
 * IPAd 的正常运作。根据返回值决定下次调用的时机。
 * 
 * @param ctx IPAd 上下文指针
 * @return @ref ipa_poll_rc 枚举值，指示下一步操作
 */
int ipa_poll(struct ipa_context *ctx);

/**
 * @brief 关闭 IPAd 上下文
 * 
 * 关闭所有活动连接并释放资源，但不释放上下文结构本身。
 * 可用于临时断开连接后重新初始化。
 * 
 * @param ctx IPAd 上下文指针
 */
void ipa_close(struct ipa_context *ctx);

/**
 * @brief 释放 IPAd 上下文
 * 
 * 完全释放 IPAd 上下文及其所有资源。在释放前会保存非易失状态。
 * 
 * @param ctx IPAd 上下文指针
 * @return 包含非易失状态的缓冲区（需调用者负责保存和释放），失败时返回 NULL
 */
struct ipa_buf *ipa_free_ctx(struct ipa_context *ctx);

/** @} */  /* 结束 IPAd 模块组 */
