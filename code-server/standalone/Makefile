TO_ROOT = ../..

VPATH=$(TO_ROOT)/build

OPTIMIZE = -O3
LIBS_LINUX = -lm -ldl -lnsl -ltcl8.4
INCLUDE_LINUX = -I$(TO_ROOT)/build
GCC_LINUX = g++ -g -DLinux -DLINUX -Dlinux $(OPTIMIZE) $(INCLUDE_LINUX)

GCC_SOLARIS = g++ -g -DSOLARIS
LIBS_SOLARIS = -L/usr/imports/lib -lsocket -lnsl -ltcl8.4 -lm -ldl -lX11
INCLUDE_SOLARIS = -I/usr/imports/include -I/hivehomes/rahulp/ann_1.1.1/include/

GCC=$(GCC_LINUX)
LIBS=$(LIBS_LINUX)
INCLUDE=$(INCLUDE_LINUX)

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
