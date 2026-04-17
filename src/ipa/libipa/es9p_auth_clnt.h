/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Author: IPAd Development Team
 *
 * See also: GSMA SGP.22, section 3.1: ES9+ Interface
 *           GSMA SGP.32, section 3.2.3.1: Direct Profile Download
 */

/**
 * @file es9p_auth_clnt.h
 * @brief ES9+.InitiateAuthentication 客户端接口
 *
 * 本文件定义了 IPA 与 SM-DP+ 服务器进行认证的接口。
 * 该接口用于 Direct Profile Download 流程中的第一步认证。
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ES9P_Auth ES9+ 认证模块
 *  @{
 */

/**
 * @brief ES9+ 认证请求结构
 *
 * 包含发起认证所需的所有参数。
 */
struct ipa_es9p_auth_clnt_req {
    /*! eUICC 生成的挑战（随机数） */
    struct ipa_buf *euicc_challenge;
    
    /*! eUICC 信息（EuiccInfo1 ASN.1 编码） */
    struct ipa_buf *euicc_info;
    
    /*! SM-DP+ 服务器地址（FQDN 或 URL） */
    const char *sm_dp_addr;
};

/**
 * @brief ES9+ 认证成功结果结构
 */
struct ipa_es9p_auth_clnt_ok {
    /*! 准备下载响应（PrepareDownloadResponse ASN.1 编码） */
    struct ipa_buf *prepare_download_response;
    
    /*! SM-DP+ 证书链 */
    struct ipa_buf *smdp_cert_chain;
    
    /*! 事务 ID（用于后续 GetBoundProfilePackage 调用） */
    const char *transaction_id;
};

/**
 * @brief ES9+ 认证错误码
 */
enum ipa_es9p_auth_err {
    IPA_ES9P_AUTH_OK = 0,              /*!< 认证成功 */
    IPA_ES9P_AUTH_ERR_NETWORK = -1,    /*!< 网络错误 */
    IPA_ES9P_AUTH_ERR_CERT = -2,       /*!< 证书验证失败 */
    IPA_ES9P_AUTH_ERR_SIGNATURE = -3,  /*!< 签名验证失败 */
    IPA_ES9P_AUTH_ERR_PROTOCOL = -4,   /*!< 协议错误 */
    IPA_ES9P_AUTH_ERR_SERVER = -5,     /*!< 服务器错误 */
    IPA_ES9P_AUTH_ERR_TIMEOUT = -6,    /*!< 超时 */
};

/**
 * @brief ES9+ 认证响应结构
 *
 * 包含认证成功或失败的完整信息。
 */
struct ipa_es9p_auth_clnt_res {
    /*! 认证错误码，0 表示成功 */
    enum ipa_es9p_auth_err auth_err;
    
    /*! 认证成功时的详细数据（仅当 auth_err == IPA_ES9P_AUTH_OK 时有效） */
    struct ipa_es9p_auth_clnt_ok *ok_data;
    
    /*! 错误描述信息（可选） */
    const char *error_msg;
};

/**
 * @brief 执行 ES9+.InitiateAuthentication
 *
 * 此函数向 SM-DP+ 服务器发起认证请求，是 Direct Profile Download
 * 流程的第一步。它实现了 GSMA SGP.22 定义的 ES9+ 接口。
 *
 * @param ctx IPAd 上下文（必须已初始化 HTTP 客户端）
 * @param req 认证请求参数
 * @return 认证响应结构，调用者负责使用 ipa_es9p_auth_clnt_res_free() 释放
 *         NULL 表示内存分配失败或其他严重错误
 *
 * @note 此函数会阻塞直到收到服务器响应或超时
 * @note 响应结构必须在使用后释放以避免内存泄漏
 *
 * @see ipa_es9p_auth_clnt_res_free()
 * @see ipa_proc_direct_prfle_dwnld()
 */
struct ipa_es9p_auth_clnt_res *ipa_es9p_auth_clnt(
    struct ipa_context *ctx,
    const struct ipa_es9p_auth_clnt_req *req);

/**
 * @brief 释放 ES9+ 认证响应结构
 *
 * 释放由 ipa_es9p_auth_clnt() 分配的响应结构及其所有成员。
 *
 * @param res 要释放的响应结构（可为 NULL）
 *
 * @note 此函数会递归释放所有嵌套分配的内存
 */
void ipa_es9p_auth_clnt_res_free(struct ipa_es9p_auth_clnt_res *res);

/** @} */

#ifdef __cplusplus
}
#endif
