TO_BASE=..
SUBDIRS=plugins/layers \
        plugins/strategies \

LIB_SRCS=hcfg.c \
         hmesg.c \
         hpoint.c \
         hperf.c \
         hsession.c \
         hsignature.c \
         hsockutil.c \
         hutil.c \
         hval.c
CLI_SRCS=hclient.c
BIN_SRCS=hserver.c \
         httpsvr.c \
         session-core.c \
         tuna.c \
         hinfo.c
SRCS=$(BIN_SRCS) $(LIB_SRCS) $(CLI_SRCS)

LIB_OBJS=$(LIB_SRCS:.c=.o)
CLI_OBJS=$(CLI_SRCS:.c=.o)

BIN_TGTS=hinfo \
         hserver \
         tuna
LIB_TGTS=libharmony.a
LIBEXEC_TGTS=session-core
LIBEXEC_COPY=http/overview.cgi \
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
INCLUDE_COPY=hclient.h

hserver: httpsvr.o $(LIB_OBJS)

session-core: REQ_LDFLAGS+=-Wl,--export-dynamic
session-core: REQ_LDLIBS+=-ldl
session-core: $(LIB_OBJS)

hinfo: REQ_LDFLAGS+=-Wl,--export-dynamic
hinfo: REQ_LDLIBS+=-ldl
hinfo: $(LIB_OBJS)

tuna: libharmony.a

libharmony.a: $(LIB_OBJS) $(CLI_OBJS)

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
