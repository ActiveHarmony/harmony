REQ_CFLAGS+=-std=c99

ifeq ($(DEBUG), 1)
    REQ_CFLAGS+=-g
    REQ_CXXFLAGS+=-g
else
    REQ_CFLAGS+=-O2
    REQ_CXXFLAGS+=-O2
endif

ifeq ($(STRICT), 1)
    REQ_CFLAGS+=-pedantic -Wall -Werror
    REQ_CXXFLAGS+=-pedantic -Wall -Werror
endif
