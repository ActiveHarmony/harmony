TO_BASE=..

override CPPFLAGS+=-I$(TO_BASE)/src

BIN_SRCS=code_generator.cxx
SRCS=$(BIN_SRCS)

BIN_TARGETS=code_generator

TARGETS=$(BIN_TARGETS)

install:
	@echo "Code server must be executed from source directory (for now)."

code_generator: code_generator.o $(TO_BASE)/src/libharmony.a
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.o: %.cxx
	$(CXX) -c $< $(CPPFLAGS) $(CXXFLAGS) -o $@

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
