
include ../gui.mk


GOSIPFRAME_INCPATH=../framework/



.PHONY: all clean set gui  cleanset cleangui install



ifeq ($(QTVERSION),3)

all:
	@echo "No qt4 or greater found, Dofi gui cannot be compiled"

clean:
	@echo "No qt4 or greater found, Dofi gui was not cleaned"
	
install:
	@echo "No qt4 or greater found, Dofi gui was not installed"	

else


all: gui


gui: 
	@echo "Generating Makefile.qt"
	$(QMAKE) DofiGui.pro -o Makefile.qt 
	+$(MAKE) -f Makefile.qt


cleangui:
ifneq ($(wildcard Makefile.qt),)
	make -f Makefile.qt clean
endif
	@rm -f Makefile.qt
	@rm -rf .obj .moc
	@echo "Clean Dofi Gui done"

	
clean: cleangui



install: 
	@echo "Installing to $(DABC_BINPATH) ..."
	@ln -fs $(PWD)/DofiGui $(DABC_BINPATH)
	
endif
