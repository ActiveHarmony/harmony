TO_BASE=../../..

ifndef CHPL_HOME
$(error $$CHPL_HOME must be set to make this example)
endif

all: synth

CHPLFLAGS=-M$(TO_BASE)/build -I$(TO_BASE)/build  -L$(TO_BASE)/build -lharmony

synth: synth.chpl
	$(MAKE) -C $(TO_BASE)/build libharmony.a
	chpl $^ hclient.h $(CHPLFLAGS) -o $@

clean:
	rm -f synth
