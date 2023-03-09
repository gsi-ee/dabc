

include ../gui.mk


.PHONY: all clean gui cleangui install



ifeq ($(QTVERSION),3)

all:
	@echo "No qt4 or greater found, Muppet gui cannot be compiled"

clean:
	@echo "No qt4 or greater found, Muppet gui was not compiled"

install:
	@echo "No qt4 or greater found, Muppet gui was not installeded"

else


all: gui

#nombspex: gui-nombspex

#gui:
#	@echo "Generating Makefile.qt"
#	$(QMAKE) GosipGui.pro -o Makefile.qt "DEFINES+=USE_MBSPEX_LIB" "QMAKE_LFLAGS += -Wl,-rpath,$(PEX_LIBPATH) -Wl,--no-as-needed"  "INCLUDEPATH += $(PEX_INCPATH)" "LIBS+= -L $(PEX_LIBPATH) -lmbspex"
#	+$(MAKE) -f Makefile.qt

gui: 
	@echo "Generating Makefile.qt without DABC libs"
	$(QMAKE) MuppetGui.pro -o Makefile.qt 
	+$(MAKE) -f Makefile.qt


cleangui:
ifneq ($(wildcard Makefile.qt),)
	make -f Makefile.qt clean
endif
	@rm -f Makefile.qt
	@rm -f libMuppetGui.a
	@rm -rf .obj .moc
	@echo "Clean Muppet Gui done"


	
clean: cleangui


install: 
	@echo "Installing to $(DABC_BINPATH) ..."
	@ln -fs $(PWD)/MuppetGui $(DABC_BINPATH)
	
endif
	
	
