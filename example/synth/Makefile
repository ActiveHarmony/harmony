TO_BASE=../..

SRCS=synth.c \
     testfunc.c
NO_INST_TGTS=synth

override CPPFLAGS+=-I$(TO_BASE)/src
override LDLIBS+=-lm

synth: $(SRCS:.c=.o) $(TO_BASE)/src/libharmony.a
	$(CC) $(LDFLAGS) $(CFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
