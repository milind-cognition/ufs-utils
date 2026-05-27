// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for scsi_bsg_util.c utility functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>

#include "test_framework.h"

/* Include source to access static functions */
#include "ufs.c"
#include "scsi_bsg_util.c"

/* ---- sense_key_string tests ---- */

static void test_sense_key_string_valid(void)
{
	TEST_ASSERT_STR_EQ(sense_key_string(0), "No Sense",
			   "sense key 0 should be No Sense");
	TEST_ASSERT_STR_EQ(sense_key_string(1), "Recovered Error",
			   "sense key 1");
	TEST_ASSERT_STR_EQ(sense_key_string(2), "Not Ready",
			   "sense key 2");
	TEST_ASSERT_STR_EQ(sense_key_string(3), "Medium Error",
			   "sense key 3");
	TEST_ASSERT_STR_EQ(sense_key_string(4), "Hardware Error",
			   "sense key 4");
	TEST_ASSERT_STR_EQ(sense_key_string(5), "Illegal Request",
			   "sense key 5");
	TEST_ASSERT_STR_EQ(sense_key_string(6), "Unit Attention",
			   "sense key 6");
	TEST_ASSERT_STR_EQ(sense_key_string(7), "Data Protect",
			   "sense key 7");
	TEST_ASSERT_STR_EQ(sense_key_string(0xE), "Miscompare",
			   "sense key 0xE");
}

static void test_sense_key_string_invalid(void)
{
	TEST_ASSERT_NULL(sense_key_string(0xF),
			 "sense key 0xF should return NULL");
	TEST_ASSERT_NULL(sense_key_string(0xFF),
			 "sense key 0xFF should return NULL");
}

/* ---- put_unaligned_be24 tests ---- */

static void test_put_unaligned_be24_zero(void)
{
	__u8 buf[3] = {0xFF, 0xFF, 0xFF};

	put_unaligned_be24(0, buf);
	TEST_ASSERT_EQ(buf[0], 0, "be24 zero byte 0");
	TEST_ASSERT_EQ(buf[1], 0, "be24 zero byte 1");
	TEST_ASSERT_EQ(buf[2], 0, "be24 zero byte 2");
}

static void test_put_unaligned_be24_max(void)
{
	__u8 buf[3] = {0};

	put_unaligned_be24(0xFFFFFF, buf);
	TEST_ASSERT_EQ(buf[0], 0xFF, "be24 max byte 0");
	TEST_ASSERT_EQ(buf[1], 0xFF, "be24 max byte 1");
	TEST_ASSERT_EQ(buf[2], 0xFF, "be24 max byte 2");
}

static void test_put_unaligned_be24_pattern(void)
{
	__u8 buf[3] = {0};

	put_unaligned_be24(0x123456, buf);
	TEST_ASSERT_EQ(buf[0], 0x12, "be24 pattern byte 0");
	TEST_ASSERT_EQ(buf[1], 0x34, "be24 pattern byte 1");
	TEST_ASSERT_EQ(buf[2], 0x56, "be24 pattern byte 2");
}

static void test_put_unaligned_be24_small(void)
{
	__u8 buf[3] = {0};

	put_unaligned_be24(1, buf);
	TEST_ASSERT_EQ(buf[0], 0, "be24 small byte 0");
	TEST_ASSERT_EQ(buf[1], 0, "be24 small byte 1");
	TEST_ASSERT_EQ(buf[2], 1, "be24 small byte 2");
}

/* ---- prepare_security_cdb tests ---- */

static void test_prepare_security_cdb_null(void)
{
	int rc = prepare_security_cdb(NULL, 512, 0, SECURITY_PROTOCOL_IN);

	TEST_ASSERT_EQ(rc, ERROR, "prepare_security_cdb with NULL should fail");
}

static void test_prepare_security_cdb_in(void)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE] = {0};
	int rc;

	rc = prepare_security_cdb(cdb, 512, 0, SECURITY_PROTOCOL_IN);
	TEST_ASSERT_EQ(rc, 0, "prepare_security_cdb in should succeed");
	TEST_ASSERT_EQ(cdb[0], SECURITY_PROTOCOL_IN, "opcode should be SEC_IN");
	TEST_ASSERT_EQ(cdb[1], SEC_PROTOCOL_UFS, "protocol should be UFS");
}

static void test_prepare_security_cdb_out(void)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE] = {0};
	int rc;

	rc = prepare_security_cdb(cdb, 1024, 0, SECURITY_PROTOCOL_OUT);
	TEST_ASSERT_EQ(rc, 0, "prepare_security_cdb out should succeed");
	TEST_ASSERT_EQ(cdb[0], SECURITY_PROTOCOL_OUT, "opcode should be SEC_OUT");
}

static void test_prepare_security_cdb_region(void)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE] = {0};
	int rc;
	__u16 sec_spec;

	rc = prepare_security_cdb(cdb, 512, 2, SECURITY_PROTOCOL_IN);
	TEST_ASSERT_EQ(rc, 0, "prepare_security_cdb with region 2 should succeed");

	sec_spec = be16toh(*(__u16 *)(cdb + SEC_SPEC_OFFSET));
	TEST_ASSERT_EQ((sec_spec >> 8) & 0xFF, 2, "region should be 2");
	TEST_ASSERT_EQ(sec_spec & 0xFF, SEC_SPECIFIC_UFS_RPMB & 0xFF,
		       "SEC_SPECIFIC should be UFS RPMB");
}

/* ---- prepare_upiu tests ---- */

static void test_prepare_upiu_read_desc(void)
{
	struct ufs_bsg_request req;

	memset(&req, 0, sizeof(req));
	prepare_upiu(&req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     255, UPIU_QUERY_OPCODE_READ_DESC, 0, 0, 0);

	TEST_ASSERT_EQ(req.msgcode, UPIU_TRANSACTION_QUERY_REQ,
		       "msgcode should be QUERY_REQ");
	TEST_ASSERT_EQ(req.upiu_req.qr.opcode, UPIU_QUERY_OPCODE_READ_DESC,
		       "opcode should be READ_DESC");
	TEST_ASSERT_EQ(req.upiu_req.qr.idn, 0, "idn should be 0");
	TEST_ASSERT_EQ(req.upiu_req.qr.index, 0, "index should be 0");
	TEST_ASSERT_EQ(req.upiu_req.qr.selector, 0, "selector should be 0");
	TEST_ASSERT_EQ(be16toh(req.upiu_req.qr.length), 255,
		       "length should be 255");
}

static void test_prepare_upiu_write_attr(void)
{
	struct ufs_bsg_request req;

	memset(&req, 0, sizeof(req));
	prepare_upiu(&req, UPIU_QUERY_FUNC_STANDARD_WRITE_REQUEST,
		     0, UPIU_QUERY_OPCODE_WRITE_ATTR, 3, 1, 2);

	TEST_ASSERT_EQ(req.upiu_req.qr.opcode, UPIU_QUERY_OPCODE_WRITE_ATTR,
		       "opcode should be WRITE_ATTR");
	TEST_ASSERT_EQ(req.upiu_req.qr.idn, 3, "idn should be 3");
	TEST_ASSERT_EQ(req.upiu_req.qr.index, 1, "index should be 1");
	TEST_ASSERT_EQ(req.upiu_req.qr.selector, 2, "selector should be 2");
}

static void test_prepare_upiu_read_flag(void)
{
	struct ufs_bsg_request req;

	memset(&req, 0, sizeof(req));
	prepare_upiu(&req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     0, UPIU_QUERY_OPCODE_READ_FLAG, 4, 0, 0);

	TEST_ASSERT_EQ(req.upiu_req.qr.opcode, UPIU_QUERY_OPCODE_READ_FLAG,
		       "opcode should be READ_FLAG");
	TEST_ASSERT_EQ(req.upiu_req.qr.idn, 4, "idn should be 4");
}

/* ---- prepare_command_upiu tests ---- */

static void test_prepare_command_upiu_read(void)
{
	struct utp_upiu_req req;
	__u8 cdb[UFS_CDB_SIZE] = {0x28, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0};

	memset(&req, 0, sizeof(req));
	prepare_command_upiu(&req, UPIU_CMD_FLAGS_READ, 0, 0,
			     cdb, UFS_CDB_SIZE, 512);

	TEST_ASSERT_EQ(req.sc.cdb[0], 0x28, "CDB opcode should be 0x28 (READ_10)");
	TEST_ASSERT_EQ(be32toh(req.sc.exp_data_transfer_len), 512,
		       "exp_data_transfer_len should be 512");
}

static void test_prepare_command_upiu_write(void)
{
	struct utp_upiu_req req;
	__u8 cdb[UFS_CDB_SIZE] = {0x2A, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0};

	memset(&req, 0, sizeof(req));
	prepare_command_upiu(&req, UPIU_CMD_FLAGS_WRITE, 1, 0,
			     cdb, UFS_CDB_SIZE, 1024);

	TEST_ASSERT_EQ(req.sc.cdb[0], 0x2A, "CDB opcode should be 0x2A (WRITE_10)");
	TEST_ASSERT_EQ(be32toh(req.sc.exp_data_transfer_len), 1024,
		       "exp_data_transfer_len should be 1024");
}

/* ---- write_buffer / read_buffer parameter validation ---- */

static void test_write_buffer_invalid_fd(void)
{
	__u8 buf[512] = {0};
	int rc = write_buffer(-1, buf, BUFFER_DATA_MODE, 0, 0, 512, SG3_TYPE);

	TEST_ASSERT(rc != 0, "write_buffer with invalid fd should fail");
}

static void test_read_buffer_invalid_fd(void)
{
	__u8 buf[512] = {0};
	int rc = read_buffer(-1, buf, BUFFER_DATA_MODE, 0, 0, 512, SG3_TYPE);

	TEST_ASSERT(rc != 0, "read_buffer with invalid fd should fail");
}

static void test_write_buffer_negative_count(void)
{
	__u8 buf[512] = {0};
	int rc = write_buffer(3, buf, BUFFER_DATA_MODE, 0, 0, -1, SG3_TYPE);

	TEST_ASSERT(rc != 0, "write_buffer with negative count should fail");
}

static void test_read_buffer_negative_count(void)
{
	__u8 buf[512] = {0};
	int rc = read_buffer(3, buf, BUFFER_DATA_MODE, 0, 0, -1, SG3_TYPE);

	TEST_ASSERT(rc != 0, "read_buffer with negative count should fail");
}

/* ---- scsi_security_in/out parameter validation ---- */

static void test_scsi_security_in_invalid_fd(void)
{
	struct rpmb_frame frame;
	int rc = scsi_security_in(-1, &frame, 1, 0, SG3_TYPE);

	TEST_ASSERT(rc != 0, "scsi_security_in with invalid fd should fail");
}

static void test_scsi_security_in_null_frame(void)
{
	int rc = scsi_security_in(3, NULL, 1, 0, SG3_TYPE);

	TEST_ASSERT(rc != 0, "scsi_security_in with NULL frame should fail");
}

static void test_scsi_security_in_zero_count(void)
{
	struct rpmb_frame frame;
	int rc = scsi_security_in(3, &frame, 0, 0, SG3_TYPE);

	TEST_ASSERT(rc != 0, "scsi_security_in with zero count should fail");
}

static void test_scsi_security_out_invalid_params(void)
{
	int rc = scsi_security_out(3, NULL, 1, 0, SG3_TYPE);

	TEST_ASSERT(rc != 0, "scsi_security_out with NULL frame should fail");
}

int main(void)
{
	TEST_INIT();
	printf("\n[test_scsi_bsg] Running SCSI BSG utility tests\n");

	RUN_TEST(test_sense_key_string_valid);
	RUN_TEST(test_sense_key_string_invalid);

	RUN_TEST(test_put_unaligned_be24_zero);
	RUN_TEST(test_put_unaligned_be24_max);
	RUN_TEST(test_put_unaligned_be24_pattern);
	RUN_TEST(test_put_unaligned_be24_small);

	RUN_TEST(test_prepare_security_cdb_null);
	RUN_TEST(test_prepare_security_cdb_in);
	RUN_TEST(test_prepare_security_cdb_out);
	RUN_TEST(test_prepare_security_cdb_region);

	RUN_TEST(test_prepare_upiu_read_desc);
	RUN_TEST(test_prepare_upiu_write_attr);
	RUN_TEST(test_prepare_upiu_read_flag);

	RUN_TEST(test_prepare_command_upiu_read);
	RUN_TEST(test_prepare_command_upiu_write);

	RUN_TEST(test_write_buffer_invalid_fd);
	RUN_TEST(test_read_buffer_invalid_fd);
	RUN_TEST(test_write_buffer_negative_count);
	RUN_TEST(test_read_buffer_negative_count);

	RUN_TEST(test_scsi_security_in_invalid_fd);
	RUN_TEST(test_scsi_security_in_null_frame);
	RUN_TEST(test_scsi_security_in_zero_count);
	RUN_TEST(test_scsi_security_out_invalid_params);

	TEST_REPORT("test_scsi_bsg");
	return TEST_RESULT();
}
