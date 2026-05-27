// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for SCSI BSG utility functions with hardware mocking */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <errno.h>

#include "../ufs.h"
#include "../scsi_bsg_util.h"
#include "../options.h"
#include "mock_hardware.h"

/* ---------- setup / teardown ---------- */

static void setup(void)
{
	mock_ioctl_reset();
	mock_file_reset();
}

/* ================================================================
 * 1. prepare_upiu tests
 * ================================================================ */

START_TEST(test_prepare_upiu_msgcode)
{
	struct ufs_bsg_request bsg_req;
	memset(&bsg_req, 0, sizeof(bsg_req));

	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     64, UPIU_QUERY_OPCODE_READ_DESC,
		     QUERY_DESC_IDN_DEVICE, 0, 0);

	ck_assert_uint_eq(bsg_req.msgcode, UPIU_TRANSACTION_QUERY_REQ);
}
END_TEST

START_TEST(test_prepare_upiu_header_dword0)
{
	struct ufs_bsg_request bsg_req;
	memset(&bsg_req, 0, sizeof(bsg_req));

	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     64, UPIU_QUERY_OPCODE_READ_DESC,
		     QUERY_DESC_IDN_DEVICE, 0, 0);

	__be32 expected = htobe32((__u32)UPIU_TRANSACTION_QUERY_REQ << 24);
	ck_assert_uint_eq(bsg_req.upiu_req.header.dword_0, expected);
}
END_TEST

START_TEST(test_prepare_upiu_header_dword1)
{
	struct ufs_bsg_request bsg_req;
	memset(&bsg_req, 0, sizeof(bsg_req));

	__u8 func = UPIU_QUERY_FUNC_STANDARD_WRITE_REQUEST;
	prepare_upiu(&bsg_req, func, 64, UPIU_QUERY_OPCODE_WRITE_DESC,
		     QUERY_DESC_IDN_CONFIGURAION, 0, 0);

	__be32 expected = htobe32((__u32)func << 16);
	ck_assert_uint_eq(bsg_req.upiu_req.header.dword_1, expected);
}
END_TEST

START_TEST(test_prepare_upiu_header_dword2_data_len)
{
	struct ufs_bsg_request bsg_req;
	memset(&bsg_req, 0, sizeof(bsg_req));

	__u16 data_len = 0x0102;
	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     data_len, UPIU_QUERY_OPCODE_READ_DESC,
		     QUERY_DESC_IDN_DEVICE, 0, 0);

	__be32 expected = htobe32(((__u32)(data_len >> 8) << 8) |
				  ((__u8)data_len));
	ck_assert_uint_eq(bsg_req.upiu_req.header.dword_2, expected);
}
END_TEST

START_TEST(test_prepare_upiu_qr_opcode)
{
	struct ufs_bsg_request bsg_req;
	memset(&bsg_req, 0, sizeof(bsg_req));

	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     64, UPIU_QUERY_OPCODE_READ_ATTR,
		     QUERY_ATTR_IDN_BOOT_LU_EN, 0, 0);

	ck_assert_uint_eq(bsg_req.upiu_req.qr.opcode,
			  UPIU_QUERY_OPCODE_READ_ATTR);
}
END_TEST

START_TEST(test_prepare_upiu_qr_idn)
{
	struct ufs_bsg_request bsg_req;
	memset(&bsg_req, 0, sizeof(bsg_req));

	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     64, UPIU_QUERY_OPCODE_READ_DESC,
		     QUERY_DESC_IDN_HEALTH, 0, 0);

	ck_assert_uint_eq(bsg_req.upiu_req.qr.idn, QUERY_DESC_IDN_HEALTH);
}
END_TEST

START_TEST(test_prepare_upiu_qr_index_selector)
{
	struct ufs_bsg_request bsg_req;
	memset(&bsg_req, 0, sizeof(bsg_req));

	__u8 index = 3;
	__u8 sel = 1;
	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     64, UPIU_QUERY_OPCODE_READ_DESC,
		     QUERY_DESC_IDN_UNIT, index, sel);

	ck_assert_uint_eq(bsg_req.upiu_req.qr.index, index);
	ck_assert_uint_eq(bsg_req.upiu_req.qr.selector, sel);
}
END_TEST

START_TEST(test_prepare_upiu_qr_length_be16)
{
	struct ufs_bsg_request bsg_req;
	memset(&bsg_req, 0, sizeof(bsg_req));

	__u16 data_len = 0x0100;
	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     data_len, UPIU_QUERY_OPCODE_READ_DESC,
		     QUERY_DESC_IDN_DEVICE, 0, 0);

	ck_assert_uint_eq(bsg_req.upiu_req.qr.length, htobe16(data_len));
}
END_TEST

START_TEST(test_prepare_upiu_zero_data_len)
{
	struct ufs_bsg_request bsg_req;
	memset(&bsg_req, 0, sizeof(bsg_req));

	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     0, UPIU_QUERY_OPCODE_READ_FLAG,
		     QUERY_FLAG_IDN_FDEVICEINIT, 0, 0);

	ck_assert_uint_eq(bsg_req.upiu_req.qr.length, htobe16(0));
	__be32 expected_dw2 = htobe32(0);
	ck_assert_uint_eq(bsg_req.upiu_req.header.dword_2, expected_dw2);
}
END_TEST

START_TEST(test_prepare_upiu_max_data_len)
{
	struct ufs_bsg_request bsg_req;
	memset(&bsg_req, 0, sizeof(bsg_req));

	__u16 data_len = 0xFFFF;
	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     data_len, UPIU_QUERY_OPCODE_READ_DESC,
		     QUERY_DESC_IDN_DEVICE, 0, 0);

	ck_assert_uint_eq(bsg_req.upiu_req.qr.length, htobe16(0xFFFF));

	__be32 expected_dw2 = htobe32(((__u32)(data_len >> 8) << 8) |
				      ((__u8)data_len));
	ck_assert_uint_eq(bsg_req.upiu_req.header.dword_2, expected_dw2);
}
END_TEST

START_TEST(test_prepare_upiu_write_request)
{
	struct ufs_bsg_request bsg_req;
	memset(&bsg_req, 0, sizeof(bsg_req));

	__u8 func = UPIU_QUERY_FUNC_STANDARD_WRITE_REQUEST;
	__u16 data_len = 0x90;
	__u8 opcode = UPIU_QUERY_OPCODE_WRITE_DESC;
	__u8 idn = QUERY_DESC_IDN_CONFIGURAION;
	__u8 index = 0;
	__u8 sel = 0;

	prepare_upiu(&bsg_req, func, data_len, opcode, idn, index, sel);

	ck_assert_uint_eq(bsg_req.msgcode, UPIU_TRANSACTION_QUERY_REQ);
	ck_assert_uint_eq(bsg_req.upiu_req.header.dword_1,
			  htobe32((__u32)func << 16));
	ck_assert_uint_eq(bsg_req.upiu_req.qr.opcode, opcode);
	ck_assert_uint_eq(bsg_req.upiu_req.qr.idn, idn);
	ck_assert_uint_eq(bsg_req.upiu_req.qr.length, htobe16(data_len));
}
END_TEST

START_TEST(test_prepare_upiu_set_flag)
{
	struct ufs_bsg_request bsg_req;
	memset(&bsg_req, 0, sizeof(bsg_req));

	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_WRITE_REQUEST,
		     0, UPIU_QUERY_OPCODE_SET_FLAG,
		     QUERY_FLAG_IDN_BKOPS_EN, 0, 0);

	ck_assert_uint_eq(bsg_req.upiu_req.qr.opcode,
			  UPIU_QUERY_OPCODE_SET_FLAG);
	ck_assert_uint_eq(bsg_req.upiu_req.qr.idn, QUERY_FLAG_IDN_BKOPS_EN);
	ck_assert_uint_eq(bsg_req.upiu_req.qr.length, htobe16(0));
}
END_TEST

START_TEST(test_prepare_upiu_read_attr)
{
	struct ufs_bsg_request bsg_req;
	memset(&bsg_req, 0, sizeof(bsg_req));

	__u8 index = 2;
	__u8 sel = 1;
	prepare_upiu(&bsg_req, UPIU_QUERY_FUNC_STANDARD_READ_REQUEST,
		     4, UPIU_QUERY_OPCODE_READ_ATTR,
		     QUERY_ATTR_IDN_ACTIVE_ICC_LVL, index, sel);

	ck_assert_uint_eq(bsg_req.upiu_req.qr.opcode,
			  UPIU_QUERY_OPCODE_READ_ATTR);
	ck_assert_uint_eq(bsg_req.upiu_req.qr.idn,
			  QUERY_ATTR_IDN_ACTIVE_ICC_LVL);
	ck_assert_uint_eq(bsg_req.upiu_req.qr.index, index);
	ck_assert_uint_eq(bsg_req.upiu_req.qr.selector, sel);
	ck_assert_uint_eq(bsg_req.upiu_req.qr.length, htobe16(4));
}
END_TEST

/* ================================================================
 * 2. prepare_command_upiu tests
 * ================================================================ */

START_TEST(test_prepare_command_upiu_dword0)
{
	struct utp_upiu_req upiu_req;
	memset(&upiu_req, 0, sizeof(upiu_req));

	__u8 flags = UPIU_CMD_FLAGS_READ;
	__u8 lun = 0x02;
	__u8 cdb[UFS_CDB_SIZE] = {0};

	prepare_command_upiu(&upiu_req, flags, lun, 0, cdb, UFS_CDB_SIZE, 4096);

	__be32 expected = htobe32((__u32)UPIU_TRANSACTION_COMMAND << 24 |
				  (__u32)flags << 16 |
				  (__u32)lun << 8);
	ck_assert_uint_eq(upiu_req.header.dword_0, expected);
}
END_TEST

START_TEST(test_prepare_command_upiu_dword1_zero)
{
	struct utp_upiu_req upiu_req;
	memset(&upiu_req, 0, sizeof(upiu_req));

	__u8 cdb[UFS_CDB_SIZE] = {0};
	prepare_command_upiu(&upiu_req, UPIU_CMD_FLAGS_READ, 0, 0,
			     cdb, UFS_CDB_SIZE, 512);

	ck_assert_uint_eq(upiu_req.header.dword_1, htobe32(0));
}
END_TEST

START_TEST(test_prepare_command_upiu_dword2_ehs)
{
	struct utp_upiu_req upiu_req;
	memset(&upiu_req, 0, sizeof(upiu_req));

	__u8 ehs_len = 0x02;
	__u8 cdb[UFS_CDB_SIZE] = {0};

	prepare_command_upiu(&upiu_req, UPIU_CMD_FLAGS_READ, 0, ehs_len,
			     cdb, UFS_CDB_SIZE, 4096);

	__be32 expected = htobe32((__u32)ehs_len << 24);
	ck_assert_uint_eq(upiu_req.header.dword_2, expected);
}
END_TEST

START_TEST(test_prepare_command_upiu_exp_data_transfer_len)
{
	struct utp_upiu_req upiu_req;
	memset(&upiu_req, 0, sizeof(upiu_req));

	__u32 transfer_len = 0x00001000; /* 4096 */
	__u8 cdb[UFS_CDB_SIZE] = {0};

	prepare_command_upiu(&upiu_req, UPIU_CMD_FLAGS_READ, 0, 0,
			     cdb, UFS_CDB_SIZE, transfer_len);

	ck_assert_uint_eq(upiu_req.sc.exp_data_transfer_len,
			  htobe32(transfer_len));
}
END_TEST

START_TEST(test_prepare_command_upiu_cdb_copied)
{
	struct utp_upiu_req upiu_req;
	memset(&upiu_req, 0, sizeof(upiu_req));

	__u8 cdb[UFS_CDB_SIZE];
	int i;
	for (i = 0; i < UFS_CDB_SIZE; i++)
		cdb[i] = (__u8)(0xA0 + i);

	prepare_command_upiu(&upiu_req, UPIU_CMD_FLAGS_READ, 0, 0,
			     cdb, UFS_CDB_SIZE, 512);

	ck_assert_int_eq(memcmp(upiu_req.sc.cdb, cdb, UFS_CDB_SIZE), 0);
}
END_TEST

START_TEST(test_prepare_command_upiu_partial_cdb)
{
	struct utp_upiu_req upiu_req;
	memset(&upiu_req, 0xFF, sizeof(upiu_req));

	__u8 cdb[10] = {0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00};
	__u8 cdb_len = 10;

	prepare_command_upiu(&upiu_req, UPIU_CMD_FLAGS_READ, 0, 0,
			     cdb, cdb_len, 4096);

	ck_assert_int_eq(memcmp(upiu_req.sc.cdb, cdb, cdb_len), 0);
}
END_TEST

START_TEST(test_prepare_command_upiu_read_flags)
{
	struct utp_upiu_req upiu_req;
	memset(&upiu_req, 0, sizeof(upiu_req));

	__u8 cdb[UFS_CDB_SIZE] = {0};
	prepare_command_upiu(&upiu_req, UPIU_CMD_FLAGS_READ, 1, 0,
			     cdb, UFS_CDB_SIZE, 4096);

	__be32 expected = htobe32((__u32)UPIU_TRANSACTION_COMMAND << 24 |
				  (__u32)UPIU_CMD_FLAGS_READ << 16 |
				  (__u32)1 << 8);
	ck_assert_uint_eq(upiu_req.header.dword_0, expected);
}
END_TEST

START_TEST(test_prepare_command_upiu_write_flags)
{
	struct utp_upiu_req upiu_req;
	memset(&upiu_req, 0, sizeof(upiu_req));

	__u8 cdb[UFS_CDB_SIZE] = {0};
	__u8 lun = 3;
	prepare_command_upiu(&upiu_req, UPIU_CMD_FLAGS_WRITE, lun, 0,
			     cdb, UFS_CDB_SIZE, 8192);

	__be32 expected = htobe32((__u32)UPIU_TRANSACTION_COMMAND << 24 |
				  (__u32)UPIU_CMD_FLAGS_WRITE << 16 |
				  (__u32)lun << 8);
	ck_assert_uint_eq(upiu_req.header.dword_0, expected);
	ck_assert_uint_eq(upiu_req.sc.exp_data_transfer_len, htobe32(8192));
}
END_TEST

START_TEST(test_prepare_command_upiu_large_transfer_len)
{
	struct utp_upiu_req upiu_req;
	memset(&upiu_req, 0, sizeof(upiu_req));

	__u32 transfer_len = 0xDEADBEEF;
	__u8 cdb[UFS_CDB_SIZE] = {0};

	prepare_command_upiu(&upiu_req, UPIU_CMD_FLAGS_WRITE, 0, 0,
			     cdb, UFS_CDB_SIZE, transfer_len);

	ck_assert_uint_eq(upiu_req.sc.exp_data_transfer_len,
			  htobe32(transfer_len));
}
END_TEST

/* ================================================================
 * 3. prepare_security_cdb tests
 * ================================================================ */

START_TEST(test_prepare_security_cdb_null)
{
	int ret = prepare_security_cdb(NULL, 512, 0, SECURITY_PROTOCOL_IN);
	ck_assert_int_eq(ret, ERROR);
}
END_TEST

START_TEST(test_prepare_security_cdb_opcode_set)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	prepare_security_cdb(cdb, 512, 0, SECURITY_PROTOCOL_IN);
	ck_assert_uint_eq(cdb[0], SECURITY_PROTOCOL_IN);
}
END_TEST

START_TEST(test_prepare_security_cdb_protocol)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	prepare_security_cdb(cdb, 512, 0, SECURITY_PROTOCOL_IN);
	ck_assert_uint_eq(cdb[1], SEC_PROTOCOL_UFS);
}
END_TEST

START_TEST(test_prepare_security_cdb_sec_specific)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	__u8 region = 2;
	prepare_security_cdb(cdb, 512, region, SECURITY_PROTOCOL_IN);

	__u16 expected_sec_spec = (region << 8) | SEC_SPECIFIC_UFS_RPMB;
	__u16 actual = *(__u16 *)(cdb + 2);
	ck_assert_uint_eq(actual, htobe16(expected_sec_spec));
}
END_TEST

START_TEST(test_prepare_security_cdb_transfer_len)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	unsigned int data_len = 0x1000;
	prepare_security_cdb(cdb, data_len, 0, SECURITY_PROTOCOL_IN);

	__u32 actual = *(__u32 *)(cdb + 6);
	ck_assert_uint_eq(actual, htobe32(data_len));
}
END_TEST

START_TEST(test_prepare_security_cdb_in_opcode)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	int ret = prepare_security_cdb(cdb, 512, 0, SECURITY_PROTOCOL_IN);
	ck_assert_int_eq(ret, 0);
	ck_assert_uint_eq(cdb[0], SECURITY_PROTOCOL_IN);
	ck_assert_uint_eq(cdb[1], SEC_PROTOCOL_UFS);
}
END_TEST

START_TEST(test_prepare_security_cdb_out_opcode)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	int ret = prepare_security_cdb(cdb, 512, 0, SECURITY_PROTOCOL_OUT);
	ck_assert_int_eq(ret, 0);
	ck_assert_uint_eq(cdb[0], SECURITY_PROTOCOL_OUT);
	ck_assert_uint_eq(cdb[1], SEC_PROTOCOL_UFS);
}
END_TEST

START_TEST(test_prepare_security_cdb_region0)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	prepare_security_cdb(cdb, 512, 0, SECURITY_PROTOCOL_IN);

	__u16 expected = (0 << 8) | SEC_SPECIFIC_UFS_RPMB;
	__u16 actual = *(__u16 *)(cdb + 2);
	ck_assert_uint_eq(actual, htobe16(expected));
}
END_TEST

START_TEST(test_prepare_security_cdb_region1)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	prepare_security_cdb(cdb, 512, 1, SECURITY_PROTOCOL_IN);

	__u16 expected = (1 << 8) | SEC_SPECIFIC_UFS_RPMB;
	__u16 actual = *(__u16 *)(cdb + 2);
	ck_assert_uint_eq(actual, htobe16(expected));
}
END_TEST

START_TEST(test_prepare_security_cdb_region2)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	prepare_security_cdb(cdb, 512, 2, SECURITY_PROTOCOL_IN);

	__u16 expected = (2 << 8) | SEC_SPECIFIC_UFS_RPMB;
	__u16 actual = *(__u16 *)(cdb + 2);
	ck_assert_uint_eq(actual, htobe16(expected));
}
END_TEST

START_TEST(test_prepare_security_cdb_region3)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	prepare_security_cdb(cdb, 512, 3, SECURITY_PROTOCOL_IN);

	__u16 expected = (3 << 8) | SEC_SPECIFIC_UFS_RPMB;
	__u16 actual = *(__u16 *)(cdb + 2);
	ck_assert_uint_eq(actual, htobe16(expected));
}
END_TEST

START_TEST(test_prepare_security_cdb_small_data_len)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	unsigned int data_len = 1;
	prepare_security_cdb(cdb, data_len, 0, SECURITY_PROTOCOL_IN);

	__u32 actual = *(__u32 *)(cdb + 6);
	ck_assert_uint_eq(actual, htobe32(data_len));
}
END_TEST

START_TEST(test_prepare_security_cdb_large_data_len)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	unsigned int data_len = 0x00100000; /* 1 MB */
	prepare_security_cdb(cdb, data_len, 0, SECURITY_PROTOCOL_OUT);

	__u32 actual = *(__u32 *)(cdb + 6);
	ck_assert_uint_eq(actual, htobe32(data_len));
}
END_TEST

START_TEST(test_prepare_security_cdb_zero_data_len)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	prepare_security_cdb(cdb, 0, 0, SECURITY_PROTOCOL_IN);

	__u32 actual = *(__u32 *)(cdb + 6);
	ck_assert_uint_eq(actual, htobe32(0));
}
END_TEST

START_TEST(test_prepare_security_cdb_return_ok)
{
	__u8 cdb[SEC_PROTOCOL_CMD_SIZE];
	memset(cdb, 0, sizeof(cdb));

	int ret = prepare_security_cdb(cdb, 512, 0, SECURITY_PROTOCOL_IN);
	ck_assert_int_eq(ret, 0);
}
END_TEST

/* ================================================================
 * 4. write_buffer / read_buffer parameter validation
 * ================================================================ */

START_TEST(test_write_buffer_negative_fd)
{
	__u8 buf[64];
	int ret = write_buffer(-1, buf, 0, 0, 0, 64, SG3_TYPE);
	ck_assert_int_eq(ret, -EINVAL);
}
END_TEST

START_TEST(test_write_buffer_negative_byte_count)
{
	__u8 buf[64];
	int ret = write_buffer(3, buf, 0, 0, 0, -1, SG3_TYPE);
	ck_assert_int_eq(ret, -EINVAL);
}
END_TEST

START_TEST(test_write_buffer_both_invalid)
{
	__u8 buf[64];
	int ret = write_buffer(-1, buf, 0, 0, 0, -1, SG3_TYPE);
	ck_assert_int_eq(ret, -EINVAL);
}
END_TEST

START_TEST(test_read_buffer_negative_fd)
{
	__u8 buf[64];
	int ret = read_buffer(-1, buf, 0, 0, 0, 64, SG3_TYPE);
	ck_assert_int_eq(ret, -EINVAL);
}
END_TEST

START_TEST(test_read_buffer_negative_byte_count)
{
	__u8 buf[64];
	int ret = read_buffer(3, buf, 0, 0, 0, -1, SG3_TYPE);
	ck_assert_int_eq(ret, -EINVAL);
}
END_TEST

START_TEST(test_read_buffer_both_invalid)
{
	__u8 buf[64];
	int ret = read_buffer(-1, buf, 0, 0, 0, -1, SG3_TYPE);
	ck_assert_int_eq(ret, -EINVAL);
}
END_TEST

/* ================================================================
 * 5. scsi_security_in / scsi_security_out parameter validation
 * ================================================================ */

START_TEST(test_scsi_security_in_negative_fd)
{
	struct rpmb_frame frame;
	int ret = scsi_security_in(-1, &frame, 1, 0, SG3_TYPE);
	ck_assert_int_eq(ret, ERROR);
}
END_TEST

START_TEST(test_scsi_security_in_null_frame)
{
	int ret = scsi_security_in(3, NULL, 1, 0, SG3_TYPE);
	ck_assert_int_eq(ret, ERROR);
}
END_TEST

START_TEST(test_scsi_security_in_zero_cnt)
{
	struct rpmb_frame frame;
	int ret = scsi_security_in(3, &frame, 0, 0, SG3_TYPE);
	ck_assert_int_eq(ret, ERROR);
}
END_TEST

START_TEST(test_scsi_security_in_negative_cnt)
{
	struct rpmb_frame frame;
	int ret = scsi_security_in(3, &frame, -1, 0, SG3_TYPE);
	ck_assert_int_eq(ret, ERROR);
}
END_TEST

START_TEST(test_scsi_security_out_negative_fd)
{
	struct rpmb_frame frame;
	int ret = scsi_security_out(-1, &frame, 1, 0, SG3_TYPE);
	ck_assert_int_eq(ret, ERROR);
}
END_TEST

START_TEST(test_scsi_security_out_null_frame)
{
	int ret = scsi_security_out(3, NULL, 1, 0, SG3_TYPE);
	ck_assert_int_eq(ret, ERROR);
}
END_TEST

START_TEST(test_scsi_security_out_zero_cnt)
{
	struct rpmb_frame frame;
	int ret = scsi_security_out(3, &frame, 0, 0, SG3_TYPE);
	ck_assert_int_eq(ret, ERROR);
}
END_TEST

/* ================================================================
 * Suite construction
 * ================================================================ */

Suite *scsi_bsg_suite(void)
{
	Suite *s;
	TCase *tc_upiu, *tc_cmd_upiu, *tc_sec_cdb, *tc_rw_buf, *tc_sec_io;

	s = suite_create("SCSI_BSG");

	/* prepare_upiu tests */
	tc_upiu = tcase_create("prepare_upiu");
	tcase_add_checked_fixture(tc_upiu, setup, NULL);
	tcase_add_test(tc_upiu, test_prepare_upiu_msgcode);
	tcase_add_test(tc_upiu, test_prepare_upiu_header_dword0);
	tcase_add_test(tc_upiu, test_prepare_upiu_header_dword1);
	tcase_add_test(tc_upiu, test_prepare_upiu_header_dword2_data_len);
	tcase_add_test(tc_upiu, test_prepare_upiu_qr_opcode);
	tcase_add_test(tc_upiu, test_prepare_upiu_qr_idn);
	tcase_add_test(tc_upiu, test_prepare_upiu_qr_index_selector);
	tcase_add_test(tc_upiu, test_prepare_upiu_qr_length_be16);
	tcase_add_test(tc_upiu, test_prepare_upiu_zero_data_len);
	tcase_add_test(tc_upiu, test_prepare_upiu_max_data_len);
	tcase_add_test(tc_upiu, test_prepare_upiu_write_request);
	tcase_add_test(tc_upiu, test_prepare_upiu_set_flag);
	tcase_add_test(tc_upiu, test_prepare_upiu_read_attr);
	suite_add_tcase(s, tc_upiu);

	/* prepare_command_upiu tests */
	tc_cmd_upiu = tcase_create("prepare_command_upiu");
	tcase_add_checked_fixture(tc_cmd_upiu, setup, NULL);
	tcase_add_test(tc_cmd_upiu, test_prepare_command_upiu_dword0);
	tcase_add_test(tc_cmd_upiu, test_prepare_command_upiu_dword1_zero);
	tcase_add_test(tc_cmd_upiu, test_prepare_command_upiu_dword2_ehs);
	tcase_add_test(tc_cmd_upiu, test_prepare_command_upiu_exp_data_transfer_len);
	tcase_add_test(tc_cmd_upiu, test_prepare_command_upiu_cdb_copied);
	tcase_add_test(tc_cmd_upiu, test_prepare_command_upiu_partial_cdb);
	tcase_add_test(tc_cmd_upiu, test_prepare_command_upiu_read_flags);
	tcase_add_test(tc_cmd_upiu, test_prepare_command_upiu_write_flags);
	tcase_add_test(tc_cmd_upiu, test_prepare_command_upiu_large_transfer_len);
	suite_add_tcase(s, tc_cmd_upiu);

	/* prepare_security_cdb tests */
	tc_sec_cdb = tcase_create("prepare_security_cdb");
	tcase_add_checked_fixture(tc_sec_cdb, setup, NULL);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_null);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_opcode_set);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_protocol);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_sec_specific);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_transfer_len);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_in_opcode);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_out_opcode);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_region0);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_region1);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_region2);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_region3);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_small_data_len);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_large_data_len);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_zero_data_len);
	tcase_add_test(tc_sec_cdb, test_prepare_security_cdb_return_ok);
	suite_add_tcase(s, tc_sec_cdb);

	/* write_buffer / read_buffer parameter validation */
	tc_rw_buf = tcase_create("read_write_buffer_validation");
	tcase_add_checked_fixture(tc_rw_buf, setup, NULL);
	tcase_add_test(tc_rw_buf, test_write_buffer_negative_fd);
	tcase_add_test(tc_rw_buf, test_write_buffer_negative_byte_count);
	tcase_add_test(tc_rw_buf, test_write_buffer_both_invalid);
	tcase_add_test(tc_rw_buf, test_read_buffer_negative_fd);
	tcase_add_test(tc_rw_buf, test_read_buffer_negative_byte_count);
	tcase_add_test(tc_rw_buf, test_read_buffer_both_invalid);
	suite_add_tcase(s, tc_rw_buf);

	/* scsi_security_in / scsi_security_out parameter validation */
	tc_sec_io = tcase_create("security_in_out_validation");
	tcase_add_checked_fixture(tc_sec_io, setup, NULL);
	tcase_add_test(tc_sec_io, test_scsi_security_in_negative_fd);
	tcase_add_test(tc_sec_io, test_scsi_security_in_null_frame);
	tcase_add_test(tc_sec_io, test_scsi_security_in_zero_cnt);
	tcase_add_test(tc_sec_io, test_scsi_security_in_negative_cnt);
	tcase_add_test(tc_sec_io, test_scsi_security_out_negative_fd);
	tcase_add_test(tc_sec_io, test_scsi_security_out_null_frame);
	tcase_add_test(tc_sec_io, test_scsi_security_out_zero_cnt);
	suite_add_tcase(s, tc_sec_io);

	return s;
}
