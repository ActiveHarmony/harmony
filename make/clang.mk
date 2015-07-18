REQ_CFLAGS+=-std=c99

ifeq ($(DEBUG), 1)
    REQ_CFLAGS+=-g
    REQ_CXXFLAGS+=-g
else
    REQ_CFLAGS+=-O2
    REQ_CXXFLAGS+=-O2
endif

ifeq ($(STRICT), 1)
    STRICT_FLAGS=$(strip -Weverything -Werror \
                         -Wno-disabled-macro-expansion \
                         -Wno-documentation \
                         -Wno-documentation-unknown-command \
                         -Wno-exit-time-destructors \
                         -Wno-float-equal \
                         -Wno-format-nonliteral \
                         -Wno-global-constructors \
                         -Wno-padded \
                         -Wno-missing-field-initializers \
                         -Wno-missing-prototypes \
                         -Wno-missing-noreturn \
                         -Wno-missing-variable-declarations \
                         -Wno-shorten-64-to-32 \
                         -Wno-sign-compare \
                         -Wno-sign-conversion \
                         -Wno-switch-enum \
                         -Wno-unknown-attributes \
                         -Wno-unused-parameter \
                  )
    REQ_CFLAGS+=$(STRICT_FLAGS)
    REQ_CXXFLAGS+=$(STRICT_FLAGS)
else
    REQ_CFLAGS+=-Wno-unknown-attributes
    REQ_CXXFLAGS+=-Wno-unknown-attributes
endif
