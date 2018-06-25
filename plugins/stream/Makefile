## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

ifndef nostream

include $(DABCSYS)/config/Makefile.config

ifeq ($(DABC_ISSTREAM),true)

ifdef DABCMAINMAKE
DABCSTREAMDIR = plugins/stream
else
DABCSTREAMDIR = .
endif

DABCSTREAMDIRI   = $(DABCSTREAMDIR)/stream
DABCSTREAMDIRS   = $(DABCSTREAMDIR)/src

DABCSTREAM_LIBNAME = $(LIB_PREFIX)DabcStream
DABCSTREAM_LIB     = $(TGTDLLPATH)/$(DABCSTREAM_LIBNAME).$(DllSuf)

DABCSTREAM_STREAMLIB = $(DABC_STREAMSYS)/lib/libStream.$(DllSuf)

DABCSTREAM_H     = $(wildcard $(DABCSTREAMDIRI)/*.$(HedSuf))
DABCSTREAM_S     = $(wildcard $(DABCSTREAMDIRS)/*.$(SrcSuf))
DABCSTREAM_O     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCSTREAM_S))
DABCSTREAM_D     = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCSTREAM_S))

DABCSTREAM_LIBS  = -lDabcMbs -lDabcHadaq -lrt -Wl,-rpath,$(DABC_STREAMSYS)/lib -L$(DABC_STREAMSYS)/lib -lStream

# used in the main Makefile

ALLHDRS           += $(patsubst $(DABCSTREAMDIR)/%.h, $(DABCINCPATH)/%.h, $(DABCSTREAM_H))
ALLDEPENDENC      += $(DABCSTREAM_D)

libs:: $(DABCSTREAM_LIB)

clean::

##### local rules #####

$(DABCINCPATH)/%.h: $(DABCSTREAMDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@


$(DABCSTREAM_LIB):   $(DABCSTREAM_O) $(DABCMBS_LIB) $(DABCHADAQ_LIB)
	@$(MakeLib) $(DABCSTREAM_LIBNAME) "$(DABCSTREAM_O)" $(TGTDLLPATH) "$(DABCSTREAM_LIBS)"

########### extra rules #############
$(DABCSTREAM_O) $(DABCSTREAM_D): INCLUDES += $(DABC_STREAMSYS)/include

#force rebuilding of sources
$(DABCSTREAM_O): $(DABCSTREAM_STREAMLIB) 

include $(DABCSYS)/config/Makefile.rules

endif

endif
