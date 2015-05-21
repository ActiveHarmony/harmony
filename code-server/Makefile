TO_BASE=..

override CPPFLAGS+=-I$(TO_BASE)/src

SRCS=code_generator.cxx
NO_INST_TGTS=code_generator

code_generator: code_generator.o $(TO_BASE)/src/libharmony.a
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.o: %.cxx
	$(CXX) -c $< $(CPPFLAGS) $(CXXFLAGS) -o $@

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
