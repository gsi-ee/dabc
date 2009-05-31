include $(DABCSYS)/config/Makefile.config

## normally should be like this for every module, but can be specific

ifdef DABCMAINMAKE
ROCDIR = plugins/roc
else
ROCDIR = .
INCLUDES += $(ROCDIR)
endif

ROCDIRI         = $(ROCDIR)/roc
ROCDIRS         = $(ROCDIR)/src
ROCDIRGO4       = $(ROCDIR)/go4monitor
NXYTERDIRI      = $(ROCDIR)/nxyter
NXYTERDIRS      = $(ROCDIR)/src_nxyter

ROC_LINKDEF     = $(ROCDIR)/LinkDef.h
ROC_DICH        = roc/Board.h \
                  roc/UdpBoard.h \
                  roc/defines.h \
                  roc/udpdefines.h \
                  roc/ADCchip.h \
                  roc/Calibrate.h \
                  roc/FEBboard.h \
                  roc/FEBs.h \
                  roc/Peripheral.h \
                  roc/Test.h \
                  nxyter/Data.h \
                  nxyter/Sorter.h \
                  nxyter/Chip.h \
                  nxyter/I2Cbus.h

ROC_LIBNAME     = $(LIB_PREFIX)DabcRoc
ROC_LIB         = $(TGTDLLPATH)/$(ROC_LIBNAME).$(DllSuf)
ROC_LIBMAP      = $(TGTDLLPATH)/$(ROC_LIBNAME).rootmap

## must be similar for every module

ROC_H           = $(wildcard $(ROCDIRI)/*.$(HedSuf))
ROC_S           = $(wildcard $(ROCDIRS)/*.$(SrcSuf))
ROC_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(ROC_S))
ROC_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(ROC_S))

NXYTER_H        = $(wildcard $(NXYTERDIRI)/*.$(HedSuf))
NXYTER_S        = $(wildcard $(NXYTERDIRS)/*.$(SrcSuf))
NXYTER_O        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(NXYTER_S))
NXYTER_D        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(NXYTER_S))

ROC_DS          = $(BLD_DIR)/$(ROCDIR)/G_Roc.$(SrcSuf)

ifdef ROOTSYS
ROC_DO          = $(patsubst %.$(SrcSuf), %.$(ObjSuf), $(ROC_DS))
ROC_DD          = $(patsubst %.$(SrcSuf), %.$(DepSuf), $(ROC_DS))
endif

# used in the main Makefile

ALLHDRS         += $(patsubst $(ROCDIR)/%.h, $(DABCINCPATH)/%.h, $(ROC_H)) \
                   $(patsubst $(ROCDIR)/%.h, $(DABCINCPATH)/%.h, $(NXYTER_H))
                   
ALLDEPENDENC    += $(ROC_D) $(NXYTER_D) $(ROC_DD)

libs:: $(ROC_LIB) 

##### local rules #####

$(DABCINCPATH)/%.h: $(ROCDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(ROC_LIB):  $(ROC_O) $(ROC_DO) $(NXYTER_O)
	@$(MakeLib) $(ROC_LIBNAME) "$(ROC_O) $(ROC_DO) $(NXYTER_O)" $(TGTDLLPATH) 

########### extra rules #############

ifdef ROOTSYS

libs:: $(ROC_LIBMAP)

$(ROC_DS): $(ROC_LINKDEF) $(patsubst %.h, $(ROCDIR)/%.h, $(ROC_DICH))
	echo "Generating root dictionary $(ROC_DS) ..."
	$(ROOTSYS)/bin/rootcint -f $@ -c -p $(INCLUDES:%=-I%) $(ROC_DICH) $(ROC_LINKDEF)

$(ROC_LIBMAP): $(ROC_LINKDEF) $(ROC_LIB)
	@echo 'Building: $@'
	@$(ROOTSYS)/bin/rlibmap -o $@ -l $(ROC_LIB) -d $(DABCDLLPATH)/libDabcMbs.$(DllSuf) $(DABCBASE_LIB) -c $(ROC_LINKDEF)


$(ROC_DO) $(ROC_DD) : INCLUDES += $(ROOTSYS)/include $(DABCINCPATH)
endif


include $(DABCSYS)/config/Makefile.rules
