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

# following lines define the packaging:
PACKAGE_DIR = ./packages
DABCPACK_VERS     = dabc_$(MAJOR)_$(MINOR)
ROCPACK_VERS     =  roc_$(MAJOR)_$(MINOR)
ABBPACK_VERS     =  abb_$(MAJOR)_$(MINOR)
DABCTAR_NAME      = dabc$(MAJOR).tar
ROCTAR_NAME      = roc$(MAJOR).tar
ABBTAR_NAME      = abb$(MAJOR).tar

DISTR_DIR  = ~/dabc_temp_packaging
DABCDISTR_DIR  = $(DISTR_DIR)/$(DABCPACK_VERS)
ROCDISTR_DIR  = $(DISTR_DIR)/$(ROCPACK_VERS)
ABBDISTR_DIR  = $(DISTR_DIR)/$(ABBPACK_VERS)

DABC_PLUGINS_PACK = plugins/mbs plugins/bnet plugins/bnet-mbs plugins/verbs plugins/root
DABC_APPLICATIONS_PACK = applications/mbs applications/bnet-mbs applications/bnet-test
DABC_SCRIPTS_PACK = script/dabclogin-distribution.sh script/dabcstartup.sh script/dabcshutdown.sh script/gdbcmd.txt

DABC_ROC_PACK = plugins/roc applications/roc 
DABC_ABB_PACK = plugins/abb applications/bnet-test 

DABC_PLUGINS = $(wildcard plugins/*)

DABC_PLUGINS += $(wildcard applications/*)

# following is for adding header to sources (done once)
DABC_HEADERS = $(wildcard base/*/*.h)
DABC_HEADERS += $(wildcard controls/*/*/*.h)
DABC_HEADERS += $(wildcard plugins/*/*/*.h)
DABC_HEADERS += $(wildcard applications/*/*.h)

DABC_IMPS = $(wildcard base/*/*.cxx)
DABC_IMPS += $(wildcard controls/*/*/*.cxx)
DABC_IMPS += $(wildcard plugins/*/*/*.cxx)
DABC_IMPS += $(wildcard applications/*/*.cxx)

DABC_JAVAS = $(wildcard gui/java/generic/src/*.java)


-include $(patsubst %, %/Makefile, $(DABC_PLUGINS))

-include gui/java/Makefile.mk

-include gui/Qt/Makefile.mk

-include doc/Makefile.mk

# APPLICATIONS_DIRS =

libs::

clean::
	
package:: clean
	@echo "Creating package $(DABCTAR_NAME) ..."
	tar cf $(DABCTAR_NAME) README.txt LICENSE.txt RELEASENOTES.txt Makefile base/ build/ config/ controls/simple controls/dimcontrol dim  gui/java $(DABC_PLUGINS_PACK) $(DABC_APPLICATIONS_PACK) $(DABC_SCRIPTS_PACK) --exclude=.svn --exclude=*.bak 
	@mkdir -p $(DISTR_DIR); cd $(DISTR_DIR); mkdir -p $(DABCPACK_VERS)
	@mv $(DABCTAR_NAME) $(DABCDISTR_DIR)
	@cd $(DABCDISTR_DIR); tar xf $(DABCTAR_NAME); rm -f $(DABCTAR_NAME)
	@cp -f script/dabclogin-distribution.sh $(DABCDISTR_DIR)/script/dabclogin.sh
	@cd $(DISTR_DIR); chmod u+w *; chmod u+w */*; chmod u+w */*/*; tar chf $(DABCTAR_NAME) $(DABCPACK_VERS) --exclude=$(DABCTAR_NAME)*; gzip -f $(DABCTAR_NAME)
	@mkdir -p $(PACKAGE_DIR)
	@mv -f $(DISTR_DIR)/$(DABCTAR_NAME).gz $(PACKAGE_DIR)
	@rm -f -r $(DISTR_DIR)/*
	@rmdir $(DISTR_DIR)
	@echo "Package $(DABCTAR_NAME).gz done in $(PACKAGE_DIR)"
	

packageroc:: clean
	@echo "Creating package $(ROCTAR_NAME) ..."
	tar cf $(ROCTAR_NAME)  $(DABC_ROC_PACK) --exclude=.svn --exclude=*.bak 
	@mkdir -p $(DISTR_DIR); cd $(DISTR_DIR); mkdir -p $(ROCPACK_VERS)
	@mv $(ROCTAR_NAME) $(ROCDISTR_DIR)
	@cd $(ROCDISTR_DIR); tar xf $(ROCTAR_NAME); rm -f $(ROCTAR_NAME)
	@cd $(DISTR_DIR); chmod u+w *; chmod u+w */*; chmod u+w */*/*; tar chf $(ROCTAR_NAME) $(ROCPACK_VERS) --exclude=$(ROCTAR_NAME)*; gzip -f $(ROCTAR_NAME)
	@mkdir -p $(PACKAGE_DIR)
	@mv -f $(DISTR_DIR)/$(ROCTAR_NAME).gz $(PACKAGE_DIR)
	@rm -f -r $(DISTR_DIR)/*
	@rmdir $(DISTR_DIR)
	@echo "Package $(ROCTAR_NAME).gz done in $(PACKAGE_DIR)"

packageabb:: clean
	@echo "Creating package $(ABBTAR_NAME) ..."
	tar cf $(ABBTAR_NAME)  $(DABC_ABB_PACK) --exclude=.svn --exclude=*.bak 
	@mkdir -p $(DISTR_DIR); cd $(DISTR_DIR); mkdir -p $(ABBPACK_VERS)
	@mv $(ABBTAR_NAME) $(ABBDISTR_DIR)
	@cd $(ABBDISTR_DIR); tar xf $(ABBTAR_NAME); rm -f $(ABBTAR_NAME)
	@cd $(DISTR_DIR); chmod u+w *; chmod u+w */*; chmod u+w */*/*; tar chf $(ABBTAR_NAME) $(ABBPACK_VERS) --exclude=$(ABBTAR_NAME)*; gzip -f $(ABBTAR_NAME)
	@mkdir -p $(PACKAGE_DIR)
	@mv -f $(DISTR_DIR)/$(ABBTAR_NAME).gz $(PACKAGE_DIR)
	@rm -f -r $(DISTR_DIR)/*
	@rmdir $(DISTR_DIR)
	@echo "Package $(ABBTAR_NAME).gz done in $(PACKAGE_DIR)"
	

addheaders:: clean
	@for FILENAME in $(DABC_HEADERS) $(DABC_IMPS) $(DABC_JAVAS); do echo $$FILENAME; done
#	@for FILENAME in $(DABC_HEADERS) $(DABC_IMPS) $(DABC_JAVAS); do . $(DABCSYS)/build/pack.ksh $$FILENAME; done
	

Dabc_Makefile_rules :=
include config/Makefile.rules
