/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file coap.h
 * @brief CoAP 客户端接口
 * 
 * 本文件定义了 IPAd 与 eIM 通信的 CoAP 客户端接口。
 * 实现基于 libcoap，支持 CoAP over UDP 和 DTLS 加密。
 * 
 * @note CoAP 是 GSMA SGP.32 v1.2+ 推荐的 IoT eSIM 通讯方式，
 *       相比 HTTPS 具有更低的资源占用和网络开销。
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/** @defgroup CoAP CoAP 客户端模块
 *  @{
 */

struct ipa_buf;

/*! @brief CoAP 响应缓冲区初始大小（字节） */
#define IPA_LEN_COAP_RESPONSE_BUF 256

/*! @brief CoAP 消息最大重传次数 */
#define IPA_COAP_MAX_RETRANSMIT 4

/*! @brief CoAP ACK 超时时间（毫秒） */
#define IPA_COAP_ACK_TIMEOUT_MS 2000

/**
 * @brief CoAP 传输模式
 */
enum ipa_coap_transport {
    IPA_COAP_UDP,      /*!< 明文 CoAP over UDP */
    IPA_COAP_DTLS,     /*!< 加密 CoAP over DTLS */
    IPA_COAP_TCP,      /*!< CoAP over TCP (RFC 8323) */
    IPA_COAP_TLS       /*!< CoAP over TLS (RFC 8323) */
};

/**
 * @brief CoAP 客户端配置
 */
struct ipa_coap_config {
    enum ipa_coap_transport transport;  /*!< 传输模式 */
    const char *ca_cert;                /*!< CA 证书路径 (DTLS/TLS 模式) */
    const char *client_cert;            /*!< 客户端证书路径 (双向认证) */
    const char *client_key;             /*!< 客户端私钥路径 (双向认证) */
    bool verify_peer;                   /*!< 验证对端证书 */
    uint16_t port;                      /*!< CoAP 服务器端口 (默认 5683/5684) */
    uint32_t timeout_ms;                /*!< 请求超时时间 (毫秒) */
    uint8_t max_retransmit;             /*!< 最大重传次数 */
};

/**
 * @brief 初始化 CoAP 客户端上下文
 * 
 * 创建并初始化 CoAP 客户端上下文，配置传输模式和 DTLS 参数。
 * 
 * @param cfg CoAP 配置参数
 * @return CoAP 上下文指针，失败时返回 NULL
 * 
 * @note DTLS 模式下必须提供 CA 证书以确保安全性
 */
void *ipa_coap_init(const struct ipa_coap_config *cfg);

/**
 * @brief 执行 CoAP 请求
 * 
 * 向指定 eIM 发送 ASN.1 编码的请求数据并返回响应。
 * 
 * @param coap_ctx CoAP 上下文指针（由 ipa_coap_init() 创建）
 * @param req 请求数据缓冲区（ASN.1 编码）
 * @param path CoAP 资源路径（如 "/gsma/rsp2/asn1"）
 * @param host 目标主机地址（IP 或域名）
 * @return 响应数据缓冲区，调用者负责用 ipa_buf_free() 释放，失败时返回 NULL
 * 
 * @note CoAP 使用 POST 方法发送 ESIPA 消息
 * @note Content-Format 自动设置为 application/x-gsma-rsp-asn1
 */
struct ipa_buf *ipa_coap_req(void *coap_ctx, const struct ipa_buf *req, 
                             const char *path, const char *host);

/**
 * @brief 关闭 CoAP 连接
 * 
 * 关闭活动的 CoAP 连接但不释放上下文。
 * 
 * @param coap_ctx CoAP 上下文指针
 */
void ipa_coap_close(void *coap_ctx);

/**
 * @brief 释放 CoAP 上下文
 * 
 * 完全释放 CoAP 客户端上下文及其所有资源。
 * 
 * @param coap_ctx CoAP 上下文指针
 */
void ipa_coap_free(void *coap_ctx);

/**
 * @brief 设置 CoAP 观察模式回调
 * 
 * 用于 eIM 主动推送通知的场景（可选功能）。
 * 
 * @param coap_ctx CoAP 上下文指针
 * @param callback 观察通知回调函数
 * @param user_data 用户数据指针
 * @return 0 成功，负值失败
 * 
 * @note 此功能为 SGP.32 v2.x 预留
 */
typedef void (*ipa_coap_observe_cb)(const struct ipa_buf *notif, void *user_data);
int ipa_coap_set_observe_cb(void *coap_ctx, ipa_coap_observe_cb callback, void *user_data);

/** @} */  /* 结束 CoAP 模块组 */
