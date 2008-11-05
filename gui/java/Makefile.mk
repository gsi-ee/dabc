
exes::
	@echo "Compiling java gui..."
	@cd $(DABCSYS)/gui/java/generic; $(MAKE) all;  $(MAKE) jar; 
clean::
	@echo "Cleaning java gui..."
	@cd $(DABCSYS)/gui/java/generic; $(MAKE) clean; 