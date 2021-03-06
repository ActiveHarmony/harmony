ifeq ($(C_COMPILER_MAKE), intel)
    REQ_CFLAGS+=-std=c99

    ifeq ($(DEBUG), 1)
        REQ_CFLAGS+=-g
    else
        REQ_CFLAGS+=-O2
    endif

    ifeq ($(STRICT), 1)
        REQ_CFLAGS+=-Wall -Werror
    endif
endif

ifeq ($(CXX_COMPILER_MAKE), intel)
    ifeq ($(DEBUG), 1)
        REQ_CXXFLAGS+=-g
    else
        REQ_CXXFLAGS+=-O2
    endif

    ifeq ($(STRICT), 1)
        REQ_CXXFLAGS+=-Wall -Werror
    endif
endif
