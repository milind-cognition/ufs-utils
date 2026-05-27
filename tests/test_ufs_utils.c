// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright (C) 2019 Western Digital Corporation or its affiliates */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <getopt.h>

#include "ufs.h"
#include "ufs_cmds.h"
#include "options.h"
#include "unipro.h"
#include "sha2.h"
#include "hmac_sha2.h"

/* ------------------------------------------------------------------ */
/* str_to_long tests                                                  */
/* ------------------------------------------------------------------ */

static void test_str_to_long_decimal(void **state)
{
	(void)state;
	long result = 0;

	optarg = "42";
	assert_int_equal(str_to_long("42", 10, &result), OK);
	assert_int_equal(result, 42);
}

static void test_str_to_long_hex(void **state)
{
	(void)state;
	long result = 0;

	optarg = "0xFF";
	assert_int_equal(str_to_long("0xFF", 16, &result), OK);
	assert_int_equal(result, 0xFF);
}

static void test_str_to_long_zero(void **state)
{
	(void)state;
	long result = -1;

	optarg = "0";
	assert_int_equal(str_to_long("0", 10, &result), OK);
	assert_int_equal(result, 0);
}

static void test_str_to_long_negative(void **state)
{
	(void)state;
	long result = 0;

	optarg = "-123";
	assert_int_equal(str_to_long("-123", 10, &result), OK);
	assert_int_equal(result, -123);
}

static void test_str_to_long_null_nptr(void **state)
{
	(void)state;
	long result = 0;

	assert_int_equal(str_to_long(NULL, 10, &result), ERROR);
}

static void test_str_to_long_null_result(void **state)
{
	(void)state;

	optarg = "42";
	assert_int_equal(str_to_long("42", 10, NULL), ERROR);
}

static void test_str_to_long_invalid_string(void **state)
{
	(void)state;
	long result = 0;

	optarg = "notanumber";
	assert_int_equal(str_to_long("notanumber", 10, &result), ERROR);
}

static void test_str_to_long_partial_number(void **state)
{
	(void)state;
	long result = 0;

	optarg = "42abc";
	assert_int_equal(str_to_long("42abc", 10, &result), ERROR);
}

static void test_str_to_long_empty_string(void **state)
{
	(void)state;
	long result = 0;

	optarg = "";
	assert_int_equal(str_to_long("", 10, &result), ERROR);
}

/* ------------------------------------------------------------------ */
/* write_file tests                                                   */
/* ------------------------------------------------------------------ */

static void test_write_file_success(void **state)
{
	(void)state;
	const char *tmpfile = "/tmp/ufs_test_write_file.bin";
	const char data[] = "hello ufs";
	int rc;

	rc = write_file(tmpfile, data, sizeof(data));
	assert_int_equal(rc, 0);

	/* Verify contents */
	FILE *f = fopen(tmpfile, "rb");
	assert_non_null(f);
	char buf[64] = {0};
	size_t n = fread(buf, 1, sizeof(buf), f);
	fclose(f);
	assert_int_equal(n, sizeof(data));
	assert_memory_equal(buf, data, sizeof(data));

	unlink(tmpfile);
}

static void test_write_file_bad_path(void **state)
{
	(void)state;
	int rc;

	rc = write_file("/nonexistent/dir/file.bin", "data", 4);
	assert_int_not_equal(rc, 0);
}

static void test_write_file_empty(void **state)
{
	(void)state;
	const char *tmpfile = "/tmp/ufs_test_write_empty.bin";
	int rc;

	rc = write_file(tmpfile, "", 0);
	assert_int_equal(rc, 0);

	/* Verify file exists and is empty */
	FILE *f = fopen(tmpfile, "rb");
	assert_non_null(f);
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fclose(f);
	assert_int_equal(sz, 0);

	unlink(tmpfile);
}

/* ------------------------------------------------------------------ */
/* UIC macro tests                                                    */
/* ------------------------------------------------------------------ */

static void test_uic_arg_mib_sel(void **state)
{
	(void)state;

	__u32 val = UIC_ARG_MIB_SEL(0x1560, 0x0001);
	assert_int_equal((val >> 16) & 0xFFFF, 0x1560);
	assert_int_equal(val & 0xFFFF, 0x0001);
}

static void test_uic_arg_mib(void **state)
{
	(void)state;

	__u32 val = UIC_ARG_MIB(0x1560);
	assert_int_equal((val >> 16) & 0xFFFF, 0x1560);
	assert_int_equal(val & 0xFFFF, 0x0000);
}

static void test_uic_get_attr_id(void **state)
{
	(void)state;

	__u32 val = UIC_ARG_MIB_SEL(0x1560, 0x0001);
	assert_int_equal(UIC_GET_ATTR_ID(val), 0x1560);
}

static void test_uic_arg_attr_type(void **state)
{
	(void)state;

	__u32 val = UIC_ARG_ATTR_TYPE(0x02);
	assert_int_equal((val >> 16) & 0xFF, 0x02);
}

/* ------------------------------------------------------------------ */
/* SHA-256 tests (RFC 6234 test vectors)                              */
/* ------------------------------------------------------------------ */

static void test_sha256_empty(void **state)
{
	(void)state;
	unsigned char digest[SHA256_DIGEST_SIZE];
	/* SHA-256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 */
	const unsigned char expected[SHA256_DIGEST_SIZE] = {
		0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
		0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
		0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
		0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
	};

	sha256((const unsigned char *)"", 0, digest);
	assert_memory_equal(digest, expected, SHA256_DIGEST_SIZE);
}

static void test_sha256_abc(void **state)
{
	(void)state;
	unsigned char digest[SHA256_DIGEST_SIZE];
	/* SHA-256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad */
	const unsigned char expected[SHA256_DIGEST_SIZE] = {
		0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
		0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
		0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
		0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
	};

	sha256((const unsigned char *)"abc", 3, digest);
	assert_memory_equal(digest, expected, SHA256_DIGEST_SIZE);
}

static void test_sha256_incremental(void **state)
{
	(void)state;
	sha256_ctx ctx;
	unsigned char digest[SHA256_DIGEST_SIZE];
	const unsigned char expected[SHA256_DIGEST_SIZE] = {
		0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
		0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
		0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
		0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
	};

	sha256_init(&ctx);
	sha256_update(&ctx, (const unsigned char *)"a", 1);
	sha256_update(&ctx, (const unsigned char *)"bc", 2);
	sha256_final(&ctx, digest);
	assert_memory_equal(digest, expected, SHA256_DIGEST_SIZE);
}

/* ------------------------------------------------------------------ */
/* HMAC-SHA-256 tests (RFC 4231 test vector #2)                       */
/* ------------------------------------------------------------------ */

static void test_hmac_sha256_rfc4231(void **state)
{
	(void)state;
	/* RFC 4231 Test Case 2: Key = "Jefe", Data = "what do ya want for nothing?" */
	const unsigned char key[] = "Jefe";
	const unsigned char data[] = "what do ya want for nothing?";
	unsigned char mac[SHA256_DIGEST_SIZE];
	const unsigned char expected[SHA256_DIGEST_SIZE] = {
		0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e,
		0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
		0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83,
		0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43
	};

	hmac_sha256(key, 4, data, 28, mac, SHA256_DIGEST_SIZE);
	assert_memory_equal(mac, expected, SHA256_DIGEST_SIZE);
}

static void test_hmac_sha256_incremental(void **state)
{
	(void)state;
	const unsigned char key[] = "Jefe";
	const unsigned char data[] = "what do ya want for nothing?";
	unsigned char mac[SHA256_DIGEST_SIZE];
	const unsigned char expected[SHA256_DIGEST_SIZE] = {
		0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e,
		0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
		0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83,
		0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43
	};
	hmac_sha256_ctx ctx;

	hmac_sha256_init(&ctx, key, 4);
	hmac_sha256_update(&ctx, data, 14);
	hmac_sha256_update(&ctx, data + 14, 14);
	hmac_sha256_final(&ctx, mac, SHA256_DIGEST_SIZE);
	assert_memory_equal(mac, expected, SHA256_DIGEST_SIZE);
}

/* ------------------------------------------------------------------ */
/* Enum / constant sanity tests                                       */
/* ------------------------------------------------------------------ */

static void test_enum_flag_idn_values(void **state)
{
	(void)state;
	assert_int_equal(QUERY_FLAG_IDN_FDEVICEINIT, 0x01);
	assert_int_equal(QUERY_FLAG_IDN_BKOPS_EN, 0x04);
	assert_int_equal(QUERY_FLAG_IDN_BUSY_RTC, 0x09);
	assert_int_equal(QUERY_FLAG_IDN_WB_EN, 0x0E);
}

static void test_enum_desc_idn_values(void **state)
{
	(void)state;
	assert_int_equal(QUERY_DESC_IDN_DEVICE, 0x0);
	assert_int_equal(QUERY_DESC_IDN_CONFIGURAION, 0x1);
	assert_int_equal(QUERY_DESC_IDN_GEOMETRY, 0x7);
	assert_int_equal(QUERY_DESC_IDN_HEALTH, 0x9);
	assert_int_equal(QUERY_DESC_IDN_MAX, 0xFF);
}

static void test_enum_query_opcode_values(void **state)
{
	(void)state;
	assert_int_equal(UPIU_QUERY_OPCODE_NOP, 0x0);
	assert_int_equal(UPIU_QUERY_OPCODE_READ_DESC, 0x1);
	assert_int_equal(UPIU_QUERY_OPCODE_WRITE_DESC, 0x2);
	assert_int_equal(UPIU_QUERY_OPCODE_READ_ATTR, 0x3);
	assert_int_equal(UPIU_QUERY_OPCODE_WRITE_ATTR, 0x4);
	assert_int_equal(UPIU_QUERY_OPCODE_READ_FLAG, 0x5);
	assert_int_equal(UPIU_QUERY_OPCODE_SET_FLAG, 0x6);
}

static void test_desc_max_sizes(void **state)
{
	(void)state;
	assert_true(QUERY_DESC_DEVICE_MAX_SIZE >= QUERY_DESC_DEVICE_MAX_SIZE_3_0);
	assert_true(QUERY_DESC_CONFIGURAION_MAX_SIZE >= QUERY_DESC_CONFIGURAION_MAX_SIZE_3_0);
	assert_true(QUERY_DESC_UNIT_MAX_SIZE >= QUERY_DESC_UNIT_MAX_SIZE_3_0);
	assert_int_equal(QUERY_DESC_STRING_MAX_SIZE, 0xFE);
	assert_int_equal(QUERY_DESC_MAX_SIZE, 255);
}

static void test_constants(void **state)
{
	(void)state;
	assert_int_equal(BLOCK_SIZE, 512);
	assert_int_equal(MAX_IOCTL_BUF_SIZE, 256L * 1024);
	assert_int_equal(BSG_REQUEST_SZ, sizeof(struct ufs_bsg_request));
	assert_int_equal(BSG_REPLY_SZ, sizeof(struct ufs_bsg_reply));
}

static void test_upiu_flags(void **state)
{
	(void)state;
	assert_int_equal(UPIU_CMD_FLAGS_NONE, 0x00);
	assert_int_equal(UPIU_CMD_FLAGS_WRITE, 0x20);
	assert_int_equal(UPIU_CMD_FLAGS_READ, 0x40);
}

static void test_config_type_enum(void **state)
{
	(void)state;
	assert_int_equal(DESC_TYPE, 0);
	assert_int_equal(ATTR_TYPE, 1);
	assert_int_equal(FLAG_TYPE, 2);
	assert_int_equal(ERR_HIST_TYPE, 3);
	assert_int_equal(UIC_TYPE, 4);
	assert_int_equal(FFU_TYPE, 5);
	assert_int_equal(VENDOR_BUFFER_TYPE, 6);
	assert_int_equal(RPMB_CMD_TYPE, 7);
}

/* ------------------------------------------------------------------ */
/* Options struct default values                                      */
/* ------------------------------------------------------------------ */

static void test_field_widths(void **state)
{
	(void)state;
	assert_int_equal(BYTE, (1 << 0));
	assert_int_equal(WORD, (1 << 1));
	assert_int_equal(DWORD, (1 << 2));
	assert_int_equal(DDWORD, (1 << 3));
}

static void test_access_modes(void **state)
{
	(void)state;
	assert_int_equal(READ_NRML, (1 << 0));
	assert_int_equal(READ_ONLY, (1 << 1));
	assert_int_equal(WRITE_ONLY, (1 << 2));
	assert_int_equal(WRITE_ONCE, (1 << 3));
}

static void test_sg_type_values(void **state)
{
	(void)state;
	assert_int_equal(SG3_TYPE, 0);
	assert_int_equal(SG4_TYPE, 1);
}

/* ------------------------------------------------------------------ */
/* SHA-512 tests                                                      */
/* ------------------------------------------------------------------ */

static void test_sha512_empty(void **state)
{
	(void)state;
	unsigned char digest[SHA512_DIGEST_SIZE];
	/* SHA-512("") known vector */
	const unsigned char expected[SHA512_DIGEST_SIZE] = {
		0xcf, 0x83, 0xe1, 0x35, 0x7e, 0xef, 0xb8, 0xbd,
		0xf1, 0x54, 0x28, 0x50, 0xd6, 0x6d, 0x80, 0x07,
		0xd6, 0x20, 0xe4, 0x05, 0x0b, 0x57, 0x15, 0xdc,
		0x83, 0xf4, 0xa9, 0x21, 0xd3, 0x6c, 0xe9, 0xce,
		0x47, 0xd0, 0xd1, 0x3c, 0x5d, 0x85, 0xf2, 0xb0,
		0xff, 0x83, 0x18, 0xd2, 0x87, 0x7e, 0xec, 0x2f,
		0x63, 0xb9, 0x31, 0xbd, 0x47, 0x41, 0x7a, 0x81,
		0xa5, 0x38, 0x32, 0x7a, 0xf9, 0x27, 0xda, 0x3e
	};

	sha512((const unsigned char *)"", 0, digest);
	assert_memory_equal(digest, expected, SHA512_DIGEST_SIZE);
}

static void test_sha512_abc(void **state)
{
	(void)state;
	unsigned char digest[SHA512_DIGEST_SIZE];
	/* SHA-512("abc") known vector */
	const unsigned char expected[SHA512_DIGEST_SIZE] = {
		0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba,
		0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
		0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
		0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
		0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8,
		0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
		0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e,
		0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f
	};

	sha512((const unsigned char *)"abc", 3, digest);
	assert_memory_equal(digest, expected, SHA512_DIGEST_SIZE);
}

/* ------------------------------------------------------------------ */
/* main                                                               */
/* ------------------------------------------------------------------ */

int main(void)
{
	const struct CMUnitTest tests[] = {
		/* str_to_long */
		cmocka_unit_test(test_str_to_long_decimal),
		cmocka_unit_test(test_str_to_long_hex),
		cmocka_unit_test(test_str_to_long_zero),
		cmocka_unit_test(test_str_to_long_negative),
		cmocka_unit_test(test_str_to_long_null_nptr),
		cmocka_unit_test(test_str_to_long_null_result),
		cmocka_unit_test(test_str_to_long_invalid_string),
		cmocka_unit_test(test_str_to_long_partial_number),
		cmocka_unit_test(test_str_to_long_empty_string),
		/* write_file */
		cmocka_unit_test(test_write_file_success),
		cmocka_unit_test(test_write_file_bad_path),
		cmocka_unit_test(test_write_file_empty),
		/* UIC macros */
		cmocka_unit_test(test_uic_arg_mib_sel),
		cmocka_unit_test(test_uic_arg_mib),
		cmocka_unit_test(test_uic_get_attr_id),
		cmocka_unit_test(test_uic_arg_attr_type),
		/* SHA-256 */
		cmocka_unit_test(test_sha256_empty),
		cmocka_unit_test(test_sha256_abc),
		cmocka_unit_test(test_sha256_incremental),
		/* HMAC-SHA-256 */
		cmocka_unit_test(test_hmac_sha256_rfc4231),
		cmocka_unit_test(test_hmac_sha256_incremental),
		/* Enums and constants */
		cmocka_unit_test(test_enum_flag_idn_values),
		cmocka_unit_test(test_enum_desc_idn_values),
		cmocka_unit_test(test_enum_query_opcode_values),
		cmocka_unit_test(test_desc_max_sizes),
		cmocka_unit_test(test_constants),
		cmocka_unit_test(test_upiu_flags),
		cmocka_unit_test(test_config_type_enum),
		/* Options */
		cmocka_unit_test(test_field_widths),
		cmocka_unit_test(test_access_modes),
		cmocka_unit_test(test_sg_type_values),
		/* SHA-512 */
		cmocka_unit_test(test_sha512_empty),
		cmocka_unit_test(test_sha512_abc),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
