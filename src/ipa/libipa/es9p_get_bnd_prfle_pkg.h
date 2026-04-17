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
 * @file es9p_get_bnd_prfle_pkg.h
 * @brief ES9+.GetBoundProfilePackage 客户端接口
 *
 * 本文件定义了 IPA 从 SM-DP+ 服务器获取已绑定配置文件包的接口。
 * 该接口用于 Direct Profile Download 流程中的第二步。
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ES9P_BPP ES9+ 配置文件包获取模块
 *  @{
 */

/**
 * @brief ES9+ GetBoundProfilePackage 请求结构
 */
struct ipa_es9p_get_bnd_prfle_pkg_req {
    /*! 准备下载响应（PrepareDownloadResponse ASN.1 编码） */
    struct ipa_buf *prepare_download_response;
    
    /*! 可选的事务 ID（某些实现需要） */
    const char *transaction_id;
};

/**
 * @brief ES9+ GetBoundProfilePackage 错误码
 */
enum ipa_es9p_get_bnd_prfle_pkg_err {
    IPA_ES9P_BPP_OK = 0,                 /*!< 成功 */
    IPA_ES9P_BPP_ERR_NETWORK = -1,       /*!< 网络错误 */
    IPA_ES9P_BPP_ERR_CERT = -2,          /*!< 证书验证失败 */
    IPA_ES9P_BPP_ERR_SIGNATURE = -3,     /*!< 签名验证失败 */
    IPA_ES9P_BPP_ERR_PROTOCOL = -4,      /*!< 协议错误 */
    IPA_ES9P_BPP_ERR_SERVER = -5,        /*!< 服务器错误 */
    IPA_ES9P_BPP_ERR_TIMEOUT = -6,       /*!< 超时 */
    IPA_ES9P_BPP_ERR_NOT_FOUND = -7,     /*!< 配置文件未找到 */
};

/**
 * @brief ES9+ GetBoundProfilePackage 响应结构
 */
struct ipa_es9p_get_bnd_prfle_pkg_res {
    /*! 错误码，0 表示成功 */
    enum ipa_es9p_get_bnd_prfle_pkg_err get_bnd_prfle_pkg_err;
    
    /*! 成功时的 BoundProfilePackage（ASN.1 编码） */
    struct ipa_buf *bound_profile_package;
    
    /*! 错误描述信息（可选） */
    const char *error_msg;
};

/**
 * @brief 执行 ES9+.GetBoundProfilePackage
 *
 * 此函数从 SM-DP+ 服务器获取已绑定的配置文件包，是 Direct Profile Download
 * 流程的第二步。它实现了 GSMA SGP.22 定义的 ES9+ 接口。
 *
 * @param ctx IPAd 上下文（必须先完成 InitiateAuthentication）
 * @param req 请求参数
 * @return 响应结构，调用者负责使用 ipa_es9p_get_bnd_prfle_pkg_res_free() 释放
 *         NULL 表示内存分配失败或其他严重错误
 *
 * @note 此函数会阻塞直到收到服务器响应或超时
 * @note 必须在成功的 InitiateAuthentication 之后调用
 *
 * @see ipa_es9p_get_bnd_prfle_pkg_res_free()
 * @see ipa_proc_direct_prfle_dwnld()
 */
struct ipa_es9p_get_bnd_prfle_pkg_res *ipa_es9p_get_bnd_prfle_pkg(
    struct ipa_context *ctx,
    const struct ipa_es9p_get_bnd_prfle_pkg_req *req);

/**
 * @brief 释放 ES9+ GetBoundProfilePackage 响应结构
 *
 * @param res 要释放的响应结构（可为 NULL）
 */
void ipa_es9p_get_bnd_prfle_pkg_res_free(struct ipa_es9p_get_bnd_prfle_pkg_res *res);

/** @} */

#ifdef __cplusplus
}
#endif
