TO_BASE=../..

CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g
CPPFLAGS=-D_ISOC99_SOURCE -D_XOPEN_SOURCE=500

SRCS=	synth.c \
	testfunc.c
OBJS=	$(SRCS:.c=.o)

TARGETS=synth

override CPPFLAGS+=-I$(TO_BASE)/src
override LDFLAGS+=-L$(TO_BASE)/src
override LDLIBS+=-lharmony -lm

.PHONY: all install libharmony clean distclean

all: $(TARGETS)

install: all
	@echo "Examples must be executed from source directory (for now)."

synth: $(OBJS)
	$(MAKE) -C $(TO_BASE)/src libharmony.a
	$(CC) $(LDFLAGS) $(CFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm -f $(TARGETS) $(OBJS)

distclean: clean
	rm -f *.d *~

#
# Auto dependency creation
#
%.d: %.c
	@rm -f $@; \
		$(CC) -MM $(CPPFLAGS) $< > $@.$$$$ 2>/dev/null; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$

-include $(SRCS:.c=.d)
