# Top make file of DABC project

DABCMAINMAKE = true

DABCSYS := .

include config/Makefile.config

## disable possibility, that rules included many times 
Dabc_Makefile_rules = true

CREATE_DIRS += $(DABCDLLPATH) $(DABCINCPATH) $(DABCBINPATH)

include base/Makefile

include plugins/mbs/Makefile


Dabc_Makefile_rules :=
include config/Makefile.rules
