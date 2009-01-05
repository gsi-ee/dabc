
exes::
	@echo "Making docs ..."
	cd $(DABCSYS)/doc; export TEXINPUTS=$(DABCSYS)/doc//:; make;


clean::
	@echo "Cleaning docs ..."
	@cd $(DABCSYS)/doc; make clean; 
	
