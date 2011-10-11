TO_ROOT = ../..

VPATH=$(TO_ROOT)/build

OPTIMIZE = -ggdb -pg

LIBS_LINUX = -lm -ldl -lnsl -ltcl8.4 -ltk8.4
INCLUDE_LINUX = -I$(TO_ROOT)/build
GCC_LINUX = g++ -g -DLinux -DLINUX -Dlinux $(OPTIMIZE) $(INCLUDE_LINUX)

GCC_SOLARIS = g++ -g -DSOLARIS -DUSE_TK
LIBS_SOLARIS = -L/usr/imports/lib -lsocket -lnsl -ltk8.4 -ltcl8.4 -lm -ldl -lX11
INCLUDE_SOLARIS = -I/usr/imports/include -I/hivehomes/rahulp/ann_1.1.1/include/

# Support for OSX (tested in Lion)
LIBS_OSX = -lm -ldl -ltcl8.5 -ltk8.5
INCLUDE_OSX = -I$(TO_ROOT)/build
GCC_OSX = llvm-g++ $(OPTIMIZE) $(INCLUDE_OSX)

OS_NOM = $(shell uname -s)
ifeq ($(OS_NOM), Darwin)
  GCC=$(GCC_OSX)
  LIBS=$(LIBS_OSX)
  INCLUDE=$(INCLUDE_OSX)
else
  GCC=$(GCC_LINUX)
  LIBS=$(LIBS_LINUX)
  INCLUDE=$(INCLUDE_LINUX)
endif

#GCC=$(GCC_SOLARIS)
#LIBS=$(LIBS_SOLARIS)
#INCLUDE=$(INCLUDE_SOLARIS)

LIBOBJS= code_generator.o Tokenizer.o

all: code_generator parse_host_file

Tokenizer.o:
	$(GCC) -c Tokenizer.cpp

$(TO_ROOT)/build/hglobal_config.o: hglobal_config.c hglobal_config.h
	$(MAKE) -C $(TO_ROOT)/build hglobal_config.o

code_generator.o: code_generator.c code_generator.h
	$(GCC) -c $< -o $@

code_generator: code_generator.o Tokenizer.o hglobal_config.o
	$(GCC) $(INCLUDE) $(LIB) $^ -o $@

parse_host_file: parse_host_file.c Tokenizer.o
	$(GCC) $^ -o $@

clean:
	rm -f code_generator core.* *.o parse_host_file
