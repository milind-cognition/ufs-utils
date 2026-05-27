// SPDX-License-Identifier: GPL-2.0-or-later
/* Mock ioctl for unit testing without real UFS hardware */

#ifndef MOCK_IOCTL_H_
#define MOCK_IOCTL_H_

#include <stdint.h>

/* Configure mock ioctl return value and behavior */
void mock_ioctl_set_return(int retval);
void mock_ioctl_set_errno(int err);
void mock_ioctl_reset(void);

/* Track ioctl calls for verification */
int mock_ioctl_get_call_count(void);

/* Configure sense buffer response for mock */
void mock_ioctl_set_sense_data(uint8_t sense_key, uint8_t asc, uint8_t ascq);

/* Configure sg_io_v4 info field response */
void mock_ioctl_set_sg4_info(uint32_t info);

/* Configure sg_io_hdr status field response */
void mock_ioctl_set_sg3_status(uint8_t status);

#endif /* MOCK_IOCTL_H_ */
