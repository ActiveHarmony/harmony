TO_BASE=../..

TEST_DIR=$(TO_BASE)/example/client_api
TEST_BIN=example_c

.PHONY: all install test client clean distclean

all: client

install: all
	@echo "Examples must be executed from source directory (for now)."

test: client
	$(TEST_DIR)/$(TEST_BIN) \
		SESSION_LAYER_LIST=constraint.so \
		CONSTRAINT_FILE=constraint.in

client:
	$(MAKE) -C $(TEST_DIR) $(TEST_BIN)

clean:

distclean: clean
	rm -f *~
