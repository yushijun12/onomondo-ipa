/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Author: IPAd Development Team
 *
 * See also: GSMA SGP.32, section 3.3: EIM Configuration Management
 */

/**
 * @file eim_config.h
 * @brief eIM 配置管理接口
 *
 * 本文件定义了 eIM 配置管理的完整接口，包括：
 * - 动态 EIM 配置更新
 * - 多 eIM 管理
 * - EIM 能力协商
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup EIM_Config eIM 配置管理模块
 *  @{
 */

/*! @brief 最大支持的 eIM 数量 */
#define IPA_MAX_EIM_COUNT 10

/*! @brief EIM ID 最大长度 */
#define IPA_EIM_ID_MAX_LEN 256

/*! @brief EIM URL 最大长度 */
#define IPA_EIM_URL_MAX_LEN 512

/**
 * @brief EIM 能力标志
 *
 * 表示 eIM 支持的功能特性。
 */
enum ipa_eim_capabilities {
    IPA_EIM_CAP_PACKAGE_RETRIEVAL = (1 << 0),   /*!< 支持 Package Retrieval */
    IPA_EIM_CAP_PACKAGE_INJECTION = (1 << 1),   /*!< 支持 Package Injection */
    IPA_EIM_CAP_NOTIFICATION_HANDLING = (1 << 2), /*!< 支持通知处理 */
    IPA_EIM_CAP_COAP_SUPPORT = (1 << 3),        /*!< 支持 CoAP 传输 */
    IPA_EIM_CAP_HTTPS_ONLY = (1 << 4),          /*!< 仅支持 HTTPS */
    IPA_EIM_CAP_BATCH_OPERATIONS = (1 << 5),    /*!< 支持批量操作 */
};

/**
 * @brief EIM 配置项结构
 *
 * 描述单个 eIM 的配置信息。
 */
struct ipa_eim_config {
    /*! EIM 唯一标识符 */
    char eim_id[IPA_EIM_ID_MAX_LEN];
    
    /*! EIM 服务器 URL（HTTPS 或 CoAPS） */
    char eim_url[IPA_EIM_URL_MAX_LEN];
    
    /*! EIM 能力标志（位掩码） */
    uint32_t capabilities;
    
    /*! CA 证书束路径（可选） */
    const char *ca_bundle;
    
    /*! 是否禁用 SSL 验证（仅用于测试） */
    bool disable_ssl_verify;
    
    /*! 连接超时时间（秒） */
    unsigned int timeout_sec;
    
    /*! 重试次数 */
    unsigned int retry_count;
    
    /*! 优先级（数字越小优先级越高） */
    unsigned int priority;
    
    /*! 是否启用此 eIM */
    bool enabled;
};

/**
 * @brief 多 eIM 管理器结构
 *
 * 管理多个 eIM 配置和状态。
 */
struct ipa_eim_manager {
    /*! eIM 配置数组 */
    struct ipa_eim_config eims[IPA_MAX_EIM_COUNT];
    
    /*! 当前 eIM 数量 */
    size_t eim_count;
    
    /*! 当前活动的 eIM 索引 */
    size_t active_eim_index;
    
    /*! 首选 eIM ID（可选） */
    char preferred_eim_id[IPA_EIM_ID_MAX_LEN];
};

/**
 * @brief EIM 能力协商结果
 */
struct ipa_eim_capability_result {
    /*! 协商是否成功 */
    bool success;
    
    /*! eIM 报告的能力标志 */
    uint32_t eim_capabilities;
    
    /*! IPA 支持的能力标志 */
    uint32_t ipa_capabilities;
    
    /*! 协商后的共同能力标志 */
    uint32_t negotiated_capabilities;
    
    /*! eIM 固件版本（可选） */
    const char *eim_firmware_version;
    
    /*! 支持的最大包大小（字节） */
    size_t max_package_size;
};

/**
 * @brief 初始化 eIM 管理器
 *
 * @param mgr eIM 管理器结构（必须已分配）
 * @return 0 成功，负值表示错误
 */
int ipa_eim_manager_init(struct ipa_eim_manager *mgr);

/**
 * @brief 添加 eIM 配置
 *
 * @param mgr eIM 管理器
 * @param config eIM 配置
 * @return 0 成功，负值表示错误（如已满）
 */
int ipa_eim_manager_add(struct ipa_eim_manager *mgr, const struct ipa_eim_config *config);

/**
 * @brief 移除 eIM 配置
 *
 * @param mgr eIM 管理器
 * @param eim_id 要移除的 eIM ID
 * @return 0 成功，负值表示未找到
 */
int ipa_eim_manager_remove(struct ipa_eim_manager *mgr, const char *eim_id);

/**
 * @brief 更新 eIM 配置
 *
 * @param mgr eIM 管理器
 * @param eim_id eIM ID
 * @param config 新配置
 * @return 0 成功，负值表示未找到
 */
int ipa_eim_manager_update(struct ipa_eim_manager *mgr, const char *eim_id, 
                           const struct ipa_eim_config *config);

/**
 * @brief 获取 eIM 配置
 *
 * @param mgr eIM 管理器
 * @param eim_id eIM ID
 * @return eIM 配置指针，NULL 表示未找到
 */
const struct ipa_eim_config *ipa_eim_manager_get(const struct ipa_eim_manager *mgr, 
                                                  const char *eim_id);

/**
 * @brief 设置活动 eIM
 *
 * @param mgr eIM 管理器
 * @param eim_id eIM ID
 * @return 0 成功，负值表示未找到
 */
int ipa_eim_manager_set_active(struct ipa_eim_manager *mgr, const char *eim_id);

/**
 * @brief 获取活动 eIM 配置
 *
 * @param mgr eIM 管理器
 * @return 活动 eIM 配置指针，NULL 表示无活动 eIM
 */
const struct ipa_eim_config *ipa_eim_manager_get_active(const struct ipa_eim_manager *mgr);

/**
 * @brief 执行 EIM 能力协商
 *
 * 与指定的 eIM 进行能力协商，确定双方共同支持的功能。
 *
 * @param ctx IPAd 上下文
 * @param eim_id eIM ID
 * @param result 协商结果输出
 * @return 0 成功，负值表示错误
 */
int ipa_eim_capability_negotiate(struct ipa_context *ctx, const char *eim_id,
                                  struct ipa_eim_capability_result *result);

/**
 * @brief 动态更新 EIM 配置
 *
 * 从 eUICC 读取最新的 EimConfigurationData 并更新本地配置。
 *
 * @param ctx IPAd 上下文
 * @return 0 成功，负值表示错误
 */
int ipa_eim_config_refresh(struct ipa_context *ctx);

/**
 * @brief 导出 eIM 配置为 JSON 格式
 *
 * @param mgr eIM 管理器
 * @param buf 输出缓冲区
 * @param buf_size 缓冲区大小
 * @return 写入的字节数，负值表示错误
 */
int ipa_eim_manager_export_json(const struct ipa_eim_manager *mgr, char *buf, size_t buf_size);

/**
 * @brief 从 JSON 导入 eIM 配置
 *
 * @param mgr eIM 管理器
 * @param json_str JSON 字符串
 * @return 0 成功，负值表示解析错误
 */
int ipa_eim_manager_import_json(struct ipa_eim_manager *mgr, const char *json_str);

/** @} */

#ifdef __cplusplus
}
#endif
