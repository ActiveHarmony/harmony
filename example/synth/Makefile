TO_BASE=../..

SRCS=	synth.c \
	testfunc.c
OBJS=	$(SRCS:.c=.o)

TARGETS=synth

override CPPFLAGS+=-I$(TO_BASE)/src
override LDLIBS+=-lm

install:
	@echo "Examples must be executed from source directory (for now)."

synth: $(OBJS) $(TO_BASE)/src/libharmony.a
	$(CC) $(LDFLAGS) $(CFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
