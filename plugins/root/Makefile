ifndef noroot

include $(DABCSYS)/config/Makefile.config

ifdef DABC_ISROOT

ifdef DABCMAINMAKE
DABCROOTDIR = plugins/root
else
DABCROOTDIR = .
endif

DABCROOTDIRI      = $(DABCROOTDIR)/root
DABCROOTDIRS      = $(DABCROOTDIR)/src
DABCROOTDIRSNIFF  = $(DABCROOTDIR)/sniffer

# move it to Makefile.config, while ROOT plugin used in some others plugins
#DABCROOT_LIBNAME  = $(LIB_PREFIX)DabcRoot
#DABCROOT_LIB      = $(TGTDLLPATH)/$(DABCROOT_LIBNAME).$(DllSuf)
#DABCROOT_MAP      = $(TGTDLLPATH)/$(DABCROOT_LIBNAME).rootmap

DABCROOT_TOPINCL = $(DABCROOTDIR)/DabcRoot.h \
                   $(DABCROOTDIRI)/TDabcEngine.h

DABCROOT_DICTINCL = $(DABCROOT_TOPINCL)
                    
DABCROOT_LIBDEP   = $(TGTDLLPATH)/libDabcHttp.$(DllSuf) \
                    $(TGTDLLPATH)/libDabcBase.$(DllSuf)

DABCROOT_EXTRALIBS = -L$(TGTDLLPATH) -lDabcBase -lDabcHttp

DABCROOT_EXTRAINC = 

## must be similar for every module

DABCROOT_H        = $(wildcard $(DABCROOTDIRI)/*.$(HedSuf))
DABCROOT_S        = $(wildcard $(DABCROOTDIRS)/*.$(SrcSuf))
DABCROOT_O        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCROOT_S))
DABCROOT_D        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCROOT_S))

DABCROOT_DICT      = $(BLD_DIR)/$(DABCROOTDIR)/G__DABCROOT
DABCROOT_DH        = $(DABCROOT_DICT).$(HedSuf)
DABCROOT_DS        = $(DABCROOT_DICT).$(SrcSuf)
DABCROOT_DO        = $(DABCROOT_DICT).$(ObjSuf) 
DABCROOT_DD        = $(DABCROOT_DICT).$(DepSuf)

# used in the main Makefile

ALLHDRS          += $(patsubst $(DABCROOTDIR)/%.h, $(DABCINCPATH)/%.h, $(DABCROOT_H))
ALLHDRS          += $(patsubst $(DABCROOTDIR)/%.h, $(DABCINCPATH)/%.h, $(DABCROOT_TOPINCL)) 
ALLDEPENDENC     += $(DABCROOT_D) $(DABCROOT_DD) 


ifeq ($(DABC_ROOT_RHHTP),false)

DABCROOT_LINKDEF  = $(DABCROOTDIR)/SnifferLinkDef.h

DABCROOT_EXTRAINC = $(DABCROOTDIRSNIFF)

DABCROOT_SNIFF_H  = $(wildcard $(DABCROOTDIRSNIFF)/*.$(HedSuf))
DABCROOT_SNIFF_S  = $(wildcard $(DABCROOTDIRSNIFF)/*.$(SrcSuf))
DABCROOT_SNIFF_O  = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCROOT_SNIFF_S))
DABCROOT_SNIFF_D  = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCROOT_SNIFF_S))

DABCROOT_DICTINCL += $(DABCROOT_SNIFF_H)

DABCROOT_D         += $(DABCROOT_SNIFF_D) 
DABCROOT_O         += $(DABCROOT_SNIFF_O) 

ALLHDRS          += $(patsubst $(DABCROOTDIRSNIFF)/%.h, $(DABCINCPATH)/%.h, $(DABCROOT_SNIFF_H)) 


ifdef DABC_FASTCGI
HTTPINCDIR += $(DABC_FASTCGI_INC)
HTTPLIBEXTRA += $(DABC_FASTCGI_LIB)
else
HTTPDEFEXTRA += HTTP_WITHOUT_FASTCGI
endif

#ifeq ($(DABC_ROOT_ASIMAGE),true)
#DABCROOT_LIBDEP += $(DABC_ROOTLIBDIR)/libASImage.$(DllSuf)
#else
#HTTPDEFEXTRA += HTTP_WITHOUT_ASIMAGE
#endif

HTTPDEFEXTRA += COMPILED_WITH_DABC

HTTPINCDIR += $(DABCROOTDIR)/../http/civetweb

DABCROOT_LIBDEP += $(DABC_ROOTLIBDIR)/libThread.$(DllSuf) \
                   $(DABC_ROOTLIBDIR)/libTree.$(DllSuf) \
                   $(DABC_ROOTLIBDIR)/libHist.$(DllSuf) \
                   $(DABC_ROOTLIBDIR)/libGpad.$(DllSuf) \
                   $(DABC_ROOTLIBDIR)/libGraf.$(DllSuf) \
                   $(DABC_ROOTLIBDIR)/libRIO.$(DllSuf)
                   
DABCROOT_EXTRALIBS += -L$(DABC_ROOTLIBDIR) -lThread -lTree -lHist -lGpad -lGraf -lRIO                    

$(DABCROOT_SNIFF_O) $(DABCROOT_SNIFF_D): INCLUDES += $(HTTPINCDIR) 

$(DABCROOT_SNIFF_O) $(DABCROOT_SNIFF_D): DEFINITIONS += $(HTTPDEFEXTRA) 

HTTPLIBEXTRA += -lXMLIO

else

# when using ROOT libRHTTP 

DABCROOT_LINKDEF  = $(DABCROOTDIR)/LinkDef.h

DABCROOT_LIBDEP += $(DABC_ROOTLIBDIR)/libRHTTP.$(DllSuf)

HTTPLIBEXTRA += -lRHTTP -lXMLIO

endif

libs:: $(DABCROOT_LIB) $(DABCROOT_MAP)

##### local rules #####

ifeq ($(DABC_ROOTVERSION),6)
HTTPLIBEXTRA += DABCROOT_EXTRALIBS
endif

$(DABCINCPATH)/%.h: $(DABCROOTDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCINCPATH)/%.h: $(DABCROOTDIRSNIFF)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCROOT_LIB): $(DABCROOT_O) $(DABCROOT_DO)
	$(MakeLib) $(DABCROOT_LIBNAME) "$(DABCROOT_O) $(DABCROOT_DO)" $(TGTDLLPATH) "$(DABC_ROOTLIBS) $(HTTPLIBEXTRA)"

$(DABCROOT_DS): $(DABCROOT_DICTINCL) $(DABCROOT_LINKDEF)
	$(DABC_ROOTCINT) $(DABCROOT_LIB) "$(DABCROOT_DICTINCL) $(DABCROOT_LINKDEF)"

$(DABCROOT_MAP) : $(DABCROOT_LIB) $(DABCROOT_LINKDEF) 
	$(DABC_ROOTMAP) $(DABCROOT_LIB) "$(DABCROOT_LIBDEP)" $(DABCROOT_DS) $(DABCROOT_LINKDEF) "$(DABCROOT_DICTINCL)"


########### extra roles #############
$(DABCROOT_O) $(DABCROOT_D): INCLUDES += $(DABC_ROOTINCDIR) $(DABCROOTDIR) $(DABCROOT_EXTRAINC)

$(DABCROOT_DO) $(DABCROOT_DD): INCLUDES += $(DABC_ROOTINCDIR) $(DABCROOTDIR) $(DABCROOT_EXTRAINC) .

$(DABCROOT_DS): INCLUDES += $(DABCROOTDIRI) $(DABCROOT_EXTRAINC)

endif

include $(DABCSYS)/config/Makefile.rules

endif