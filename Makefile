# Top make file of DABC project

DABCMAINMAKE = true

DABCSYS := $(CURDIR)

include config/Makefile.config

## disable possibility, that rules included many times 
Dabc_Makefile_rules = true

CREATE_DIRS += $(DABCDLLPATH) $(DABCINCPATH) $(DABCBINPATH)

include base/Makefile.mk

include controls/simple/Makefile.mk

#DABC_PLUGINS = plugins/mbs plugins/bnet plugins/verbs plugins/roc plugins/abb

DABC_PLUGINS = $(wildcard plugins/*)

DABC_PLUGINS += $(wildcard applications/*)

-include $(patsubst %, %/Makefile, $(DABC_PLUGINS))

-include controls/xdaq/Makefile.mk

-include gui/java/Makefile.mk

-include gui/Qt/Makefile.mk

APPLICATIONS_DIRS =

libs::

clean::
	
package:: clean
	tar cf dabc.tar Makefile base/ build/ config/ controls/simple $(DABC_PLUGINS) --exclude=.svn --exclude=*.bak 
	gzip dabc.tar
	echo "dabc.tar.gz done" 


Dabc_Makefile_rules :=
include config/Makefile.rules
