// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for options parsing and validation functions */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "../ufs.h"
#include "../options.h"

/* Options parsing tests */

START_TEST(test_options_placeholder)
{
	/* placeholder - child session will implement */
	ck_assert_int_eq(0, 0);
}
END_TEST

Suite *options_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("Options");

	tc = tcase_create("parsing");
	tcase_add_test(tc, test_options_placeholder);
	suite_add_tcase(s, tc);

	return s;
}
