TO_BASE=../..

CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g

SRCS=	synth.c \
	testfunc.c
OBJS=	$(SRCS:.c=.o)

TARGETS=synth

override CPPFLAGS+=-I$(TO_BASE)/src
override LDFLAGS+=-L$(TO_BASE)/src
override LDLIBS+=-lharmony -lm

.PHONY: libharmony

install:
	@echo "Examples must be executed from source directory (for now)."

synth: $(OBJS)
	$(MAKE) -C $(TO_BASE)/src libharmony.a
	$(CC) $(LDFLAGS) $(CFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
