include $(DABCSYS)/config/Makefile.config

## normally should be like this for every module, but can be specific

ifndef USERSYS
USERSYS=../..
endif

ifdef DABCMAINMAKE
USERDIR = plugins/user
else
USERDIR = .
endif

USER_EXTRALIBS = 

USERDIRI         = $(USERDIR)/user
USERDIRS         = $(USERDIR)/src

USER_LIBNAME     = $(LIB_PREFIX)DabcUser
USER_LIB         = $(TGTDLLPATH)/$(USER_LIBNAME).$(DllSuf)
EXTRA_LIB       = $(EXTRA)/$(USER_LIBNAME).$(DllSuf)


## must be similar for every module

USER_H           = $(wildcard $(USERDIRI)/*.$(HedSuf))
USER_S           = $(wildcard $(USERDIRS)/*.$(SrcSuf))
USER_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(USER_S))
USER_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(USER_S))

# used in the main Makefile

ALLHDRS           += $(patsubst $(USERDIR)/%.h, $(DABCINCPATH)/%.h, $(USER_H))
ALLDEPENDENC    += $(USER_D)

libs:: $(USER_LIB) 

##### local rules #####

$(DABCINCPATH)/%.h: $(USERDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(USER_O) $(USER_D) : INCLUDES += $(USERSYS)/user/include .

$(USER_LIB):  $(USER_O) 
	@$(MakeLib) $(USER_LIBNAME) "$(USER_O)" $(TGTDLLPATH) 


install: $(USER_LIB) 
	@cp -f $(TGTDLLPATH)/$(USER_LIBNAME).$(DllSuf) $(USERSYS)/user/lib/


ifdef EXTRA

libs:: $(EXTRA_LIB)

$(EXTRA_LIB): $(USER_LIB)
	@echo "Extra user lib: $@" 
	@cp -f $< $@	

endif

########### extra rules #############

include $(DABCSYS)/config/Makefile.rules
