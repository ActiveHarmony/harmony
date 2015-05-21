TO_BASE=../../..
INST_DIR=$(PREFIX)/libexec

override CFLAGS+=-fPIC
override CPPFLAGS+=-I$(TO_BASE)/src

TCL_INCDIR?=/usr/include
TCL_LIBDIR?=/usr/lib64

LIBEXEC_SRCS=libvertex.c \
             exhaustive.c \
             random.c \
             nm.c \
             pro.c \
             nemo.c

SRCS=$(LIBEXEC_SRCS)

LIBEXEC_TARGETS=exhaustive.so \
                random.so \
                nm.so \
                pro.so \
                nemo.so

TARGETS=$(LIBEXEC_TARGETS)

install:
	@if [ ! -d $(INST_DIR) ]; then       \
		echo mkdir -p $(INST_DIR);   \
		mkdir -p $(INST_DIR);        \
	fi &&                                \
	for i in $(LIBEXEC_TARGETS); do      \
		echo cp $$i $(INST_DIR)/$$i; \
		cp $$i $(INST_DIR)/$$i;      \
	done

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
