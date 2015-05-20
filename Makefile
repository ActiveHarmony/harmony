TO_BASE=.

SUBDIRS=src \
	code-server \
	example/client_api \
	example/constraint \
	example/code_generation \
	example/synth \
	example/taudb \
	example/xml

.PHONY: doc

doc:
	$(MAKE) -C doc

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
