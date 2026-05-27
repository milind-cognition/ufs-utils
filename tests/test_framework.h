/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Minimal unit test framework for ufs-utils */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int _test_pass_count;
static int _test_fail_count;
static int _test_total_count;

#define TEST_INIT() do { \
	_test_pass_count = 0; \
	_test_fail_count = 0; \
	_test_total_count = 0; \
} while (0)

#define TEST_ASSERT(cond, msg) do { \
	_test_total_count++; \
	if (!(cond)) { \
		fprintf(stderr, "  FAIL: %s:%d: %s\n", __FILE__, __LINE__, msg); \
		_test_fail_count++; \
	} else { \
		_test_pass_count++; \
	} \
} while (0)

#define TEST_ASSERT_EQ(a, b, msg) do { \
	_test_total_count++; \
	if ((a) != (b)) { \
		fprintf(stderr, "  FAIL: %s:%d: %s (got %ld, expected %ld)\n", \
			__FILE__, __LINE__, msg, (long)(a), (long)(b)); \
		_test_fail_count++; \
	} else { \
		_test_pass_count++; \
	} \
} while (0)

#define TEST_ASSERT_STR_EQ(a, b, msg) do { \
	_test_total_count++; \
	if (strcmp((a), (b)) != 0) { \
		fprintf(stderr, "  FAIL: %s:%d: %s (got \"%s\", expected \"%s\")\n", \
			__FILE__, __LINE__, msg, (a), (b)); \
		_test_fail_count++; \
	} else { \
		_test_pass_count++; \
	} \
} while (0)

#define TEST_ASSERT_NULL(ptr, msg) do { \
	_test_total_count++; \
	if ((ptr) != NULL) { \
		fprintf(stderr, "  FAIL: %s:%d: %s (expected NULL)\n", \
			__FILE__, __LINE__, msg); \
		_test_fail_count++; \
	} else { \
		_test_pass_count++; \
	} \
} while (0)

#define TEST_ASSERT_NOT_NULL(ptr, msg) do { \
	_test_total_count++; \
	if ((ptr) == NULL) { \
		fprintf(stderr, "  FAIL: %s:%d: %s (got NULL)\n", \
			__FILE__, __LINE__, msg); \
		_test_fail_count++; \
	} else { \
		_test_pass_count++; \
	} \
} while (0)

#define TEST_ASSERT_MEM_EQ(a, b, len, msg) do { \
	_test_total_count++; \
	if (memcmp((a), (b), (len)) != 0) { \
		fprintf(stderr, "  FAIL: %s:%d: %s (memory mismatch)\n", \
			__FILE__, __LINE__, msg); \
		_test_fail_count++; \
	} else { \
		_test_pass_count++; \
	} \
} while (0)

#define RUN_TEST(func) do { \
	printf("  Running %s...\n", #func); \
	func(); \
} while (0)

#define TEST_REPORT(suite_name) do { \
	printf("\n[%s] Results: %d/%d passed", \
		suite_name, _test_pass_count, _test_total_count); \
	if (_test_fail_count > 0) \
		printf(", %d FAILED", _test_fail_count); \
	printf("\n"); \
} while (0)

#define TEST_RESULT() (_test_fail_count == 0 ? 0 : 1)

#endif /* TEST_FRAMEWORK_H */
