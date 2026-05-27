// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for ufs.c core utility functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>

#include "test_framework.h"

/* Include the source file to access static functions */
#include "ufs.c"

/* ---- str_to_long tests ---- */

static void test_str_to_long_decimal(void)
{
	long result = 0;

	optarg = "42";
	TEST_ASSERT_EQ(str_to_long("42", 10, &result), OK,
		       "str_to_long decimal should succeed");
	TEST_ASSERT_EQ(result, 42, "str_to_long decimal value");
}

static void test_str_to_long_hex(void)
{
	long result = 0;

	optarg = "0xFF";
	TEST_ASSERT_EQ(str_to_long("0xFF", 16, &result), OK,
		       "str_to_long hex should succeed");
	TEST_ASSERT_EQ(result, 255, "str_to_long hex value");
}

static void test_str_to_long_zero(void)
{
	long result = -1;

	optarg = "0";
	TEST_ASSERT_EQ(str_to_long("0", 10, &result), OK,
		       "str_to_long zero should succeed");
	TEST_ASSERT_EQ(result, 0, "str_to_long zero value");
}

static void test_str_to_long_negative(void)
{
	long result = 0;

	optarg = "-10";
	TEST_ASSERT_EQ(str_to_long("-10", 10, &result), OK,
		       "str_to_long negative should succeed");
	TEST_ASSERT_EQ(result, -10, "str_to_long negative value");
}

static void test_str_to_long_null_ptr(void)
{
	long result = 0;

	TEST_ASSERT_EQ(str_to_long(NULL, 10, &result), ERROR,
		       "str_to_long null nptr should fail");
	TEST_ASSERT_EQ(str_to_long("42", 10, NULL), ERROR,
		       "str_to_long null result should fail");
}

static void test_str_to_long_invalid_string(void)
{
	long result = 0;

	optarg = "abc";
	TEST_ASSERT_EQ(str_to_long("abc", 10, &result), ERROR,
		       "str_to_long invalid string should fail");
}

static void test_str_to_long_partial_conversion(void)
{
	long result = 0;

	optarg = "42abc";
	TEST_ASSERT_EQ(str_to_long("42abc", 10, &result), ERROR,
		       "str_to_long partial conversion should fail");
}

static void test_str_to_long_empty_string(void)
{
	long result = 0;

	optarg = "";
	TEST_ASSERT_EQ(str_to_long("", 10, &result), ERROR,
		       "str_to_long empty string should fail");
}

/* ---- get_prgname tests ---- */

static void test_get_prgname_with_path(void)
{
	char *result = get_prgname("/usr/bin/ufs-utils");

	TEST_ASSERT_STR_EQ(result, "ufs-utils",
			   "get_prgname with path should return basename");
}

static void test_get_prgname_no_path(void)
{
	char *result = get_prgname("ufs-utils");

	TEST_ASSERT_STR_EQ(result, "ufs-utils",
			   "get_prgname without path should return as-is");
}

static void test_get_prgname_nested_path(void)
{
	char *result = get_prgname("/a/b/c/d/prog");

	TEST_ASSERT_STR_EQ(result, "prog",
			   "get_prgname with nested path");
}

static void test_get_prgname_trailing_slash(void)
{
	char *result = get_prgname("dir/");

	TEST_ASSERT_STR_EQ(result, "",
			   "get_prgname with trailing slash returns empty");
}

/* ---- initialized_options tests ---- */

static void test_initialized_options_defaults(void)
{
	struct tool_options opts;

	initialized_options(&opts);

	TEST_ASSERT_EQ(opts.path[0], '\0', "path should be empty string");
	TEST_ASSERT_EQ(opts.keypath[0], '\0', "keypath should be empty string");
	TEST_ASSERT_NULL(opts.data, "data should be NULL");
	TEST_ASSERT_EQ(opts.sg_type, SG3_TYPE, "sg_type should default to SG3_TYPE");
	TEST_ASSERT_EQ(opts.set_type, ATTR_SET_NOR, "set_type should default to ATTR_SET_NOR");
	TEST_ASSERT_EQ(opts.idn, INVALID, "idn should be INVALID");
	TEST_ASSERT_EQ(opts.opr, INVALID, "opr should be INVALID");
	TEST_ASSERT_EQ(opts.index, INVALID, "index should be INVALID");
	TEST_ASSERT_EQ(opts.selector, INVALID, "selector should be INVALID");
	TEST_ASSERT_EQ(opts.target, INVALID, "target should be INVALID");
}

/* ---- write_file tests ---- */

static void test_write_file_success(void)
{
	const char *tmpfile = "/tmp/ufs_test_write.bin";
	const char *data = "Hello UFS";
	int rc;
	char buf[32] = {0};
	FILE *f;

	rc = write_file(tmpfile, data, strlen(data));
	TEST_ASSERT_EQ(rc, 0, "write_file should succeed");

	f = fopen(tmpfile, "r");
	TEST_ASSERT_NOT_NULL(f, "written file should be readable");
	if (f) {
		size_t n = fread(buf, 1, strlen(data), f);
		TEST_ASSERT_EQ((long)n, (long)strlen(data),
			       "fread should return full length");
		fclose(f);
		TEST_ASSERT_MEM_EQ(buf, data, strlen(data),
				   "written file content should match");
	}

	unlink(tmpfile);
}

static void test_write_file_invalid_path(void)
{
	int rc = write_file("/nonexistent/dir/file.bin", "data", 4);

	TEST_ASSERT(rc != 0, "write_file to invalid path should fail");
}

/* ---- print_command_help coverage ---- */

static void test_print_command_help_all_types(void)
{
	int types[] = {DESC_TYPE, ATTR_TYPE, FLAG_TYPE, ERR_HIST_TYPE,
		       FFU_TYPE, UIC_TYPE, VENDOR_BUFFER_TYPE,
		       RPMB_CMD_TYPE, ARPMB_CMD_TYPE, HMR_TYPE,
		       SPEC_VERSION, BSG_LIST_TYPE, EMON_TYPE, 99};
	int i;

	for (i = 0; i < (int)(sizeof(types) / sizeof(types[0])); i++)
		print_command_help("test", types[i]);

	TEST_ASSERT(1, "print_command_help should not crash for any type");
}

/* ---- parse_args tests ---- */

static void test_parse_args_version(void)
{
	char *argv[] = {"ufs-utils", "-v"};
	command_function func = NULL;
	struct tool_options opts;
	int rc;

	initialized_options(&opts);
	optind = 1;
	rc = parse_args(2, argv, &func, &opts);
	TEST_ASSERT_EQ(rc, OK, "parse_args -v should succeed");
}

static void test_parse_args_no_args(void)
{
	char *argv[] = {"ufs-utils"};
	command_function func = NULL;
	struct tool_options opts;
	int rc;

	initialized_options(&opts);
	optind = 1;
	rc = parse_args(1, argv, &func, &opts);
	TEST_ASSERT_EQ(rc, OK, "parse_args with no args should succeed (shows help)");
}

static void test_parse_args_help(void)
{
	char *argv[] = {"ufs-utils", "--help"};
	command_function func = NULL;
	struct tool_options opts;
	int rc;

	initialized_options(&opts);
	optind = 1;
	rc = parse_args(2, argv, &func, &opts);
	TEST_ASSERT_EQ(rc, OK, "parse_args --help should succeed");
}

static void test_parse_args_invalid_command(void)
{
	char *argv[] = {"ufs-utils", "invalid_cmd", "-t", "1"};
	command_function func = NULL;
	struct tool_options opts;
	int rc;

	initialized_options(&opts);
	optind = 1;
	rc = parse_args(4, argv, &func, &opts);
	TEST_ASSERT(rc != OK, "parse_args with invalid command should fail");
}

static void test_parse_args_command_help(void)
{
	char *argv[] = {"ufs-utils", "desc", "--help"};
	command_function func = NULL;
	struct tool_options opts;
	int rc;

	initialized_options(&opts);
	optind = 1;
	rc = parse_args(3, argv, &func, &opts);
	TEST_ASSERT_EQ(rc, OK, "parse_args 'desc --help' should succeed");
	TEST_ASSERT(func == 0, "func should be NULL after help request");
}

int main(void)
{
	TEST_INIT();
	printf("\n[test_ufs_core] Running core utility tests\n");

	RUN_TEST(test_str_to_long_decimal);
	RUN_TEST(test_str_to_long_hex);
	RUN_TEST(test_str_to_long_zero);
	RUN_TEST(test_str_to_long_negative);
	RUN_TEST(test_str_to_long_null_ptr);
	RUN_TEST(test_str_to_long_invalid_string);
	RUN_TEST(test_str_to_long_partial_conversion);
	RUN_TEST(test_str_to_long_empty_string);

	RUN_TEST(test_get_prgname_with_path);
	RUN_TEST(test_get_prgname_no_path);
	RUN_TEST(test_get_prgname_nested_path);
	RUN_TEST(test_get_prgname_trailing_slash);

	RUN_TEST(test_initialized_options_defaults);

	RUN_TEST(test_write_file_success);
	RUN_TEST(test_write_file_invalid_path);

	RUN_TEST(test_print_command_help_all_types);

	RUN_TEST(test_parse_args_version);
	RUN_TEST(test_parse_args_no_args);
	RUN_TEST(test_parse_args_help);
	RUN_TEST(test_parse_args_invalid_command);
	RUN_TEST(test_parse_args_command_help);

	TEST_REPORT("test_ufs_core");
	return TEST_RESULT();
}
