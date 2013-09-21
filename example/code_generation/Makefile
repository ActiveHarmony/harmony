TO_BASE=../..

MPICC=mpicc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g
CPPFLAGS=-D_ISOC99_SOURCE -D_XOPEN_SOURCE=500

override CPPFLAGS+=-I$(TO_BASE)/src
override LDFLAGS+=-L$(TO_BASE)/src -lharmony -ldl

.PHONY: all install clean distclean

all: gemm

install: gemm
	@echo "Examples must be executed from source directory (for now)."

gemm: gemm.c $(TO_BASE)/src/libharmony.a
	@if [ -x "`which $(MPICC) 2>&1`" ]; then                           \
		echo "$(MPICC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@"; \
		$(MPICC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@;        \
	else                                                               \
		echo "Skipping build of $@: $(MPICC) not found.";          \
	fi

$(TO_BASE)/src/libharmony.a:
	$(MAKE) -C $(TO_BASE)/src libharmony.a

clean:
	rm -f core gemm

distclean: clean
