TO_BASE=../..

SRCS=example.c
NO_INST_TGTS=example

example: REQ_CPPFLAGS+=-I$(TO_BASE)/src
example: REQ_LDLIBS+=-lm
example: $(TO_BASE)/src/libharmony.a

# ifeq (0, $(shell which $(FC) > /dev/null 2>&1; echo $$?))
#     SRCS+=fortran.f
#     NO_INST_TGTS+=fortran
#
#     fortran: REQ_FFLAGS+=-fno-underscoring
#     fortran: fortran.o $(TO_BASE)/src/libharmony.a
# else
#     $(info Skipping fortran client_api example: $(FC) not found.)
# endif

# CHPLC=chpl
# ifeq (0, $(shell which $(CHPLC) > /dev/null 2>&1; echo $$?))
#     SRCS+=chapel.chpl
#     NO_INST_TGTS+=chapel
#
#     chapel: chapel.chpl $(TO_BASE)/src/libharmony.a
# 	$(CHPLC) $(CHPLFLAGS) $^ $(TO_BASE)/src/hclient.h -o $@
# else
#     $(info Skipping chapel client_api example: $(CHPLC) not found.)
# endif

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
