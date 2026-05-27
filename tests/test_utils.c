// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for core utility functions: str_to_long, write_file, etc. */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>

#include "../ufs.h"
#include "../options.h"
#include "../unipro.h"
#include "test_helpers.h"

/* ---------- str_to_long tests ---------- */

START_TEST(test_str_to_long_decimal)
{
	long result = -1;

	optarg = "123";
	ck_assert_int_eq(str_to_long(optarg, 10, &result), OK);
	ck_assert_int_eq(result, 123);
}
END_TEST

START_TEST(test_str_to_long_hex)
{
	long result = -1;

	optarg = "0xFF";
	ck_assert_int_eq(str_to_long(optarg, 16, &result), OK);
	ck_assert_int_eq(result, 255);
}
END_TEST

START_TEST(test_str_to_long_octal)
{
	long result = -1;

	optarg = "077";
	ck_assert_int_eq(str_to_long(optarg, 8, &result), OK);
	ck_assert_int_eq(result, 63);
}
END_TEST

START_TEST(test_str_to_long_zero)
{
	long result = -1;

	optarg = "0";
	ck_assert_int_eq(str_to_long(optarg, 10, &result), OK);
	ck_assert_int_eq(result, 0);
}
END_TEST

START_TEST(test_str_to_long_negative)
{
	long result = 0;

	optarg = "-42";
	ck_assert_int_eq(str_to_long(optarg, 10, &result), OK);
	ck_assert_int_eq(result, -42);
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
	optarg = "123";
	ck_assert_int_eq(str_to_long(optarg, 10, NULL), ERROR);
}
END_TEST

START_TEST(test_str_to_long_invalid_string)
{
	long result = 0;

	optarg = "abc";
	ck_assert_int_eq(str_to_long(optarg, 10, &result), ERROR);
}
END_TEST

START_TEST(test_str_to_long_overflow)
{
	long result = 0;

	optarg = "99999999999999999999";
	ck_assert_int_eq(str_to_long(optarg, 10, &result), ERROR);
}
END_TEST

START_TEST(test_str_to_long_partial)
{
	long result = 0;

	optarg = "123abc";
	ck_assert_int_eq(str_to_long(optarg, 10, &result), ERROR);
}
END_TEST

START_TEST(test_str_to_long_empty)
{
	long result = 0;

	optarg = "";
	ck_assert_int_eq(str_to_long(optarg, 10, &result), ERROR);
}
END_TEST

/* ---------- write_file tests ---------- */

START_TEST(test_write_file_success)
{
	const char *path = "/tmp/test_write_file_success";
	const char *data = "hello ufs";
	int rc;
	char buf[64];
	FILE *fp;
	size_t n;

	rc = write_file(path, data, strlen(data));
	ck_assert_int_eq(rc, 0);

	fp = fopen(path, "r");
	ck_assert_ptr_nonnull(fp);
	n = fread(buf, 1, sizeof(buf), fp);
	fclose(fp);

	ck_assert_uint_eq(n, strlen(data));
	ck_assert_int_eq(memcmp(buf, data, strlen(data)), 0);

	unlink(path);
}
END_TEST

START_TEST(test_write_file_invalid_path)
{
	const char *data = "test";
	int rc;

	rc = write_file("/nonexistent/dir/file.txt", data, strlen(data));
	ck_assert_int_eq(rc, -ENOENT);
}
END_TEST

START_TEST(test_write_file_empty)
{
	const char *path = "/tmp/test_write_file_empty";
	int rc;
	struct stat st;

	rc = write_file(path, "", 0);
	ck_assert_int_eq(rc, 0);

	ck_assert_int_eq(stat(path, &st), 0);
	ck_assert_int_eq(st.st_size, 0);

	unlink(path);
}
END_TEST

START_TEST(test_write_file_permissions)
{
	const char *path = "/tmp/test_write_file_perms";
	const char *data = "perm";
	int rc;
	struct stat st;

	rc = write_file(path, data, strlen(data));
	ck_assert_int_eq(rc, 0);

	ck_assert_int_eq(stat(path, &st), 0);
	ck_assert_int_eq(st.st_mode & 0777, 0600);

	unlink(path);
}
END_TEST

/* ---------- get_prgname tests ---------- */

START_TEST(test_get_prgname_full_path)
{
	char input[] = "/usr/bin/ufs-utils";
	char *result = get_prgname(input);

	ck_assert_str_eq(result, "ufs-utils");
}
END_TEST

START_TEST(test_get_prgname_no_path)
{
	char input[] = "ufs-utils";
	char *result = get_prgname(input);

	ck_assert_str_eq(result, "ufs-utils");
}
END_TEST

START_TEST(test_get_prgname_trailing_slash)
{
	char input[] = "dir/";
	char *result = get_prgname(input);

	ck_assert_str_eq(result, "");
}
END_TEST

START_TEST(test_get_prgname_empty)
{
	char input[] = "";
	char *result = get_prgname(input);

	ck_assert_str_eq(result, "");
}
END_TEST

/* ---------- initialized_options tests ---------- */

START_TEST(test_initialized_options_fields_invalid)
{
	struct tool_options opts;

	initialized_options(&opts);

	ck_assert_int_eq((int)opts.config_type_inx, INVALID);
	ck_assert_int_eq(opts.idn, INVALID);
	ck_assert_int_eq(opts.opr, INVALID);
	ck_assert_int_eq(opts.index, INVALID);
	ck_assert_int_eq(opts.selector, INVALID);
	ck_assert_int_eq(opts.target, INVALID);
	ck_assert_int_eq(opts.size, INVALID);
	ck_assert_int_eq(opts.offset, INVALID);
	ck_assert_int_eq(opts.len, INVALID);
}
END_TEST

START_TEST(test_initialized_options_path_empty)
{
	struct tool_options opts;

	initialized_options(&opts);
	ck_assert_int_eq(opts.path[0], '\0');
}
END_TEST

START_TEST(test_initialized_options_keypath_empty)
{
	struct tool_options opts;

	initialized_options(&opts);
	ck_assert_int_eq(opts.keypath[0], '\0');
}
END_TEST

START_TEST(test_initialized_options_data_null)
{
	struct tool_options opts;

	initialized_options(&opts);
	ck_assert_ptr_null(opts.data);
}
END_TEST

START_TEST(test_initialized_options_sg_type)
{
	struct tool_options opts;

	initialized_options(&opts);
	ck_assert_int_eq(opts.sg_type, SG3_TYPE);
}
END_TEST

START_TEST(test_initialized_options_set_type)
{
	struct tool_options opts;

	initialized_options(&opts);
	ck_assert_int_eq(opts.set_type, ATTR_SET_NOR);
}
END_TEST

/* ---------- print_error / print_warn smoke tests ---------- */

START_TEST(test_print_error_smoke)
{
	print_error("test error %d", 42);
	print_error("simple message");
	print_error("%s %s", "hello", "world");
}
END_TEST

START_TEST(test_print_warn_smoke)
{
	print_warn("test warning %d", 99);
	print_warn("simple warning");
	print_warn("%s %s", "warn", "msg");
}
END_TEST

/* ---------- Suite creation ---------- */

Suite *util_suite(void)
{
	Suite *s;
	TCase *tc_strtol;
	TCase *tc_write;
	TCase *tc_prgname;
	TCase *tc_init;
	TCase *tc_print;

	s = suite_create("Utilities");

	tc_strtol = tcase_create("str_to_long");
	tcase_add_test(tc_strtol, test_str_to_long_decimal);
	tcase_add_test(tc_strtol, test_str_to_long_hex);
	tcase_add_test(tc_strtol, test_str_to_long_octal);
	tcase_add_test(tc_strtol, test_str_to_long_zero);
	tcase_add_test(tc_strtol, test_str_to_long_negative);
	tcase_add_test(tc_strtol, test_str_to_long_null_nptr);
	tcase_add_test(tc_strtol, test_str_to_long_null_result);
	tcase_add_test(tc_strtol, test_str_to_long_invalid_string);
	tcase_add_test(tc_strtol, test_str_to_long_overflow);
	tcase_add_test(tc_strtol, test_str_to_long_partial);
	tcase_add_test(tc_strtol, test_str_to_long_empty);
	suite_add_tcase(s, tc_strtol);

	tc_write = tcase_create("write_file");
	tcase_add_test(tc_write, test_write_file_success);
	tcase_add_test(tc_write, test_write_file_invalid_path);
	tcase_add_test(tc_write, test_write_file_empty);
	tcase_add_test(tc_write, test_write_file_permissions);
	suite_add_tcase(s, tc_write);

	tc_prgname = tcase_create("get_prgname");
	tcase_add_test(tc_prgname, test_get_prgname_full_path);
	tcase_add_test(tc_prgname, test_get_prgname_no_path);
	tcase_add_test(tc_prgname, test_get_prgname_trailing_slash);
	tcase_add_test(tc_prgname, test_get_prgname_empty);
	suite_add_tcase(s, tc_prgname);

	tc_init = tcase_create("initialized_options");
	tcase_add_test(tc_init, test_initialized_options_fields_invalid);
	tcase_add_test(tc_init, test_initialized_options_path_empty);
	tcase_add_test(tc_init, test_initialized_options_keypath_empty);
	tcase_add_test(tc_init, test_initialized_options_data_null);
	tcase_add_test(tc_init, test_initialized_options_sg_type);
	tcase_add_test(tc_init, test_initialized_options_set_type);
	suite_add_tcase(s, tc_init);

	tc_print = tcase_create("print_functions");
	tcase_add_test(tc_print, test_print_error_smoke);
	tcase_add_test(tc_print, test_print_warn_smoke);
	suite_add_tcase(s, tc_print);

	return s;
}
