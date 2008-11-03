ifdef XDAQ_ROOT


exes::
	cd $(DABCSYS)/xdaq/dabc; $(MAKE)

else

exes::
	@echo "XDAQ not installed/configured"


endif
