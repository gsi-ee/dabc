ifdef DIMDIR

ifneq ($(shell which javac),)

exes::
	@echo "Compiling java gui..."
	cd $(DABCSYS)/gui/java/generic; export DIMDIR=$(DIMDIR); make 
#	cd $(DABCSYS)/gui/java/generic/application; export DIMDIR=$(DIMDIR); make

docs::
	@echo "Compiling java gui docs..."
	cd $(DABCSYS)/gui/java/generic; export DIMDIR=$(DIMDIR); make doc 

else

exes::
	@echo "Java is not installed on the system"

endif

endif


clean::
	@echo "Cleaning java gui..."
	@cd $(DABCSYS)/gui/java/generic; $(MAKE) clean
#	@cd $(DABCSYS)/gui/java/generic/application; $(MAKE) clean 
	