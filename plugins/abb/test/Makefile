include $(DABCSYS)/config/Makefile.config

INCLUDES+=..
LIBS_FULLSET+=-L../$(DABC_ARCH)/lib -lDabcAbb
LIBS_FULLSET+=-L../linuxdrivers/mprace/lib/ -lmprace
LIBS_FULLSET+=-L../linuxdrivers/pciDriver2/lib/ -lpcidriver 
ABBTEST_EXENAME     = abb_test
ABBTEST_EXEO        = $(BLD_DIR)/$(ABBTEST_EXENAME).$(ObjSuf)
ABBTEST_EXES        = $(ABBTEST_EXENAME).$(SrcSuf)
ABBTEST_EXE         = $(ABBTEST_EXENAME)$(ExeSuf)   
ABBTEST_DEP         =  $(ABBTEST_EXEO:.$(ObjSuf)=.$(DepSuf))



  
ABBTEST_READNAME     = abb_test_read
ABBTEST_READO        = $(BLD_DIR)/$(ABBTEST_READNAME).$(ObjSuf)
ABBTEST_READS        = $(ABBTEST_READNAME).$(SrcSuf)
ABBTEST_READ         = $(ABBTEST_READNAME)$(ExeSuf)   
ABBTEST_READDEP         =  $(ABBTEST_READO:.$(ObjSuf)=.$(DepSuf))


   
ABBTEST_WRITENAME     = abb_test_write
ABBTEST_WRITEO        = $(BLD_DIR)/$(ABBTEST_WRITENAME).$(ObjSuf)
ABBTEST_WRITES        = $(ABBTEST_WRITENAME).$(SrcSuf)
ABBTEST_WRITE         = $(ABBTEST_WRITENAME)$(ExeSuf)   
ABBTEST_WRITEDEP         =  $(ABBTEST_WRITEO:.$(ObjSuf)=.$(DepSuf))



ALLDEPENDENC    += $(ABBTEST_DEP) $(ABBTEST_READDEP) $(ABBTEST_WRITEDEP)

libs::  $(ABBTEST_READ) $(ABBTEST_WRITE) $(ABBTEST_EXE)
$(ABBTEST_READ):   $(ABBTEST_READO) $(wildcard $(DABCDLLPATH)/*.$(DllSuf))
	$(LD) $(LDFLAGS) $(ABBTEST_READO) $(LIBS_FULLSET) $(OutPutOpt) $(ABBTEST_READ)
	@echo "$@  done"    


$(ABBTEST_WRITE):   $(ABBTEST_WRITEO) $(wildcard $(DABCDLLPATH)/*.$(DllSuf))
	$(LD) $(LDFLAGS) $(ABBTEST_WRITEO) $(LIBS_FULLSET) $(OutPutOpt) $(ABBTEST_WRITE)
	@echo "$@  done"    
    

$(ABBTEST_EXE):   $(ABBTEST_EXEO) $(wildcard $(DABCDLLPATH)/*.$(DllSuf))
	$(LD) $(LDFLAGS) $(ABBTEST_EXEO) $(LIBS_FULLSET) $(OutPutOpt) $(ABBTEST_EXE)
	@echo "$@  done"
  


clean:: 
	@rm -f $(ABBTEST_DEP) $(ABBTEST_EXEO) $(ABBTEST_EXE) $(ABBTEST_WRITEDEP) $(ABBTEST_WRITEO) $(ABBTEST_WRITE) $(ABBTEST_READDEP) $(ABBTEST_READO) $(ABBTEST_READ) 

########### extra roles #############
$(ABBTEST_EXEO): INCLUDES += $(MPRACE_INCLUDES:%=-I%)
$(ABBTEST_EXEO): DEFINITIONS += $(MPRACE_DEFINITIONS:%=-D%)
$(ABBTEST_READO): INCLUDES += $(MPRACE_INCLUDES:%=-I%)
$(ABBTEST_READO): DEFINITIONS += $(MPRACE_DEFINITIONS:%=-D%)
$(ABBTEST_WRITEO): INCLUDES += $(MPRACE_INCLUDES:%=-I%)
$(ABBTEST_WRITEO): DEFINITIONS += $(MPRACE_DEFINITIONS:%=-D%)


include $(DABCSYS)/config/Makefile.rules
