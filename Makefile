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

DABC_PLUGINS = $(wildcard plugins/*)

DABC_PLUGINS += $(wildcard applications/*)

-include $(patsubst %, %/Makefile, $(DABC_PLUGINS))

-include gui/java/Makefile.mk

#-include gui/Qt/Makefile.mk

-include doc/Makefile.mk

libs::

clean::





# following lines define the packaging:
PACKAGE_DIR    = ./packages
DABCPACK_VERS  = dabc_v$(MAJOR).$(MINOR)
ROCPACK_VERS   = roc_v$(MAJOR).$(MINOR)
ABBPACK_VERS   = abb_v$(MAJOR).$(MINOR)
GUIPACK_VERS   = dabcgui_v$(MAJOR).$(MINOR)
DABCTAR_NAME   = dabc_v$(MAJOR).$(MINOR).tar
ROCTAR_NAME    = roc_v$(MAJOR).$(MINOR).tar
ABBTAR_NAME    = abb_v$(MAJOR).$(MINOR).tar
GUITAR_NAME    = dabcgui_v$(MAJOR).$(MINOR).tar

DISTR_DIR  = ~/dabc_temp_packaging
DABCDISTR_DIR  = $(DISTR_DIR)/$(DABCPACK_VERS)
ROCDISTR_DIR   = $(DISTR_DIR)/$(ROCPACK_VERS)
ABBDISTR_DIR   = $(DISTR_DIR)/$(ABBPACK_VERS)
GUIDISTR_DIR   = $(DISTR_DIR)/$(GUIPACK_VERS)

DABC_PLUGINS_PACK = plugins/mbs plugins/bnet plugins/bnet-mbs plugins/verbs
DABC_APPLICATIONS_PACK = applications/mbs applications/bnet-mbs applications/bnet-test
DABC_SCRIPTS_PACK = script/dabclogin-distribution.sh script/dabcstartup.sh script/dabcshutdown.sh script/gdbcmd.txt

DABC_ROC_PACK = plugins/roc applications/roc 
DABC_ABB_PACK = plugins/abb applications/bnet-test 


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
DABC_JAVA_APPS = $(wildcard gui/java/generic/application/*.java)


package: clean
	@echo "Creating package $(DABCTAR_NAME) ..."
	tar cf $(DABCTAR_NAME) README.txt LICENSE.txt RELEASENOTES.txt  Makefile base/ build/ config/ controls/simple controls/dimcontrol dim  gui/java $(DABC_PLUGINS_PACK) $(DABC_APPLICATIONS_PACK) $(DABC_SCRIPTS_PACK) --exclude=.svn --exclude=*.log --exclude=*.bak 
	@mkdir -p $(DISTR_DIR); cd $(DISTR_DIR); mkdir -p $(DABCPACK_VERS)
	@mv $(DABCTAR_NAME) $(DABCDISTR_DIR)
	@cd $(DABCDISTR_DIR); tar xf $(DABCTAR_NAME); rm -f $(DABCTAR_NAME)
	@mv -f $(DABCDISTR_DIR)/script/dabclogin-distribution.sh $(DABCDISTR_DIR)/script/dabclogin.sh
#	@mv -f $(DABCDISTR_DIR)/doc/main-all.pdf $(DABCDISTR_DIR)/doc/dabcmanual.pdf
	@cd $(DISTR_DIR); chmod u+w *; chmod u+w */*; chmod u+w */*/*; tar chf $(DABCTAR_NAME) $(DABCPACK_VERS) --exclude=$(DABCTAR_NAME)*; gzip -f $(DABCTAR_NAME)
	@mkdir -p $(PACKAGE_DIR)
	@mv -f $(DISTR_DIR)/$(DABCTAR_NAME).gz $(PACKAGE_DIR)
	@rm -f -r $(DISTR_DIR)/*
	@rmdir $(DISTR_DIR)
	@echo "Package $(DABCTAR_NAME).gz done in $(PACKAGE_DIR)"
	

packageroc: clean
	@echo "Creating package $(ROCTAR_NAME) ..."
	@mkdir -p $(DISTR_DIR); cd $(DISTR_DIR); mkdir -p $(ROCPACK_VERS)
	tar cf $(ROCTAR_NAME)  $(DABC_ROC_PACK) --exclude=.svn --exclude=*.bak --exclude=*.log 
	@mv $(ROCTAR_NAME) $(ROCDISTR_DIR)
	@cd $(ROCDISTR_DIR); tar xf $(ROCTAR_NAME); rm -f $(ROCTAR_NAME)
	@cd $(DISTR_DIR); chmod u+w *; chmod u+w */*; chmod u+w */*/*; cd $(ROCPACK_VERS); tar chf $(ROCTAR_NAME) * --exclude=$(ROCTAR_NAME)*; gzip -f $(ROCTAR_NAME)
	@mkdir -p $(PACKAGE_DIR)
	@mv -f $(DISTR_DIR)/$(ROCPACK_VERS)/$(ROCTAR_NAME).gz $(PACKAGE_DIR)
	@rm -f -r $(DISTR_DIR)/*
	@rmdir $(DISTR_DIR)
	@echo "Package $(ROCTAR_NAME).gz done in $(PACKAGE_DIR)"

packageabb: clean
	@echo "Creating package $(ABBTAR_NAME) ..."
	tar cf $(ABBTAR_NAME)  $(DABC_ABB_PACK) --exclude=.svn --exclude=*.bak --exclude=*.log 
	@mkdir -p $(DISTR_DIR); cd $(DISTR_DIR); mkdir -p $(ABBPACK_VERS)
	@mv $(ABBTAR_NAME) $(ABBDISTR_DIR)
	@cd $(ABBDISTR_DIR); tar xf $(ABBTAR_NAME); rm -f $(ABBTAR_NAME)
	@cd $(DISTR_DIR); chmod u+w *; chmod u+w */*; chmod u+w */*/*; cd $(ABBPACK_VERS) ;tar chf $(ABBTAR_NAME) *  --exclude=$(ABBTAR_NAME)*; gzip -f $(ABBTAR_NAME)
	@mkdir -p $(PACKAGE_DIR)
	@mv -f $(DISTR_DIR)/$(ABBPACK_VERS)/$(ABBTAR_NAME).gz $(PACKAGE_DIR)
	@rm -f -r $(DISTR_DIR)/*
	@rmdir $(DISTR_DIR)
	@echo "Package $(ABBTAR_NAME).gz done in $(PACKAGE_DIR)"
	
packagegui: all
	@echo "Creating package $(GUITAR_NAME) ..."
	@mkdir -p $(DISTR_DIR); cd $(DISTR_DIR); mkdir -p $(GUIPACK_VERS)
	cp gui/java/packages/xgui.jar $(GUIDISTR_DIR)
	cp gui/java/generic/application/*.java $(GUIDISTR_DIR) 
	cp gui/java/generic/application/Makefile $(GUIDISTR_DIR) 
	cp gui/java/generic/application/guilogin.sh $(GUIDISTR_DIR) 
	@cd $(GUIDISTR_DIR); chmod u+rw *; chmod a-x *; 
	@cd $(DISTR_DIR); tar chf $(GUITAR_NAME) $(GUIPACK_VERS); gzip -f $(GUITAR_NAME)
	@mkdir -p $(PACKAGE_DIR)
	@mv -f $(DISTR_DIR)/$(GUITAR_NAME).gz $(PACKAGE_DIR)
	@rm -rf $(DISTR_DIR)
	@echo "Package $(GUITAR_NAME).gz done in $(PACKAGE_DIR)"

packages: packagegui package packageroc packageabb src
	
src: clean
	tar chf dabc.tar Makefile *.txt base config build script controls/simple dim controls/dimcontrol --exclude=.svn
	tar rhf dabc.tar plugins applications gui/java --exclude=plugins/abb/linuxdrivers --exclude=.svn
	rm -f dabc.tar.gz 
	gzip dabc.tar
	@mkdir -p $(PACKAGE_DIR); mv -f dabc.tar.gz $(PACKAGE_DIR)
	@echo "Source package dabc.tar.gz done"  
	

addheaders:: clean
	@for FILENAME in $(DABC_HEADERS) $(DABC_IMPS) $(DABC_JAVAS); do echo $$FILENAME; done
#	@for FILENAME in $(DABC_HEADERS) $(DABC_IMPS) $(DABC_JAVAS); do . $(DABCSYS)/build/pack.ksh $$FILENAME; done
	

Dabc_Makefile_rules :=
include config/Makefile.rules
