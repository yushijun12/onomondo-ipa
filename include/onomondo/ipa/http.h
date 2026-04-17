/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file http.h
 * @brief HTTP 客户端接口
 * 
 * 本文件定义了 IPAd 与 eIM 通信的 HTTP 客户端接口。
 * 实现基于 libcurl，支持 HTTPS 和 SSL 证书验证。
 */

#pragma once

#include <stdbool.h>

/** @defgroup HTTP HTTP 客户端模块
 *  @{
 */

struct ipa_buf;

/*! @brief HTTP 响应缓冲区初始大小（字节） */
#define IPA_LEN_HTTP_RESPONSE_BUF 512

/**
 * @brief 初始化 HTTP 客户端上下文
 * 
 * 创建并初始化 HTTP 客户端上下文，配置 SSL 证书验证等参数。
 * 
 * @param cabundle CA 证书束文件路径（NULL 使用系统默认）
 * @param no_verif 禁用 SSL 证书验证（测试环境用）
 * @return HTTP 上下文指针，失败时返回 NULL
 */
void *ipa_http_init(const char *cabundle, bool no_verif);

/**
 * @brief 执行 HTTP 请求
 * 
 * 向指定 URL 发送 ASN.1 编码的请求数据并返回响应。
 * 
 * @param http_ctx HTTP 上下文指针（由 ipa_http_init() 创建）
 * @param req 请求数据缓冲区（ASN.1 编码）
 * @param url 目标 URL
 * @return 响应数据缓冲区，调用者负责用 ipa_buf_free() 释放，失败时返回 NULL
 */
struct ipa_buf *ipa_http_req(void *http_ctx, const struct ipa_buf *req, const char *url);

/**
 * @brief 关闭 HTTP 连接
 * 
 * 关闭活动的 HTTP 连接但不释放上下文。
 * 
 * @param http_ctx HTTP 上下文指针
 */
void ipa_http_close(void *http_ctx);

/**
 * @brief 释放 HTTP 上下文
 * 
 * 完全释放 HTTP 客户端上下文及其所有资源。
 * 
 * @param http_ctx HTTP 上下文指针
 */
void ipa_http_free(void *http_ctx);

/** @} */  /* 结束 HTTP 模块组 */
