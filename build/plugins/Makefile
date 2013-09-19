TO_BASE=../..
PREFIX=$(TO_BASE)

CC=gcc

CFLAGS=-std=c99 -pedantic -Wall -Werror -g

override CFLAGS+=-fPIC
override CPPFLAGS+=-I.. -D_ISOC99_SOURCE -D_XOPEN_SOURCE=500

SRCS=agg.c \
     codegen.c \
     codegen-helper.c \
     log.c \
     TAUdb.c \
     xmlWriter.c \
     constraint.c

LIBEXEC_TARGETS=agg.so \
                codegen.so \
                codegen-helper \
                log.so \
                constraint.so
TARGETS=$(LIBEXEC_TARGETS)

LIBXML2=
LIBXML2_INCDIR=$(LIBXML2)/include/libxml2
LIBXML2_LIBDIR=$(LIBXML2)/lib

ifneq (, $(wildcard $(LIBXML2_INCDIR)/libxml/xmlwriter.h))
	LIBEXEC_TARGETS += xmlWriter.so
endif

LIBTAUDB=
LIBTAUDB_INCDIR=$(LIBTAUDB)/include
LIBTAUDB_LIBDIR=$(LIBTAUDB)/lib

ifneq (, $(wildcard $(LIBTAUDB_LIBDIR)/libtaudb.a))
	LIBEXEC_TARGETS += TAUdb.so
endif

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

TAUdb.so: override LDLIBS+=$(LIBTAUDB_LIBDIR)/libtaudb.a -L.. -lharmony
TAUdb.so: override CFLAGS+=-I$(LIBTAUDB_INCDIR)

xmlWriter.so: override LDLIBS+=-L${LIBXML2_LIBDIR} -lxml2
xmlWriter.so: override CFLAGS+=-I${LIBXML2_INCDIR}

%.so: %.o ../libharmony.a
	$(CC) -shared $(LDFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

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
