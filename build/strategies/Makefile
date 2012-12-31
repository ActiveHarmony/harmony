TO_BASE=../..
PREFIX=$(TO_BASE)
INST_DIR=$(PREFIX)/libexec

CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g

override CFLAGS+=-fPIC
override CPPFLAGS+=-I.. -D_ISOC99_SOURCE -D_XOPEN_SOURCE=500

TCL_INCDIR?=/usr/include
TCL_LIBDIR?=/usr/lib64

LIBEXEC_SRCS=brute.c \
             random.c \
             nelder_mead.c \
             pro.c
SRCS=$(LIBEXEC_SRCS)

LIBEXEC_FILES=hconfig.nm.tcl \
              tcl/nm/parseApp.tcl \
              tcl/nm/newparseApp.tcl \
              tcl/nm/utilities.tcl \
              tcl/nm/nm.tcl \
              tcl/nm/nm_init.tcl \
              hconfig.pro.tcl \
              tcl/pro/utilities.tcl \
              tcl/pro/parseApp_version_8.tcl \
              tcl/pro/proparseApp_version_8.tcl \
              tcl/pro/pro.tcl \
              tcl/pro/initial_simplex_construction.tcl \
              tcl/pro/random_uniform.tcl \
              tcl/pro/matrix.tcl \
              tcl/pro/pro_transformations.tcl \
              tcl/pro/projection.tcl \
              tcl/pro/combine.tcl \
              tcl/pro/pro_init.tcl

LIBEXEC_TARGETS=brute.so \
                random.so \
                nelder_mead.so \
                pro.so
TARGETS=$(LIBEXEC_TARGETS)

.PHONY: all install clean distclean

all: $(TARGETS)

install: $(TARGETS)
	@if [ ! -d $(INST_DIR) ]; then                   \
		echo mkdir $(INST_DIR);                  \
		mkdir $(INST_DIR);                       \
	fi;                                              \
	if [ ! -d $(INST_DIR)/tcl ]; then                \
		echo mkdir $(INST_DIR)/tcl;              \
		mkdir $(INST_DIR)/tcl;                   \
	fi;                                              \
	if [ ! -d $(INST_DIR)/tcl/nm ]; then             \
		echo mkdir $(INST_DIR)/tcl/nm;           \
		mkdir $(INST_DIR)/tcl/nm;                \
	fi;                                              \
	if [ ! -d $(INST_DIR)/tcl/pro ]; then            \
		echo mkdir $(INST_DIR)/tcl/pro;          \
		mkdir $(INST_DIR)/tcl/pro;               \
	fi;                                              \
	for i in $(LIBEXEC_TARGETS) $(LIBEXEC_FILES); do \
		echo cp $$i $(INST_DIR)/$$i;             \
		cp $$i $(INST_DIR)/$$i;                  \
	done

nelder_mead.o: override CPPFLAGS+=-I$(TCL_INCDIR)
nelder_mead.so: override LDFLAGS+=-L$(TCL_LIBDIR) -ltcl

pro.o: override CPPFLAGS+=-I$(TCL_INCDIR)
pro.so: override LDFLAGS+=-L$(TCL_LIBDIR) -ltcl

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
