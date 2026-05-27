// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for options.c option parsing and validation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>

#include "test_framework.h"

/* Include source files to access static functions */
#include "ufs.c"
#include "options.c"

/* Helper to reset options state between tests */
static void reset_options(struct tool_options *opts)
{
	initialized_options(opts);
	optind = 1;
	opterr = 0;
}

/* ---- init_options with descriptor read ---- */

static void test_init_options_desc_read(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r", "-p", "/dev/ufs-bsg"};
	int argc = 7;

	reset_options(&opts);
	opts.config_type_inx = DESC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options desc read should succeed");
	TEST_ASSERT_EQ(opts.idn, 0, "idn should be 0");
	TEST_ASSERT_EQ(opts.opr, READ, "opr should be READ");
	TEST_ASSERT_STR_EQ(opts.path, "/dev/ufs-bsg", "path should be set");
}

static void test_init_options_desc_read_all(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "desc", "-a", "-t", "0", "-p", "/dev/ufs-bsg"};
	int argc = 7;

	reset_options(&opts);
	opts.config_type_inx = DESC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options desc read all should succeed");
	TEST_ASSERT_EQ(opts.opr, READ_ALL, "opr should be READ_ALL");
}

/* ---- init_options with flags ---- */

static void test_init_options_flag_set(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "fl", "-t", "4", "-e", "-p", "/dev/ufs-bsg"};
	int argc = 7;

	reset_options(&opts);
	opts.config_type_inx = FLAG_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options flag set should succeed");
	TEST_ASSERT_EQ(opts.opr, SET_FLAG, "opr should be SET_FLAG");
	TEST_ASSERT_EQ(opts.idn, 4, "idn should be 4");
}

static void test_init_options_flag_clear(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "fl", "-t", "4", "-c", "-p", "/dev/ufs-bsg"};
	int argc = 7;

	reset_options(&opts);
	opts.config_type_inx = FLAG_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options flag clear should succeed");
	TEST_ASSERT_EQ(opts.opr, CLEAR_FLAG, "opr should be CLEAR_FLAG");
}

static void test_init_options_flag_toggle(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "fl", "-t", "4", "-o", "-p", "/dev/ufs-bsg"};
	int argc = 7;

	reset_options(&opts);
	opts.config_type_inx = FLAG_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options flag toggle should succeed");
	TEST_ASSERT_EQ(opts.opr, TOGGLE_FLAG, "opr should be TOGGLE_FLAG");
}

/* ---- init_options with attributes ---- */

static void test_init_options_attr_read(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "attr", "-t", "5", "-r", "-p", "/dev/ufs-bsg"};
	int argc = 7;

	reset_options(&opts);
	opts.config_type_inx = ATTR_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options attr read should succeed");
	TEST_ASSERT_EQ(opts.idn, 5, "idn should be 5");
	TEST_ASSERT_EQ(opts.opr, READ, "opr should be READ");
}

/* ---- init_options with UIC ---- */

static void test_init_options_uic_local(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "uic", "-t", "0", "-i", "0x1560", "--local", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	reset_options(&opts);
	opts.config_type_inx = UIC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options uic local should succeed");
	TEST_ASSERT_EQ(opts.target, DME_LOCAL, "target should be DME_LOCAL");
}

static void test_init_options_uic_peer(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "uic", "-t", "0", "-i", "0x1560", "--peer", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	reset_options(&opts);
	opts.config_type_inx = UIC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options uic peer should succeed");
	TEST_ASSERT_EQ(opts.target, DME_PEER, "target should be DME_PEER");
}

/* ---- init_options with invalid idn ---- */

static void test_init_options_desc_invalid_idn(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "desc", "-t", "999", "-r", "-p", "/dev/ufs-bsg"};
	int argc = 7;

	reset_options(&opts);
	opts.config_type_inx = DESC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT(rc != OK, "init_options with invalid desc idn should fail");
}

static void test_init_options_uic_invalid_idn(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "uic", "-t", "99", "-p", "/dev/ufs-bsg"};
	int argc = 6;

	reset_options(&opts);
	opts.config_type_inx = UIC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT(rc != OK, "init_options with invalid UIC idn should fail");
}

/* ---- Missing device path ---- */

static void test_init_options_missing_path(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r"};
	int argc = 5;

	reset_options(&opts);
	opts.config_type_inx = DESC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT(rc != OK, "init_options missing path should fail");
}

/* ---- Index option ---- */

static void test_init_options_with_index(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "desc", "-t", "5", "-i", "2", "-p", "/dev/ufs-bsg"};
	int argc = 8;

	reset_options(&opts);
	opts.config_type_inx = DESC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options with index should succeed");
	TEST_ASSERT_EQ(opts.index, 2, "index should be 2");
}

static void test_init_options_duplicate_index(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-i", "1", "-i", "2", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	reset_options(&opts);
	opts.config_type_inx = DESC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT(rc != OK, "init_options duplicate index should fail");
}

/* ---- Selector option ---- */

static void test_init_options_with_selector(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-s", "1", "-p", "/dev/ufs-bsg"};
	int argc = 8;

	reset_options(&opts);
	opts.config_type_inx = DESC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options with selector should succeed");
	TEST_ASSERT_EQ(opts.selector, 1, "selector should be 1");
}

/* ---- SG type option ---- */

static void test_init_options_sg_type(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-g", "1", "-p", "/dev/ufs-bsg"};
	int argc = 8;

	reset_options(&opts);
	opts.config_type_inx = DESC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options with sg type should succeed");
	TEST_ASSERT_EQ(opts.sg_type, SG4_TYPE, "sg_type should be SG4_TYPE");
}

static void test_init_options_sg_type_invalid(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-g", "5", "-p", "/dev/ufs-bsg"};
	int argc = 8;

	reset_options(&opts);
	opts.config_type_inx = DESC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT(rc != OK, "init_options with invalid sg type should fail");
}

/* ---- Default operation ---- */

static void test_init_options_default_opr(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-p", "/dev/ufs-bsg"};
	int argc = 6;

	reset_options(&opts);
	opts.config_type_inx = DESC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options should default to READ operation");
	TEST_ASSERT_EQ(opts.opr, READ, "default opr should be READ");
}

/* ---- FFU chunk size ---- */

static void test_init_options_ffu_chunk_size(void)
{
	struct tool_options opts;
	int rc;
	/* FFU check status mode (idn=1) does not require a FW file */
	char *argv[] = {"ufs-utils", "ffu", "-t", "1", "-s", "4", "-p", "/dev/ufs-bsg"};
	int argc = 8;

	reset_options(&opts);
	opts.config_type_inx = FFU_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options ffu chunk size should succeed");
	TEST_ASSERT_EQ(opts.size, 4096, "size should be 4096 (4KB)");
}

static void test_init_options_ffu_chunk_size_invalid(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "ffu", "-t", "0", "-s", "0", "-p", "/dev/ufs-bsg"};
	int argc = 8;

	reset_options(&opts);
	opts.config_type_inx = FFU_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT(rc != OK, "init_options ffu zero chunk size should fail");
}

/* ---- UIC static attribute set type ---- */

static void test_init_options_uic_static(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "uic", "-t", "0", "-i", "0x1560", "--static", "--local", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	reset_options(&opts);
	opts.config_type_inx = UIC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options uic static should succeed");
	TEST_ASSERT_EQ(opts.set_type, ATTR_SET_ST, "set_type should be ATTR_SET_ST");
}

/* ---- Hex idn values ---- */

static void test_init_options_hex_idn(void)
{
	struct tool_options opts;
	int rc;
	char *argv[] = {"ufs-utils", "desc", "-t", "0x9", "-p", "/dev/ufs-bsg"};
	int argc = 6;

	reset_options(&opts);
	opts.config_type_inx = DESC_TYPE;
	rc = init_options(argc, argv, &opts);
	TEST_ASSERT_EQ(rc, OK, "init_options with hex idn should succeed");
	TEST_ASSERT_EQ(opts.idn, 9, "idn should be 9 (0x9)");
}

int main(void)
{
	TEST_INIT();
	printf("\n[test_options] Running option parsing tests\n");

	RUN_TEST(test_init_options_desc_read);
	RUN_TEST(test_init_options_desc_read_all);
	RUN_TEST(test_init_options_flag_set);
	RUN_TEST(test_init_options_flag_clear);
	RUN_TEST(test_init_options_flag_toggle);
	RUN_TEST(test_init_options_attr_read);
	RUN_TEST(test_init_options_uic_local);
	RUN_TEST(test_init_options_uic_peer);
	RUN_TEST(test_init_options_desc_invalid_idn);
	RUN_TEST(test_init_options_uic_invalid_idn);
	RUN_TEST(test_init_options_missing_path);
	RUN_TEST(test_init_options_with_index);
	RUN_TEST(test_init_options_duplicate_index);
	RUN_TEST(test_init_options_with_selector);
	RUN_TEST(test_init_options_sg_type);
	RUN_TEST(test_init_options_sg_type_invalid);
	RUN_TEST(test_init_options_default_opr);
	RUN_TEST(test_init_options_ffu_chunk_size);
	RUN_TEST(test_init_options_ffu_chunk_size_invalid);
	RUN_TEST(test_init_options_uic_static);
	RUN_TEST(test_init_options_hex_idn);

	TEST_REPORT("test_options");
	return TEST_RESULT();
}
