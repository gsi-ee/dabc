## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
DABCHTTPDIR = plugins/http
else
DABCHTTPDIR = .
INCLUDES += $(DABCHTTPDIR)
endif

DABCHTTPDIRI     = $(DABCHTTPDIR)/http
DABCHTTPDIRS     = $(DABCHTTPDIR)/src
CIVETWEBDIR      = $(DABCHTTPDIR)/civetweb

DABCHTTP_INCDIRS = $(DABCHTTPDIR)
DABCHTTP_EXTRALIBS = -ldl
DABCHTTP_DEFS =

DABCHTTP_LIBNAME = $(LIB_PREFIX)DabcHttp
DABCHTTP_LIB     = $(TGTDLLPATH)/$(DABCHTTP_LIBNAME).$(DllSuf)

DABCHTTP_H       = $(wildcard $(DABCHTTPDIRI)/*.$(HedSuf))
DABCHTTP_S       = $(wildcard $(DABCHTTPDIRS)/*.$(SrcSuf))
DABCHTTP_O       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCHTTP_S))
DABCHTTP_D       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCHTTP_S))

CIVETWEB_CS      = $(CIVETWEBDIR)/civetweb.c
CIVETWEB_CH      = $(CIVETWEBDIR)/civetweb.h 
CIVETWEB_CO      = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(ObjSuf), $(CIVETWEB_CS))
CIVETWEB_CD      = $(patsubst %.$(ObjSuf), %.$(DepSuf), $(CIVETWEB_CO))



ifdef DABC_FASTCGI
DABCHTTP_INCDIRS += $(DABC_FASTCGI_INC)
DABCHTTP_EXTRALIBS += $(DABC_FASTCGI_LIB)
else
DABCHTTP_DEFS += DABC_WITHOUT_FASTCGI
endif


ifdef DABC_ZLIB
DABCHTTP_INCDIRS += $(DABC_ZLIB_INC)
DABCHTTP_EXTRALIBS += $(DABC_ZLIB_LIB)
else
DABCHTTP_DEFS += DABC_WITHOUT_ZLIB
endif


ifdef DABC_SSL
DABCHTTP_DEFS +=   OPENSSL_API_1_1
DABCHTTP_INCDIRS += $(DABC_SSL_INC)
DABCHTTP_EXTRALIBS += $(DABC_SSL_LIB)
else
DABCHTTP_DEFS +=   NO_SSL
endif


# used in the main Makefile

#ALLHDRS           += $(patsubst $(DABCHTTPDIR)/%.h, $(DABCINCPATH)/%.h, $(DABCHTTP_H))
ALLDEPENDENC      += $(DABCHTTP_D) $(CIVETWEB_CD)

libs:: $(DABCHTTP_LIB)

##### local rules #####

#$(DABCINCPATH)/%.h: $(DABCHTTPDIR)/%.h
#	@echo "Header: $@" 
#	@cp -f $< $@


$(DABCHTTP_LIB):   $(DABCHTTP_O) $(CIVETWEB_CO)
	@$(MakeLib) $(DABCHTTP_LIBNAME) "$(DABCHTTP_O) $(CIVETWEB_CO)" $(TGTDLLPATH) "$(DABCHTTP_EXTRALIBS)" 

$(DABCHTTP_O) $(DABCHTTP_D) : INCLUDES += $(DABCHTTP_INCDIRS) 
$(DABCHTTP_O) $(DABCHTTP_D) : DEFINITIONS += $(DABCHTTP_DEFS)
${CIVETWEB_CO} ${CIVETWEB_CD} : C_DEFINITIONS += $(DABCHTTP_DEFS)

include $(DABCSYS)/config/Makefile.rules
