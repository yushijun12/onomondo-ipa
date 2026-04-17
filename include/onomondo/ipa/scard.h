/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file scard.h
 * @brief 智能卡接口
 * 
 * 本文件定义了 IPAd 与 eUICC 通信的智能卡接口。
 * 实现基于 PC/SC API，支持 APDU 传输和卡片管理。
 */

#pragma once

#include <stdint.h>

/** @defgroup SCARD 智能卡模块
 *  @{
 */

struct ipa_buf;

/**
 * @brief 初始化智能卡上下文
 * 
 * 连接指定的智能卡读卡器并创建上下文。
 * 
 * @param reader_num 读卡器编号（从 0 开始）
 * @return 智能卡上下文指针，失败时返回 NULL
 */
void *ipa_scard_init(unsigned int reader_num);

/**
 * @brief 重置智能卡
 * 
 * 对智能卡执行复位操作。
 * 
 * @param scard_ctx 智能卡上下文指针（由 ipa_scard_init() 创建）
 * @return 0 成功，负值表示错误代码
 */
int ipa_scard_reset(void *scard_ctx);

/**
 * @brief 获取 ATR（Answer To Reset）
 * 
 * 读取智能卡的 ATR 数据。
 * 
 * @param scard_ctx 智能卡上下文指针
 * @param atr 用于存储 ATR 数据的缓冲区（调用者分配）
 * @return 0 成功，负值表示错误代码
 */
int ipa_scard_atr(void *scard_ctx, struct ipa_buf *atr);

/**
 * @brief 传输 APDU 命令
 * 
 * 向智能卡发送 APDU 命令并接收响应。
 * 
 * @param scard_ctx 智能卡上下文指针
 * @param res 用于存储响应的缓冲区（调用者分配）
 * @param req 包含 APDU 命令的请求缓冲区
 * @return 0 成功，负值表示错误代码
 */
int ipa_scard_transceive(void *scard_ctx, struct ipa_buf *res,
			 const struct ipa_buf *req);

/**
 * @brief 释放智能卡上下文
 * 
 * 断开与智能卡的连接并释放所有资源。
 * 
 * @param scard_ctx 智能卡上下文指针
 * @return 0 成功，负值表示错误代码
 */
int ipa_scard_free(void *scard_ctx);

/** @} */  /* 结束 SCARD 模块组 */
