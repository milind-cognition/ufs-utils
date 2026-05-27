# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2019 Western Digital Corporation or its affiliates

export CC := $(CROSS_COMPILE)gcc
AM_CFLAGS = -D_FILE_OFFSET_BITS=64 -D_FORTIFY_SOURCE=2
CFLAGS ?= -g -O2 -static -D_GNU_SOURCE

ifneq ($(CROSS_COMPILE),)
	LDFLAGS += -static
endif

#CXXFLAGS = -DDEBUG

objects = \
	ufs.o \
	ufs_cmds.o \
	options.o \
	scsi_bsg_util.o \
	ufs_err_hist.o \
	unipro.o \
	ufs_ffu.o \
	ufs_vendor.o\
	hmac_sha2.o \
	sha2.o \
	ufs_rpmb.o \
	ufs_arpmb.o \
	ufs_hmr.o \
	ufs_emon.o \

CHECKFLAGS = -Wall  -Wundef -Wno-missing-braces -fcommon

DEPFLAGS = -Wp,-MMD,$(@D)/.$(@F).d,-MT,$@
override CFLAGS := $(CHECKFLAGS) $(AM_CFLAGS) $(CFLAGS) $(INC_DIR) $(CXXFLAGS)
progs = ufs-utils
ifdef C
	check = sparse $(CHECKFLAGS)
endif

.c.o:
ifdef C
	$(check) $<
endif
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

ufs-utils:$(objects)
	$(CC) $(CFLAGS) -o $@ $(objects) $(LDFLAGS) $(LIBS)

help:
	@echo "\033[31m==============Build Instructions==============\033[0m"
	@echo "\033[92mTo build ufs_utils follow the following steps\033[0m"
	@echo "\033[92m1 Set CROSS_COMPILE variable\033[0m"
	@echo "\033[92m2 Build the tool using \"make\"\033[0m"
	@echo "\033[92m3 Clean the tool using \"make clean\"\033[0m"

# Unit test configuration
TEST_CFLAGS = $(CHECKFLAGS) $(AM_CFLAGS) -g -O2 -D_GNU_SOURCE $(INC_DIR) $(CXXFLAGS)
TEST_LDFLAGS = -lcmocka

# Objects needed by tests (everything except ufs.o which contains main())
test_objects = \
	ufs_cmds.o \
	options.o \
	scsi_bsg_util.o \
	ufs_err_hist.o \
	unipro.o \
	ufs_ffu.o \
	ufs_vendor.o \
	hmac_sha2.o \
	sha2.o \
	ufs_rpmb.o \
	ufs_arpmb.o \
	ufs_hmr.o \
	ufs_emon.o

# Build a special ufs.o without main() for test linking
ufs_no_main.o: ufs.c
	$(CC) $(CPPFLAGS) $(TEST_CFLAGS) -Dmain=__ufs_main_unused $(DEPFLAGS) -c $< -o $@

tests/test_ufs_utils.o: tests/test_ufs_utils.c
	$(CC) $(CPPFLAGS) $(TEST_CFLAGS) -I. $(DEPFLAGS) -c $< -o $@

tests/test_ufs_utils: tests/test_ufs_utils.o ufs_no_main.o $(test_objects)
	$(CC) $(TEST_CFLAGS) -o $@ $^ $(TEST_LDFLAGS)

test: tests/test_ufs_utils
	@echo "Running unit tests..."
	@./tests/test_ufs_utils

clean:
	@rm -f $(progs) $(objects) .*.o.d ufs_no_main.o \
		tests/test_ufs_utils.o tests/test_ufs_utils
.PHONY: all clean test
