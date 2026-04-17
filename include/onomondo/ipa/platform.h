/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file platform.h
 * @brief Platform Abstraction Layer (PAL) for IPAd
 * 
 * This header defines the platform abstraction layer that allows IPAd to run
 * on different platforms by providing implementations for HTTP and smart card
 * access.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Buffer structure for data exchange
 */
struct ipa_buf {
    size_t len;        /**< Total allocated length */
    size_t data_len;   /**< Actual data length */
    uint8_t data[];    /**< Flexible array member for data */
};

/*****************************************************************************
 * HTTP Interface
 *****************************************************************************/

/**
 * @brief Initialize HTTP client context
 * 
 * @param cabundle Path to CA bundle file (NULL for default)
 * @param no_verify Disable SSL certificate verification
 * @return Opaque HTTP context pointer, or NULL on failure
 */
void *ipa_http_init(const char *cabundle, bool no_verify);

/**
 * @brief Perform HTTP request
 * 
 * @param http_ctx HTTP context from ipa_http_init()
 * @param req Request buffer (ASN.1 encoded)
 * @param url Target URL
 * @return Response buffer, or NULL on failure. Caller must free with ipa_buf_free().
 */
struct ipa_buf *ipa_http_req(void *http_ctx, const struct ipa_buf *req, const char *url);

/**
 * @brief Close HTTP connection
 * 
 * @param http_ctx HTTP context from ipa_http_init()
 */
void ipa_http_close(void *http_ctx);

/**
 * @brief Free HTTP context
 * 
 * @param http_ctx HTTP context from ipa_http_init()
 */
void ipa_http_free(void *http_ctx);

/*****************************************************************************
 * Smart Card Interface
 *****************************************************************************/

/**
 * @brief Initialize smart card context
 * 
 * @param reader_num Reader number (0-based index)
 * @return Opaque smart card context pointer, or NULL on failure
 */
void *ipa_scard_init(unsigned int reader_num);

/**
 * @brief Reset smart card
 * 
 * @param scard_ctx Smart card context from ipa_scard_init()
 * @return 0 on success, negative on failure
 */
int ipa_scard_reset(void *scard_ctx);

/**
 * @brief Get ATR (Answer To Reset) from smart card
 * 
 * @param scard_ctx Smart card context from ipa_scard_init()
 * @param atr Buffer to store ATR. Caller must allocate.
 * @return 0 on success, negative on failure
 */
int ipa_scard_atr(void *scard_ctx, struct ipa_buf *atr);

/**
 * @brief Transmit APDU command to smart card
 * 
 * @param scard_ctx Smart card context from ipa_scard_init()
 * @param res Buffer to store response. Caller must allocate.
 * @param req Request buffer containing APDU command
 * @return 0 on success, negative on failure
 */
int ipa_scard_transceive(void *scard_ctx, struct ipa_buf *res,
                         const struct ipa_buf *req);

/**
 * @brief Free smart card context
 * 
 * @param scard_ctx Smart card context from ipa_scard_init()
 * @return 0 on success, negative on failure
 */
int ipa_scard_free(void *scard_ctx);

/*****************************************************************************
 * Memory Management
 *****************************************************************************/

/**
 * @brief Allocate memory
 * 
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void *ipa_malloc(size_t size);

/**
 * @brief Allocate and zero-initialize memory
 * 
 * @param nmemb Number of elements
 * @param size Size of each element
 * @return Pointer to allocated memory, or NULL on failure
 */
void *ipa_calloc(size_t nmemb, size_t size);

/**
 * @brief Reallocate memory
 * 
 * @param ptr Pointer to previously allocated memory
 * @param size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
void *ipa_realloc(void *ptr, size_t size);

/**
 * @brief Free memory
 * 
 * @param ptr Pointer to memory to free
 */
void ipa_free(void *ptr);

/*****************************************************************************
 * Buffer Management
 *****************************************************************************/

/**
 * @brief Allocate a new buffer
 * 
 * @param size Initial size of the buffer
 * @return Pointer to new buffer, or NULL on failure
 */
struct ipa_buf *ipa_buf_alloc(size_t size);

/**
 * @brief Allocate a new buffer and copy data
 * 
 * @param data_len Length of data to copy
 * @param data Data to copy
 * @return Pointer to new buffer, or NULL on failure
 */
struct ipa_buf *ipa_buf_alloc_data(size_t data_len, const uint8_t *data);

/**
 * @brief Reallocate buffer to new size
 * 
 * @param buf Buffer to reallocate
 * @param new_len New total length
 * @return Pointer to reallocated buffer, or NULL on failure
 */
struct ipa_buf *ipa_buf_realloc(struct ipa_buf *buf, size_t new_len);

/**
 * @brief Free buffer
 * 
 * @param buf Buffer to free
 */
void ipa_buf_free(struct ipa_buf *buf);

/**
 * @brief Deserialize buffer from binary data
 * 
 * @param data Binary data containing serialized buffer
 * @param len Length of binary data
 * @return Deserialized buffer, or NULL on failure
 */
struct ipa_buf *ipa_buf_deserialize(const uint8_t *data, size_t len);

/*****************************************************************************
 * Logging Interface
 *****************************************************************************/

/**
 * @brief Log levels
 */
enum ipa_log_level {
    IPA_LOG_ERROR = 0,
    IPA_LOG_WARNING = 1,
    IPA_LOG_INFO = 2,
    IPA_LOG_DEBUG = 3,
};

/**
 * @brief Log a message
 * 
 * @param level Log level
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void ipa_log(enum ipa_log_level level, const char *format, ...);

/**
 * @brief Set minimum log level
 * 
 * @param level Minimum level to log
 */
void ipa_set_log_level(enum ipa_log_level level);

#ifdef __cplusplus
}
#endif
