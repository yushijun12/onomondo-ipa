/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Author: IPAd Development Team
 *
 * See also: GSMA SGP.32, section 3.1.1.2: eIM Package Injection
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

struct ipa_context;

/**
 * @brief 处理 eIM Package Injection 流程
 * 
 * 本函数解析外部实体提供的 TransferEimPackageRequest，执行相应的操作，
 * 并生成 ProvideEimPackageResult 响应。支持以下包类型：
 * - EuiccPackageRequest: 触发 eUICC 配置文件下载
 * - IpaEuiccDataRequest: 请求 IPA/eUICC 数据
 * - ProfileDownloadTriggerRequest: 触发间接配置文件下载
 * - EimAcknowledgements: eIM 确认（无需特殊处理）
 * 
 * @param ctx IPAd 上下文指针
 * @param package 指向包含 TransferEimPackageRequest ASN.1 DER 编码数据的缓冲区
 * @param package_size 包数据大小（字节）
 * @return 0 成功，负值表示错误代码：
 *         -EINVAL: 无效的包格式或解码失败
 *         -ENOMEM: 内存分配失败
 *         其他：底层处理函数返回的错误代码
 * 
 * @note 此函数是同步的，会在返回前完成所有处理
 * @note 调用者负责确保 package 缓冲区在函数调用期间保持有效
 * 
 * @see GSMA SGP.32 v1.2 §3.1.1.2
 * @see GSMA SGP.32 v1.2 §5.14.5 (TransferEimPackageRequest)
 * @see GSMA SGP.32 v1.2 §5.14.6 (ProvideEimPackageResult)
 */
int ipa_proc_eim_pkg_inject(struct ipa_context *ctx, const uint8_t *package, size_t package_size);
