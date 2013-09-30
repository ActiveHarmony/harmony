TO_BASE=../..

CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g
CPPFLAGS=-D_ISOC99_SOURCE -D_XOPEN_SOURCE=500
FC=gfortran
FFLAGS=-Wall -g
CHPLC=chpl

override CPPFLAGS+=-I$(TO_BASE)/src
override FFLAGS+=-fno-underscoring
override CHPLFLAGS+=-M$(TO_BASE)/src -I$(TO_BASE)/src
override LDFLAGS+=-L$(TO_BASE)/src -lharmony -lm

TARGETS=example_c #example_f example_chpl

.PHONY: all install clean distclean

all: $(TARGETS)

install: $(TARGETS)
	@echo "Examples must be executed from source directory (for now)."

example_c: example.c $(TO_BASE)/src/libharmony.a
	@if [ -x "`which $(CC) 2>&1`" ]; then                           \
		echo "$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@"; \
		$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@;        \
	else                                                            \
		echo "Skipping build of $@: $(CC) not found.";          \
	fi

example_f: example.f $(TO_BASE)/src/libharmony.a
	@if [ -x "`which $(FC) 2>&1`" ]; then                  \
		echo "$(FC) $(FFLAGS) $< $(LDFLAGS) -o $@";    \
		$(FC) $(FFLAGS) $< $(LDFLAGS) -o $@;           \
	else                                                   \
		echo "Skipping build of $@: $(FC) not found."; \
	fi

example_chpl: example.chpl $(TO_BASE)/src/libharmony.a
	@if [ -x "`which $(CHPLC) 2>&1`" ]; then                            \
		echo "$(CHPLC) $(CHPLFLAGS) $< hclient.h $(LDFLAGS) -o $@"; \
		$(CHPLC) $(CHPLFLAGS) $< hclient.h $(LDFLAGS) -o $@;        \
	else                                                                \
		echo "Skipping build of $@: chpl not found.";               \
	fi

$(TO_BASE)/src/libharmony.a:
	$(MAKE) -C $(TO_BASE)/src libharmony.a

clean:
	rm -f core $(TARGETS)

distclean: clean
	rm -f *~
