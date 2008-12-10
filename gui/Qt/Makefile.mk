exes::	
	@echo ----------- build of Qt mbs-configurator currently disabled
#ifdef QTDIR
#
#ifneq ($(wildcard $(QTDIR)/bin/qmake),)
#DABCQTGUI  = true
#endif
#
#exes::
#ifdef DABCQTGUI
#	@echo "Compiling Qt gui..."
#	@cd $(DABCSYS)/gui/Qt/mbs-configurator; $(MAKE)
#else
#	@echo QTDIR is set to $(QTDIR), but could not find required Qt dev installation there. Skip Qt gui.
#endif
#
#clean::	
#ifdef DABCQTGUI
#	@echo "Cleaning Qt gui..."
#	@cd $(DABCSYS)/gui/Qt/mbs-configurator; $(MAKE) clean
#endif
#
#endif	