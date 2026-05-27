// SPDX-License-Identifier: GPL-2.0-or-later
/* Main test runner - aggregates all test suites */

#include <check.h>
#include <stdlib.h>

/* External suite declarations - each test file provides a suite creator */
Suite *util_suite(void);
Suite *options_suite(void);
Suite *scsi_bsg_suite(void);

int main(void)
{
	int number_failed;
	SRunner *sr;

	sr = srunner_create(util_suite());
	srunner_add_suite(sr, options_suite());
	srunner_add_suite(sr, scsi_bsg_suite());

	srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
