

all: 
	@qmake Configurator.pro -o Makefile.qt
	-@make -f Makefile.qt

clean: 

ifneq ($(wildcard Makefile.qt),)
	@make -f Makefile.qt clean
	-@rm -rf Makefile.qt
endif

