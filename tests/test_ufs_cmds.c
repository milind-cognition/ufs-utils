// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for UFS command functions (descriptors, attributes, flags) */

/*
 * Include the source file directly to access static functions.
 * This is a common C testing pattern when functions are static.
 */
#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <endian.h>
#include <unistd.h>
#include <linux/types.h>

#include "../ufs.h"
#include "../ufs_cmds.h"
#include "../options.h"

/*
 * Include the .c file to access static functions for testing.
 * We guard against symbol conflicts by only including in this
 * translation unit.
 */
#include "../ufs_cmds.c"

/* ========== create_str_desc_data tests ========== */

START_TEST(test_create_str_desc_data_basic)
{
	__u8 buf[QUERY_DESC_STRING_MAX_SIZE] = {0};
	const char *str = "ABC";

	create_str_desc_data(buf, str, 3);

	/* Length should be len*2 + 2 */
	ck_assert_int_eq(buf[0], 8);
	/* Descriptor type should be STRING */
	ck_assert_int_eq(buf[1], QUERY_DESC_IDN_STRING);
	/* Characters placed at odd offsets starting from 3 */
	ck_assert_int_eq(buf[3], 'A');
	ck_assert_int_eq(buf[5], 'B');
	ck_assert_int_eq(buf[7], 'C');
}
END_TEST

START_TEST(test_create_str_desc_data_empty)
{
	__u8 buf[QUERY_DESC_STRING_MAX_SIZE] = {0};

	create_str_desc_data(buf, "", 0);

	ck_assert_int_eq(buf[0], 2);
	ck_assert_int_eq(buf[1], QUERY_DESC_IDN_STRING);
}
END_TEST

START_TEST(test_create_str_desc_data_single_char)
{
	__u8 buf[QUERY_DESC_STRING_MAX_SIZE] = {0};

	create_str_desc_data(buf, "X", 1);

	ck_assert_int_eq(buf[0], 4);
	ck_assert_int_eq(buf[1], QUERY_DESC_IDN_STRING);
	ck_assert_int_eq(buf[3], 'X');
}
END_TEST

START_TEST(test_create_str_desc_data_unicode_layout)
{
	__u8 buf[QUERY_DESC_STRING_MAX_SIZE] = {0};
	const char *str = "Hi";

	create_str_desc_data(buf, str, 2);

	/* Verify even positions (high byte of unicode) are zero */
	ck_assert_int_eq(buf[2], 0);
	ck_assert_int_eq(buf[4], 0);
	/* Verify odd positions contain ASCII chars */
	ck_assert_int_eq(buf[3], 'H');
	ck_assert_int_eq(buf[5], 'i');
}
END_TEST

/* ========== access_type_string tests ========== */

START_TEST(test_access_type_string_attr_boot_lun)
{
	char access_string[100] = {0};
	char *result;

	result = access_type_string(QUERY_ATTR_IDN_BOOT_LU_EN, ATTR_TYPE,
				    access_string);
	ck_assert_ptr_nonnull(result);
	ck_assert_ptr_nonnull(strstr(result, "ReadOnly"));
	ck_assert_ptr_nonnull(strstr(result, "Persistent"));
}
END_TEST

START_TEST(test_access_type_string_attr_bkops)
{
	char access_string[100] = {0};
	char *result;

	result = access_type_string(QUERY_ATTR_IDN_BKOPS_STATUS, ATTR_TYPE,
				    access_string);
	ck_assert_ptr_nonnull(result);
	ck_assert_ptr_nonnull(strstr(result, "ReadOnly"));
}
END_TEST

START_TEST(test_access_type_string_flag_device_init)
{
	char access_string[100] = {0};
	char *result;

	result = access_type_string(QUERY_FLAG_IDN_FDEVICEINIT, FLAG_TYPE,
				    access_string);
	ck_assert_ptr_nonnull(result);
	ck_assert_ptr_nonnull(strstr(result, "Read"));
	ck_assert_ptr_nonnull(strstr(result, "SetOnly"));
}
END_TEST

START_TEST(test_access_type_string_flag_busy_rtc)
{
	char access_string[100] = {0};
	char *result;

	result = access_type_string(QUERY_FLAG_IDN_BUSY_RTC, FLAG_TYPE,
				    access_string);
	ck_assert_ptr_nonnull(result);
	ck_assert_ptr_nonnull(strstr(result, "ReadOnly"));
}
END_TEST

START_TEST(test_access_type_string_invalid_config)
{
	char access_string[100] = {0};
	char *result;

	result = access_type_string(0, 99, access_string);
	ck_assert_ptr_null(result);
}
END_TEST

START_TEST(test_access_type_string_attr_volatile)
{
	char access_string[100] = {0};
	char *result;

	/* bOutOfOrderDataEn (idx 4) is Read | WriteOnce */
	result = access_type_string(QUERY_ATTR_IDN_OOO_DATA_EN, ATTR_TYPE,
				    access_string);
	ck_assert_ptr_nonnull(result);
	ck_assert_ptr_nonnull(strstr(result, "WriteOnce"));
}
END_TEST

/* ========== store_data_file tests ========== */

START_TEST(test_store_data_file_success)
{
	const char *tmpfile = "/tmp/test_store_data.bin";
	__u8 data[] = {0x01, 0x02, 0x03, 0x04};
	int rc;

	rc = store_data_file((char *)tmpfile, data, sizeof(data));
	ck_assert_int_gt(rc, 0);

	/* Verify file contents */
	FILE *f = fopen(tmpfile, "rb");
	ck_assert_ptr_nonnull(f);

	__u8 buf[10] = {0};
	size_t nread = fread(buf, 1, sizeof(data), f);
	fclose(f);
	ck_assert_int_eq(nread, sizeof(data));
	ck_assert_mem_eq(buf, data, sizeof(data));

	unlink(tmpfile);
}
END_TEST

START_TEST(test_store_data_file_invalid_path)
{
	__u8 data[] = {0x01};
	int rc;

	rc = store_data_file("/nonexistent/dir/file.bin", data, sizeof(data));
	ck_assert_int_eq(rc, ERROR);
}
END_TEST

/* ========== query_response_error tests ========== */

START_TEST(test_query_response_error_does_not_crash)
{
	/* Just verify it doesn't crash/segfault */
	query_response_error(0xFE, 0x01);
	query_response_error(0xF0, 0x00);
	query_response_error(0xFF, 0x05);
}
END_TEST

/* ========== print_power_desc_icc boundary tests ========== */

START_TEST(test_print_power_desc_icc_invalid_index)
{
	__u8 desc_buf[256] = {0};

	/* Should not crash with invalid index */
	print_power_desc_icc(desc_buf, 0);
	print_power_desc_icc(desc_buf, 1);
}
END_TEST

START_TEST(test_print_power_desc_icc_valid_index)
{
	__u8 desc_buf[256] = {0};

	/* Fill some test data */
	desc_buf[0x02] = 0x01;
	desc_buf[0x03] = 0x00;

	/* Should not crash with valid index */
	print_power_desc_icc(desc_buf, 2);
}
END_TEST

/* ========== check_read_desc_size tests ========== */

START_TEST(test_check_read_desc_size_device_valid)
{
	__u8 data_buf[QUERY_DESC_MAX_SIZE] = {0};
	int rc;

	data_buf[0] = QUERY_DESC_DEVICE_MAX_SIZE;
	rc = check_read_desc_size(QUERY_DESC_IDN_DEVICE, data_buf);
	ck_assert_int_eq(rc, OK);
}
END_TEST

START_TEST(test_check_read_desc_size_device_3_0)
{
	__u8 data_buf[QUERY_DESC_MAX_SIZE] = {0};
	int rc;

	data_buf[0] = QUERY_DESC_DEVICE_MAX_SIZE_3_0;
	rc = check_read_desc_size(QUERY_DESC_IDN_DEVICE, data_buf);
	ck_assert_int_eq(rc, OK);
}
END_TEST

START_TEST(test_check_read_desc_size_device_unofficial)
{
	__u8 data_buf[QUERY_DESC_MAX_SIZE] = {0};
	int rc;

	data_buf[0] = 0x10;
	rc = check_read_desc_size(QUERY_DESC_IDN_DEVICE, data_buf);
	ck_assert_int_eq(rc, WARNING);
	unlink("unofficial.dat");
}
END_TEST

START_TEST(test_check_read_desc_size_geometry_valid)
{
	__u8 data_buf[QUERY_DESC_MAX_SIZE] = {0};
	int rc;

	data_buf[0] = QUERY_DESC_GEOMETRY_MAX_SIZE;
	rc = check_read_desc_size(QUERY_DESC_IDN_GEOMETRY, data_buf);
	ck_assert_int_eq(rc, OK);
}
END_TEST

START_TEST(test_check_read_desc_size_health_valid)
{
	__u8 data_buf[QUERY_DESC_MAX_SIZE] = {0};
	int rc;

	data_buf[0] = QUERY_DESC_HEALTH_MAX_SIZE;
	rc = check_read_desc_size(QUERY_DESC_IDN_HEALTH, data_buf);
	ck_assert_int_eq(rc, OK);
}
END_TEST

START_TEST(test_check_read_desc_size_interconnect_valid)
{
	__u8 data_buf[QUERY_DESC_MAX_SIZE] = {0};
	int rc;

	data_buf[0] = QUERY_DESC_INTERCONNECT_MAX_SIZE;
	rc = check_read_desc_size(QUERY_DESC_IDN_INTERCONNECT, data_buf);
	ck_assert_int_eq(rc, OK);
}
END_TEST

START_TEST(test_check_read_desc_size_power_valid)
{
	__u8 data_buf[QUERY_DESC_MAX_SIZE] = {0};
	int rc;

	data_buf[0] = QUERY_DESC_POWER_MAX_SIZE;
	rc = check_read_desc_size(QUERY_DESC_IDN_POWER, data_buf);
	ck_assert_int_eq(rc, OK);
}
END_TEST

START_TEST(test_check_read_desc_size_fbo_valid)
{
	__u8 data_buf[QUERY_DESC_MAX_SIZE] = {0};
	int rc;

	data_buf[0] = QUERY_DESC_FBO_MAX_SIZE;
	rc = check_read_desc_size(QUERY_DESC_IDN_FBO, data_buf);
	ck_assert_int_eq(rc, OK);
}
END_TEST

/* ========== Descriptor field arrays sanity tests ========== */

START_TEST(test_device_desc_field_name_not_empty)
{
	ck_assert_int_gt(
		sizeof(device_desc_field_name) / sizeof(device_desc_field_name[0]),
		0);
	ck_assert_str_eq(device_desc_field_name[0].name, "bLength");
}
END_TEST

START_TEST(test_geometry_desc_field_name_first_entry)
{
	ck_assert_str_eq(device_geo_desc_conf_field_name[0].name, "bLength");
	ck_assert_int_eq(device_geo_desc_conf_field_name[0].offset, 0x00);
}
END_TEST

START_TEST(test_health_desc_field_name_first_entry)
{
	ck_assert_str_eq(device_health_desc_conf_field_name[0].name, "bLength");
	ck_assert_int_eq(device_health_desc_conf_field_name[0].offset, 0x00);
}
END_TEST

START_TEST(test_unit_desc_field_name_first_entry)
{
	ck_assert_str_eq(device_unit_desc_field_name[0].name, "bLength");
	ck_assert_int_eq(device_unit_desc_field_name[0].offset, 0x00);
}
END_TEST

START_TEST(test_ufs_attrs_device_init)
{
	/* First attribute: bBootLunEn */
	ck_assert_str_eq(ufs_attrs[0].name, "bBootLunEn");
	ck_assert_int_eq(ufs_attrs[0].width_in_bytes, BYTE);
}
END_TEST

START_TEST(test_ufs_flags_device_init)
{
	/* Flag 1: fDeviceInit */
	ck_assert_str_eq(ufs_flags[1].name, "fDeviceInit");
}
END_TEST

/* ========== Test Suite Setup ========== */

Suite *ufs_cmds_suite(void)
{
	Suite *s;
	TCase *tc_str_desc;
	TCase *tc_access_type;
	TCase *tc_store_data;
	TCase *tc_query_err;
	TCase *tc_power_desc;
	TCase *tc_desc_size;
	TCase *tc_field_arrays;

	s = suite_create("UFS Commands");

	tc_str_desc = tcase_create("create_str_desc_data");
	tcase_add_test(tc_str_desc, test_create_str_desc_data_basic);
	tcase_add_test(tc_str_desc, test_create_str_desc_data_empty);
	tcase_add_test(tc_str_desc, test_create_str_desc_data_single_char);
	tcase_add_test(tc_str_desc, test_create_str_desc_data_unicode_layout);
	suite_add_tcase(s, tc_str_desc);

	tc_access_type = tcase_create("access_type_string");
	tcase_add_test(tc_access_type, test_access_type_string_attr_boot_lun);
	tcase_add_test(tc_access_type, test_access_type_string_attr_bkops);
	tcase_add_test(tc_access_type, test_access_type_string_flag_device_init);
	tcase_add_test(tc_access_type, test_access_type_string_flag_busy_rtc);
	tcase_add_test(tc_access_type, test_access_type_string_invalid_config);
	tcase_add_test(tc_access_type, test_access_type_string_attr_volatile);
	suite_add_tcase(s, tc_access_type);

	tc_store_data = tcase_create("store_data_file");
	tcase_add_test(tc_store_data, test_store_data_file_success);
	tcase_add_test(tc_store_data, test_store_data_file_invalid_path);
	suite_add_tcase(s, tc_store_data);

	tc_query_err = tcase_create("query_response_error");
	tcase_add_test(tc_query_err, test_query_response_error_does_not_crash);
	suite_add_tcase(s, tc_query_err);

	tc_power_desc = tcase_create("print_power_desc_icc");
	tcase_add_test(tc_power_desc, test_print_power_desc_icc_invalid_index);
	tcase_add_test(tc_power_desc, test_print_power_desc_icc_valid_index);
	suite_add_tcase(s, tc_power_desc);

	tc_desc_size = tcase_create("check_read_desc_size");
	tcase_add_test(tc_desc_size, test_check_read_desc_size_device_valid);
	tcase_add_test(tc_desc_size, test_check_read_desc_size_device_3_0);
	tcase_add_test(tc_desc_size, test_check_read_desc_size_device_unofficial);
	tcase_add_test(tc_desc_size, test_check_read_desc_size_geometry_valid);
	tcase_add_test(tc_desc_size, test_check_read_desc_size_health_valid);
	tcase_add_test(tc_desc_size, test_check_read_desc_size_interconnect_valid);
	tcase_add_test(tc_desc_size, test_check_read_desc_size_power_valid);
	tcase_add_test(tc_desc_size, test_check_read_desc_size_fbo_valid);
	suite_add_tcase(s, tc_desc_size);

	tc_field_arrays = tcase_create("Descriptor Field Arrays");
	tcase_add_test(tc_field_arrays, test_device_desc_field_name_not_empty);
	tcase_add_test(tc_field_arrays, test_geometry_desc_field_name_first_entry);
	tcase_add_test(tc_field_arrays, test_health_desc_field_name_first_entry);
	tcase_add_test(tc_field_arrays, test_unit_desc_field_name_first_entry);
	tcase_add_test(tc_field_arrays, test_ufs_attrs_device_init);
	tcase_add_test(tc_field_arrays, test_ufs_flags_device_init);
	suite_add_tcase(s, tc_field_arrays);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = ufs_cmds_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
