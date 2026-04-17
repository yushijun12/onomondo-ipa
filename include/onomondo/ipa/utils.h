/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file utils.h
 * @brief 工具函数和缓冲区管理
 * 
 * 本文件定义了 IPAd 的工具函数、缓冲区管理结构和相关操作接口。
 * ipa_buf 是 IPAd 中用于数据交换的核心数据结构。
 */

#pragma once

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "mem.h"
#include "log.h"

/** @defgroup UTILS 工具模块
 *  @{
 */

/**
 * @brief 获取数组元素数量
 * 
 * 编译时计算数组的元素个数。
 * 
 * @param x 数组名
 * @return 数组元素数量
 */
#define IPA_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/**
 * @brief 分配并零初始化对象内存
 * 
 * 为指定类型的对象分配内存，确保分配成功并初始化为零。
 * 
 * @param obj 要分配的对象类型（struct）
 * @return 指向已分配内存的指针（已断言检查非 NULL）
 */
#define IPA_ALLOC_ZERO(obj) ({ \
obj *__ptr; \
__ptr = IPA_ALLOC(obj); \
assert(__ptr); \
memset(__ptr, 0, sizeof(*__ptr)); \
__ptr; \
})

/**
 * @brief 分配 N 字节并零初始化
 * 
 * 分配指定数量的字节并确保内存初始化为零。
 * 
 * @param n 要分配的字节数
 * @return 指向已分配内存的指针（已断言检查非 NULL）
 */
#define IPA_ALLOC_N_ZERO(n) ({ \
void *__ptr; \
__ptr = IPA_ALLOC_N(n); \
assert(__ptr); \
memset(__ptr, 0, n); \
__ptr; \
})

/**
 * @brief 将二进制数据转换为十六进制字符串
 * 
 * @param data 输入二进制数据
 * @param len 数据长度
 * @return 十六进制字符串（调用者负责释放）
 */
char *ipa_hexdump(const uint8_t *data, size_t len);

/**
 * @brief 缓冲区结构
 * 
 * IPAd 中用于数据交换的核心结构。包含数据指针、总长度和有效数据长度。
 * 采用灵活内存布局：header + data 连续存储。
 */
struct ipa_buf {
uint8_t *data;        /*!< 指向已分配内存的指针 */
size_t data_len;      /*!< 已分配内存的实际长度（由系统设置，用户不应修改） */
size_t len;           /*!< 有效数据长度（用户可修改） */
};

/**
 * @brief 生成 ipa_buf 的十六进制转储字符串
 * 
 * @param buf ipa_buf 对象指针
 * @return 人类可读的十六进制字符串，NULL 时返回 "(null)"
 */
static inline char *ipa_buf_hexdump(const struct ipa_buf *buf)
{
if (!buf)
return "(null)";
return ipa_hexdump(buf->data, buf->len);
}

/**
 * @brief 多行十六进制转储（原始数据）
 * 
 * @param data 输入数据
 * @param len 数据长度
 * @param width 每行显示的字节数
 * @param indent 缩进空格数
 * @param log_subsys 日志子系统
 * @param log_level 日志级别
 */
void ipa_hexdump_multiline(const uint8_t *data, size_t len, size_t width, uint8_t indent, enum log_subsys log_subsys,
   enum log_level log_level);

/**
 * @brief 多行十六进制转储（ipa_buf）
 * 
 * @param buf ipa_buf 对象
 * @param width 每行显示的字节数
 * @param indent 缩进空格数
 * @param log_subsys 日志子系统
 * @param log_level 日志级别
 */
void ipa_buf_hexdump_multiline(const struct ipa_buf *buf, size_t width, uint8_t indent, enum log_subsys log_subsys,
       enum log_level log_level);

/**
 * @brief 分配新的 ipa_buf
 * 
 * 分配一个新的缓冲区对象，包含 header 和数据区。
 * 新缓冲区的 len 初始化为 0。
 * 
 * @param len 数据区字节数
 * @return 指向新 ipa_buf 的指针（已断言检查非 NULL）
 */
static inline struct ipa_buf *ipa_buf_alloc(size_t len)
{
struct ipa_buf *buf = IPA_ALLOC_N(sizeof(*buf) + len);
assert(buf);

memset(buf, 0, sizeof(*buf));
buf->data = (uint8_t *) buf + sizeof(*buf);
buf->data_len = len;

/* 新分配的 ipa_buf 自然有 0 字节的有效数据 */
buf->len = 0;

return buf;
}

/**
 * @brief 重新分配 ipa_buf 大小
 * 
 * 调整现有缓冲区的大小，保留已有数据。
 * 
 * @param buf 要调整的缓冲区
 * @param len 新的数据区字节数
 * @return 指向重新分配的 ipa_buf 的指针（已断言检查非 NULL）
 */
static inline struct ipa_buf *ipa_buf_realloc(struct ipa_buf *buf, size_t len)
{
buf = IPA_REALLOC(buf, sizeof(*buf) + len);
assert(buf);

buf->data = (uint8_t *) buf + sizeof(*buf);
buf->data_len = len;
memset(buf->data + buf->len, 0, len - buf->len);

return buf;
}

/**
 * @brief 静态分配 ipa_buf 宏
 * 
 * 创建静态分配的 ipa_buf 对象（不能使用 ipa_buf_free 或 ipa_buf_realloc）。
 * 
 * @param name ipa_buf 的符号名
 * @param size ipa_buf 的大小
 */
#define IPA_BUF_STATIC(name, size) \
uint8_t __name_buf[size]; \
struct ipa_buf name = { __name_buf, size, 0 };

/**
 * @brief 分配 ipa_buf 并用数据初始化
 * 
 * @param len 数据区字节数
 * @param data 要复制的数据
 * @return 指向新 ipa_buf 的指针（已断言检查非 NULL）
 */
static inline struct ipa_buf *ipa_buf_alloc_data(size_t len, uint8_t *data)
{
struct ipa_buf *buf = ipa_buf_alloc(len);
assert(buf);

buf->len = len;
memcpy(buf->data, data, len);

return buf;
}

/**
 * @brief 复制 ipa_buf（完整副本）
 * 
 * 创建另一个 ipa_buf 对象的精确副本（包括未使用的空间）。
 * 
 * @param buf 要复制的 ipa_buf 对象
 * @return 指向新 ipa_buf 的指针
 */
static inline struct ipa_buf *ipa_buf_dup(const struct ipa_buf *buf)
{
struct ipa_buf *buf_dup = ipa_buf_alloc(buf->data_len);
memcpy(buf_dup->data, buf->data, buf->data_len);
buf_dup->len = buf->len;
return buf_dup;
}

/**
 * @brief 复制 ipa_buf（仅有效数据）
 * 
 * 创建另一个 ipa_buf 对象的副本，仅复制有效数据部分。
 * 
 * @param buf 要复制的 ipa_buf 对象
 * @return 指向新 ipa_buf 的指针
 */
static inline struct ipa_buf *ipa_buf_copy(const struct ipa_buf *buf)
{
struct ipa_buf *buf_dup = ipa_buf_alloc(buf->len);
memcpy(buf_dup->data, buf->data, buf->len);
buf_dup->len = buf->len;
return buf_dup;
}

/**
 * @brief 从用户内存分配并复制
 * 
 * 从用户提供的内存分配新的 ipa_buf 并复制数据。
 * 
 * @param in 用户提供的内存
 * @param len 要复制的字节数
 * @return 指向新 ipa_buf 的指针
 */
static inline struct ipa_buf *ipa_buf_alloc_and_cpy(const uint8_t *in, size_t len)
{
struct ipa_buf *buf = ipa_buf_alloc(len);
memcpy(buf->data, in, len);
buf->len = len;
return buf;
}

/**
 * @brief 追加数据到 ipa_buf
 * 
 * 将数据追加到现有缓冲区的末尾。
 * 
 * @param buf 目标 ipa_buf
 * @param in 要复制的用户内存
 * @param len 要复制的字节数
 */
static inline void ipa_buf_cpy(struct ipa_buf *buf, const uint8_t *in, size_t len)
{
assert(buf->len + len <= buf->data_len);
memcpy(buf->data + buf->len, in, len);
buf->len += len;
}

/**
 * @brief 赋值外部内存到 ipa_buf
 * 
 * 将已存在的内存区域赋值给未初始化的 ipa_buf 结构。
 * 注意：结果 ipa_buf 不能用 ipa_buf_free() 释放。
 * 
 * @param buf 未初始化的 ipa_buf（可能是静态分配的）
 * @param data 要赋值给用户提供的内存
 * @param len 用户提供的内存长度
 */
static inline void ipa_buf_assign(struct ipa_buf *buf, const uint8_t *data, size_t len)
{
/*! 此函数的目的是提供一种简单的方式将已存在的内存位置
 *  赋值给 ipa_buf 结构。结果是有效的 ipa_buf 结构，但不能
 *  使用 ipa_buf_free() 释放。 */
memset(buf, 0, sizeof(*buf));
buf->data = (uint8_t *) data;
buf->data_len = len;
buf->len = len;
}

/**
 * @brief 反序列化 ipa_buf
 * 
 * 从二进制数据（可能来自文件）反序列化 ipa_buf。
 * 
 * @param data 包含序列化 ipa_buf 的用户内存
 * @param len 包含序列化 ipa_buf 的用户内存长度
 * @return 反序列化的 ipa_buf 指针
 */
static inline struct ipa_buf *ipa_buf_deserialize(uint8_t *data, size_t len)
{
/*! ipa_buf 通过将其头部写入文件并在其后直接附加数据区来序列化。
 *  由于 ipa_buf_alloc 已经以这种方式分配 ipa_buf 对象，因此无需额外操作。
 *  只需将指向 ipa_buf 对象的指针传递给 memcpy，并使用 sizeof(*buf) + buf->data_len
 *  作为长度即可。 */

struct ipa_buf *buf_serialized;
struct ipa_buf *buf;

/* 这将给我们一个几乎可用的 ipa_buf（数据指针将失效） */
buf_serialized = (struct ipa_buf *)data;

/* 首先我们从序列化缓冲区中的数据分配一个新缓冲区。我们不能信任数据指针，因为
 * 这个序列化缓冲区可能来自不同机器上的不同进程，所以我们必须自己计算数据的开始位置。
 * 我们还必须确保复制完整的内存。 */
buf = ipa_buf_alloc_data(buf_serialized->data_len, (uint8_t *) buf_serialized + sizeof(*buf_serialized));

/* 原始缓冲区可能没有利用所有可用内存，所以我们恢复长度。 */
buf->len = buf_serialized->len;

return buf;
}

/**
 * @brief 释放 ipa_buf
 * 
 * @param buf 要释放的 ipa_buf 指针
 */
static inline void ipa_buf_free(struct ipa_buf *buf)
{
IPA_FREE(buf);
}

/**
 * @brief 从十六进制字符串转换为二进制
 * 
 * @param binary 输出二进制缓冲区
 * @param binary_len 二进制缓冲区长度
 * @param hexstr 输入十六进制字符串
 * @return 转换的二进制字节数
 */
size_t ipa_binary_from_hexstr(uint8_t *binary, size_t binary_len, const char *hexstr);

/** @} */  /* 结束 UTILS 模块组 */
