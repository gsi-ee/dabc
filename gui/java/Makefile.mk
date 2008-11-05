ifdef DIMDIR

ifneq ($(wildcard $(DIMDIR)),)
USEDIM  = true
endif


exes::
ifdef USEDIM
	@echo "Compiling java gui..."
	@cd $(DABCSYS)/gui/java/generic; $(MAKE) all;  $(MAKE) jar; 
else
	@echo "DIM not set, do not build java gui"
endif

clean::
	@echo "Cleaning java gui..."
	@cd $(DABCSYS)/gui/java/generic; $(MAKE) clean; 
	
endif