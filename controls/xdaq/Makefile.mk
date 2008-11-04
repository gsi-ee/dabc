ifdef XDAQ_ROOT


libs::
	cd $(DABCSYS)/controls/xdaq/dabc; $(MAKE)

clean::
	cd $(DABCSYS)/controls/xdaq/dabc/nodecontrol ; rm -rf lib; rm -rf src/*/*/*.o; rm -rf src/*/*/*.d
	

else

libs::
	@echo "XDAQ not installed/configured"


endif
