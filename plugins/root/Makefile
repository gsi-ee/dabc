ifndef noroot

include $(DABCSYS)/config/Makefile.config

ifdef DABC_ISROOT

ifdef DABCMAINMAKE
DABCROOT_DIR = plugins/root
else
DABCROOT_DIR = .
endif

DABCROOT_DIRI      = $(DABCROOT_DIR)/root
DABCROOT_DIRS      = $(DABCROOT_DIR)/src
DABCROOT_DIRSNIFF  = $(DABCROOT_DIR)/sniffer

ifneq ($(shell if [ $(DABC_ROOTIVERSION) -gt 53422 ] ; then echo gt ; fi),)
DABCROOT_DIRVER    = $(DABCROOT_DIR)/534-23
else
ifneq ($(shell if [ $(DABC_ROOTIVERSION) -gt 53419 ] ; then echo gt ; fi),)
DABCROOT_DIRVER    = $(DABCROOT_DIR)/534-20
else
DABCROOT_DIRVER    = $(DABCROOT_DIR)/534-19
endif
endif

# move it to Makefile.config, while ROOT plugin used in some others plugins
#DABCROOT_LIBNAME  = $(LIB_PREFIX)DabcRoot
#DABCROOT_LIB      = $(TGTDLLPATH)/$(DABCROOT_LIBNAME).$(DllSuf)
#DABCROOT_MAP      = $(TGTDLLPATH)/$(DABCROOT_LIBNAME).rootmap

DABCROOT_TOPINCL = $(DABCROOT_DIR)/DabcRoot.h \
                   $(DABCROOT_DIRI)/TDabcEngine.h

DABCROOT_DICTINCL = $(DABCROOT_TOPINCL)
                    
DABCROOT_LIBDEP   = $(TGTDLLPATH)/libDabcHttp.$(DllSuf) \
                    $(TGTDLLPATH)/libDabcBase.$(DllSuf)

DABCROOT_EXTRALIBS = -L$(TGTDLLPATH) -lDabcBase -lDabcHttp

DABCROOT_EXTRAINC = 

## must be similar for every module

DABCROOT_H        = $(wildcard $(DABCROOT_DIRI)/*.$(HedSuf))
DABCROOT_S        = $(wildcard $(DABCROOT_DIRS)/*.$(SrcSuf))
DABCROOT_O        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCROOT_S))
DABCROOT_D        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCROOT_S))

DABCROOT_DICT      = $(BLD_DIR)/$(DABCROOT_DIR)/G__DABCROOT
DABCROOT_DH        = $(DABCROOT_DICT).$(HedSuf)
DABCROOT_DS        = $(DABCROOT_DICT).$(SrcSuf)
DABCROOT_DO        = $(DABCROOT_DICT).$(ObjSuf) 
DABCROOT_DD        = $(DABCROOT_DICT).$(DepSuf)

DABCROOT_ALL_O     = $(DABCROOT_O)

# used in the main Makefile

ALLHDRS          += $(patsubst $(DABCROOT_DIR)/%.h, $(DABCINCPATH)/%.h, $(DABCROOT_H))
ALLHDRS          += $(patsubst $(DABCROOT_DIR)/%.h, $(DABCINCPATH)/%.h, $(DABCROOT_TOPINCL)) 
ALLDEPENDENC     += $(DABCROOT_D) $(DABCROOT_DD) 


ifeq ($(DABC_ROOT_RHHTP),false)

DABCROOT_LINKDEF  = $(DABCROOT_DIR)/SnifferLinkDef.h

DABCROOT_EXTRAINC = $(DABCROOT_DIRSNIFF) $(DABCROOT_DIRVER)

DABCROOT_SNIFF_H  = $(wildcard $(DABCROOT_DIRSNIFF)/*.$(HedSuf))
DABCROOT_SNIFF_S  = $(wildcard $(DABCROOT_DIRSNIFF)/*.$(SrcSuf))
DABCROOT_SNIFF_O  = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCROOT_SNIFF_S))
DABCROOT_SNIFF_D  = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCROOT_SNIFF_S))

DABCROOT_SNIFF2_H  = $(wildcard $(DABCROOT_DIRVER)/*.$(HedSuf))
DABCROOT_SNIFF2_S  = $(wildcard $(DABCROOT_DIRVER)/*.$(SrcSuf))
DABCROOT_SNIFF2_O  = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCROOT_SNIFF2_S))
DABCROOT_SNIFF2_D  = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCROOT_SNIFF2_S))

DABCROOT_DICTINCL += $(DABCROOT_SNIFF_H) $(DABCROOT_SNIFF2_H)

DABCROOT_ALL_O    += $(DABCROOT_SNIFF_O) $(DABCROOT_SNIFF2_O) 

ALLHDRS           += $(patsubst $(DABCROOT_DIRSNIFF)/%.h, $(DABCINCPATH)/%.h, $(DABCROOT_SNIFF_H)) 
ALLHDRS           += $(patsubst $(DABCROOT_DIRVER)/%.h, $(DABCINCPATH)/%.h, $(DABCROOT_SNIFF2_H)) 
ALLDEPENDENC      += $(DABCROOT_SNIFF_D) $(DABCROOT_SNIFF2_D)


ifdef DABC_FASTCGI
HTTPINCDIR += $(DABC_FASTCGI_INC)
HTTPLIBEXTRA += $(DABC_FASTCGI_LIB)
else
HTTPDEFEXTRA += HTTP_WITHOUT_FASTCGI
endif

HTTPDEFEXTRA += COMPILED_WITH_DABC

HTTPINCDIR += $(DABCROOT_DIR)/../http/civetweb

DABCROOT_LIBDEP += $(DABC_ROOTLIBDIR)/libThread.$(DllSuf) \
                   $(DABC_ROOTLIBDIR)/libTree.$(DllSuf) \
                   $(DABC_ROOTLIBDIR)/libHist.$(DllSuf) \
                   $(DABC_ROOTLIBDIR)/libGpad.$(DllSuf) \
                   $(DABC_ROOTLIBDIR)/libGraf.$(DllSuf) \
                   $(DABC_ROOTLIBDIR)/libRIO.$(DllSuf)
                   
DABCROOT_EXTRALIBS += -L$(DABC_ROOTLIBDIR) -lThread -lTree -lHist -lGpad -lGraf -lRIO                    

# we force inlcudes for sniffer - use only ROOT includes, no any DABC includes 
$(DABCROOT_SNIFF_O) $(DABCROOT_SNIFF_D) $(DABCROOT_SNIFF2_O) $(DABCROOT_SNIFF2_D): INCLUDES = $(DABCROOT_DIRSNIFF) $(HTTPINCDIR) $(DABC_ROOTINCDIR) 

$(DABCROOT_SNIFF_O) $(DABCROOT_SNIFF_D) $(DABCROOT_SNIFF2_O) $(DABCROOT_SNIFF2_D): DEFINITIONS += $(HTTPDEFEXTRA) 

HTTPLIBEXTRA += -lXMLIO

else

# when using ROOT libRHTTP 

DABCROOT_LINKDEF  = $(DABCROOT_DIR)/LinkDef.h

DABCROOT_LIBDEP += $(DABC_ROOTLIBDIR)/libRHTTP.$(DllSuf)

HTTPLIBEXTRA += -lRHTTP -lXMLIO

endif

libs:: $(DABCROOT_LIB) $(DABCROOT_MAP)

##### local rules #####

ifeq ($(DABC_ROOTMAJORVER),6)
HTTPLIBEXTRA += $(DABCROOT_EXTRALIBS)
#$(DABCROOT_DO) : CXXFLAGS = $(shell $(DABC_ROOTBIN)root-config --cflags) -fPIC -I$(DABC_ROOTINCDIR) -I$(DABCROOT_DIR) $(DABCROOT_EXTRAINC:%=-I%) -I.
endif

$(DABCINCPATH)/%.h: $(DABCROOT_DIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

ifeq ($(DABC_ROOT_RHHTP),false)

$(DABCINCPATH)/%.h: $(DABCROOT_DIRSNIFF)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCINCPATH)/%.h: $(DABCROOT_DIRVER)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

endif


$(DABCROOT_LIB): $(DABCROOT_ALL_O) $(DABCROOT_DO)
	$(MakeLib) $(DABCROOT_LIBNAME) "$(DABCROOT_ALL_O) $(DABCROOT_DO)" $(TGTDLLPATH) "$(DABC_ROOTLIBS) $(HTTPLIBEXTRA)"

$(DABCROOT_DS): $(DABCROOT_DICTINCL) $(DABCROOT_LINKDEF)
	$(DABC_ROOTCINT) $(DABCROOT_LIB) "$(DABCROOT_DICTINCL) $(DABCROOT_LINKDEF)"

$(DABCROOT_MAP) : $(DABCROOT_LIB) $(DABCROOT_LINKDEF) 
	$(DABC_ROOTMAP) $(DABCROOT_LIB) "$(DABCROOT_LIBDEP)" $(DABCROOT_DS) $(DABCROOT_LINKDEF) "$(DABCROOT_DICTINCL)"


########### extra rules #############
# $(DABCROOT_O) $(DABCROOT_D): INCLUDES = $(DABCROOT_EXTRAINC) $(DABCROOT_DIR) $(DABC_ROOTINCDIR)  

# $(DABCROOT_DO) $(DABCROOT_DD): INCLUDES = $(DABCROOT_EXTRAINC) $(DABC_ROOTINCDIR) .

# $(DABCROOT_DS): INCLUDES = $(DABCROOT_EXTRAINC) $(DABCROOT_DIRI) 

$(DABCROOT_O) $(DABCROOT_DO) $(DABCROOT_D) $(DABCROOT_DD) : CXXFLAGS += $(DABC_ROOTCFLAGS)

$(DABCROOT_DO) : INCLUDES += .

$(DABCROOT_D) $(DABCROOT_DD) : INCLUDES += $(DABCROOT_EXTRAINC) $(DABCROOT_DIR) .


endif

include $(DABCSYS)/config/Makefile.rules

endif