ifdef XDAQ_ROOT


exes::
	cd $(DABCSYS)/xdaq/dabc; $(MAKE)

clean::
	@echo "clean XDAQ here"

else

exes::
	@echo "XDAQ not installed/configured"


endif
