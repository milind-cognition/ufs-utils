// SPDX-License-Identifier: GPL-2.0-or-later
/* Mock implementations for hardware dependencies */

#include "mock_hardware.h"
#include <string.h>

struct mock_ioctl_state g_mock_ioctl;
struct mock_file_state g_mock_file;

void mock_ioctl_reset(void)
{
	memset(&g_mock_ioctl, 0, sizeof(g_mock_ioctl));
}

void mock_ioctl_set_return(int ret)
{
	g_mock_ioctl.return_value = ret;
}

void mock_ioctl_set_sense(const uint8_t *sense, int len)
{
	if (len > (int)sizeof(g_mock_ioctl.sense_data))
		len = sizeof(g_mock_ioctl.sense_data);
	memcpy(g_mock_ioctl.sense_data, sense, len);
	g_mock_ioctl.sense_data_len = len;
}

void mock_file_reset(void)
{
	memset(&g_mock_file, 0, sizeof(g_mock_file));
	g_mock_file.open_return_fd = 3;
	g_mock_file.write_return = 0;
	g_mock_file.close_return = 0;
}

void mock_file_set_open_return(int fd)
{
	g_mock_file.open_return_fd = fd;
}

void mock_file_set_write_return(ssize_t ret)
{
	g_mock_file.write_return = ret;
}
