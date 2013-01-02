TO_BASE=../..

CC=mpicc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g
CPPFLAGS=-D_ISOC99_SOURCE -D_XOPEN_SOURCE=500

override CPPFLAGS+=-I$(TO_BASE)/build
override LDFLAGS+=-L$(TO_BASE)/build -lharmony

.PHONY: all install clean distclean

all: gemm

install: gemm
	@echo "Examples must be executed from source directory (for now)."

gemm: gemm.c $(TO_BASE)/build/libharmony.a
	@if [ -x "`which $(CC) 2>&1`" ]; then				\
		echo "$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@";	\
		$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@;	\
	else								\
		echo "Skipping build of $@: $(CC) not found.";		\
	fi

$(TO_BASE)/build/libharmony.a:
	$(MAKE) -C $(TO_BASE)/build libharmony.a

clean:
	rm -f core gemm

distclean: clean
