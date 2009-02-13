# Top make file of DABC project

DABCMAINMAKE = true

DABCSYS := $(CURDIR)

include config/Makefile.config

## disable possibility, that rules included many times 
Dabc_Makefile_rules = true

CREATE_DIRS += $(DABCDLLPATH) $(DABCINCPATH) $(DABCBINPATH)

include base/Makefile.mk

include controls/simple/Makefile.mk

-include dim/Makefile.mk

-include controls/dimcontrol/Makefile.mk


DABC_PLUGINS_PACK = plugins/mbs plugins/bnet plugins/bnet-mbs plugins/verbs plugins/root
DABC_APPLICATIONS_PACK = applications/mbs applications/bnet-mbs applications/bnet-test
DABC_SCRIPTS_PACK = script/dabclogin-distribution.sh script/dabcstartup.sh script/dabcshutdown.sh script/gdbcmd.txt

DABC_ROC_PACK = plugins/roc applications/roc 
DABC_ABB_PACK = plugins/abb applications/bnet-test 

DABC_PLUGINS = $(wildcard plugins/*)

DABC_PLUGINS += $(wildcard applications/*)

-include $(patsubst %, %/Makefile, $(DABC_PLUGINS))

-include gui/java/Makefile.mk

-include gui/Qt/Makefile.mk

-include doc/Makefile.mk

# APPLICATIONS_DIRS =

libs::

clean::
	
package:: clean
	rm -f dabc.tar.gz
	tar cf dabc.tar Makefile base/ build/ config/ controls/simple controls/dimcontrol dim  gui $(DABC_PLUGINS_PACK) $(DABC_APPLICATIONS_PACK) $(DABC_SCRIPTS_PACK) --exclude=.svn --exclude=*.bak 
	gzip dabc.tar
	echo "dabc.tar.gz done" 

packageroc:: clean
	rm -f roc.tar.gz
	tar cf roc.tar $(DABC_ROC_PACK) --exclude=.svn --exclude=*.bak 
	gzip roc.tar
	echo "roc.tar.gz done" 

packageabb:: clean
	rm -f abb.tar.gz
	tar cf abb.tar $(DABC_ABB_PACK) --exclude=.svn --exclude=*.bak 
	gzip abb.tar
	echo "abb.tar.gz done" 


Dabc_Makefile_rules :=
include config/Makefile.rules
