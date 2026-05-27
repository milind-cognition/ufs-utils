// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for SCSI/BSG utility functions with hardware mocking */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <endian.h>
#include <linux/types.h>

#include "../ioctl.h"
#include "../ufs.h"
#include "../scsi_bsg_util.h"
#include "../ufs_rpmb.h"
#include "mocks/mock_ioctl.h"

/* ========== prepare_upiu tests ========== */

START_TEST(test_prepare_upiu_read_desc)
{
	struct ufs_bsg_request bsg_req;

	memset(&bsg_req, 0, sizeof(bsg_req));
	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     QUERY_DESC_MAX_SIZE, UPIU_QUERY_OPCODE_READ_DESC,
		     QUERY_DESC_IDN_DEVICE, 0, 0);

	ck_assert_int_eq(bsg_req.msgcode, UPIU_TRANSACTION_QUERY_REQ);
	ck_assert_int_eq(bsg_req.upiu_req.qr.opcode, UPIU_QUERY_OPCODE_READ_DESC);
	ck_assert_int_eq(bsg_req.upiu_req.qr.idn, QUERY_DESC_IDN_DEVICE);
	ck_assert_int_eq(bsg_req.upiu_req.qr.index, 0);
	ck_assert_int_eq(bsg_req.upiu_req.qr.selector, 0);
	ck_assert_int_eq(be16toh(bsg_req.upiu_req.qr.length), QUERY_DESC_MAX_SIZE);
}
END_TEST

START_TEST(test_prepare_upiu_write_attr)
{
	struct ufs_bsg_request bsg_req;

	memset(&bsg_req, 0, sizeof(bsg_req));
	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_WRITE_REQUEST,
		     0, UPIU_QUERY_OPCODE_WRITE_ATTR,
		     QUERY_ATTR_IDN_ACTIVE_ICC_LVL, 0, 0);

	ck_assert_int_eq(bsg_req.msgcode, UPIU_TRANSACTION_QUERY_REQ);
	ck_assert_int_eq(bsg_req.upiu_req.qr.opcode, UPIU_QUERY_OPCODE_WRITE_ATTR);
	ck_assert_int_eq(bsg_req.upiu_req.qr.idn, QUERY_ATTR_IDN_ACTIVE_ICC_LVL);
}
END_TEST

START_TEST(test_prepare_upiu_with_index_selector)
{
	struct ufs_bsg_request bsg_req;

	memset(&bsg_req, 0, sizeof(bsg_req));
	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     QUERY_DESC_MAX_SIZE, UPIU_QUERY_OPCODE_READ_DESC,
		     QUERY_DESC_IDN_UNIT, 3, 1);

	ck_assert_int_eq(bsg_req.upiu_req.qr.index, 3);
	ck_assert_int_eq(bsg_req.upiu_req.qr.selector, 1);
	ck_assert_int_eq(bsg_req.upiu_req.qr.idn, QUERY_DESC_IDN_UNIT);
}
END_TEST

START_TEST(test_prepare_upiu_set_flag)
{
	struct ufs_bsg_request bsg_req;

	memset(&bsg_req, 0, sizeof(bsg_req));
	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_WRITE_REQUEST,
		     0, UPIU_QUERY_OPCODE_SET_FLAG,
		     QUERY_FLAG_IDN_FDEVICEINIT, 0, 0);

	ck_assert_int_eq(bsg_req.upiu_req.qr.opcode, UPIU_QUERY_OPCODE_SET_FLAG);
	ck_assert_int_eq(bsg_req.upiu_req.qr.idn, QUERY_FLAG_IDN_FDEVICEINIT);
}
END_TEST

START_TEST(test_prepare_upiu_data_length_encoding)
{
	struct ufs_bsg_request bsg_req;
	__u16 test_len = 0x100;

	memset(&bsg_req, 0, sizeof(bsg_req));
	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     test_len, UPIU_QUERY_OPCODE_READ_DESC,
		     QUERY_DESC_IDN_GEOMETRY, 0, 0);

	ck_assert_int_eq(be16toh(bsg_req.upiu_req.qr.length), test_len);
}
END_TEST

/* ========== prepare_command_upiu tests ========== */

START_TEST(test_prepare_command_upiu_read)
{
	struct utp_upiu_req upiu_req;
	__u8 cdb[UFS_CDB_SIZE] = {0x28, 0, 0, 0, 0, 0, 0, 0, 1, 0};

	memset(&upiu_req, 0, sizeof(upiu_req));
	prepare_command_upiu(&upiu_req, UPIU_CMD_FLAGS_READ, 0, 0,
			     cdb, UFS_CDB_SIZE, 512);

	ck_assert_int_eq(be32toh(upiu_req.sc.exp_data_transfer_len), 512);
	ck_assert_mem_eq(upiu_req.sc.cdb, cdb, UFS_CDB_SIZE);
}
END_TEST

START_TEST(test_prepare_command_upiu_write)
{
	struct utp_upiu_req upiu_req;
	__u8 cdb[UFS_CDB_SIZE] = {0x2A, 0, 0, 0, 0, 0, 0, 0, 1, 0};

	memset(&upiu_req, 0, sizeof(upiu_req));
	prepare_command_upiu(&upiu_req, UPIU_CMD_FLAGS_WRITE, 1, 0,
			     cdb, UFS_CDB_SIZE, 4096);

	ck_assert_int_eq(be32toh(upiu_req.sc.exp_data_transfer_len), 4096);
	ck_assert_mem_eq(upiu_req.sc.cdb, cdb, UFS_CDB_SIZE);
}
END_TEST

START_TEST(test_prepare_command_upiu_with_ehs)
{
	struct utp_upiu_req upiu_req;
	__u8 cdb[UFS_CDB_SIZE] = {0};

	memset(&upiu_req, 0, sizeof(upiu_req));
	prepare_command_upiu(&upiu_req, UPIU_CMD_FLAGS_NONE, 0, 2,
			     cdb, UFS_CDB_SIZE, 0);

	/* Verify EHS length is encoded in header dword_2 */
	__u32 dw2 = be32toh(upiu_req.header.dword_2);
	ck_assert_int_eq((dw2 >> 24) & 0xFF, 2);
}
END_TEST

/* ========== prepare_security_cdb tests ========== */

START_TEST(test_prepare_security_cdb_in)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE] = {0};
	int rc;

	rc = prepare_security_cdb(cdb, 512, 0, SECURITY_PROTOCOL_IN);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(cdb[0], SECURITY_PROTOCOL_IN);
	ck_assert_int_eq(cdb[1], SEC_PROTOCOL_UFS);
}
END_TEST

START_TEST(test_prepare_security_cdb_out)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE] = {0};
	int rc;

	rc = prepare_security_cdb(cdb, 512, 0, SECURITY_PROTOCOL_OUT);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(cdb[0], SECURITY_PROTOCOL_OUT);
	ck_assert_int_eq(cdb[1], SEC_PROTOCOL_UFS);
}
END_TEST

START_TEST(test_prepare_security_cdb_null)
{
	int rc = prepare_security_cdb(NULL, 512, 0, SECURITY_PROTOCOL_IN);
	ck_assert_int_eq(rc, ERROR);
}
END_TEST

START_TEST(test_prepare_security_cdb_with_region)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE] = {0};
	int rc;

	rc = prepare_security_cdb(cdb, 1024, 2, SECURITY_PROTOCOL_IN);
	ck_assert_int_eq(rc, 0);

	/* Verify region is encoded in security specific field */
	__u16 sec_spec = be16toh(*(__u16 *)(cdb + 2));
	ck_assert_int_eq((sec_spec >> 8) & 0xFF, 2);
	ck_assert_int_eq(sec_spec & 0xFF, SEC_SPECIFIC_UFS_RPMB & 0xFF);
}
END_TEST

START_TEST(test_prepare_security_cdb_data_length)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE] = {0};
	__u32 test_len = 4096;
	int rc;

	rc = prepare_security_cdb(cdb, test_len, 0, SECURITY_PROTOCOL_IN);
	ck_assert_int_eq(rc, 0);

	/* Verify data length is encoded at offset 6 in big-endian */
	__u32 encoded_len = be32toh(*(__u32 *)(cdb + 6));
	ck_assert_uint_eq(encoded_len, test_len);
}
END_TEST

/* ========== write_buffer parameter validation tests ========== */

START_TEST(test_write_buffer_negative_fd)
{
	__u8 buf[10] = {0};
	int rc = write_buffer(-1, buf, 0, 0, 0, 10, SG3_TYPE);
	ck_assert_int_eq(rc, -EINVAL);
}
END_TEST

START_TEST(test_write_buffer_negative_byte_count)
{
	__u8 buf[10] = {0};
	int rc = write_buffer(5, buf, 0, 0, 0, -1, SG3_TYPE);
	ck_assert_int_eq(rc, -EINVAL);
}
END_TEST

/* ========== read_buffer parameter validation tests ========== */

START_TEST(test_read_buffer_negative_fd)
{
	__u8 buf[10] = {0};
	int rc = read_buffer(-1, buf, 0, 0, 0, 10, SG3_TYPE);
	ck_assert_int_eq(rc, -EINVAL);
}
END_TEST

START_TEST(test_read_buffer_negative_byte_count)
{
	__u8 buf[10] = {0};
	int rc = read_buffer(5, buf, 0, 0, 0, -1, SG3_TYPE);
	ck_assert_int_eq(rc, -EINVAL);
}
END_TEST

/* ========== scsi_security_in parameter validation tests ========== */

START_TEST(test_scsi_security_in_negative_fd)
{
	struct rpmb_frame frame;
	int rc = scsi_security_in(-1, &frame, 1, 0, SG3_TYPE);
	ck_assert_int_eq(rc, ERROR);
}
END_TEST

START_TEST(test_scsi_security_in_null_frame)
{
	int rc = scsi_security_in(5, NULL, 1, 0, SG3_TYPE);
	ck_assert_int_eq(rc, ERROR);
}
END_TEST

START_TEST(test_scsi_security_in_zero_count)
{
	struct rpmb_frame frame;
	int rc = scsi_security_in(5, &frame, 0, 0, SG3_TYPE);
	ck_assert_int_eq(rc, ERROR);
}
END_TEST

/* ========== scsi_security_out parameter validation tests ========== */

START_TEST(test_scsi_security_out_negative_fd)
{
	struct rpmb_frame frame;
	int rc = scsi_security_out(-1, &frame, 1, 0, SG3_TYPE);
	ck_assert_int_eq(rc, ERROR);
}
END_TEST

START_TEST(test_scsi_security_out_null_frame)
{
	int rc = scsi_security_out(5, NULL, 1, 0, SG3_TYPE);
	ck_assert_int_eq(rc, ERROR);
}
END_TEST

START_TEST(test_scsi_security_out_zero_count)
{
	struct rpmb_frame frame;
	int rc = scsi_security_out(5, &frame, 0, 0, SG3_TYPE);
	ck_assert_int_eq(rc, ERROR);
}
END_TEST

/* ========== Test Suite Setup ========== */

Suite *scsi_bsg_suite(void)
{
	Suite *s;
	TCase *tc_prepare_upiu;
	TCase *tc_prepare_cmd_upiu;
	TCase *tc_prepare_security;
	TCase *tc_write_buf;
	TCase *tc_read_buf;
	TCase *tc_security_in;
	TCase *tc_security_out;

	s = suite_create("SCSI BSG Utils");

	/* prepare_upiu test case */
	tc_prepare_upiu = tcase_create("prepare_upiu");
	tcase_add_test(tc_prepare_upiu, test_prepare_upiu_read_desc);
	tcase_add_test(tc_prepare_upiu, test_prepare_upiu_write_attr);
	tcase_add_test(tc_prepare_upiu, test_prepare_upiu_with_index_selector);
	tcase_add_test(tc_prepare_upiu, test_prepare_upiu_set_flag);
	tcase_add_test(tc_prepare_upiu, test_prepare_upiu_data_length_encoding);
	suite_add_tcase(s, tc_prepare_upiu);

	/* prepare_command_upiu test case */
	tc_prepare_cmd_upiu = tcase_create("prepare_command_upiu");
	tcase_add_test(tc_prepare_cmd_upiu, test_prepare_command_upiu_read);
	tcase_add_test(tc_prepare_cmd_upiu, test_prepare_command_upiu_write);
	tcase_add_test(tc_prepare_cmd_upiu, test_prepare_command_upiu_with_ehs);
	suite_add_tcase(s, tc_prepare_cmd_upiu);

	/* prepare_security_cdb test case */
	tc_prepare_security = tcase_create("prepare_security_cdb");
	tcase_add_test(tc_prepare_security, test_prepare_security_cdb_in);
	tcase_add_test(tc_prepare_security, test_prepare_security_cdb_out);
	tcase_add_test(tc_prepare_security, test_prepare_security_cdb_null);
	tcase_add_test(tc_prepare_security, test_prepare_security_cdb_with_region);
	tcase_add_test(tc_prepare_security, test_prepare_security_cdb_data_length);
	suite_add_tcase(s, tc_prepare_security);

	/* write_buffer parameter validation */
	tc_write_buf = tcase_create("write_buffer_validation");
	tcase_add_test(tc_write_buf, test_write_buffer_negative_fd);
	tcase_add_test(tc_write_buf, test_write_buffer_negative_byte_count);
	suite_add_tcase(s, tc_write_buf);

	/* read_buffer parameter validation */
	tc_read_buf = tcase_create("read_buffer_validation");
	tcase_add_test(tc_read_buf, test_read_buffer_negative_fd);
	tcase_add_test(tc_read_buf, test_read_buffer_negative_byte_count);
	suite_add_tcase(s, tc_read_buf);

	/* scsi_security_in parameter validation */
	tc_security_in = tcase_create("scsi_security_in_validation");
	tcase_add_test(tc_security_in, test_scsi_security_in_negative_fd);
	tcase_add_test(tc_security_in, test_scsi_security_in_null_frame);
	tcase_add_test(tc_security_in, test_scsi_security_in_zero_count);
	suite_add_tcase(s, tc_security_in);

	/* scsi_security_out parameter validation */
	tc_security_out = tcase_create("scsi_security_out_validation");
	tcase_add_test(tc_security_out, test_scsi_security_out_negative_fd);
	tcase_add_test(tc_security_out, test_scsi_security_out_null_frame);
	tcase_add_test(tc_security_out, test_scsi_security_out_zero_count);
	suite_add_tcase(s, tc_security_out);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = scsi_bsg_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
