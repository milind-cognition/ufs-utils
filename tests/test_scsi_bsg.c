// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for SCSI BSG utility functions with hardware mocking */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>

#include "../ufs.h"
#include "../scsi_bsg_util.h"
#include "mock_hardware.h"

/* SCSI BSG tests */

START_TEST(test_scsi_bsg_placeholder)
{
	/* placeholder - child session will implement */
	ck_assert_int_eq(0, 0);
}
END_TEST

Suite *scsi_bsg_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("SCSI_BSG");

	tc = tcase_create("bsg_utils");
	tcase_add_test(tc, test_scsi_bsg_placeholder);
	suite_add_tcase(s, tc);

	return s;
}
