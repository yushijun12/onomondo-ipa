/*
 * Copyright (c) 2025 Onomondo ApS & sysmocom - s.f.m.c. GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file test_ipa_buf.c
 * @brief Unit tests for ipa_buf buffer management functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <onomondo/ipa/utils.h>
#include <onomondo/ipa/log.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) static void name(void)
#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    tests_run++; \
    name(); \
    tests_passed++; \
    printf("PASSED\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAILED at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        exit(1); \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NEQ(a, b) ASSERT((a) != (b))
#define ASSERT_NOTNULL(ptr) ASSERT((ptr) != NULL)

TEST(test_buf_alloc_empty)
{
    struct ipa_buf *buf = ipa_buf_alloc(0);
    ASSERT_NOTNULL(buf);
    ASSERT_EQ(buf->len, 0);
    ASSERT_EQ(buf->data_len, 0);
    ipa_buf_free(buf);
}

TEST(test_buf_alloc_with_size)
{
    struct ipa_buf *buf = ipa_buf_alloc(100);
    ASSERT_NOTNULL(buf);
    ASSERT_EQ(buf->len, 100);
    ASSERT_EQ(buf->data_len, 100);
    ipa_buf_free(buf);
}

TEST(test_buf_alloc_data)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    struct ipa_buf *buf = ipa_buf_alloc_data(sizeof(data), data);
    
    ASSERT_NOTNULL(buf);
    ASSERT_EQ(buf->len, sizeof(data));
    ASSERT_EQ(buf->data_len, sizeof(data));
    ASSERT_EQ(memcmp(buf->data, data, sizeof(data)), 0);
    
    ipa_buf_free(buf);
}

TEST(test_buf_realloc_grow)
{
    struct ipa_buf *buf = ipa_buf_alloc(10);
    ASSERT_NOTNULL(buf);
    
    buf->data[0] = 0x42;
    buf->data_len = 1;
    
    buf = ipa_buf_realloc(buf, 100);
    ASSERT_NOTNULL(buf);
    ASSERT_EQ(buf->len, 100);
    ASSERT_EQ(buf->data[0], 0x42);
    
    ipa_buf_free(buf);
}

TEST(test_buf_realloc_shrink)
{
    struct ipa_buf *buf = ipa_buf_alloc(100);
    ASSERT_NOTNULL(buf);
    
    buf = ipa_buf_realloc(buf, 10);
    ASSERT_NOTNULL(buf);
    ASSERT_EQ(buf->len, 10);
    
    ipa_buf_free(buf);
}

TEST(test_buf_serialize_deserialize)
{
    uint8_t original_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    struct ipa_buf *original = ipa_buf_alloc_data(sizeof(original_data), original_data);
    ASSERT_NOTNULL(original);
    
    /* Serialize */
    size_t ser_len = sizeof(struct ipa_buf) + original->data_len;
    uint8_t *ser_buf = malloc(ser_len);
    ASSERT_NOTNULL(ser_buf);
    memcpy(ser_buf, original, ser_len);
    
    /* Deserialize */
    struct ipa_buf *restored = ipa_buf_deserialize(ser_buf, ser_len);
    ASSERT_NOTNULL(restored);
    
    ASSERT_EQ(restored->len, original->len);
    ASSERT_EQ(restored->data_len, original->data_len);
    ASSERT_EQ(memcmp(restored->data, original->data, original->data_len), 0);
    
    ipa_buf_free(restored);
    free(ser_buf);
    ipa_buf_free(original);
}

TEST(test_buf_multiple_operations)
{
    struct ipa_buf *buf = ipa_buf_alloc(50);
    ASSERT_NOTNULL(buf);
    
    /* Write some data */
    for (int i = 0; i < 10; i++) {
        buf->data[i] = i;
    }
    buf->len = 10;
    
    /* Realloc larger */
    buf = ipa_buf_realloc(buf, 200);
    ASSERT_NOTNULL(buf);
    ASSERT_EQ(buf->len, 10);
    ASSERT_EQ(buf->data_len, 200);
    
    /* Verify data preserved */
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(buf->data[i], i);
    }
    
    /* Add more data */
    for (int i = 10; i < 50; i++) {
        buf->data[i] = i;
    }
    buf->len = 50;
    
    /* Realloc smaller but still fits data */
    buf = ipa_buf_realloc(buf, 100);
    ASSERT_NOTNULL(buf);
    ASSERT_EQ(buf->len, 50);
    ASSERT_EQ(buf->data_len, 100);
    
    /* Verify all data preserved */
    for (int i = 0; i < 50; i++) {
        ASSERT_EQ(buf->data[i], i);
    }
    
    ipa_buf_free(buf);
}

TEST(test_buf_dup)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    struct ipa_buf *original = ipa_buf_alloc_data(sizeof(data), data);
    ASSERT_NOTNULL(original);
    
    struct ipa_buf *duplicate = ipa_buf_dup(original);
    ASSERT_NOTNULL(duplicate);
    
    ASSERT_EQ(duplicate->len, original->len);
    ASSERT_EQ(duplicate->data_len, original->data_len);
    ASSERT_EQ(memcmp(duplicate->data, original->data, original->data_len), 0);
    
    /* Modify original, ensure duplicate is unchanged */
    original->data[0] = 0xFF;
    ASSERT_EQ(duplicate->data[0], 0x01);
    
    ipa_buf_free(duplicate);
    ipa_buf_free(original);
}

TEST(test_buf_copy)
{
    uint8_t data[] = {0xAA, 0xBB, 0xCC};
    struct ipa_buf *original = ipa_buf_alloc_data(sizeof(data), data);
    ASSERT_NOTNULL(original);
    
    original->len = 2; /* Only use first 2 bytes */
    
    struct ipa_buf *copy = ipa_buf_copy(original);
    ASSERT_NOTNULL(copy);
    
    ASSERT_EQ(copy->len, original->len);
    ASSERT_EQ(copy->data_len, original->len);
    ASSERT_EQ(memcmp(copy->data, original->data, copy->len), 0);
    
    ipa_buf_free(copy);
    ipa_buf_free(original);
}

int main(void)
{
    printf("=== IPA Buffer Unit Tests ===\n\n");
    
    RUN_TEST(test_buf_alloc_empty);
    RUN_TEST(test_buf_alloc_with_size);
    RUN_TEST(test_buf_alloc_data);
    RUN_TEST(test_buf_realloc_grow);
    RUN_TEST(test_buf_realloc_shrink);
    RUN_TEST(test_buf_serialize_deserialize);
    RUN_TEST(test_buf_multiple_operations);
    RUN_TEST(test_buf_dup);
    RUN_TEST(test_buf_copy);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
