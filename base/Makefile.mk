## special makefile, cannot be copied out of DABC

DABC_BASEDIR     = base
DABC_BASEDIRI    = $(DABC_BASEDIR)/dabc
DABC_BASEDIRS    = $(DABC_BASEDIR)/src
DABC_BASEDIRRUN  = $(DABC_BASEDIR)/run

DABC_BASEEXE      = $(DABCBINPATH)/dabc_exe
DABC_XMLEXE       = $(DABCBINPATH)/dabc_xml
DABC_BASESH       = $(DABCBINPATH)/dabc_run

BASE_H            = $(wildcard $(DABC_BASEDIRI)/*.$(HedSuf))
BASE_S            = $(wildcard $(DABC_BASEDIRS)/*.$(SrcSuf))
BASE_O            = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BASE_S))
BASE_D            = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BASE_S))


BASE_SOCKET_O     = $(filter $(BLD_DIR)/$(DABC_BASEDIRS)/Socket%, $(BASE_O))  

BASE_CORE_O       = $(filter-out $(BASE_SOCKET_O), $(BASE_O))  

DABC_BASESUB_S    = $(DABC_BASEDIRS)/threads.$(SrcSuf) \
                    $(DABC_BASEDIRS)/timing.$(SrcSuf) \
                    $(DABC_BASEDIRS)/logging.$(SrcSuf) \
                    $(DABC_BASEDIRS)/string.$(SrcSuf) \
                    $(DABC_BASEDIRS)/XmlEngine.$(SrcSuf) \
                    $(DABC_BASEDIRS)/ConfigBase.$(SrcSuf)
DABC_BASESUB_O    = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABC_BASESUB_S))

BASERUN_S         = $(wildcard $(DABC_BASEDIRRUN)/dabc_exe.$(SrcSuf))
BASERUN_O         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BASERUN_S))
BASERUN_D         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BASERUN_S))

DABC_XMLEXES      = $(wildcard $(DABC_BASEDIRRUN)/dabc_xml.$(SrcSuf))
DABC_XMLEXEO      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABC_XMLEXES))
DABC_XMLEXED      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABC_XMLEXES))

BASERUN_SH        = $(DABC_BASEDIRRUN)/dabc_run

######### used in main Makefile

ALLHDRS          += $(DABCINCPATH)/dabc/defines.h $(patsubst $(DABC_BASEDIR)/%.h, $(DABCINCPATH)/%.h, $(BASE_H))
ALLDEPENDENC     += $(BASE_D) $(BASERUN_D) $(DABC_XMLEXED)

libs:: $(DABCBASE_LIB)

exes:: $(DABC_BASEEXE) $(DABC_XMLEXE) $(DABC_BASESH)

##### local rules #####

#$(DABCINCPATH)/dabc:
#	@mkdir -p $@

$(DABCINCPATH)/dabc/defines.h: config/Makefile.config Makefile $(DABCINCPATH)/dabc
	@echo "Producing $@"
	@echo "#ifndef DABC_defines" > $@
	@echo "#define DABC_defines" >> $@
	@echo "" >> $@
	@echo "// Automatically generated file, do not change" >> $@
	@echo "" >> $@
	@echo "   #ifndef DEBUGLEVEL" >> $@
ifdef debug
	@echo "   #define DEBUGLEVEL $(debug)" >> $@
else
	@echo "   #define DEBUGLEVEL 2" >> $@
endif
	@echo "   #endif" >> $@
ifdef extrachecks
	@echo "" >> $@
	@echo "   #define DABC_EXTRA_CHECKS" >> $@
endif
	@echo "" >> $@
	@echo "#endif" >> $@

$(DABCINCPATH)/%.h: $(DABC_BASEDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCBASE_LIB):   $(BASE_CORE_O) $(BASE_SOCKET_O)
	@$(MakeLib) $(DABCBASE_LIBNAME) "$(BASE_CORE_O) $(BASE_SOCKET_O)" $(DABCDLLPATH) "$(LIBS_SYSSET)"

#$(DABCSOCKET_LIB): $(BASE_SOCKET_O)
#	@$(MakeLib) $(DABCSOCKET_LIBNAME) "$(BASE_SOCKET_O)" $(DABCDLLPATH) "-lpthread -ldl $(LIBRT)"
#	@echo sockets: $(BLD_DIR) $(BASE_SOCKET_O)
#	@echo base: $(BASE_O)

#$(DABCBASE_SLIB): $(BASE_O)
#	$(AR) $(ARFLAGS) $(DABCBASE_SLIB) $(BASE_O)

$(DABC_BASEEXE):  $(BASERUN_O) $(DABCBASE_LIB)
	$(LD) -Wl,--no-as-needed -O $(BASERUN_O) $(LIBS_CORESET) -o $(DABC_BASEEXE)

$(DABC_XMLEXE) : $(DABC_XMLEXEO) $(DABC_BASESUB_O) 
	$(LD) -Wl,--no-as-needed -O $(DABC_XMLEXEO) $(DABC_BASESUB_O) -lpthread $(LIBRT) -o $(DABC_XMLEXE)

$(DABC_BASESH): $(BASERUN_SH)
	@echo "Produce $@"
	@cp -f $< $@

$(BASE_D) $(BASERUN_D) $(DABC_XMLEXED) : $(DABCINCPATH)/dabc/defines.h
