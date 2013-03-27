TO_BASE=../..
PREFIX=$(TO_BASE)

CC=gcc

#XML_INCLUDE=-I/usr/include/libxml2
#XML_LIBS = -lxml2

PQDIR=/fs/mashie/zhouyf/postgresql/postgresql-9.1.2/src
PQ_LIBS=-L$(PQDIR)/interfaces/libpq -lpq -Wl,-rpath=$(PQDIR)/interfaces/libpq
TAUDB_LIB=

CFLAGS=-std=c99 -pedantic -Wall -Werror -g -pg
LDFLAGS=-luuid -ldl -lnsl -lm -lxml2


override CFLAGS+=-fPIC
override CPPFLAGS+=-I.. -D_ISOC99_SOURCE -D_XOPEN_SOURCE=500

SRCS=codegen.c \
     codegen-helper.c \
#	 tauDB.c

LIBEXEC_TARGETS=codegen.so \
				codegen-helper \
#				tauDB.so

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
tauDB.so: override LDFLAGS+=${PQ_LIBS} -L.. -lharmony -L -ltaudb
tauDB.so: override CFLAGS+=
#xmlWriter.so: override LDFLAGS+=${XML_LIBS}
#xmlWriter.so: override CFLAGS+=${XML_INCLUDE}


%.so: %.o libharmony.a ${TAUDB_LIB}
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
