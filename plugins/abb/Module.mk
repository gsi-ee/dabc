## normally should be like this for every module, but can be specific

ABBDIR          = abb
ABBDIRI         = $(ABBDIR)/inc
ABBDIRS         = $(ABBDIR)/src
ABBTESTDIR      = $(ABBDIR)/test

ABB_NOTLIBF     =

DABCABB_LIBNAME  = $(LIB_PREFIX)DabcAbb
DABCABB_LIB      = $(DABCDLLPATH)/$(DABCABB_LIBNAME).$(DllSuf)

## must be similar for every module

ABB_DEFINITIONS = $(MPRACE_DEFINITIONS) $(PCIDRIVER_DEFINITIONS)
ABB_INCLUDES    = $(MPRACE_INCLUDES) $(PCIDRIVER_INCLUDES)
ABB_NOTLIBF = 

ABB_H           = $(filter-out $(ABB_NOTLIBF), $(wildcard $(ABBDIRI)/*.$(HedSuf)))
ABB_S           = $(filter-out $(ABB_NOTLIBF), $(wildcard $(ABBDIRS)/*.$(SrcSuf)))
ABB_O           = $(ABB_S:.$(SrcSuf)=.$(ObjSuf))
ABB_DEP         = $(ABB_O:.$(ObjSuf)=.$(DepSuf))

# used in the main Makefile

ALLHDRS +=  $(patsubst $(ABBDIRI)/%.h, $(DABCINCPATH)/%.h, $(ABB_H))

BUILDDABCLIBS      += $(DABCABB_LIB)

ALLDEPENDENC       += $(ABB_DEP) 

DISTRFILES         += $(ABB_S) $(ABB_H) $(ABB_NOTLIBF)
DISTRFILES         += $(ABBTESTDIR)/Makefile $(ABBTESTDIR)/*.cxx

##### local rules #####

$(DABCINCPATH)/%.h: $(ABBDIRI)/%.h
	@cp -f $< $@

$(DABCABB_LIB):   $(ABB_O)
	@$(MakeLib) $(DABCABB_LIBNAME) "$(ABB_O)" $(DABCDLLPATH) "-L$(MPRACE_LIBDIR) $(MPRACE_LIBS) -L$(PCIDRIVER_LIBDIR) $(PCIDRIVER_LIBS)"

all-abb:  $(DABCABB_LIB)

clean-abb:
	@rm -f $(ABB_O) 
	@$(CleanLib) $(DABCABB_LIBNAME) $(DABCDLLPATH)
	@rm -f $(ABB_DEP)

abb-test: 
	@export DABCSYSCORE=$(CURDIR); cd $(ABBTESTDIR); make all

abb-clean-test:
	@export DABCSYSCORE=$(CURDIR); cd $(ABBTESTDIR); make clean


clean:: clean-abb

all:: all-abb

########### extra roles #############
$(ABB_O) $(ABB_DEP): INCLUDES += $(ABB_INCLUDES:%=-I%) 
$(ABB_O) $(ABB_DEP): DEFINITIONS += $(ABB_DEFINITIONS:%=-D%)
