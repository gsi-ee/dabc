ifdef DIMDIR
#ifneq ($(wildcard $(DIMDIR)),)

exes::
	@echo "Compiling java gui..."
	cd $(DABCSYS)/gui/java/generic; export DIMDIR=$(DIMDIR); make -j1 all 

docs::
	@echo "Compiling java gui docs..."
	cd $(DABCSYS)/gui/java/generic; export DIMDIR=$(DIMDIR); make -j1 doc 

#endif

clean::
	@echo "Cleaning java gui..."
	@cd $(DABCSYS)/gui/java/generic; $(MAKE) clean
	
endif
