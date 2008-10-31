# Top make file of DABC project

DABCMAINMAKE = true

DABCSYS := $(CURDIR)

include config/Makefile.config

## disable possibility, that rules included many times 
Dabc_Makefile_rules = true

CREATE_DIRS += $(DABCDLLPATH) $(DABCINCPATH) $(DABCBINPATH)

include base/Makefile.mk

include controls/simplecontrol/Makefile.mk

DABC_PLUGINS = plugins/mbs plugins/bnet

DABC_APPLICATIONS = applications/bnet-test applications/bnet-mbs
  
include $(patsubst %, %/Makefile, $(DABC_PLUGINS))

TEST_DIRS += $(DABC_APPLICATIONS)

libs:: $(DABCSYS)/config/Makefile.plugins

$(DABCSYS)/config/Makefile.plugins: $(LIBS_PLUGINS)
	@rm -f $@
	@echo "LIBS_EXTRA = $(LIBS_EXTRA)" > $@

clean::
	rm -f $(DABCSYS)/config/Makefile.plugins
	
package:: clean
	tar cf dabc.tar Makefile base/ build/ config/ controls/simplecontrol $(DABC_PLUGINS) $(DABC_APPLICATIONS) --exclude=.svn --exclude=*.bak 
	gzip dabc.tar
	echo "dabc.tar.gz done" 


Dabc_Makefile_rules :=
include config/Makefile.rules
