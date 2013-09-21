TO_BASE=..
PREFIX=$(TO_BASE)

CXX=g++
CXXFLAGS=-Wall -pedantic -Werror -g

override CPPFLAGS+=-I$(TO_BASE)/src

BIN_SRCS=code_generator.cxx
SRCS=$(BIN_SRCS)

BIN_TARGETS=code_generator

.PHONY: all install clean distclean

all: $(BIN_TARGETS)

install: $(BIN_TARGETS)
	@echo "Code server must be executed from source directory (for now)."

code_generator: code_generator.o $(TO_BASE)/src/libharmony.a
	$(CXX) -L$(TO_BASE)/src $(LDFLAGS) $< -lharmony $(LDLIBS) -o $@

%.o: %.cxx
	$(CXX) -c $< $(CPPFLAGS) $(CXXFLAGS) -o $@

$(TO_BASE)/src/libharmony.a:
	$(MAKE) -C $(TO_BASE)/src libharmony.a

clean:
	rm -f core.* *.o $(BIN_TARGETS)

distclean: clean
	rm -f *~ *.d

#
# Auto dependency creation
#
%.d: %.cxx
	@rm -f $@; \
		$(CXX) -MM $(CPPFLAGS) $< > $@.$$$$ 2>/dev/null; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$

-include $(SRCS:.cxx=.d)
