/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Author: IPAd Development Team
 *
 * See also: GSMA SGP.32, section 3.2.3.1: Direct Profile Download
 */

#ifndef __PROC_DIRECT_PRFLE_DWNLD_H
#define __PROC_DIRECT_PRFLE_DWNLD_H

#include <stdint.h>
#include <stdbool.h>
#include "esipa.h"

/**
 * @file proc_direct_prfle_dwnld.h
 * @brief Direct Profile Download Procedure (直接 Profile 下载流程)
 * 
 * 实现 GSMA SGP.32 §3.2.3.1 定义的直接 Profile 下载流程。
 * 与 Indirect Profile Download 不同，此模式允许 IPA 直接连接 SM-DP+ 服务器，
 * 无需通过 eIM 中转。
 * 
 * 适用场景：
 * - 无 eIM 部署的环境
 * - 需要减少通讯延迟的场景
 * - 离线 Profile 下载
 * 
 * 依赖接口：
 * - ES9p.InitiateAuthentication
 * - ES9p.GetBoundProfilePackage
 * - ES10b.PrepareDownload (可选)
 * - Profile Installation
 */

/**
 * @brief 直接 Profile 下载参数结构
 */
struct ipa_proc_direct_prfle_dwnld_pars {
    const char *ac;                          /*!< 激活码 (Activation Code) */
    struct ipa_buf *euicc_challenge;         /*!< eUICC 挑战 (32 字节随机数) */
    struct ipa_buf *euicc_info;              /*!< eUICC 信息 (编码后的 EuiccInfo1) */
    struct ipa_buf *allowed_ca;              /*!< 允许的证书颁发者 PKID (可选) */
    bool installation_flag;                  /*!< 安装标志：true=下载后自动安装 */
};

/**
 * @brief 直接 Profile 下载结果结构
 */
struct ipa_proc_direct_prfle_dwnld_res {
    bool success;                            /*!< 是否成功 */
    uint8_t *iccid;                          /*!< ICCID (如果成功) */
    int error_code;                          /*!< 错误代码 (如果失败) */
};

/**
 * @brief 执行直接 Profile 下载流程
 * 
 * 此函数实现完整的直接 Profile 下载流程，包括：
 * 1. 解析激活码获取 SM-DP+ 地址
 * 2. 建立与 SM-DP+ 的 HTTPS 连接
 * 3. 执行 ES9+.InitiateAuthentication
 * 4. 执行 Common Metadata Authentication
 * 5. 执行 ES9+.GetBoundProfilePackage
 * 6. 准备 Profile 安装
 * 
 * @param[inout] ctx IPAd 上下文指针
 * @param[in] pars 下载参数结构指针
 * @return 0 表示成功，负值表示错误代码
 * 
 * @note 此函数会阻塞直到下载完成或超时
 * @note 失败时会自动调用 Common Cancel Session 流程
 * @see ipa_proc_indirect_prfle_dwnlod() 间接下载模式
 * @see GSMA SGP.32 §3.2.3.1
 */
int ipa_proc_direct_prfle_dwnld(struct ipa_context *ctx,
                                const struct ipa_proc_direct_prfle_dwnld_pars *pars);

/**
 * @brief 释放直接 Profile 下载结果资源
 * 
 * @param res 要释放的结果结构指针，可以为 NULL
 * 
 * @note 此函数会安全处理 NULL 输入
 */
void ipa_proc_direct_prfle_dwnld_res_free(struct ipa_proc_direct_prfle_dwnld_res *res);

#endif /* __PROC_DIRECT_PRFLE_DWNLD_H */
