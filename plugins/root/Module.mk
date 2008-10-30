## normally should be like this for every module, but can be specific

DABCROOTDIR          = root
DABCROOTDIRI         = $(DABCROOTDIR)/inc
DABCROOTDIRS         = $(DABCROOTDIR)/src

DABCROOT_NOTLIBF     =

DABCROOT_LIBNAME  = $(LIB_PREFIX)DabcRoot
DABCROOT_LIB      = $(DABCDLLPATH)/$(DABCROOT_LIBNAME).$(DllSuf)

## must be similar for every module

DABCROOT_H           = $(filter-out $(DABCROOT_NOTLIBF), $(wildcard $(DABCROOTDIRI)/*.$(HedSuf)))
DABCROOT_S           = $(filter-out $(DABCROOT_NOTLIBF), $(wildcard $(DABCROOTDIRS)/*.$(SrcSuf)))
DABCROOT_O           = $(DABCROOT_S:.$(SrcSuf)=.$(ObjSuf))
DABCROOT_DEP         = $(DABCROOT_O:.$(ObjSuf)=.$(DepSuf))

# used in the main Makefile

ALLHDRS +=  $(patsubst $(DABCROOTDIRI)/%.h, $(DABCINCPATH)/%.h, $(DABCROOT_H))

BUILDDABCLIBS      += $(DABCROOT_LIB)

ALLDEPENDENC       += $(DABCROOT_DEP) 

DISTRFILES         += $(DABCROOT_S) $(DABCROOT_H) $(DABCROOT_NOTLIBF)

##### local rules #####

$(DABCINCPATH)/%.h: $(DABCROOTDIRI)/%.h
	@cp -f $< $@

$(DABCROOT_LIB):   $(DABCROOT_O)
	@$(MakeLib) $(DABCROOT_LIBNAME) "$(DABCROOT_O)" $(DABCDLLPATH) "$(ROOT_LIBS)"

all-root:  $(DABCROOT_LIB)

clean-root:
	@rm -f $(DABCROOT_O) 
	@$(CleanLib) $(DABCROOT_LIBNAME) $(DABCDLLPATH)
	@rm -f $(DABCROOT_DEP)

clean:: clean-root

########### extra roles #############
$(DABCROOT_O) $(DABCROOT_DEP): INCLUDES += -I$(ROOTSYS)/include
