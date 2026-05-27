// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for options parsing and validation functions */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <getopt.h>

#include "../ufs.h"
#include "../options.h"
#include "../unipro.h"
#include "../ufs_ffu.h"
#include "../ufs_rpmb.h"
#include "../ufs_hmr.h"
#include "../ufs_emon.h"

static struct tool_options opts;

static void setup_options(void)
{
	memset(&opts, INVALID, sizeof(opts));
	opts.path[0] = '\0';
	opts.keypath[0] = '\0';
	opts.data = NULL;
	opts.sg_type = SG3_TYPE;
	opts.set_type = ATTR_SET_NOR;
}

static void teardown_options(void)
{
	if (opts.data) {
		free(opts.data);
		opts.data = NULL;
	}
}

/*
 * Helper: call init_options with the given argv.
 * Resets getopt global state before each call.
 */
static int call_init(int argc, char *argv[])
{
	optind = 0;
	return init_options(argc, argv, &opts);
}

/* ------------------------------------------------------------------ */
/* Basic read operation parsing                                        */
/* ------------------------------------------------------------------ */

START_TEST(test_parse_read_descriptor)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.opr, READ);
	ck_assert_int_eq(opts.idn, 0);
	ck_assert_str_eq(opts.path, "/dev/ufs-bsg");

	teardown_options();
}
END_TEST

START_TEST(test_default_operation_is_read)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0",
			"-p", "/dev/ufs-bsg"};
	int argc = 6;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.opr, READ);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Write operation parsing                                             */
/* ------------------------------------------------------------------ */

START_TEST(test_parse_write_descriptor)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "5", "-w", "hello",
			"-i", "1", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.opr, WRITE);
	ck_assert_int_eq(opts.idn, QUERY_DESC_IDN_STRING);
	ck_assert_ptr_nonnull(opts.data);

	teardown_options();
}
END_TEST

START_TEST(test_parse_write_attribute)
{
	char *argv[] = {"ufs-utils", "attr", "-t", "3", "-w", "42",
			"-p", "/dev/ufs-bsg"};
	int argc = 8;

	setup_options();
	opts.config_type_inx = ATTR_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.opr, WRITE);
	ck_assert_int_eq(opts.idn, QUERY_ATTR_IDN_ACTIVE_ICC_LVL);
	ck_assert_ptr_nonnull(opts.data);
	ck_assert_int_eq(*(__u32 *)opts.data, 42);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Flag operations: set, clear, toggle                                 */
/* ------------------------------------------------------------------ */

START_TEST(test_parse_flag_set)
{
	char *argv[] = {"ufs-utils", "fl", "-t", "1", "-e",
			"-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = FLAG_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.opr, SET_FLAG);

	teardown_options();
}
END_TEST

START_TEST(test_parse_flag_clear)
{
	char *argv[] = {"ufs-utils", "fl", "-t", "1", "-c",
			"-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = FLAG_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.opr, CLEAR_FLAG);

	teardown_options();
}
END_TEST

START_TEST(test_parse_flag_toggle)
{
	char *argv[] = {"ufs-utils", "fl", "-t", "1", "-o",
			"-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = FLAG_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.opr, TOGGLE_FLAG);

	teardown_options();
}
END_TEST

START_TEST(test_flag_opr_wrong_type)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-e",
			"-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Index and selector parsing                                          */
/* ------------------------------------------------------------------ */

START_TEST(test_parse_index)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "5", "-r",
			"-i", "3", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.index, 3);

	teardown_options();
}
END_TEST

START_TEST(test_parse_selector)
{
	char *argv[] = {"ufs-utils", "attr", "-t", "3", "-r",
			"-s", "2", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = ATTR_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.selector, 2);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Read-all operation                                                  */
/* ------------------------------------------------------------------ */

START_TEST(test_parse_read_all)
{
	char *argv[] = {"ufs-utils", "desc", "-a",
			"-p", "/dev/ufs-bsg"};
	int argc = 5;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.opr, READ_ALL);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Invalid arguments                                                   */
/* ------------------------------------------------------------------ */

START_TEST(test_invalid_option)
{
	char *argv[] = {"ufs-utils", "desc", "-Z",
			"-p", "/dev/ufs-bsg"};
	int argc = 5;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Duplicate option detection                                          */
/* ------------------------------------------------------------------ */

START_TEST(test_duplicate_idn)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-t", "1",
			"-r", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_duplicate_index)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "5", "-r",
			"-i", "1", "-i", "2", "-p", "/dev/ufs-bsg"};
	int argc = 11;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_duplicate_selector)
{
	char *argv[] = {"ufs-utils", "attr", "-t", "3", "-r",
			"-s", "1", "-s", "2", "-p", "/dev/ufs-bsg"};
	int argc = 11;

	setup_options();
	opts.config_type_inx = ATTR_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_duplicate_read)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r", "-r",
			"-p", "/dev/ufs-bsg"};
	int argc = 8;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_duplicate_target)
{
	char *argv[] = {"ufs-utils", "uic", "-t", "0", "-r", "-i", "0",
			"--peer", "--peer", "-p", "/dev/ufs-bsg"};
	int argc = 11;

	setup_options();
	opts.config_type_inx = UIC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* IDN validation per config type                                      */
/* ------------------------------------------------------------------ */

START_TEST(test_invalid_desc_idn)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "99999",
			"-r", "-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_valid_desc_idn_max)
{
	/* QUERY_DESC_IDN_MAX = 0xFF, should be accepted */
	char *argv[] = {"ufs-utils", "desc", "-t", "255",
			"-r", "-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.idn, 0xFF);

	teardown_options();
}
END_TEST

START_TEST(test_invalid_uic_idn)
{
	/* MAX_UNIPRO_IDN = 4, so 4 should be invalid */
	char idn_str[16];
	snprintf(idn_str, sizeof(idn_str), "%d", MAX_UNIPRO_IDN);

	char *argv[] = {"ufs-utils", "uic", "-t", idn_str,
			"-r", "-i", "0", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = UIC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_invalid_ffu_idn)
{
	char idn_str[16];
	snprintf(idn_str, sizeof(idn_str), "%d", UFS_FFU_MAX);

	char *argv[] = {"ufs-utils", "ffu", "-t", idn_str,
			"-p", "/dev/ufs-bsg"};
	int argc = 6;

	setup_options();
	opts.config_type_inx = FFU_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_invalid_rpmb_idn)
{
	char idn_str[16];
	snprintf(idn_str, sizeof(idn_str), "%d", RPMB_CMD_MAX);

	char *argv[] = {"ufs-utils", "rpmb", "-t", idn_str,
			"-r", "-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = RPMB_CMD_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_negative_idn)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "-1",
			"-r", "-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Missing device path (verify_arg_and_set_default)                    */
/* ------------------------------------------------------------------ */

START_TEST(test_missing_device_path)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r"};
	int argc = 5;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Missing idn for types that require it                               */
/* ------------------------------------------------------------------ */

START_TEST(test_missing_idn_desc)
{
	char *argv[] = {"ufs-utils", "desc", "-r",
			"-p", "/dev/ufs-bsg"};
	int argc = 5;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_idn_not_required_for_err_hist)
{
	char *argv[] = {"ufs-utils", "err_hist", "-r",
			"-p", "/dev/ufs-bsg"};
	int argc = 5;

	setup_options();
	opts.config_type_inx = ERR_HIST_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* String descriptor without index                                     */
/* ------------------------------------------------------------------ */

START_TEST(test_string_desc_missing_index)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "5", "-r",
			"-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_string_desc_with_index)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "5", "-r",
			"-i", "1", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.idn, QUERY_DESC_IDN_STRING);
	ck_assert_int_eq(opts.index, 1);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* FFU chunk size validation                                           */
/* ------------------------------------------------------------------ */

START_TEST(test_ffu_chunk_size_valid)
{
	/* 4KB = valid chunk size */
	char *argv[] = {"ufs-utils", "ffu", "-t", "0", "-s", "4",
			"-w", "/tmp/fw.bin", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = FFU_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.size, 4 * 1024);

	teardown_options();
}
END_TEST

START_TEST(test_ffu_chunk_size_zero)
{
	char *argv[] = {"ufs-utils", "ffu", "-t", "0", "-s", "0",
			"-w", "/tmp/fw.bin", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = FFU_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_ffu_chunk_size_not_aligned)
{
	/* 3KB is not a multiple of 4KB */
	char *argv[] = {"ufs-utils", "ffu", "-t", "0", "-s", "3",
			"-w", "/tmp/fw.bin", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = FFU_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_ffu_chunk_size_too_large)
{
	/* MAX_IOCTL_BUF_SIZE = 256KB, try 512KB */
	char *argv[] = {"ufs-utils", "ffu", "-t", "0", "-s", "512",
			"-w", "/tmp/fw.bin", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = FFU_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_ffu_default_chunk_size)
{
	char *argv[] = {"ufs-utils", "ffu", "-t", "0",
			"-w", "/tmp/fw.bin", "-p", "/dev/ufs-bsg"};
	int argc = 8;

	setup_options();
	opts.config_type_inx = FFU_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.size, MAX_IOCTL_BUF_SIZE);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* RPMB-specific options                                               */
/* ------------------------------------------------------------------ */

START_TEST(test_rpmb_start_address)
{
	char *argv[] = {"ufs-utils", "rpmb", "-t", "2", "-s", "100",
			"-D", "/tmp/out", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = RPMB_CMD_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.start_block, 100);

	teardown_options();
}
END_TEST

START_TEST(test_rpmb_start_address_out_of_range)
{
	/* MAX_ADDRESS is 0xFFFF = 65535, try 70000 */
	char *argv[] = {"ufs-utils", "rpmb", "-t", "2", "-s", "70000",
			"-D", "/tmp/out", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = RPMB_CMD_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_rpmb_num_blocks)
{
	char *argv[] = {"ufs-utils", "rpmb", "-t", "2", "-n", "10",
			"-D", "/tmp/out", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = RPMB_CMD_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.num_block, 10);

	teardown_options();
}
END_TEST

START_TEST(test_rpmb_key_path)
{
	char *argv[] = {"ufs-utils", "rpmb", "-t", "0",
			"-k", "/tmp/key",
			"-p", "/dev/ufs-bsg"};
	int argc = 8;

	setup_options();
	opts.config_type_inx = RPMB_CMD_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_str_eq(opts.keypath, "/tmp/key");

	teardown_options();
}
END_TEST

START_TEST(test_rpmb_key_path_non_rpmb_type)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0",
			"-k", "/tmp/key",
			"-r", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* HMR method and unit validation                                      */
/* ------------------------------------------------------------------ */

START_TEST(test_hmr_method_valid)
{
	/* HMR_METHOD_FORCE = 1 */
	char *argv[] = {"ufs-utils", "hmr", "-x", "1",
			"-p", "/dev/ufs-bsg"};
	int argc = 6;

	setup_options();
	opts.config_type_inx = HMR_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.hmr_method, HMR_METHOD_FORCE);

	teardown_options();
}
END_TEST

START_TEST(test_hmr_method_invalid)
{
	/* HMR_METHOD_MAX = 3, so 3 should be invalid */
	char method_str[16];
	snprintf(method_str, sizeof(method_str), "%d", HMR_METHOD_MAX);

	char *argv[] = {"ufs-utils", "hmr", "-x", method_str,
			"-p", "/dev/ufs-bsg"};
	int argc = 6;

	setup_options();
	opts.config_type_inx = HMR_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_hmr_unit_valid)
{
	/* HMR_UNIT_MIN = 0 */
	char *argv[] = {"ufs-utils", "hmr", "-y", "0",
			"-p", "/dev/ufs-bsg"};
	int argc = 6;

	setup_options();
	opts.config_type_inx = HMR_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.hmr_unit, HMR_UNIT_MIN);

	teardown_options();
}
END_TEST

START_TEST(test_hmr_unit_invalid)
{
	char unit_str[16];
	snprintf(unit_str, sizeof(unit_str), "%d", HMR_UNIT_MAX);

	char *argv[] = {"ufs-utils", "hmr", "-y", unit_str,
			"-p", "/dev/ufs-bsg"};
	int argc = 6;

	setup_options();
	opts.config_type_inx = HMR_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_hmr_defaults)
{
	char *argv[] = {"ufs-utils", "hmr",
			"-p", "/dev/ufs-bsg"};
	int argc = 4;

	setup_options();
	opts.config_type_inx = HMR_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.hmr_method, HMR_METHOD_SELECTIVE);
	ck_assert_int_eq(opts.hmr_unit, HMR_UNIT_MIN);

	teardown_options();
}
END_TEST

START_TEST(test_hmr_duplicate_method)
{
	char *argv[] = {"ufs-utils", "hmr", "-x", "1", "-x", "2",
			"-p", "/dev/ufs-bsg"};
	int argc = 8;

	setup_options();
	opts.config_type_inx = HMR_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_hmr_duplicate_unit)
{
	char *argv[] = {"ufs-utils", "hmr", "-y", "0", "-y", "1",
			"-p", "/dev/ufs-bsg"};
	int argc = 8;

	setup_options();
	opts.config_type_inx = HMR_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* SG struct type validation                                           */
/* ------------------------------------------------------------------ */

START_TEST(test_sg_struct_sg3)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-g", "0", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.sg_type, SG3_TYPE);

	teardown_options();
}
END_TEST

START_TEST(test_sg_struct_sg4)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-g", "1", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.sg_type, SG4_TYPE);

	teardown_options();
}
END_TEST

START_TEST(test_sg_struct_invalid)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-g", "5", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* LUN validation                                                      */
/* ------------------------------------------------------------------ */

START_TEST(test_lun_valid)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-d", "2", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.lun, 2);

	teardown_options();
}
END_TEST

START_TEST(test_lun_zero)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-d", "0", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.lun, 0);

	teardown_options();
}
END_TEST

START_TEST(test_lun_invalid)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-d", "abc", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Region validation                                                   */
/* ------------------------------------------------------------------ */

START_TEST(test_region_valid)
{
	char *argv[] = {"ufs-utils", "rpmb", "-t", "2", "-m", "2",
			"-D", "/tmp/out", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = RPMB_CMD_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.region, 2);

	teardown_options();
}
END_TEST

START_TEST(test_region_max_valid)
{
	char *argv[] = {"ufs-utils", "rpmb", "-t", "2", "-m", "3",
			"-D", "/tmp/out", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = RPMB_CMD_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.region, 3);

	teardown_options();
}
END_TEST

START_TEST(test_region_invalid)
{
	char *argv[] = {"ufs-utils", "rpmb", "-t", "2", "-m", "4",
			"-D", "/tmp/out", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = RPMB_CMD_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_region_negative)
{
	char *argv[] = {"ufs-utils", "rpmb", "-t", "2", "-m", "-1",
			"-D", "/tmp/out", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = RPMB_CMD_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* UIC peer/local target                                               */
/* ------------------------------------------------------------------ */

START_TEST(test_uic_peer_target)
{
	char *argv[] = {"ufs-utils", "uic", "-t", "0", "-r", "-i", "0",
			"--peer", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = UIC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.target, DME_PEER);

	teardown_options();
}
END_TEST

START_TEST(test_uic_local_target)
{
	char *argv[] = {"ufs-utils", "uic", "-t", "0", "-r", "-i", "0",
			"--local", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = UIC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.target, DME_LOCAL);

	teardown_options();
}
END_TEST

START_TEST(test_uic_write_requires_target)
{
	char *argv[] = {"ufs-utils", "uic", "-t", "0", "-w", "42",
			"-i", "0", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = UIC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_uic_write_with_target)
{
	char *argv[] = {"ufs-utils", "uic", "-t", "0", "-w", "42",
			"-i", "0", "--peer", "-p", "/dev/ufs-bsg"};
	int argc = 11;

	setup_options();
	opts.config_type_inx = UIC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.target, DME_PEER);

	teardown_options();
}
END_TEST

START_TEST(test_uic_read_requires_index)
{
	char *argv[] = {"ufs-utils", "uic", "-t", "0", "-r",
			"-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = UIC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Length and offset validation                                        */
/* ------------------------------------------------------------------ */

START_TEST(test_length_valid)
{
	char *argv[] = {"ufs-utils", "vendor", "-L", "1024",
			"-p", "/dev/ufs-bsg"};
	int argc = 6;

	setup_options();
	opts.config_type_inx = VENDOR_BUFFER_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.len, 1024);

	teardown_options();
}
END_TEST

START_TEST(test_length_duplicate)
{
	char *argv[] = {"ufs-utils", "vendor", "-L", "1024",
			"-L", "512", "-p", "/dev/ufs-bsg"};
	int argc = 8;

	setup_options();
	opts.config_type_inx = VENDOR_BUFFER_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_offset_valid)
{
	char *argv[] = {"ufs-utils", "vendor", "-O", "256",
			"-p", "/dev/ufs-bsg"};
	int argc = 6;

	setup_options();
	opts.config_type_inx = VENDOR_BUFFER_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.offset, 256);

	teardown_options();
}
END_TEST

START_TEST(test_offset_duplicate)
{
	char *argv[] = {"ufs-utils", "vendor", "-O", "256",
			"-O", "128", "-p", "/dev/ufs-bsg"};
	int argc = 8;

	setup_options();
	opts.config_type_inx = VENDOR_BUFFER_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_vendor_default_length)
{
	char *argv[] = {"ufs-utils", "vendor",
			"-p", "/dev/ufs-bsg"};
	int argc = 4;

	setup_options();
	opts.config_type_inx = VENDOR_BUFFER_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.len, BLOCK_SIZE);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Device path validation                                              */
/* ------------------------------------------------------------------ */

START_TEST(test_duplicate_device_path)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-p", "/dev/a", "-p", "/dev/b"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Write operation for FLAG_TYPE should error (use -c/-e/-o instead)    */
/* ------------------------------------------------------------------ */

START_TEST(test_write_flag_error)
{
	char *argv[] = {"ufs-utils", "fl", "-t", "1", "-w", "1",
			"-p", "/dev/ufs-bsg"};
	int argc = 8;

	setup_options();
	opts.config_type_inx = FLAG_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Static set type for UIC                                             */
/* ------------------------------------------------------------------ */

START_TEST(test_uic_static_set_type)
{
	char *argv[] = {"ufs-utils", "uic", "-t", "0", "-r",
			"-i", "0", "--static", "-p", "/dev/ufs-bsg"};
	int argc = 10;

	setup_options();
	opts.config_type_inx = UIC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.set_type, ATTR_SET_ST);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Default values when not specified                                   */
/* ------------------------------------------------------------------ */

START_TEST(test_default_index_zero)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.index, 0);

	teardown_options();
}
END_TEST

START_TEST(test_default_selector_zero)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.selector, 0);

	teardown_options();
}
END_TEST

START_TEST(test_default_test_count)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.test_count, DEFAULT_TEST_COUNT);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* FFU missing firmware file                                           */
/* ------------------------------------------------------------------ */

START_TEST(test_ffu_missing_fw_file)
{
	char *argv[] = {"ufs-utils", "ffu", "-t", "0",
			"-p", "/dev/ufs-bsg"};
	int argc = 6;

	setup_options();
	opts.config_type_inx = FFU_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_ffu_check_status_no_file_needed)
{
	char idn_str[16];
	snprintf(idn_str, sizeof(idn_str), "%d", UFS_CHECK_FFU_STATUS);

	char *argv[] = {"ufs-utils", "ffu", "-t", idn_str,
			"-p", "/dev/ufs-bsg"};
	int argc = 6;

	setup_options();
	opts.config_type_inx = FFU_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* RPMB num_block / start_block for non-RPMB type errors               */
/* ------------------------------------------------------------------ */

START_TEST(test_start_addr_non_rpmb_error)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-s", "100",
			"-r", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	/* -s for DESC_TYPE goes to verify_and_set_selector, not start_addr.
	 * selector=100 is valid, so this should succeed.
	 */
	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.selector, 100);

	teardown_options();
}
END_TEST

START_TEST(test_num_block_non_rpmb_error)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-n", "10",
			"-r", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Output mode                                                         */
/* ------------------------------------------------------------------ */

START_TEST(test_output_mode_raw)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-P", "raw", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;
	gl_pr_type = VERBOSE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(gl_pr_type, RAW_VALUE);

	gl_pr_type = VERBOSE;
	teardown_options();
}
END_TEST

START_TEST(test_output_mode_json)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-P", "json", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;
	gl_pr_type = VERBOSE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(gl_pr_type, JSON);

	gl_pr_type = VERBOSE;
	teardown_options();
}
END_TEST

START_TEST(test_output_mode_invalid)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "0", "-r",
			"-P", "invalid_mode", "-p", "/dev/ufs-bsg"};
	int argc = 9;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* EMON type tests                                                     */
/* ------------------------------------------------------------------ */

START_TEST(test_emon_test_count)
{
	char *argv[] = {"ufs-utils", "emon", "-t", "0", "-n", "50",
			"-p", "/dev/ufs-bsg"};
	int argc = 8;

	setup_options();
	opts.config_type_inx = EMON_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.test_count, 50);

	teardown_options();
}
END_TEST

START_TEST(test_emon_test_count_out_of_range)
{
	/* test_count must be > 0 and <= 0x7F (127) */
	char *argv[] = {"ufs-utils", "emon", "-t", "0", "-n", "200",
			"-p", "/dev/ufs-bsg"};
	int argc = 8;

	setup_options();
	opts.config_type_inx = EMON_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

START_TEST(test_emon_idn_valid)
{
	/* For EMON_TYPE, idn must be <= DME_PEER (1) */
	char *argv[] = {"ufs-utils", "emon", "-t", "1",
			"-p", "/dev/ufs-bsg"};
	int argc = 6;

	setup_options();
	opts.config_type_inx = EMON_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.idn, DME_PEER);

	teardown_options();
}
END_TEST

START_TEST(test_emon_idn_invalid)
{
	/* DME_PEER = 1, so 2 should be invalid */
	char *argv[] = {"ufs-utils", "emon", "-t", "2",
			"-p", "/dev/ufs-bsg"};
	int argc = 6;

	setup_options();
	opts.config_type_inx = EMON_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Combined options parsing                                            */
/* ------------------------------------------------------------------ */

START_TEST(test_combined_desc_read_all_fields)
{
	char *argv[] = {"ufs-utils", "desc", "-t", "2", "-r",
			"-i", "1", "-s", "0", "-d", "3",
			"-g", "1", "-p", "/dev/ufs-bsg"};
	int argc = 15;

	setup_options();
	opts.config_type_inx = DESC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.idn, QUERY_DESC_IDN_UNIT);
	ck_assert_int_eq(opts.opr, READ);
	ck_assert_int_eq(opts.index, 1);
	ck_assert_int_eq(opts.selector, 0);
	ck_assert_int_eq(opts.lun, 3);
	ck_assert_int_eq(opts.sg_type, SG4_TYPE);
	ck_assert_str_eq(opts.path, "/dev/ufs-bsg");

	teardown_options();
}
END_TEST

START_TEST(test_attr_write_hex_data)
{
	char *argv[] = {"ufs-utils", "attr", "-t", "3", "-w", "0xFF",
			"-p", "/dev/ufs-bsg"};
	int argc = 8;

	setup_options();
	opts.config_type_inx = ATTR_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(opts.opr, WRITE);
	ck_assert_ptr_nonnull(opts.data);
	ck_assert_int_eq(*(__u32 *)opts.data, 0xFF);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* UIC missing idn                                                     */
/* ------------------------------------------------------------------ */

START_TEST(test_uic_missing_idn)
{
	char *argv[] = {"ufs-utils", "uic", "-r", "-i", "0",
			"-p", "/dev/ufs-bsg"};
	int argc = 7;

	setup_options();
	opts.config_type_inx = UIC_TYPE;

	int rc = call_init(argc, argv);
	ck_assert_int_ne(rc, 0);

	teardown_options();
}
END_TEST

/* ------------------------------------------------------------------ */
/* Suite creation                                                      */
/* ------------------------------------------------------------------ */

Suite *options_suite(void)
{
	Suite *s;
	TCase *tc_parsing;
	TCase *tc_validation;
	TCase *tc_defaults;
	TCase *tc_duplicates;
	TCase *tc_ffu;
	TCase *tc_rpmb;
	TCase *tc_hmr;
	TCase *tc_uic;
	TCase *tc_emon;

	s = suite_create("Options");

	/* Basic parsing tests */
	tc_parsing = tcase_create("parsing");
	tcase_add_test(tc_parsing, test_parse_read_descriptor);
	tcase_add_test(tc_parsing, test_default_operation_is_read);
	tcase_add_test(tc_parsing, test_parse_write_descriptor);
	tcase_add_test(tc_parsing, test_parse_write_attribute);
	tcase_add_test(tc_parsing, test_parse_flag_set);
	tcase_add_test(tc_parsing, test_parse_flag_clear);
	tcase_add_test(tc_parsing, test_parse_flag_toggle);
	tcase_add_test(tc_parsing, test_parse_index);
	tcase_add_test(tc_parsing, test_parse_selector);
	tcase_add_test(tc_parsing, test_parse_read_all);
	tcase_add_test(tc_parsing, test_invalid_option);
	tcase_add_test(tc_parsing, test_combined_desc_read_all_fields);
	tcase_add_test(tc_parsing, test_attr_write_hex_data);
	tcase_add_test(tc_parsing, test_output_mode_raw);
	tcase_add_test(tc_parsing, test_output_mode_json);
	tcase_add_test(tc_parsing, test_output_mode_invalid);
	suite_add_tcase(s, tc_parsing);

	/* Validation tests */
	tc_validation = tcase_create("validation");
	tcase_add_test(tc_validation, test_invalid_desc_idn);
	tcase_add_test(tc_validation, test_valid_desc_idn_max);
	tcase_add_test(tc_validation, test_invalid_uic_idn);
	tcase_add_test(tc_validation, test_invalid_ffu_idn);
	tcase_add_test(tc_validation, test_invalid_rpmb_idn);
	tcase_add_test(tc_validation, test_negative_idn);
	tcase_add_test(tc_validation, test_missing_device_path);
	tcase_add_test(tc_validation, test_missing_idn_desc);
	tcase_add_test(tc_validation, test_idn_not_required_for_err_hist);
	tcase_add_test(tc_validation, test_string_desc_missing_index);
	tcase_add_test(tc_validation, test_string_desc_with_index);
	tcase_add_test(tc_validation, test_flag_opr_wrong_type);
	tcase_add_test(tc_validation, test_write_flag_error);
	tcase_add_test(tc_validation, test_sg_struct_sg3);
	tcase_add_test(tc_validation, test_sg_struct_sg4);
	tcase_add_test(tc_validation, test_sg_struct_invalid);
	tcase_add_test(tc_validation, test_lun_valid);
	tcase_add_test(tc_validation, test_lun_zero);
	tcase_add_test(tc_validation, test_lun_invalid);
	tcase_add_test(tc_validation, test_region_valid);
	tcase_add_test(tc_validation, test_region_max_valid);
	tcase_add_test(tc_validation, test_region_invalid);
	tcase_add_test(tc_validation, test_region_negative);
	tcase_add_test(tc_validation, test_num_block_non_rpmb_error);
	tcase_add_test(tc_validation, test_start_addr_non_rpmb_error);
	suite_add_tcase(s, tc_validation);

	/* Default values */
	tc_defaults = tcase_create("defaults");
	tcase_add_test(tc_defaults, test_default_index_zero);
	tcase_add_test(tc_defaults, test_default_selector_zero);
	tcase_add_test(tc_defaults, test_default_test_count);
	tcase_add_test(tc_defaults, test_vendor_default_length);
	suite_add_tcase(s, tc_defaults);

	/* Duplicate detection */
	tc_duplicates = tcase_create("duplicates");
	tcase_add_test(tc_duplicates, test_duplicate_idn);
	tcase_add_test(tc_duplicates, test_duplicate_index);
	tcase_add_test(tc_duplicates, test_duplicate_selector);
	tcase_add_test(tc_duplicates, test_duplicate_read);
	tcase_add_test(tc_duplicates, test_duplicate_target);
	tcase_add_test(tc_duplicates, test_duplicate_device_path);
	tcase_add_test(tc_duplicates, test_length_duplicate);
	tcase_add_test(tc_duplicates, test_offset_duplicate);
	suite_add_tcase(s, tc_duplicates);

	/* FFU tests */
	tc_ffu = tcase_create("ffu");
	tcase_add_test(tc_ffu, test_ffu_chunk_size_valid);
	tcase_add_test(tc_ffu, test_ffu_chunk_size_zero);
	tcase_add_test(tc_ffu, test_ffu_chunk_size_not_aligned);
	tcase_add_test(tc_ffu, test_ffu_chunk_size_too_large);
	tcase_add_test(tc_ffu, test_ffu_default_chunk_size);
	tcase_add_test(tc_ffu, test_ffu_missing_fw_file);
	tcase_add_test(tc_ffu, test_ffu_check_status_no_file_needed);
	suite_add_tcase(s, tc_ffu);

	/* RPMB tests */
	tc_rpmb = tcase_create("rpmb");
	tcase_add_test(tc_rpmb, test_rpmb_start_address);
	tcase_add_test(tc_rpmb, test_rpmb_start_address_out_of_range);
	tcase_add_test(tc_rpmb, test_rpmb_num_blocks);
	tcase_add_test(tc_rpmb, test_rpmb_key_path);
	tcase_add_test(tc_rpmb, test_rpmb_key_path_non_rpmb_type);
	suite_add_tcase(s, tc_rpmb);

	/* HMR tests */
	tc_hmr = tcase_create("hmr");
	tcase_add_test(tc_hmr, test_hmr_method_valid);
	tcase_add_test(tc_hmr, test_hmr_method_invalid);
	tcase_add_test(tc_hmr, test_hmr_unit_valid);
	tcase_add_test(tc_hmr, test_hmr_unit_invalid);
	tcase_add_test(tc_hmr, test_hmr_defaults);
	tcase_add_test(tc_hmr, test_hmr_duplicate_method);
	tcase_add_test(tc_hmr, test_hmr_duplicate_unit);
	suite_add_tcase(s, tc_hmr);

	/* UIC tests */
	tc_uic = tcase_create("uic");
	tcase_add_test(tc_uic, test_uic_peer_target);
	tcase_add_test(tc_uic, test_uic_local_target);
	tcase_add_test(tc_uic, test_uic_write_requires_target);
	tcase_add_test(tc_uic, test_uic_write_with_target);
	tcase_add_test(tc_uic, test_uic_read_requires_index);
	tcase_add_test(tc_uic, test_uic_static_set_type);
	tcase_add_test(tc_uic, test_uic_missing_idn);
	suite_add_tcase(s, tc_uic);

	/* EMON tests */
	tc_emon = tcase_create("emon");
	tcase_add_test(tc_emon, test_emon_test_count);
	tcase_add_test(tc_emon, test_emon_test_count_out_of_range);
	tcase_add_test(tc_emon, test_emon_idn_valid);
	tcase_add_test(tc_emon, test_emon_idn_invalid);
	suite_add_tcase(s, tc_emon);

	/* Length/Offset tests */
	TCase *tc_len_off = tcase_create("length_offset");
	tcase_add_test(tc_len_off, test_length_valid);
	tcase_add_test(tc_len_off, test_offset_valid);
	suite_add_tcase(s, tc_len_off);

	return s;
}
