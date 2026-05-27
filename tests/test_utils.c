// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for core utility functions: str_to_long, write_file, etc. */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

#include "../ufs.h"
#include "../options.h"

/* str_to_long tests */

START_TEST(test_str_to_long_decimal)
{
	/* placeholder - child session will implement */
	ck_assert_int_eq(0, 0);
}
END_TEST

Suite *util_suite(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("Utilities");

	tc = tcase_create("str_to_long");
	tcase_add_test(tc, test_str_to_long_decimal);
	suite_add_tcase(s, tc);

	return s;
}
