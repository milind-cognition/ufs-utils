# SPDX-License-Identifier: GPL-2.0-or-later
# Makefile for ufs-utils unit tests using Check framework

CC ?= gcc
CFLAGS = -g -O0 -Wall -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -I..
CHECK_CFLAGS = $(shell pkg-config --cflags check)
CHECK_LIBS = $(shell pkg-config --libs check)

# Test sources
TEST_SRCS = test_main.c test_utils.c test_options.c test_scsi_bsg.c
TEST_OBJS = $(TEST_SRCS:.c=.o)

# Mock sources
MOCK_SRCS = mock_hardware.c
MOCK_OBJS = $(MOCK_SRCS:.c=.o)

# Source files from the main project needed by tests.
# We compile them here with test-specific flags (no -static, with -fPIC).
SRC_DIR = ..
SRC_COMPILE_SRCS = ufs_cmds.c options.c scsi_bsg_util.c unipro.c \
                   ufs_ffu.c ufs_vendor.c ufs_rpmb.c ufs_arpmb.c \
                   ufs_hmr.c ufs_emon.c ufs_err_hist.c hmac_sha2.c sha2.c
SRC_COMPILE_OBJS = $(SRC_COMPILE_SRCS:.c=.o)

# The main ufs.c contains main() so we compile a special version
# that excludes main() for linking with tests.
UFS_TEST_OBJ = ufs_testable.o

TEST_BIN = test_runner

.PHONY: all clean check

all: $(TEST_BIN)

$(TEST_BIN): $(TEST_OBJS) $(MOCK_OBJS) $(SRC_COMPILE_OBJS) $(UFS_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(CHECK_LIBS)

# Compile test sources
%.o: %.c
	$(CC) $(CFLAGS) $(CHECK_CFLAGS) -c $< -o $@

# Compile main project sources with test flags
$(SRC_COMPILE_OBJS): %.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile ufs.c without main() for test linking
$(UFS_TEST_OBJ): $(SRC_DIR)/ufs.c
	$(CC) $(CFLAGS) -DTEST_BUILD -c $< -o $@

check: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(TEST_BIN) $(TEST_OBJS) $(MOCK_OBJS) $(SRC_COMPILE_OBJS) $(UFS_TEST_OBJ)
