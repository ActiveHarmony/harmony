TO_BASE=../../..

override CFLAGS+=-fPIC
override CPPFLAGS+=-I$(TO_BASE)/src

TCL_INCDIR?=/usr/include
TCL_LIBDIR?=/usr/lib64

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

exhaustive.so: override LDLIBS+=-lm
exhaustive.so: libvertex.o

random.so: override LDLIBS+=-lm
random.so: libvertex.o

nm.so: override LDLIBS+=-lm
nm.so: libvertex.o

pro.so: override LDLIBS+=-lm
pro.so: libvertex.o

nemo.so: override LDLIBS+=-lm
nemo.so: libvertex.o

%.so: %.o
	$(CC) -shared $(LDFLAGS) $(CFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
