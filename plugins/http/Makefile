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
MONGOOSEDIR      = $(DABCHTTPDIR)/mongoose

DABCHTTP_LIBNAME = $(LIB_PREFIX)DabcHttp
DABCHTTP_LIB     = $(TGTDLLPATH)/$(DABCHTTP_LIBNAME).$(DllSuf)

DABCHTTP_H       = $(wildcard $(DABCHTTPDIRI)/*.$(HedSuf))
DABCHTTP_S       = $(wildcard $(DABCHTTPDIRS)/*.$(SrcSuf))
DABCHTTP_O       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCHTTP_S))
DABCHTTP_D       = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCHTTP_S))

MONGOOSE_CS      = $(MONGOOSEDIR)/mongoose.c
MONGOOSE_CH      = $(MONGOOSEDIR)/mongoose.h 
MONGOOSE_CO      = $(patsubst %.$(CSuf), $(BLD_DIR)/%.$(ObjSuf), $(MONGOOSE_CS))
MONGOOSE_CD      = $(patsubst %.$(ObjSuf), %.$(DepSuf), $(MONGOOSE_CO))

# used in the main Makefile

#ALLHDRS           += $(patsubst $(DABCHTTPDIR)/%.h, $(DABCINCPATH)/%.h, $(DABCHTTP_H))
ALLDEPENDENC      += $(DABCHTTP_D) $(MONGOOSE_CD)

libs:: $(DABCHTTP_LIB)

##### local rules #####

#$(DABCINCPATH)/%.h: $(DABCHTTPDIR)/%.h
#	@echo "Header: $@" 
#	@cp -f $< $@


$(DABCHTTP_LIB): LDFLAGS += -ldl

$(DABCHTTP_LIB):   $(DABCHTTP_O) $(MONGOOSE_CO)
	@$(MakeLib) $(DABCHTTP_LIBNAME) "$(DABCHTTP_O) $(MONGOOSE_CO)" $(TGTDLLPATH) 

$(DABCHTTP_O) $(DABCHTTP_D) : INCLUDES += $(MONGOOSEDIR) $(DABCHTTPDIR) 

include $(DABCSYS)/config/Makefile.rules
