include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
MBSPRINTDIR       = applications/mbsprint/
MBSPRINT_EXE      = $(DABCBINPATH)/mbsprint
else
MBSPRINTDIR       = 
MBSPRINT_EXE      = mbsprint
endif

MBSPRINT_EXES    = $(MBSPRINTDIR)mbsprint.$(SrcSuf)
MBSPRINT_EXEO    = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(MBSPRINT_EXES))
MBSPRINT_EXED    = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(MBSPRINT_EXES))


ALLDEPENDENC += $(MBSPRINT_EXED)

exes::  $(MBSPRINT_EXE)

clean::
	rm -f $(MBSPRINT_EXE)

$(MBSPRINT_EXE) : $(MBSPRINT_EXEO) $(DABCBASE_LIB) $(DABCMBS_LIB)
	$(LD) -Wl,--no-as-needed -O $(MBSPRINT_EXEO) $(LIBS_CORESET) -lDabcMbs -o $(MBSPRINT_EXE)

include $(DABCSYS)/config/Makefile.rules
