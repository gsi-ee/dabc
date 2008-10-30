# User makefile

all:
	@echo "Building all DABC packages"
ifneq ($(wildcard core),)
	@echo "Build core package"
	cd core; make all
endif 
ifneq ($(wildcard xdaq/dabc),)
	@echo "Build xdaq package"
	cd xdaq/dabc; make
endif 
ifneq ($(wildcard gui/java),)
	@echo "Build java gui package"
	cd gui/java/generic; make
endif 


unpack:
#ifeq ($(DABCSYS),)
#$(error DABCSYS not defined)
#endif
ifneq ($(wildcard dabc_core.tar.gz),)
	@echo "Unpack core package"
	mkdir core
	cd core; tar xzf ../dabc_core.tar.gz
endif 
ifneq ($(wildcard dabc_xdaq.tar.gz),)
	@echo "Unpack xdaq package"
	tar xzf dabc_xdaq.tar.gz
endif 
ifneq ($(wildcard dabc_gui.tar.gz),)
	@echo "Unpack gui package"
	tar xzf dabc_gui.tar.gz
endif 
	@echo "Unpack all DABC packages done"


package:
	@echo "Building set of DABC packages"
	mkdir arc; rm -rf arc/*
	cd core; make package
	mv -f core/dabc.tar.gz arc/dabc_core.tar.gz
	@echo "Core package done"
	tar chf dabc_xdaq.tar xdaq/config/Makefile.rules
	tar rhf dabc_xdaq.tar xdaq/dabc/Makefile
	tar rhf dabc_xdaq.tar xdaq/dabc/nodecontrol/Makefile
	tar rhf dabc_xdaq.tar xdaq/dabc/nodecontrol/include/*.h
	tar rhf dabc_xdaq.tar xdaq/dabc/nodecontrol/src/common/*.cc
	tar rhf dabc_xdaq.tar --exclude=*.proj xdaq/dabc/nodecontrol/xml
	tar rhf dabc_xdaq.tar script/*.sh
	gzip dabc_xdaq.tar
	mv -f dabc_xdaq.tar.gz arc
	@echo "Xdaq package done"
	tar chf dabc_gui.tar gui/java/generic/Makefile
	tar rhf dabc_gui.tar gui/java/generic/*.java
	tar rhf dabc_gui.tar gui/java/generic/*.txt
	tar rhf dabc_gui.tar gui/java/generic/src/*.java
	tar rhf dabc_gui.tar --exclude=*.proj gui/java/generic/icons
	tar rhf dabc_gui.tar --exclude=*.proj gui/java/generic/config
	tar rhf dabc_gui.tar gui/java/packages/*.jar
	gzip dabc_gui.tar
	mv -f dabc_gui.tar.gz arc
	@echo "Gui package done"
	cp -f Makefile arc
	cp -f dabclogin.sh arc/dabclogin
	@echo "DABC packages collected in ./arc directory"
	