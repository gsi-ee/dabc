
docs::
	@echo "Making docs ..."
	cd $(DABCSYS)/doc; export TEXINPUTS=$(DABCSYS)/doc//:; export BIBINPUTS=$(DABCSYS)/doc; make;


clean::
	@echo "Cleaning docs ..."
	@cd $(DABCSYS)/doc; make clean; 
	
