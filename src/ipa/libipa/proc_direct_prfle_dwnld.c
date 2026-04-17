/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Author: IPAd Development Team
 *
 * See also: GSMA SGP.32, section 3.2.3.1: Direct Profile Download
 *           GSMA SGP.22, section 3.1: ES9+ Interface
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <onomondo/ipa/mem.h>
#include <onomondo/ipa/utils.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/http.h>
#include "context.h"
#include "utils.h"
#include "activation_code.h"
#include "es9p_auth_clnt.h"
#include "es9p_get_bnd_prfle_pkg.h"
#include "proc_cmn_mtl_auth.h"
#include "proc_cmn_cancel_sess.h"
#include "proc_prfle_inst.h"
#include "proc_direct_prfle_dwnld.h"

/**
 * @brief 初始化与 SM-DP+ 服务器的 ES9+ 连接
 * 
 * @param ctx IPAd 上下文
 * @param sm_dp_addr SM-DP+ 服务器地址
 * @return 0 成功，负值表示错误代码
 */
static int es9p_connect(struct ipa_context *ctx, const char *sm_dp_addr)
{
    int rc;
    
    IPA_LOGP(SDP, LINFO, "connecting to SM-DP+ server: %s\\n", sm_dp_addr);
    
    /* 复用 HTTP 客户端基础设施 */
    rc = ipa_http_set_base_url(ctx->http_ctx, sm_dp_addr);
    if (rc < 0) {
        IPA_LOGP(SDP, LERROR, "failed to set SM-DP+ base URL\\n");
        goto error;
    }
    
    IPA_LOGP(SDP, LINFO, "ES9+ connection established\\n");
    return 0;
    
error:
    return rc;
}

/**
 * @brief 执行 ES9+.InitiateAuthentication
 * 
 * @param ctx IPAd 上下文
 * @param euicc_challenge eUICC 挑战
 * @param euicc_info eUICC 信息
 * @param sm_dp_addr SM-DP+ 地址
 * @return 认证响应，NULL 表示失败
 */
static struct ipa_es9p_auth_clnt_res *es9p_initiate_authentication(
    struct ipa_context *ctx,
    struct ipa_buf *euicc_challenge,
    struct ipa_buf *euicc_info,
    const char *sm_dp_addr)
{
    struct ipa_es9p_auth_clnt_req req = { 0 };
    struct ipa_es9p_auth_clnt_res *res = NULL;
    
    IPA_LOGP(SDP, LINFO, "initiating ES9+ authentication\\n");
    
    req.euicc_challenge = euicc_challenge;
    req.euicc_info = euicc_info;
    req.sm_dp_addr = sm_dp_addr;
    
    res = ipa_es9p_auth_clnt(ctx, &req);
    if (!res) {
        IPA_LOGP(SDP, LERROR, "ES9+ authentication failed\\n");
        goto error;
    }
    
    if (res->auth_err) {
        IPA_LOGP(SDP, LERROR, "ES9+ authentication returned error: %d\\n", res->auth_err);
        goto error;
    }
    
    IPA_LOGP(SDP, LINFO, "ES9+ authentication successful\\n");
    return res;
    
error:
    if (res)
        ipa_es9p_auth_clnt_res_free(res);
    return NULL;
}

/**
 * @brief 执行 ES9+.GetBoundProfilePackage
 * 
 * @param ctx IPAd 上下文
 * @param auth_clnt_ok 认证通过的数据
 * @return BoundProfilePackage 响应，NULL 表示失败
 */
static struct ipa_es9p_get_bnd_prfle_pkg_res *es9p_get_bound_profile_package(
    struct ipa_context *ctx,
    struct ipa_es9p_auth_clnt_ok *auth_clnt_ok)
{
    struct ipa_es9p_get_bnd_prfle_pkg_req req = { 0 };
    struct ipa_es9p_get_bnd_prfle_pkg_res *res = NULL;
    
    IPA_LOGP(SDP, LINFO, "requesting BoundProfilePackage\\n");
    
    req.prepare_download_response = auth_clnt_ok->prepare_download_response;
    
    res = ipa_es9p_get_bnd_prfle_pkg(ctx, &req);
    if (!res) {
        IPA_LOGP(SDP, LERROR, "GetBoundProfilePackage failed\\n");
        goto error;
    }
    
    if (res->get_bnd_prfle_pkg_err) {
        IPA_LOGP(SDP, LERROR, "GetBoundProfilePackage returned error: %d\\n", 
                 res->get_bnd_prfle_pkg_err);
        goto error;
    }
    
    IPA_LOGP(SDP, LINFO, "BoundProfilePackage received successfully\\n");
    return res;
    
error:
    if (res)
        ipa_es9p_get_bnd_prfle_pkg_res_free(res);
    return NULL;
}

/**
 * @brief 执行直接 Profile 下载流程
 * 
 * @param ctx IPAd 上下文
 * @param pars 下载参数
 * @return 0 成功，负值表示错误代码
 */
int ipa_proc_direct_prfle_dwnld(struct ipa_context *ctx, 
                                const struct ipa_proc_direct_prfle_dwnld_pars *pars)
{
    struct ipa_activation_code *activation_code = NULL;
    struct ipa_es9p_auth_clnt_res *auth_clnt_res = NULL;
    struct ipa_es9p_get_bnd_prfle_pkg_res *get_bnd_prfle_pkg_res = NULL;
    struct ipa_proc_cmn_mtl_auth_pars cmn_mtl_auth_pars = { 0 };
    struct ipa_proc_cmn_cancel_sess_pars cmn_cancel_sess_pars = { 0 };
    struct ipa_proc_prfle_inst_pars prfle_inst_pars = { 0 };
    int rc = 0;
    
    IPA_LOGP(SDP, LINFO, "starting Direct Profile Download procedure\\n");
    
    /* 步骤 1: 解析激活码 */
    activation_code = ipa_activation_code_parse(pars->ac);
    if (!activation_code) {
        IPA_LOGP(SDP, LERROR, "invalid activation code\\n");
        rc = -EINVAL;
        goto error;
    }
    
    ipa_activation_code_dump(activation_code, 0, SDP, LDEBUG);
    
    if (!activation_code->sm_dp_plus_address) {
        IPA_LOGP(SDP, LERROR, "activation code missing SM-DP+ address\\n");
        rc = -EINVAL;
        goto error;
    }
    
    /* 步骤 2: 连接到 SM-DP+ */
    rc = es9p_connect(ctx, activation_code->sm_dp_plus_address);
    if (rc < 0) {
        IPA_LOGP(SDP, LERROR, "failed to connect to SM-DP+\\n");
        goto error;
    }
    
    /* 步骤 3: ES9+.InitiateAuthentication */
    auth_clnt_res = es9p_initiate_authentication(
        ctx, 
        pars->euicc_challenge,
        pars->euicc_info,
        activation_code->sm_dp_plus_address
    );
    if (!auth_clnt_res) {
        rc = -EIO;
        goto error;
    }
    
    /* 步骤 4: Common Metadata Authentication */
    if (auth_clnt_res->auth_ok) {
        cmn_mtl_auth_pars.auth_clnt_ok = auth_clnt_res->auth_ok;
        cmn_mtl_auth_pars.allowed_ca = pars->allowed_ca;
        rc = ipa_proc_cmn_mtl_auth(ctx, &cmn_mtl_auth_pars);
        if (rc < 0) {
            IPA_LOGP(SDP, LERROR, "metadata authentication failed\\n");
            goto error;
        }
        IPA_LOGP(SDP, LINFO, "metadata authentication successful\\n");
    }
    
    /* 步骤 5: ES9+.GetBoundProfilePackage */
    if (auth_clnt_res->auth_ok) {
        get_bnd_prfle_pkg_res = es9p_get_bound_profile_package(ctx, auth_clnt_res->auth_ok);
        if (!get_bnd_prfle_pkg_res) {
            rc = -EIO;
            goto error;
        }
    }
    
    /* 步骤 6: Profile 安装准备 */
    if (get_bnd_prfle_pkg_res && get_bnd_prfle_pkg_res->get_bnd_prfle_pkg_ok) {
        prfle_inst_pars.bnd_prfle_pkg = get_bnd_prfle_pkg_res->get_bnd_prfle_pkg_ok->boundProfilePackage;
        prfle_inst_pars.installation_flag = pars->installation_flag;
        
        rc = ipa_proc_prfle_inst(ctx, &prfle_inst_pars);
        if (rc < 0) {
            IPA_LOGP(SDP, LERROR, "profile installation failed\\n");
            goto error;
        }
        IPA_LOGP(SDP, LINFO, "profile installed successfully\\n");
    }
    
    IPA_LOGP(SDP, LINFO, "Direct Profile Download completed successfully\\n");
    rc = 0;
    goto cleanup;
    
error:
    IPA_LOGP(SDP, LERROR, "Direct Profile Download failed with error %d\\n", rc);
    
    /* 尝试取消会话 */
    if (ctx->transaction_id) {
        cmn_cancel_sess_pars.transaction_id = ctx->transaction_id;
        cmn_cancel_sess_pars.reason = CANCEL_REASON_DOWNLOAD_FAILURE;
        ipa_proc_cmn_cancel_sess(ctx, &cmn_cancel_sess_pars);
    }
    
cleanup:
    ipa_activation_code_free(activation_code);
    ipa_es9p_auth_clnt_res_free(auth_clnt_res);
    ipa_es9p_get_bnd_prfle_pkg_res_free(get_bnd_prfle_pkg_res);
    
    return rc;
}

/**
 * @brief 释放直接下载资源
 * 
 * @param res 要释放的资源结构
 */
void ipa_proc_direct_prfle_dwnld_res_free(struct ipa_proc_direct_prfle_dwnld_res *res)
{
    if (!res)
        return;
    
    IPA_FREE(res);
}
