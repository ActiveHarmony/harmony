TO_BASE=../..

TEST_DIR=$(TO_BASE)/example/client_api
TEST_BIN=example_c

.PHONY: test client

all: client

install:
	@echo "Examples must be executed from source directory (for now)."

test: client
	$(TEST_DIR)/$(TEST_BIN) LAYERS=constraint.so OC_FILE=constraint.in

client:
	$(MAKE) -C $(TEST_DIR) $(TEST_BIN)

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
