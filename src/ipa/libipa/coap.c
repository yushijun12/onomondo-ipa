/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/coap.h>
#include "esipa.h"
#include "context.h"
#include "utils.h"

#define COAP_DEFAULT_PORT_UDP 5683
#define COAP_DEFAULT_PORT_DTLS 5684
#define COAP_PATH_ESIPA "/gsma/rsp2/asn1"
#define COAP_CONTENT_FORMAT_ASN1 65535  /* application/x-gsma-rsp-asn1 */

/**
 * @brief CoAP 客户端上下文结构
 */
struct ipa_coap_context {
    struct ipa_coap_config cfg;      /*!< CoAP 配置 */
    void *coap_session;              /*!< libcoap 会话指针 */
    void *coap_ctx;                  /*!< libcoap 上下文指针 */
    char current_host[256];          /*!< 当前连接的主机 */
    uint16_t current_port;           /*!< 当前连接的端口 */
};

/**
 * @brief 获取 eIM 的 CoAP 地址
 * 
 * @param ctx IPAd 上下文
 * @param host 输出主机地址缓冲区
 * @param port 输出端口号
 * @return 0 成功，负值失败
 */
static int ipa_coap_get_eim_addr(struct ipa_context *ctx, char *host, uint16_t *port)
{
    assert(ctx && ctx->eim_fqdn);
    
    /* TODO: 从 eIM 配置中解析 CoAP 地址 */
    /* 目前复用 HTTPS 的 FQDN，实际部署时应使用独立的 CoAP 端点 */
    strncpy(host, ctx->eim_fqdn, 255);
    host[255] = '\0';
    
    if (ctx->cfg->eim_disable_ssl) {
        *port = COAP_DEFAULT_PORT_UDP;
    } else {
        *port = COAP_DEFAULT_PORT_DTLS;
    }
    
    return 0;
}

/**
 * @brief 初始化 CoAP 会话
 * 
 * @param ctx IPAd 上下文
 * @return CoAP 上下文指针，失败时返回 NULL
 */
static struct ipa_coap_context *ipa_coap_session_init(struct ipa_context *ctx)
{
    struct ipa_coap_context *coap_ctx;
    struct ipa_coap_config cfg;
    char host[256];
    uint16_t port;
    
    if (ipa_coap_get_eim_addr(ctx, host, &port) != 0) {
        IPA_LOGP(SCOAP, LERROR, "无法获取 eIM CoAP 地址\n");
        return NULL;
    }
    
    memset(&cfg, 0, sizeof(cfg));
    cfg.transport = ctx->cfg->eim_disable_ssl ? IPA_COAP_UDP : IPA_COAP_DTLS;
    cfg.ca_cert = ctx->cfg->eim_cabundle;
    cfg.verify_peer = !ctx->cfg->eim_disable_ssl_verif;
    cfg.port = port;
    cfg.timeout_ms = 5000;
    cfg.max_retransmit = IPA_COAP_MAX_RETRANSMIT;
    
    coap_ctx = IPA_ALLOC_ZERO(struct ipa_coap_context);
    if (!coap_ctx) {
        IPA_LOGP(SCOAP, LERROR, "内存分配失败\n");
        return NULL;
    }
    
    coap_ctx->cfg = cfg;
    strncpy(coap_ctx->current_host, host, sizeof(coap_ctx->current_host) - 1);
    coap_ctx->current_port = port;
    
    /* TODO: 调用 libcoap API 初始化会话 */
    /* coap_ctx->coap_ctx = coap_new_context(NULL); */
    /* coap_ctx->coap_session = coap_new_client_session(...); */
    
    IPA_LOGP(SCOAP, LDEBUG, "CoAP 会话初始化：%s:%u (%s)\n", 
             host, port, cfg.transport == IPA_COAP_DTLS ? "DTLS" : "UDP");
    
    return coap_ctx;
}

/**
 * @brief 执行 CoAP 请求
 * 
 * @param ctx IPAd 上下文
 * @param req ASN.1 编码的请求数据
 * @param function_name 函数名（用于日志）
 * @return 响应数据缓冲区，失败时返回 NULL
 */
struct ipa_buf *ipa_coap_req(struct ipa_context *ctx, const struct ipa_buf *req, 
                             const char *function_name)
{
    struct ipa_coap_context *coap_ctx;
    struct ipa_buf *resp = NULL;
    char host[256];
    uint16_t port;
    
    assert(ctx && req && function_name);
    
    /* 获取或创建 CoAP 会话 */
    if (!ctx->coap_ctx) {
        ctx->coap_ctx = ipa_coap_session_init(ctx);
        if (!ctx->coap_ctx) {
            IPA_LOGP_ESIPA(function_name, LERROR, "CoAP 会话初始化失败\n");
            return NULL;
        }
    }
    coap_ctx = (struct ipa_coap_context *)ctx->coap_ctx;
    
    /* 检查是否需要重新连接（主机地址变化） */
    if (ipa_coap_get_eim_addr(ctx, host, &port) != 0) {
        IPA_LOGP_ESIPA(function_name, LERROR, "无法获取 eIM CoAP 地址\n");
        return NULL;
    }
    
    if (strcmp(host, coap_ctx->current_host) != 0 || port != coap_ctx->current_port) {
        IPA_LOGP(SCOAP, LDEBUG, "eIM 地址变更，重建 CoAP 会话\n");
        ipa_coap_free(ctx->coap_ctx);
        ctx->coap_ctx = ipa_coap_session_init(ctx);
        if (!ctx->coap_ctx) {
            return NULL;
        }
        coap_ctx = (struct ipa_coap_context *)ctx->coap_ctx;
    }
    
    IPA_LOGP_ESIPA(function_name, LDEBUG, "发送 CoAP 请求到 %s:%u%s\n", 
                   host, port, COAP_PATH_ESIPA);
    IPA_LOGP(SCOAP, LDEBUG, "请求数据:\n");
    ipa_buf_hexdump_multiline(req, 64, 1, SCOAP, LDEBUG);
    
    /* TODO: 使用 libcoap 发送请求 */
    /* 
     * coap_pdu_t *pdu = coap_new_pdu(COAP_MESSAGE_CON, COAP_REQUEST_POST, coap_ctx->coap_session);
     * coap_add_option(pdu, COAP_OPTION_CONTENT_FORMAT, ...);
     * coap_add_data(pdu, req->len, req->data);
     * coap_send(coap_ctx->coap_session, pdu);
     * coap_read(coap_ctx->coap_ctx, timeout);
     */
    
    /* 模拟响应（实际实现应调用 libcoap） */
    IPA_LOGP_ESIPA(function_name, LWARNING, "CoAP 功能尚未完全实现，需要集成 libcoap\n");
    
    return resp;
}

/**
 * @brief 关闭 CoAP 连接
 * 
 * @param ctx IPAd 上下文
 */
void ipa_coap_close(struct ipa_context *ctx)
{
    if (!ctx || !ctx->coap_ctx)
        return;
    
    IPA_LOGP(SCOAP, LDEBUG, "关闭 CoAP 连接\n");
    
    /* TODO: 调用 libcoap API 关闭会话 */
    /* coap_session_release(coap_ctx->coap_session); */
}

/**
 * @brief 释放 CoAP 上下文
 * 
 * @param coap_ctx CoAP 上下文指针
 */
void ipa_coap_free_impl(void *coap_ctx)
{
    struct ipa_coap_context *ctx = (struct ipa_coap_context *)coap_ctx;
    
    if (!ctx)
        return;
    
    IPA_LOGP(SCOAP, LDEBUG, "释放 CoAP 上下文\n");
    
    /* TODO: 调用 libcoap API 释放资源 */
    /* coap_free_context(ctx->coap_ctx); */
    
    IPA_FREE(ctx);
}
