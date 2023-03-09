# this is the top level Makefile of the dofi/muppet gui package
# JAM 21-Jun-2017 original taken from mbspex gosipgui packages
# JAM 02-March-2023 begin to convert for dofi commands
#

GUI_FRAME  = framework
GUI_DOFI = dofi

.PHONY: all nodofilib clean install  



all: 
	cd $(GUI_FRAME); make
	cd $(GUI_DOFI); make
	
	 
install: 
	cd $(GUI_DOFI); make install	

clean:
	cd $(GUI_FRAME); make clean
	cd $(GUI_DOFI); make clean	


