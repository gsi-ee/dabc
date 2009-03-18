
docs::
	@echo "Making docs ..."
	cd $(DABCSYS)/doc; export TEXINPUTS=$(DABCSYS)/doc//:; export BIBINPUTS=$(DABCSYS)/doc; make;
	cd $(DABCSYS)/doc; export TEXINPUTS=$(DABCSYS)/doc//:; export BIBINPUTS=$(DABCSYS)/doc; ./makedoc user;

clean::
	@echo "Cleaning docs ..."
	@cd $(DABCSYS)/doc; make clean;
