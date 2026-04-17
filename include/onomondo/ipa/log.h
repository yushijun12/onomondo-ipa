/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file log.h
 * @brief 日志系统接口
 * 
 * 本文件定义了 IPAd 的日志系统接口，支持分级日志和子系统分类。
 */

#pragma once

#include <stdio.h>
#include <stdint.h>

/** @defgroup LOG 日志模块
 *  @{
 */

/**
 * @brief 日志子系统枚举
 * 
 * 定义不同的日志子系统，用于分类日志输出。
 */
enum log_subsys {
SMAIN,      /*!< 主程序模块 */
SHTTP,      /*!< HTTP 通信模块 */
SCARD,      /*!< 智能卡模块 (同 SSCARD) */
SSCARD = SCARD, /*!< 智能卡模块别名 (向后兼容) */
SIPA,       /*!< IPAd 核心模块 */
SES10X,     /*!< ES10x 接口模块 */
SES10B,     /*!< ES10b 接口模块 */
SEUICC,     /*!< eUICC 操作模块 */
SESIPA,     /*!< ESIPA 协议模块 */
SCOAP,      /*!< CoAP 通信模块 */
SDP,        /*!< SM-DP+ 接口模块 (ES9+) */
_NUM_LOG_SUBSYS  /*!< 子系统数量（用于边界检查） */
};

/**
 * @brief 日志级别枚举
 * 
 * 定义日志的严重级别，从错误到调试。
 */
enum log_level {
LERROR,     /*!< 错误级别：严重影响功能的问题 */
LINFO,      /*!< 信息级别：正常操作信息 */
LDEBUG,     /*!< 调试级别：详细调试信息 */
_NUM_LOG_LEVEL  /*!< 日志级别数量（用于边界检查） */
};

/**
 * @brief 打印日志宏
 * 
 * 自动包含文件名和行号信息的日志打印宏。
 * 
 * @param subsys 日志子系统标识符（@ref log_subsys）
 * @param level 日志级别标识符（@ref log_level）
 * @param fmt 格式化字符串
 * @param ... 格式化参数
 */
#define IPA_LOGP(subsys, level, fmt, args...) \
ipa_logp(subsys, level, __FILE__, __LINE__, fmt, ## args)

/**
 * @brief 打印日志函数
 * 
 * 底层日志打印函数，通常通过 @ref IPA_LOGP 宏调用。
 * 
 * @param subsys 日志子系统标识符
 * @param level 日志级别标识符
 * @param file 源文件名（由宏自动填入）
 * @param line 源文件行号（由宏自动填入）
 * @param format 格式化字符串
 * @param ... 格式化参数
 */
void ipa_logp(uint32_t subsys, uint32_t level, const char *file, int line,
      const char *format, ...)
    __attribute__((format(printf, 5, 6)));

/** @} */  /* 结束 LOG 模块组 */
