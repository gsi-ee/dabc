# Top make file of DABC project

DABCMAINMAKE = true

#debug=4
#extrachecks = true

DABCSYS := $(CURDIR)

include config/Makefile.config

## disable possibility, that rules included many times 
Dabc_Makefile_rules = true

CREATE_DIRS += $(DABCDLLPATH) $(DABCINCPATH) $(DABCBINPATH)


include base/Makefile.mk

DABC_PLUGINS = $(wildcard plugins/*)

DABC_PLUGINS += $(wildcard applications/*)

-include $(patsubst %, %/Makefile, $(DABC_PLUGINS))

-include doc/Makefile.mk

libs:: dabclogin

clean::

dabclogin: build/dabclogin.sh
	@sed -e "s|\`pwd\`|$(CURDIR)|" -e "s|version|$(VERSSUF)|" \
	< build/dabclogin.sh > dabclogin; chmod 755 dabclogin; echo "Create dabclogin"

Dabc_Makefile_rules :=

-include config/Makefile.packaging

include config/Makefile.rules
