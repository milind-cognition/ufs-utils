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

clean:
	@rm -f $(progs) $(objects) .*.o.d
	@rm -f tests/test_ufs_utils tests/test_options tests/test_scsi_bsg tests/test_ufs_cmds
	@rm -f tests/*.o tests/mocks/*.o

# ============ Unit Test Targets ============

CHECK_CFLAGS = $(shell pkg-config --cflags check)
CHECK_LIBS = $(shell pkg-config --libs check)
TEST_CFLAGS = -Wall -Wundef -Wno-missing-braces -fcommon -D_FILE_OFFSET_BITS=64 -D_FORTIFY_SOURCE=2 -g -O0 -D_GNU_SOURCE

# Objects needed for tests (excluding ufs.o which contains main())
test_common_objs = ufs_cmds.o options.o scsi_bsg_util.o ufs_err_hist.o unipro.o ufs_ffu.o \
                   ufs_vendor.o hmac_sha2.o sha2.o ufs_rpmb.o ufs_arpmb.o ufs_hmr.o ufs_emon.o

# Build a library object for ufs.c without main()
tests/ufs_no_main.o: ufs.c
	$(CC) $(TEST_CFLAGS) $(CHECK_CFLAGS) -Dmain=__ufs_main_unused -c $< -o $@

tests/test_ufs_utils: tests/test_ufs_utils.c tests/ufs_no_main.o $(test_common_objs)
	$(CC) $(TEST_CFLAGS) $(CHECK_CFLAGS) -o $@ $< tests/ufs_no_main.o $(test_common_objs) $(CHECK_LIBS)

tests/test_options: tests/test_options.c $(test_common_objs) tests/ufs_no_main.o
	$(CC) $(TEST_CFLAGS) $(CHECK_CFLAGS) -o $@ $< tests/ufs_no_main.o $(test_common_objs) $(CHECK_LIBS)

tests/test_scsi_bsg: tests/test_scsi_bsg.c tests/ufs_no_main.o $(test_common_objs)
	$(CC) $(TEST_CFLAGS) $(CHECK_CFLAGS) -o $@ $< tests/ufs_no_main.o $(test_common_objs) $(CHECK_LIBS)

# test_ufs_cmds includes ufs_cmds.c directly, so we exclude ufs_cmds.o and link others
test_cmds_objs = options.o scsi_bsg_util.o ufs_err_hist.o unipro.o ufs_ffu.o \
                 ufs_vendor.o hmac_sha2.o sha2.o ufs_rpmb.o ufs_arpmb.o ufs_hmr.o ufs_emon.o
tests/test_ufs_cmds: tests/test_ufs_cmds.c tests/ufs_no_main.o $(test_cmds_objs)
	$(CC) $(TEST_CFLAGS) $(CHECK_CFLAGS) -o $@ $< tests/ufs_no_main.o $(test_cmds_objs) $(CHECK_LIBS)

test: tests/test_ufs_utils tests/test_options tests/test_scsi_bsg tests/test_ufs_cmds
	@echo "======== Running Unit Tests ========"
	@echo "--- test_ufs_utils ---"
	@./tests/test_ufs_utils
	@echo "--- test_options ---"
	@./tests/test_options
	@echo "--- test_scsi_bsg ---"
	@./tests/test_scsi_bsg
	@echo "--- test_ufs_cmds ---"
	@./tests/test_ufs_cmds
	@echo "======== All Tests Passed ========"

.PHONY: all clean test
