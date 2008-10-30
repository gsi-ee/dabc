# Top make file of DABC project

DABCMAINMAKE = true

DABCSYS := .

include config/Makefile.config

## disable possibility, that rules included many times 
Dabc_Makefile_rules = true

BLD_DIRS += $(DABCDLLPATH)
BLD_DIRS += $(DABCINCPATH)

include base/Makefile


Dabc_Makefile_rules :=
include config/Makefile.rules
