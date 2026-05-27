// SPDX-License-Identifier: GPL-2.0-or-later
/* Mock ioctl implementation for unit testing */

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/types.h>

#include "mock_ioctl.h"
#include "../../ioctl.h"

static int mock_retval = 0;
static int mock_errno_val = 0;
static int mock_call_count = 0;
static uint8_t mock_sense_key = 0;
static uint8_t mock_asc = 0;
static uint8_t mock_ascq = 0;
static uint32_t mock_sg4_info = 0;
static uint8_t mock_sg3_status = 0;

void mock_ioctl_set_return(int retval)
{
	mock_retval = retval;
}

void mock_ioctl_set_errno(int err)
{
	mock_errno_val = err;
}

void mock_ioctl_reset(void)
{
	mock_retval = 0;
	mock_errno_val = 0;
	mock_call_count = 0;
	mock_sense_key = 0;
	mock_asc = 0;
	mock_ascq = 0;
	mock_sg4_info = 0;
	mock_sg3_status = 0;
}

int mock_ioctl_get_call_count(void)
{
	return mock_call_count;
}

void mock_ioctl_set_sense_data(uint8_t sense_key, uint8_t asc, uint8_t ascq)
{
	mock_sense_key = sense_key;
	mock_asc = asc;
	mock_ascq = ascq;
}

void mock_ioctl_set_sg4_info(uint32_t info)
{
	mock_sg4_info = info;
}

void mock_ioctl_set_sg3_status(uint8_t status)
{
	mock_sg3_status = status;
}

/*
 * Wrapper for ioctl that intercepts SG_IO calls.
 * Uses --wrap linker flag to replace the real ioctl.
 */
int __wrap_ioctl(int fd, unsigned long request, ...)
{
	mock_call_count++;

	if (mock_errno_val) {
		errno = mock_errno_val;
		return -1;
	}

	return mock_retval;
}
