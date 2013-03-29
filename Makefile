all_subsystems = build code-server example/client_api example/code_generation example/synth example/tauDB

.PHONY: all install clean distclean

all install clean distclean:
	+@for subsystem in $(all_subsystems); do \
	    $(MAKE) -C $$subsystem $@;           \
	    RETVAL=$$?;                          \
	    if [ $$RETVAL != 0 ]; then           \
		exit $$RETVAL;                   \
	    fi;                                  \
	done
