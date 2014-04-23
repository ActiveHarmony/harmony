TO_BASE=..
PREFIX=$(TO_BASE)

CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g
CPPFLAGS=-D_ISOC99_SOURCE -D_XOPEN_SOURCE=500

SHR_SRCS=hcfg.c \
         hmesg.c \
         hpoint.c \
         hperf.c \
         hsession.c \
         hsignature.c \
         hsockutil.c \
         hutil.c \
         hval.c
LIB_SRCS=hclient.c
BIN_SRCS=hserver.c \
         httpsvr.c \
         session-core.c \
         tuna.c
SRCS=$(SHR_SRCS) $(LIB_SRCS) $(BIN_SRCS)

SHR_OBJS=$(SHR_SRCS:.c=.o)
LIB_OBJS=$(LIB_SRCS:.c=.o)

BIN_FILES=harmony.cfg
INCLUDE_FILES=hclient.h
HTTP_FILES=http/overview.cgi \
           http/overview.js \
           http/session-view.cgi \
           http/session-view.js \
           http/common.js \
           http/activeharmony.css \
           http/jquery.min.js \
           http/jquery.flot.min.js \
           http/jquery.flot.time.min.js \
           http/jquery.flot.resize.min.js \
           http/jquery.flot.selection.min.js \
           http/excanvas.min.js

BIN_TARGETS=hserver \
            tuna
LIB_TARGETS=libharmony.a
LIBEXEC_TARGETS=session-core
TARGETS=$(BIN_TARGETS) $(LIB_TARGETS) $(LIBEXEC_TARGETS)

SUBDIRS=plugins/strategies \
        plugins/layers

.PHONY: all install clean distclean $(SUBDIRS)

all: $(TARGETS) $(SUBDIRS)

install: $(TARGETS) $(SUBDIRS)
	@if [ ! -d $(PREFIX)/bin ]; then                     \
		echo mkdir -p $(PREFIX)/bin;                 \
		mkdir -p $(PREFIX)/bin;                      \
	fi &&                                                \
	echo cp $(BIN_TARGETS) $(BIN_FILES) $(PREFIX)/bin && \
	cp $(BIN_TARGETS) $(BIN_FILES) $(PREFIX)/bin &&      \
                                                             \
	if [ ! -d $(PREFIX)/lib ]; then                      \
		echo mkdir -p $(PREFIX)/lib;                 \
		mkdir -p $(PREFIX)/lib;                      \
	fi &&                                                \
	echo cp $(LIB_TARGETS) $(PREFIX)/lib &&              \
	cp $(LIB_TARGETS) $(PREFIX)/lib &&                   \
                                                             \
	if [ ! -d $(PREFIX)/include ]; then                  \
		echo mkdir -p $(PREFIX)/include;             \
		mkdir -p $(PREFIX)/include;                  \
	fi &&                                                \
	echo cp $(INCLUDE_FILES) $(PREFIX)/include &&        \
	cp $(INCLUDE_FILES) $(PREFIX)/include &&             \
                                                             \
	if [ ! -d $(PREFIX)/libexec ]; then                  \
		echo mkdir -p $(PREFIX)/libexec;             \
		mkdir -p $(PREFIX)/libexec;                  \
	fi &&                                                \
	echo cp $(LIBEXEC_TARGETS) $(PREFIX)/libexec &&      \
	cp $(LIBEXEC_TARGETS) $(PREFIX)/libexec &&           \
                                                             \
	if [ ! -d $(PREFIX)/libexec/html ]; then             \
		echo mkdir -p $(PREFIX)/libexec/html;        \
		mkdir -p $(PREFIX)/libexec/html;             \
	fi &&                                                \
	echo cp $(HTTP_FILES) $(PREFIX)/libexec/html &&      \
	cp $(HTTP_FILES) $(PREFIX)/libexec/html

hserver: httpsvr.o $(SHR_OBJS)

session-core: override LDFLAGS+=-Wl,--export-dynamic
session-core: override LDLIBS+=-ldl
session-core: $(SHR_OBJS)

tuna.o: override CFLAGS+=-D_BSD_SOURCE
tuna: libharmony.a

libharmony.a: $(LIB_OBJS) $(SHR_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(LIB_OBJS) $(SHR_OBJS): override CFLAGS+=-fPIC

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

clean: $(SUBDIRS)
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
