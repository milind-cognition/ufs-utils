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

# Objects shared by tests (excludes ufs.o, options.o, scsi_bsg_util.o
# since tests may include those .c files directly for static fn access)
test_support_objects = \
	ufs_cmds.o \
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

TEST_CFLAGS := $(CHECKFLAGS) $(AM_CFLAGS) -g -O2 -D_GNU_SOURCE $(INC_DIR) $(CXXFLAGS)

# test_ufs_core includes ufs.c directly; links options.o and scsi_bsg_util.o
tests/test_ufs_core: tests/test_ufs_core.c ufs.c ufs.h options.h $(test_support_objects) options.o scsi_bsg_util.o
	$(CC) $(TEST_CFLAGS) -I. -DTEST_MODE -o $@ tests/test_ufs_core.c options.o scsi_bsg_util.o $(test_support_objects) $(LDFLAGS) $(LIBS)

# test_options includes ufs.c and options.c directly; links scsi_bsg_util.o
tests/test_options: tests/test_options.c options.c ufs.c ufs.h options.h $(test_support_objects) scsi_bsg_util.o
	$(CC) $(TEST_CFLAGS) -I. -DTEST_MODE -o $@ tests/test_options.c scsi_bsg_util.o $(test_support_objects) $(LDFLAGS) $(LIBS)

# test_sha2_hmac is self-contained with sha2.c and hmac_sha2.c
tests/test_sha2_hmac: tests/test_sha2_hmac.c sha2.c sha2.h hmac_sha2.c hmac_sha2.h
	$(CC) $(TEST_CFLAGS) -I. -o $@ tests/test_sha2_hmac.c sha2.c hmac_sha2.c $(LDFLAGS) $(LIBS)

# test_scsi_bsg includes ufs.c and scsi_bsg_util.c directly; links options.o
tests/test_scsi_bsg: tests/test_scsi_bsg.c scsi_bsg_util.c ufs.c scsi_bsg_util.h ufs.h $(test_support_objects) options.o
	$(CC) $(TEST_CFLAGS) -I. -DTEST_MODE -o $@ tests/test_scsi_bsg.c options.o $(test_support_objects) $(LDFLAGS) $(LIBS)

test: tests/test_ufs_core tests/test_options tests/test_sha2_hmac tests/test_scsi_bsg
	@bash tests/run_tests.sh

clean:
	@rm -f $(progs) $(objects) .*.o.d
	@rm -f tests/test_ufs_core tests/test_options tests/test_sha2_hmac tests/test_scsi_bsg tests/*.o

.PHONY: all clean test
