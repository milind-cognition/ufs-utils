/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Mock layer for hardware dependencies - allows testing without real UFS devices */

#ifndef MOCK_HARDWARE_H_
#define MOCK_HARDWARE_H_

#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>

/* Mock state for ioctl calls */
struct mock_ioctl_state {
	int return_value;
	int call_count;
	unsigned long last_request;
	void *last_arg;
	/* Sense buffer to fill on SG_IO calls */
	uint8_t sense_data[18];
	int sense_data_len;
};

extern struct mock_ioctl_state g_mock_ioctl;

void mock_ioctl_reset(void);
void mock_ioctl_set_return(int ret);
void mock_ioctl_set_sense(const uint8_t *sense, int len);

/* Mock state for file operations */
struct mock_file_state {
	int open_return_fd;
	ssize_t write_return;
	int close_return;
	int open_call_count;
	int write_call_count;
	int close_call_count;
	char last_open_path[256];
	int last_open_flags;
	const void *last_write_buf;
	size_t last_write_len;
};

extern struct mock_file_state g_mock_file;

void mock_file_reset(void);
void mock_file_set_open_return(int fd);
void mock_file_set_write_return(ssize_t ret);

#endif /* MOCK_HARDWARE_H_ */
