// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for option parsing and validation functions */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "../ufs.h"
#include "../options.h"
#include "../ufs_cmds.h"
#include "../unipro.h"

extern char gl_pr_type;

static void reset_options(struct tool_options *options)
{
	memset(options, INVALID, sizeof(*options));
	options->path[0] = '\0';
	options->keypath[0] = '\0';
	options->data = NULL;
	options->sg_type = SG3_TYPE;
	options->set_type = ATTR_SET_NOR;
}

/* ========== init_options via descriptor read ========== */

START_TEST(test_init_options_desc_read)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r", "-p", "/dev/ufs-bsg", NULL};
	int argc = 7;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.idn, 0);
	ck_assert_int_eq(options.opr, READ);
	ck_assert_str_eq(options.path, "/dev/ufs-bsg");
}
END_TEST

START_TEST(test_init_options_desc_read_all)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-a", "-p", "/dev/ufs-bsg", NULL};
	int argc = 5;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.opr, READ_ALL);
}
END_TEST

/* ========== init_options via attribute read ========== */

START_TEST(test_init_options_attr_read)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = ATTR_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "attr", "-t", "3", "-r", "-p", "/dev/ufs-bsg", NULL};
	int argc = 7;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.idn, 3);
	ck_assert_int_eq(options.opr, READ);
}
END_TEST

/* ========== init_options via flag operations ========== */

START_TEST(test_init_options_flag_set)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = FLAG_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "fl", "-t", "1", "-e", "-p", "/dev/ufs-bsg", NULL};
	int argc = 7;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.idn, 1);
	ck_assert_int_eq(options.opr, SET_FLAG);
}
END_TEST

START_TEST(test_init_options_flag_clear)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = FLAG_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "fl", "-t", "4", "-c", "-p", "/dev/ufs-bsg", NULL};
	int argc = 7;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.idn, 4);
	ck_assert_int_eq(options.opr, CLEAR_FLAG);
}
END_TEST

START_TEST(test_init_options_flag_toggle)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = FLAG_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "fl", "-t", "4", "-o", "-p", "/dev/ufs-bsg", NULL};
	int argc = 7;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.opr, TOGGLE_FLAG);
}
END_TEST

/* ========== Flag operation on non-flag type (error case) ========== */

START_TEST(test_init_options_flag_op_on_desc_type)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-e", "-p", "/dev/ufs-bsg", NULL};
	int argc = 7;

	rc = init_options(argc, argv, &options);
	ck_assert_int_ne(rc, 0);
}
END_TEST

/* ========== Index and Selector ========== */

START_TEST(test_init_options_with_index)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "2", "-r", "-i", "3", "-p", "/dev/ufs-bsg", NULL};
	int argc = 9;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.index, 3);
}
END_TEST

START_TEST(test_init_options_with_selector)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r", "-s", "1", "-p", "/dev/ufs-bsg", NULL};
	int argc = 9;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.selector, 1);
}
END_TEST

/* ========== Invalid IDN tests ========== */

START_TEST(test_init_options_invalid_idn_string)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "abc", "-p", "/dev/ufs-bsg", NULL};
	int argc = 6;

	rc = init_options(argc, argv, &options);
	ck_assert_int_ne(rc, 0);
}
END_TEST

START_TEST(test_init_options_negative_idn)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "-5", "-p", "/dev/ufs-bsg", NULL};
	int argc = 6;

	rc = init_options(argc, argv, &options);
	ck_assert_int_ne(rc, 0);
}
END_TEST

/* ========== Missing device path ========== */

START_TEST(test_init_options_missing_path)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r", NULL};
	int argc = 5;

	rc = init_options(argc, argv, &options);
	ck_assert_int_ne(rc, 0);
}
END_TEST

/* ========== Duplicate options detection ========== */

START_TEST(test_init_options_duplicate_idn)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-t", "1", "-p", "/dev/ufs-bsg", NULL};
	int argc = 8;

	rc = init_options(argc, argv, &options);
	ck_assert_int_ne(rc, 0);
}
END_TEST

/* ========== SG type selection ========== */

START_TEST(test_init_options_sg3_type)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r", "-g", "0", "-p", "/dev/ufs-bsg", NULL};
	int argc = 9;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.sg_type, SG3_TYPE);
}
END_TEST

START_TEST(test_init_options_sg4_type)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r", "-g", "1", "-p", "/dev/ufs-bsg", NULL};
	int argc = 9;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.sg_type, SG4_TYPE);
}
END_TEST

START_TEST(test_init_options_invalid_sg_type)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r", "-g", "5", "-p", "/dev/ufs-bsg", NULL};
	int argc = 9;

	rc = init_options(argc, argv, &options);
	ck_assert_int_ne(rc, 0);
}
END_TEST

/* ========== Output mode tests ========== */

START_TEST(test_init_options_output_mode_json)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;
	gl_pr_type = VERBOSE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r", "-P", "json", "-p", "/dev/ufs-bsg", NULL};
	int argc = 9;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(gl_pr_type, JSON);
	gl_pr_type = VERBOSE;
}
END_TEST

START_TEST(test_init_options_output_mode_raw)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;
	gl_pr_type = VERBOSE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r", "-P", "raw", "-p", "/dev/ufs-bsg", NULL};
	int argc = 9;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(gl_pr_type, RAW_VALUE);
	gl_pr_type = VERBOSE;
}
END_TEST

START_TEST(test_init_options_output_mode_invalid)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;
	gl_pr_type = VERBOSE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r", "-P", "xml", "-p", "/dev/ufs-bsg", NULL};
	int argc = 9;

	rc = init_options(argc, argv, &options);
	ck_assert_int_ne(rc, 0);
	gl_pr_type = VERBOSE;
}
END_TEST

/* ========== Default value tests ========== */

START_TEST(test_init_options_default_operation_is_read)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-p", "/dev/ufs-bsg", NULL};
	int argc = 6;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.opr, READ);
}
END_TEST

START_TEST(test_init_options_default_index_zero)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r", "-p", "/dev/ufs-bsg", NULL};
	int argc = 7;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.index, 0);
}
END_TEST

START_TEST(test_init_options_default_selector_zero)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r", "-p", "/dev/ufs-bsg", NULL};
	int argc = 7;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.selector, 0);
}
END_TEST

/* ========== Hex IDN input ========== */

START_TEST(test_init_options_hex_idn)
{
	struct tool_options options;
	int rc;

	reset_options(&options);
	options.config_type_inx = DESC_TYPE;

	optind = 1;
	char *argv[] = {"ufs-utils", "desc", "-t", "0x7", "-r", "-p", "/dev/ufs-bsg", NULL};
	int argc = 7;

	rc = init_options(argc, argv, &options);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(options.idn, 7);
}
END_TEST

/* ========== Test Suite Setup ========== */

Suite *options_suite(void)
{
	Suite *s;
	TCase *tc_desc;
	TCase *tc_attr;
	TCase *tc_flags;
	TCase *tc_index_selector;
	TCase *tc_invalid;
	TCase *tc_sg_type;
	TCase *tc_output_mode;
	TCase *tc_defaults;

	s = suite_create("Options Parsing");

	tc_desc = tcase_create("Descriptor Options");
	tcase_add_test(tc_desc, test_init_options_desc_read);
	tcase_add_test(tc_desc, test_init_options_desc_read_all);
	suite_add_tcase(s, tc_desc);

	tc_attr = tcase_create("Attribute Options");
	tcase_add_test(tc_attr, test_init_options_attr_read);
	suite_add_tcase(s, tc_attr);

	tc_flags = tcase_create("Flag Options");
	tcase_add_test(tc_flags, test_init_options_flag_set);
	tcase_add_test(tc_flags, test_init_options_flag_clear);
	tcase_add_test(tc_flags, test_init_options_flag_toggle);
	tcase_add_test(tc_flags, test_init_options_flag_op_on_desc_type);
	suite_add_tcase(s, tc_flags);

	tc_index_selector = tcase_create("Index and Selector");
	tcase_add_test(tc_index_selector, test_init_options_with_index);
	tcase_add_test(tc_index_selector, test_init_options_with_selector);
	tcase_add_test(tc_index_selector, test_init_options_hex_idn);
	suite_add_tcase(s, tc_index_selector);

	tc_invalid = tcase_create("Invalid Options");
	tcase_add_test(tc_invalid, test_init_options_invalid_idn_string);
	tcase_add_test(tc_invalid, test_init_options_negative_idn);
	tcase_add_test(tc_invalid, test_init_options_missing_path);
	tcase_add_test(tc_invalid, test_init_options_duplicate_idn);
	suite_add_tcase(s, tc_invalid);

	tc_sg_type = tcase_create("SG Type");
	tcase_add_test(tc_sg_type, test_init_options_sg3_type);
	tcase_add_test(tc_sg_type, test_init_options_sg4_type);
	tcase_add_test(tc_sg_type, test_init_options_invalid_sg_type);
	suite_add_tcase(s, tc_sg_type);

	tc_output_mode = tcase_create("Output Mode");
	tcase_add_test(tc_output_mode, test_init_options_output_mode_json);
	tcase_add_test(tc_output_mode, test_init_options_output_mode_raw);
	tcase_add_test(tc_output_mode, test_init_options_output_mode_invalid);
	suite_add_tcase(s, tc_output_mode);

	tc_defaults = tcase_create("Default Values");
	tcase_add_test(tc_defaults, test_init_options_default_operation_is_read);
	tcase_add_test(tc_defaults, test_init_options_default_index_zero);
	tcase_add_test(tc_defaults, test_init_options_default_selector_zero);
	suite_add_tcase(s, tc_defaults);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = options_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
