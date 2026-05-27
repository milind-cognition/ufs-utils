/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Declarations for internal functions exposed via TEST_BUILD */

#ifndef TEST_HELPERS_H_
#define TEST_HELPERS_H_

#include "../ufs.h"
#include "../options.h"

/* Functions from ufs.c made non-static via TEST_STATIC */
char *get_prgname(char *programname);
void initialized_options(struct tool_options *options);

#endif /* TEST_HELPERS_H_ */
