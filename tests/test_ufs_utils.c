// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for core utility functions in ufs.c */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>

#include "../ufs.h"
#include "../options.h"

/* ========== str_to_long tests ========== */

START_TEST(test_str_to_long_decimal)
{
	long result = 0;
	optarg = "42";
	ck_assert_int_eq(str_to_long("42", 10, &result), OK);
	ck_assert_int_eq(result, 42);
}
END_TEST

START_TEST(test_str_to_long_hex_with_prefix)
{
	long result = 0;
	optarg = "0xFF";
	ck_assert_int_eq(str_to_long("0xFF", 16, &result), OK);
	ck_assert_int_eq(result, 255);
}
END_TEST

START_TEST(test_str_to_long_hex_base_zero)
{
	long result = 0;
	optarg = "0x1A";
	ck_assert_int_eq(str_to_long("0x1A", 0, &result), OK);
	ck_assert_int_eq(result, 26);
}
END_TEST

START_TEST(test_str_to_long_zero)
{
	long result = -1;
	optarg = "0";
	ck_assert_int_eq(str_to_long("0", 10, &result), OK);
	ck_assert_int_eq(result, 0);
}
END_TEST

START_TEST(test_str_to_long_negative)
{
	long result = 0;
	optarg = "-100";
	ck_assert_int_eq(str_to_long("-100", 10, &result), OK);
	ck_assert_int_eq(result, -100);
}
END_TEST

START_TEST(test_str_to_long_null_nptr)
{
	long result = 0;
	ck_assert_int_eq(str_to_long(NULL, 10, &result), ERROR);
}
END_TEST

START_TEST(test_str_to_long_null_result)
{
	optarg = "42";
	ck_assert_int_eq(str_to_long("42", 10, NULL), ERROR);
}
END_TEST

START_TEST(test_str_to_long_invalid_string)
{
	long result = 0;
	optarg = "abc";
	ck_assert_int_eq(str_to_long("abc", 10, &result), ERROR);
}
END_TEST

START_TEST(test_str_to_long_partial_conversion)
{
	long result = 0;
	optarg = "42xyz";
	ck_assert_int_eq(str_to_long("42xyz", 10, &result), ERROR);
}
END_TEST

START_TEST(test_str_to_long_empty_string)
{
	long result = 0;
	optarg = "";
	ck_assert_int_eq(str_to_long("", 10, &result), ERROR);
}
END_TEST

START_TEST(test_str_to_long_overflow)
{
	long result = 0;
	char buf[64];
	snprintf(buf, sizeof(buf), "%ld0", LONG_MAX);
	optarg = buf;
	ck_assert_int_eq(str_to_long(buf, 10, &result), ERROR);
}
END_TEST

START_TEST(test_str_to_long_max_value)
{
	long result = 0;
	char buf[64];
	snprintf(buf, sizeof(buf), "%ld", LONG_MAX);
	optarg = buf;
	/* LONG_MAX triggers the overflow check in str_to_long */
	ck_assert_int_eq(str_to_long(buf, 10, &result), ERROR);
}
END_TEST

START_TEST(test_str_to_long_octal)
{
	long result = 0;
	optarg = "077";
	ck_assert_int_eq(str_to_long("077", 0, &result), OK);
	ck_assert_int_eq(result, 63);
}
END_TEST

/* ========== write_file tests ========== */

START_TEST(test_write_file_success)
{
	const char *tmpfile = "/tmp/test_ufs_write.bin";
	const char data[] = "Hello UFS";
	int rc;

	rc = write_file(tmpfile, data, sizeof(data));
	ck_assert_int_eq(rc, 0);

	/* Verify file contents */
	FILE *f = fopen(tmpfile, "rb");
	ck_assert_ptr_nonnull(f);

	char buf[64] = {0};
	size_t nread = fread(buf, 1, sizeof(data), f);
	fclose(f);
	ck_assert_int_eq(nread, sizeof(data));
	ck_assert_mem_eq(buf, data, sizeof(data));

	unlink(tmpfile);
}
END_TEST

START_TEST(test_write_file_binary_data)
{
	const char *tmpfile = "/tmp/test_ufs_write_bin.bin";
	unsigned char data[] = {0x00, 0xFF, 0xAB, 0xCD, 0x01, 0x02};
	int rc;

	rc = write_file(tmpfile, data, sizeof(data));
	ck_assert_int_eq(rc, 0);

	FILE *f = fopen(tmpfile, "rb");
	ck_assert_ptr_nonnull(f);

	unsigned char buf[64] = {0};
	size_t nread = fread(buf, 1, sizeof(data), f);
	fclose(f);
	ck_assert_int_eq(nread, sizeof(data));
	ck_assert_mem_eq(buf, data, sizeof(data));

	unlink(tmpfile);
}
END_TEST

START_TEST(test_write_file_invalid_path)
{
	const char data[] = "test";
	int rc = write_file("/nonexistent/path/file.bin", data, sizeof(data));
	ck_assert_int_lt(rc, 0);
}
END_TEST

START_TEST(test_write_file_zero_length)
{
	const char *tmpfile = "/tmp/test_ufs_write_zero.bin";
	const char data[] = "x";
	int rc;

	rc = write_file(tmpfile, data, 0);
	ck_assert_int_eq(rc, 0);

	struct stat st;
	ck_assert_int_eq(stat(tmpfile, &st), 0);
	ck_assert_int_eq(st.st_size, 0);

	unlink(tmpfile);
}
END_TEST

START_TEST(test_write_file_overwrite_existing)
{
	const char *tmpfile = "/tmp/test_ufs_write_overwrite.bin";
	const char data1[] = "first data block here";
	const char data2[] = "new";
	int rc;

	rc = write_file(tmpfile, data1, strlen(data1));
	ck_assert_int_eq(rc, 0);

	rc = write_file(tmpfile, data2, strlen(data2));
	ck_assert_int_eq(rc, 0);

	/* Verify it was truncated and overwritten */
	struct stat st;
	ck_assert_int_eq(stat(tmpfile, &st), 0);
	ck_assert_int_eq(st.st_size, strlen(data2));

	unlink(tmpfile);
}
END_TEST

/* ========== Test Suite Setup ========== */

Suite *ufs_utils_suite(void)
{
	Suite *s;
	TCase *tc_str_to_long;
	TCase *tc_write_file;

	s = suite_create("UFS Utils Core");

	/* str_to_long test case */
	tc_str_to_long = tcase_create("str_to_long");
	tcase_add_test(tc_str_to_long, test_str_to_long_decimal);
	tcase_add_test(tc_str_to_long, test_str_to_long_hex_with_prefix);
	tcase_add_test(tc_str_to_long, test_str_to_long_hex_base_zero);
	tcase_add_test(tc_str_to_long, test_str_to_long_zero);
	tcase_add_test(tc_str_to_long, test_str_to_long_negative);
	tcase_add_test(tc_str_to_long, test_str_to_long_null_nptr);
	tcase_add_test(tc_str_to_long, test_str_to_long_null_result);
	tcase_add_test(tc_str_to_long, test_str_to_long_invalid_string);
	tcase_add_test(tc_str_to_long, test_str_to_long_partial_conversion);
	tcase_add_test(tc_str_to_long, test_str_to_long_empty_string);
	tcase_add_test(tc_str_to_long, test_str_to_long_overflow);
	tcase_add_test(tc_str_to_long, test_str_to_long_max_value);
	tcase_add_test(tc_str_to_long, test_str_to_long_octal);
	suite_add_tcase(s, tc_str_to_long);

	/* write_file test case */
	tc_write_file = tcase_create("write_file");
	tcase_add_test(tc_write_file, test_write_file_success);
	tcase_add_test(tc_write_file, test_write_file_binary_data);
	tcase_add_test(tc_write_file, test_write_file_invalid_path);
	tcase_add_test(tc_write_file, test_write_file_zero_length);
	tcase_add_test(tc_write_file, test_write_file_overwrite_existing);
	suite_add_tcase(s, tc_write_file);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = ufs_utils_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
