TO_BASE=../../..

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

install:
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

%.so: %.o $(TO_BASE)/src/libharmony.a
	$(CC) -shared $(LDFLAGS) $(CFLAGS) $< $(LOADLIBES) $(LDLIBS) -o $@

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
