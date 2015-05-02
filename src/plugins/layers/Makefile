TO_BASE=../../..
PREFIX=$(TO_BASE)

CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g

override CFLAGS+=-fPIC
override CPPFLAGS+=-I$(TO_BASE)/src
override LDFLAGS+=-L$(TO_BASE)/src
override LDLIBS+=-lharmony

SRCS=agg.c \
     cache.c \
     codegen.c \
     codegen-helper.c \
     constraint.c \
     group.c \
     httpinfo.c \
     log.c \
     TAUdb.c \
     xmlWriter.c

LIBEXEC_TARGETS=agg.so \
                cache.so \
                codegen.so \
                codegen-helper \
                constraint.so \
                group.so \
                httpinfo.so \
                log.so
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

.PHONY: all install libharmony clean distclean

all: $(TARGETS)

install: $(TARGETS)
	@if [ ! -d $(PREFIX)/libexec ]; then                             \
		echo mkdir -p $(PREFIX)/libexec;                         \
		mkdir -p $(PREFIX)/libexec;                              \
	fi &&                                                            \
	echo cp $(LIBEXEC_TARGETS) $(LIBEXEC_FILES) $(PREFIX)/libexec && \
	cp $(LIBEXEC_TARGETS) $(LIBEXEC_FILES) $(PREFIX)/libexec

TAUdb.so: override LDLIBS+=$(LIBTAUDB_LIBDIR)/libtaudb.a
TAUdb.so: override CPPFLAGS+=-I$(LIBTAUDB_INCDIR)

xmlWriter.so: override LDLIBS+=-L${LIBXML2_LIBDIR} -lxml2
xmlWriter.so: override CPPFLAGS+=-I${LIBXML2_INCDIR}

%.so: %.o libharmony
	$(CC) -shared $(LDFLAGS) $(CFLAGS) $< $(LOADLIBES) $(LDLIBS) -o $@

libharmony:
	$(MAKE) -C $(TO_BASE)/src libharmony.a

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
