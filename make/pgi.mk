REQ_CFLAGS+=-c99

ifeq ($(DEBUG), 1)
    REQ_CFLAGS+=-g -Minform=severe
    REQ_CXXFLAGS+=-g -Minform=severe
else
    REQ_CFLAGS+=-fast -Minform=fatal
    REQ_CXXFLAGS+=-fast -Minform=fatal
endif

ifeq ($(STRICT), 1)
    REQ_CFLAGS+=-Minform=warn
    REQ_CXXFLAGS+=-Minform=warn
endif
