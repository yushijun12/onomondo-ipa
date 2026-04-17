/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Author: IPAd Development Team
 *
 * See also: GSMA SGP.32, section 3.1.1.2: eIM Package Injection
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <onomondo/ipa/mem.h>
#include <onomondo/ipa/utils.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/ipad.h>
#include "context.h"
#include "utils.h"
#include "esipa.h"
#include "esipa_prvde_eim_pkg_rslt.h"
#include "proc_euicc_pkg_dwnld_exec.h"
#include "proc_euicc_data_req.h"
#include "proc_indirect_prfle_dwnld.h"
#include "proc_eim_pkg_inject.h"
#include "es10b_get_eim_cfg_data.h"

/**
 * @brief 从 eUICC 获取 CI PKID
 * 
 * @param ctx IPAd 上下文
 * @param[out] pkid 输出的 PKID 缓冲区
 * @return 0 成功，负值表示错误代码
 */
static int get_euicc_ci_pkid(struct ipa_context *ctx, struct ipa_buf **pkid)
{
    struct ipa_es10b_eim_cfg_data *eim_cfg_data = NULL;
    struct EimConfigurationData *eim_cfg_data_item = NULL;

    *pkid = NULL;

    eim_cfg_data = ipa_es10b_get_eim_cfg_data(ctx);
    if (!eim_cfg_data) {
        IPA_LOGP(SIPA, LERROR, "cannot read EimConfigurationData from eUICC\n");
        goto error;
    }

    eim_cfg_data_item = ipa_es10b_get_eim_cfg_data_filter(eim_cfg_data, ctx->eim_id);
    if (!eim_cfg_data_item) {
        IPA_LOGP(SIPA, LERROR, "no EimConfigurationData item for eimId %s present!\n", ctx->eim_id);
        goto error;
    }

    if (eim_cfg_data_item->euiccCiPKId)
        *pkid = IPA_BUF_FROM_ASN(eim_cfg_data_item->euiccCiPKId);

    ipa_es10b_get_eim_cfg_data_free(eim_cfg_data);
    return 0;
error:
    IPA_LOGP(SIPA, LERROR, "unable to retrieve EimConfigurationData\n");
    ipa_es10b_get_eim_cfg_data_free(eim_cfg_data);
    return -EINVAL;
}

/**
 * @brief 解析传入的 TransferEimPackageRequest 并提取有效负载
 * 
 * @param ctx IPAd 上下文
 * @param package 原始包数据
 * @param package_size 包大小
 * @param[out] transfer_req 解析后的 TransferEimPackageRequest
 * @return 0 成功，负值表示错误代码
 */
static int parse_transfer_eim_package(struct ipa_context *ctx, 
                                       const uint8_t *package, size_t package_size,
                                       TransferEimPackageRequest_t **transfer_req)
{
    struct ipa_buf *buf = NULL;
    int rc;

    *transfer_req = NULL;

    /* 将输入数据复制到缓冲区 */
    buf = ipa_buf_new(package, package_size);
    if (!buf) {
        IPA_LOGP(SIPA, LERROR, "failed to allocate buffer for eIM package\n");
        rc = -ENOMEM;
        goto error;
    }

    /* 解码 ASN.1 TransferEimPackageRequest */
    *transfer_req = ipa_esipa_msg_from_ext_dec(buf, "TransferEimPackageRequest");
    if (!*transfer_req) {
        IPA_LOGP(SIPA, LERROR, "failed to decode TransferEimPackageRequest\n");
        rc = -EINVAL;
        goto error;
    }

    IPA_LOGP(SIPA, LDEBUG, "successfully decoded TransferEimPackageRequest (type=%d)\n", 
             (*transfer_req)->present);

    ipa_buf_free(buf);
    return 0;

error:
    ipa_buf_free(buf);
    if (*transfer_req) {
        ASN_STRUCT_FREE(asn_DEF_TransferEimPackageRequest, *transfer_req);
        *transfer_req = NULL;
    }
    return rc;
}

/**
 * @brief 处理注入的 EuiccPackageRequest
 * 
 * @param ctx IPAd 上下文
 * @param euicc_pkg_req EuiccPackageRequest 内容
 * @return 0 成功，负值表示错误代码
 */
static int handle_injected_euicc_package(struct ipa_context *ctx,
                                          EuiccPackageRequest_t *euicc_pkg_req)
{
    int rc;

    IPA_LOGP(SIPA, LINFO, "processing injected EuiccPackageRequest\n");

    /* 复用现有的 eUICC 包下载执行逻辑 */
    ctx->proc_eucc_pkg_dwnld_exec_res = ipa_proc_eucc_pkg_dwnld_exec(ctx, euicc_pkg_req);
    if (!ctx->proc_eucc_pkg_dwnld_exec_res) {
        IPA_LOGP(SIPA, LERROR, "failed to process EuiccPackageRequest\n");
        rc = -EINVAL;
        goto error;
    }

    /* 如果不需要调用 onset，立即清理结果 */
    if (ctx->proc_eucc_pkg_dwnld_exec_res && !ctx->proc_eucc_pkg_dwnld_exec_res->call_onset) {
        ipa_proc_eucc_pkg_dwnld_exec_res_free(ctx->proc_eucc_pkg_dwnld_exec_res);
        ctx->proc_eucc_pkg_dwnld_exec_res = NULL;
    }

    IPA_LOGP(SIPA, LINFO, "EuiccPackageRequest processing completed\n");
    return 0;

error:
    if (ctx->proc_eucc_pkg_dwnld_exec_res) {
        ipa_proc_eucc_pkg_dwnld_exec_res_free(ctx->proc_eucc_pkg_dwnld_exec_res);
        ctx->proc_eucc_pkg_dwnld_exec_res = NULL;
    }
    return rc;
}

/**
 * @brief 处理注入的 IpaEuiccDataRequest
 * 
 * @param ctx IPAd 上下文
 * @param ipa_data_req IpaEuiccDataRequest 内容
 * @return 0 成功，负值表示错误代码
 */
static int handle_injected_ipa_data_request(struct ipa_context *ctx,
                                             IpaEuiccDataRequest_t *ipa_data_req)
{
    struct ipa_proc_euicc_data_req_pars euicc_data_req_pars = { 0 };
    int rc;

    IPA_LOGP(SIPA, LINFO, "processing injected IpaEuiccDataRequest\n");

    euicc_data_req_pars.ipa_euicc_data_request = ipa_data_req;
    rc = ipa_proc_euicc_data_req(ctx, &euicc_data_req_pars);
    if (rc < 0) {
        IPA_LOGP(SIPA, LERROR, "failed to process IpaEuiccDataRequest\n");
        goto error;
    }

    IPA_LOGP(SIPA, LINFO, "IpaEuiccDataRequest processing completed\n");
    return 0;

error:
    return rc;
}

/**
 * @brief 处理注入的 ProfileDownloadTriggerRequest
 * 
 * @param ctx IPAd 上下文
 * @param dwnld_trigger_req ProfileDownloadTriggerRequest 内容
 * @return 0 成功，负值表示错误代码
 */
static int handle_injected_download_trigger(struct ipa_context *ctx,
                                             ProfileDownloadTriggerRequest_t *dwnld_trigger_req)
{
    struct ipa_proc_indirect_prfle_dwnlod_pars indirect_prfle_dwnlod_pars = { 0 };
    struct ipa_buf *allowed_ca_pkid = NULL;
    int rc;

    IPA_LOGP(SIPA, LINFO, "processing injected ProfileDownloadTriggerRequest\n");

    /* 验证是否包含 ProfileDownloadData */
    if (!dwnld_trigger_req->profileDownloadData) {
        IPA_LOGP(SIPA, LERROR, "ProfileDownloadTriggerRequest missing profileDownloadData\n");
        rc = -EINVAL;
        goto error;
    }

    /* 验证是否为 activationCode 类型 */
    if (dwnld_trigger_req->profileDownloadData->present !=
        ProfileDownloadData_PR_activationCode) {
        IPA_LOGP(SIPA, LERROR, "ProfileDownloadData is not activationCode type\n");
        rc = -EINVAL;
        goto error;
    }

    /* 获取 eUICC CI PKID */
    rc = get_euicc_ci_pkid(ctx, &allowed_ca_pkid);
    if (rc < 0) {
        IPA_LOGP(SIPA, LERROR, "failed to retrieve eUICC CI PKID\n");
        goto error;
    }

    /* 准备间接下载参数 */
    indirect_prfle_dwnlod_pars.allowed_ca = allowed_ca_pkid;
    indirect_prfle_dwnlod_pars.tac = ctx->cfg->tac;
    indirect_prfle_dwnlod_pars.ac = IPA_STR_FROM_ASN(&dwnld_trigger_req->profileDownloadData->
                                                      choice.activationCode);

    /* 执行间接下载流程 */
    rc = ipa_proc_indirect_prfle_dwnlod(ctx, &indirect_prfle_dwnlod_pars);
    IPA_FREE((void *)indirect_prfle_dwnlod_pars.ac);
    if (rc < 0) {
        IPA_LOGP(SIPA, LERROR, "indirect profile download failed\n");
        goto error;
    }

    IPA_LOGP(SIPA, LINFO, "ProfileDownloadTriggerRequest processing completed\n");
    rc = 0;

error:
    IPA_FREE(allowed_ca_pkid);
    return rc;
}

/**
 * @brief 处理注入的 EimAcknowledgements
 * 
 * @param ctx IPAd 上下文
 * @param eim_ack EimAcknowledgements 内容
 * @return 0 成功
 */
static int handle_injected_eim_acknowledgements(struct ipa_context *ctx,
                                                 EimAcknowledgements_t *eim_ack)
{
    IPA_LOGP(SIPA, LINFO, "processing EimAcknowledgements (no action required)\n");
    /* EimAcknowledgements 仅用于确认，无需特殊处理 */
    return 0;
}

/**
 * @brief 生成 ProvideEimPackageResult 响应
 * 
 * @param ctx IPAd 上下文
 * @param success 是否成功
 * @param result_type 结果类型
 * @param[out] result 生成的响应结构
 * @return 0 成功，负值表示错误代码
 */
static int generate_provide_eim_result(struct ipa_context *ctx, bool success,
                                        ProvideEimPackageResult_PR result_type,
                                        struct ipa_esipa_prvde_eim_pkg_rslt_res **result)
{
    struct ipa_esipa_prvde_eim_pkg_rslt_req req = { 0 };
    
    if (!success) {
        req.eim_pkg_err = ProvideEimPackageResult__eimPackageError_undefinedError;
    }

    *result = ipa_esipa_prvde_eim_pkg_rslt(ctx, &req);
    if (!*result) {
        IPA_LOGP(SIPA, LERROR, "failed to generate ProvideEimPackageResult\n");
        return -EINVAL;
    }

    IPA_LOGP(SIPA, LDEBUG, "generated ProvideEimPackageResult (type=%d)\n", result_type);
    return 0;
}

/**
 * @brief 主注入处理函数
 * 
 * 解析传入的 eIM 包，根据类型路由到相应处理器，并生成响应
 * 
 * @param ctx IPAd 上下文
 * @param package 原始包数据
 * @param package_size 包大小
 * @return 0 成功，负值表示错误代码
 */
int ipa_proc_eim_pkg_inject(struct ipa_context *ctx, const uint8_t *package, size_t package_size)
{
    TransferEimPackageRequest_t *transfer_req = NULL;
    struct ipa_esipa_prvde_eim_pkg_rslt_res *provide_result = NULL;
    int rc = 0;

    IPA_LOGP(SIPA, LINFO, "starting eIM Package Injection procedure\n");

    /* 步骤 1: 解析 TransferEimPackageRequest */
    rc = parse_transfer_eim_package(ctx, package, package_size, &transfer_req);
    if (rc < 0) {
        IPA_LOGP(SIPA, LERROR, "failed to parse TransferEimPackageRequest\n");
        goto error;
    }

    /* 步骤 2: 根据类型路由到相应处理器 */
    switch (transfer_req->present) {
    case TransferEimPackageRequest_PR_euiccPackageRequest:
        rc = handle_injected_euicc_package(ctx, &transfer_req->choice.euiccPackageRequest);
        break;

    case TransferEimPackageRequest_PR_ipaEuiccDataRequest:
        rc = handle_injected_ipa_data_request(ctx, &transfer_req->choice.ipaEuiccDataRequest);
        break;

    case TransferEimPackageRequest_PR_profileDownloadTriggerRequest:
        rc = handle_injected_download_trigger(ctx, &transfer_req->choice.profileDownloadTriggerRequest);
        break;

    case TransferEimPackageRequest_PR_eimAcknowledgements:
        rc = handle_injected_eim_acknowledgements(ctx, &transfer_req->choice.eimAcknowledgements);
        break;

    case TransferEimPackageRequest_PR_NOTHING:
    default:
        IPA_LOGP(SIPA, LERROR, "unsupported or empty TransferEimPackageRequest type: %d\n",
                 transfer_req->present);
        rc = -EINVAL;
        goto error;
    }

    /* 步骤 3: 生成 ProvideEimPackageResult 响应 */
    if (rc < 0) {
        IPA_LOGP(SIPA, LWARNING, "eIM Package Injection completed with errors\n");
    } else {
        IPA_LOGP(SIPA, LINFO, "eIM Package Injection completed successfully\n");
    }

    /* 生成并发送响应（可选，取决于是否需要同步返回结果） */
    rc = generate_provide_eim_result(ctx, (rc == 0), 
                                      (rc == 0) ? ProvideEimPackageResult_PR_ePRAndNotifications :
                                                  ProvideEimPackageResult_PR_eimPackageError,
                                      &provide_result);
    if (rc < 0) {
        IPA_LOGP(SIPA, LWARNING, "failed to generate ProvideEimPackageResult (non-fatal)\n");
    }

    /* 清理资源 */
    if (provide_result) {
        ipa_esipa_prvde_eim_pkg_rslt_free(provide_result);
    }
    if (transfer_req) {
        ASN_STRUCT_FREE(asn_DEF_TransferEimPackageRequest, transfer_req);
    }

    return (rc < 0) ? rc : 0;

error:
    if (provide_result) {
        ipa_esipa_prvde_eim_pkg_rslt_free(provide_result);
    }
    if (transfer_req) {
        ASN_STRUCT_FREE(asn_DEF_TransferEimPackageRequest, transfer_req);
    }
    IPA_LOGP(SIPA, LERROR, "eIM Package Injection failed with error %d\n", rc);
    return rc;
}
