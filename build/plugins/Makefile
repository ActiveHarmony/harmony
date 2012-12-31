TO_BASE=../..
PREFIX=$(TO_BASE)

CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g

override CFLAGS+=-fPIC
override CPPFLAGS+=-D_ISOC99_SOURCE -D_XOPEN_SOURCE=500 -I..

SRCS=codegen.c \
     codegen-helper.c

LIBEXEC_TARGETS=codegen.so \
                codegen-helper
TARGETS=$(LIBEXEC_TARGETS)

.PHONY: all install clean distclean

all: $(TARGETS)

install: $(TARGETS)
	@if [ ! -d $(PREFIX)/libexec ]; then                           \
		echo mkdir $(PREFIX)/libexec;                          \
		mkdir $(PREFIX)/libexec;                               \
	fi;                                                            \
	echo cp $(LIBEXEC_TARGETS) $(LIBEXEC_FILES) $(PREFIX)/libexec; \
	cp $(LIBEXEC_TARGETS) $(LIBEXEC_FILES) $(PREFIX)/libexec

codegen-helper: override LDLIBS+=-L.. -lharmony
codegen-helper: codegen-helper.o

%.so: %.o
	$(CC) -shared $(LDFLAGS) $(CFLAGS) $^ -o $@

clean:
	rm -f core a.out *.o $(TARGETS)

distclean: clean
	rm -f *~ *.d

#
# Auto dependency creation
#
%.d: %.c
	@rm -f $@; \
		$(CC) -MM $(CPPFLAGS) $< > $@.$$$$ 2>/dev/null; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$

-include $(SRCS:.c=.d)
