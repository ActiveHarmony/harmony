TO_BASE=../../..

REQ_CFLAGS+=-fPIC
REQ_CPPFLAGS+=-I$(TO_BASE)/src

SRCS=libvertex.c \
     exhaustive.c \
     random.c \
     nm.c \
     pro.c \
     nemo.c

LIBEXEC_TGTS=exhaustive.so \
             random.so \
             nm.so \
             pro.so \
             nemo.so

exhaustive.so: REQ_LDLIBS+=-lm
exhaustive.so: libvertex.o

random.so: REQ_LDLIBS+=-lm
random.so: libvertex.o

nm.so: REQ_LDLIBS+=-lm
nm.so: libvertex.o

pro.so: REQ_LDLIBS+=-lm
pro.so: libvertex.o

nemo.so: REQ_LDLIBS+=-lm
nemo.so: libvertex.o

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
