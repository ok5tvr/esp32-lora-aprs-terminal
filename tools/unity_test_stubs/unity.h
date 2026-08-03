#pragma once
#include <cassert>
#include <cmath>
#include <cstring>
#define UNITY_BEGIN() do {} while (0)
#define UNITY_END() 0
#define RUN_TEST(fn) do { fn(); } while (0)
#define TEST_ASSERT_TRUE(v) assert((v))
#define TEST_ASSERT_FALSE(v) assert(!(v))
#define TEST_ASSERT_EQUAL(a,b) assert((a)==(b))
#define TEST_ASSERT_EQUAL_CHAR(a,b) assert((a)==(b))
#define TEST_ASSERT_EQUAL_HEX8(a,b) assert((a)==(b))
#define TEST_ASSERT_EQUAL_UINT(a,b) assert((a)==(b))
#define TEST_ASSERT_EQUAL_UINT8(a,b) assert((a)==(b))
#define TEST_ASSERT_EQUAL_UINT16(a,b) assert((a)==(b))
#define TEST_ASSERT_EQUAL_UINT32(a,b) assert((a)==(b))
#define TEST_ASSERT_EQUAL_STRING(a,b) assert(std::strcmp((a),(b))==0)
#define TEST_ASSERT_FLOAT_WITHIN(d,a,b) assert(std::fabs(static_cast<double>(a)-static_cast<double>(b)) <= static_cast<double>(d))
#define TEST_ASSERT_DOUBLE_WITHIN(d,a,b) assert(std::fabs(static_cast<double>(a)-static_cast<double>(b)) <= static_cast<double>(d))
