JARDIR=../packages
PACKAGE=xgui
SOURCEDIR=./src
CLASSPATH=.:./$(PACKAGE):$(DIMDIR)/jdim/classes
JARFILE=$(JARDIR)/$(PACKAGE).jar 


#CLASSPATH=$(PACKAGE):/misc/goofy/cbm/daq/controls/smi/dim_v16r10/jdim/classes
# package directory is created automatically
# One MUST specify where with -d. The package directory MUST NOT specified here
# because the package directory is created under the specified directory, even if it
# IS the package directory
FLAGS=-d . -classpath .:./$(PACKAGE):$(DIMDIR)/jdim/classes
 
all: classes $(JARFILE)

classes: \
	$(PACKAGE)/xForm.class \
	$(PACKAGE)/xFormMbs.class \
	$(PACKAGE)/xFormDabc.class \
	$(PACKAGE)/xTimer.class \
	$(PACKAGE)/xLayout.class \
	$(PACKAGE)/xCrypt.class \
	$(PACKAGE)/xSet.class \
	$(PACKAGE)/xSetup.class \
	$(PACKAGE)/xPanelLogger.class \
	$(PACKAGE)/xiPanelItem.class \
	$(PACKAGE)/xLogger.class \
	$(PACKAGE)/xRemoteShell.class \
	$(PACKAGE)/xConvert.class \
	$(PACKAGE)/xPanelPrompt.class \
	$(PACKAGE)/xPanelSetup.class \
	$(PACKAGE)/xDimNameInfo.class \
	$(PACKAGE)/xInternalFrame.class \
	$(PACKAGE)/xInternalCompound.class \
	$(PACKAGE)/xHisto.class \
	$(PACKAGE)/xRate.class \
	$(PACKAGE)/xMeter.class \
	$(PACKAGE)/xState.class \
	$(PACKAGE)/xInfo.class \
	$(PACKAGE)/xXmlParser.class \
	$(PACKAGE)/xiParser.class \
	$(PACKAGE)/xParser.class \
	$(PACKAGE)/xRecord.class \
	$(PACKAGE)/xRecordMeter.class \
	$(PACKAGE)/xRecordHisto.class \
	$(PACKAGE)/xRecordState.class \
	$(PACKAGE)/xRecordInfo.class \
	$(PACKAGE)/xParTable.class \
	$(PACKAGE)/xiDimCommand.class \
	$(PACKAGE)/xDimCommand.class \
	$(PACKAGE)/xPanelGraphics.class \
	$(PACKAGE)/xPanelState.class \
	$(PACKAGE)/xPanelInfo.class \
	$(PACKAGE)/xPanelMeter.class \
	$(PACKAGE)/xPanelHisto.class \
	$(PACKAGE)/xiDimParameter.class \
	$(PACKAGE)/xiUserInfoHandler.class \
	$(PACKAGE)/xiDimBrowser.class \
	$(PACKAGE)/xiDesktop.class \
	$(PACKAGE)/xDimParameter.class \
	$(PACKAGE)/xDimBrowser.class \
	$(PACKAGE)/xiUserCommand.class \
	$(PACKAGE)/xiUserPanel.class \
	$(PACKAGE)/xPanelDabc.class \
	$(PACKAGE)/xPanelMbs.class \
	$(PACKAGE)/xPanelDabcMbs.class \
	$(PACKAGE)/xPanelCommand.class \
	$(PACKAGE)/xPanelParameter.class \
	$(PACKAGE)/xSaveRestore.class \
	$(PACKAGE)/xPanelSelect.class \
	$(PACKAGE)/xDesktop.class \
	$(PACKAGE)/xGui.class
$(PACKAGE)/xPanelSelect.class : $(SOURCEDIR)/xPanelSelect.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelSelect.java
$(PACKAGE)/xSaveRestore.class : $(SOURCEDIR)/xSaveRestore.java
	javac $(FLAGS) $(SOURCEDIR)/xSaveRestore.java
$(PACKAGE)/xDesktop.class : $(SOURCEDIR)/xDesktop.java
	javac $(FLAGS) $(SOURCEDIR)/xDesktop.java
$(PACKAGE)/xGui.class : $(SOURCEDIR)/xGui.java
	javac $(FLAGS) $(SOURCEDIR)/xGui.java
$(PACKAGE)/xForm.class : $(SOURCEDIR)/xForm.java
	javac $(FLAGS) $(SOURCEDIR)/xForm.java
$(PACKAGE)/xFormMbs.class : $(SOURCEDIR)/xFormMbs.java
	javac $(FLAGS) $(SOURCEDIR)/xFormMbs.java
$(PACKAGE)/xFormDabc.class : $(SOURCEDIR)/xFormDabc.java
	javac $(FLAGS) $(SOURCEDIR)/xFormDabc.java
$(PACKAGE)/xTimer.class : $(SOURCEDIR)/xTimer.java
	javac $(FLAGS) $(SOURCEDIR)/xTimer.java
$(PACKAGE)/xCrypt.class : $(SOURCEDIR)/xCrypt.java
	javac $(FLAGS) $(SOURCEDIR)/xCrypt.java
$(PACKAGE)/xSetup.class : $(SOURCEDIR)/xSetup.java
	javac $(FLAGS) $(SOURCEDIR)/xSetup.java
$(PACKAGE)/xPanelSetup.class : $(SOURCEDIR)/xPanelSetup.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelSetup.java
$(PACKAGE)/xiDesktop.class : $(SOURCEDIR)/xiDesktop.java
	javac $(FLAGS) $(SOURCEDIR)/xiDesktop.java
$(PACKAGE)/xiDimBrowser.class : $(SOURCEDIR)/xiDimBrowser.java
	javac $(FLAGS) $(SOURCEDIR)/xiDimBrowser.java
$(PACKAGE)/xiDimCommand.class : $(SOURCEDIR)/xiDimCommand.java
	javac $(FLAGS) $(SOURCEDIR)/xiDimCommand.java
$(PACKAGE)/xiDimParameter.class : $(SOURCEDIR)/xiDimParameter.java
	javac $(FLAGS) $(SOURCEDIR)/xiDimParameter.java
$(PACKAGE)/xiUserCommand.class : $(SOURCEDIR)/xiUserCommand.java
	javac $(FLAGS) $(SOURCEDIR)/xiUserCommand.java
$(PACKAGE)/xiUserPanel.class : $(SOURCEDIR)/xiUserPanel.java
	javac $(FLAGS) $(SOURCEDIR)/xiUserPanel.java
$(PACKAGE)/xiUserInfoHandler.class : $(SOURCEDIR)/xiUserInfoHandler.java
	javac $(FLAGS) $(SOURCEDIR)/xiUserInfoHandler.java
$(PACKAGE)/xRecord.class : $(SOURCEDIR)/xRecord.java
	javac $(FLAGS) $(SOURCEDIR)/xRecord.java
$(PACKAGE)/xRecordMeter.class : $(SOURCEDIR)/xRecordMeter.java
	javac $(FLAGS) $(SOURCEDIR)/xRecordMeter.java
$(PACKAGE)/xRecordHisto.class : $(SOURCEDIR)/xRecordHisto.java
	javac $(FLAGS) $(SOURCEDIR)/xRecordHisto.java
$(PACKAGE)/xRecordState.class : $(SOURCEDIR)/xRecordState.java
	javac $(FLAGS) $(SOURCEDIR)/xRecordState.java
$(PACKAGE)/xRecordInfo.class : $(SOURCEDIR)/xRecordInfo.java
	javac $(FLAGS) $(SOURCEDIR)/xRecordInfo.java
$(PACKAGE)/xLayout.class : $(SOURCEDIR)/xLayout.java
	javac $(FLAGS) $(SOURCEDIR)/xLayout.java
$(PACKAGE)/xSet.class : $(SOURCEDIR)/xSet.java
	javac $(FLAGS) $(SOURCEDIR)/xSet.java
$(PACKAGE)/xRemoteShell.class : $(SOURCEDIR)/xRemoteShell.java
	javac $(FLAGS) $(SOURCEDIR)/xRemoteShell.java
$(PACKAGE)/xConvert.class : $(SOURCEDIR)/xConvert.java
	javac $(FLAGS) $(SOURCEDIR)/xConvert.java
$(PACKAGE)/xHisto.class : $(SOURCEDIR)/xHisto.java
	javac $(FLAGS) $(SOURCEDIR)/xHisto.java
$(PACKAGE)/xRate.class : $(SOURCEDIR)/xRate.java
	javac $(FLAGS) $(SOURCEDIR)/xRate.java
$(PACKAGE)/xMeter.class : $(SOURCEDIR)/xMeter.java
	javac $(FLAGS) $(SOURCEDIR)/xMeter.java
$(PACKAGE)/xState.class : $(SOURCEDIR)/xState.java
	javac $(FLAGS) $(SOURCEDIR)/xState.java
$(PACKAGE)/xInfo.class : $(SOURCEDIR)/xInfo.java
	javac $(FLAGS) $(SOURCEDIR)/xInfo.java
$(PACKAGE)/xLogger.class : $(SOURCEDIR)/xLogger.java
	javac $(FLAGS) $(SOURCEDIR)/xLogger.java
$(PACKAGE)/xDimCommand.class : $(SOURCEDIR)/xDimCommand.java
	javac $(FLAGS) $(SOURCEDIR)/xDimCommand.java
$(PACKAGE)/xDimParameter.class : $(SOURCEDIR)/xDimParameter.java
	javac $(FLAGS) $(SOURCEDIR)/xDimParameter.java
$(PACKAGE)/xXmlParser.class : $(SOURCEDIR)/xXmlParser.java
	javac $(FLAGS) $(SOURCEDIR)/xXmlParser.java
$(PACKAGE)/xiParser.class : $(SOURCEDIR)/xiParser.java
	javac $(FLAGS) $(SOURCEDIR)/xiParser.java
$(PACKAGE)/xParser.class : $(SOURCEDIR)/xParser.java
	javac $(FLAGS) $(SOURCEDIR)/xParser.java
$(PACKAGE)/xDimBrowser.class : $(SOURCEDIR)/xDimBrowser.java
	javac $(FLAGS) $(SOURCEDIR)/xDimBrowser.java
$(PACKAGE)/xPanelHisto.class : $(SOURCEDIR)/xPanelHisto.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelHisto.java
$(PACKAGE)/xiPanelItem.class : $(SOURCEDIR)/xiPanelItem.java
	javac $(FLAGS) $(SOURCEDIR)/xiPanelItem.java
$(PACKAGE)/xPanelGraphics.class : $(SOURCEDIR)/xPanelGraphics.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelGraphics.java
$(PACKAGE)/xPanelMeter.class : $(SOURCEDIR)/xPanelMeter.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelMeter.java
$(PACKAGE)/xPanelLogger.class : $(SOURCEDIR)/xPanelLogger.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelLogger.java
$(PACKAGE)/xPanelPrompt.class : $(SOURCEDIR)/xPanelPrompt.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelPrompt.java
$(PACKAGE)/xDimNameInfo.class : $(SOURCEDIR)/xDimNameInfo.java
	javac $(FLAGS) $(SOURCEDIR)/xDimNameInfo.java
$(PACKAGE)/xPanelDabc.class : $(SOURCEDIR)/xPanelDabc.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelDabc.java
$(PACKAGE)/xPanelDabcMbs.class : $(SOURCEDIR)/xPanelDabcMbs.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelDabcMbs.java
$(PACKAGE)/xPanelMbs.class : $(SOURCEDIR)/xPanelMbs.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelMbs.java
$(PACKAGE)/xPanelState.class : $(SOURCEDIR)/xPanelState.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelState.java
$(PACKAGE)/xPanelInfo.class : $(SOURCEDIR)/xPanelInfo.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelInfo.java
$(PACKAGE)/xPanelCommand.class : $(SOURCEDIR)/xPanelCommand.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelCommand.java
$(PACKAGE)/xPanelParameter.class : $(SOURCEDIR)/xPanelParameter.java
	javac $(FLAGS) $(SOURCEDIR)/xPanelParameter.java
$(PACKAGE)/xInternalFrame.class : $(SOURCEDIR)/xInternalFrame.java
	javac $(FLAGS) $(SOURCEDIR)/xInternalFrame.java
$(PACKAGE)/xInternalCompound.class : $(SOURCEDIR)/xInternalCompound.java
	javac $(FLAGS) $(SOURCEDIR)/xInternalCompound.java
$(PACKAGE)/xParTable.class : $(SOURCEDIR)/xParTable.java
	javac $(FLAGS) $(SOURCEDIR)/xParTable.java

clean:
	rm -f $(PACKAGE)/*.class
	rm -f $(PACKAGE)/*.html
	rm -f $(JARFILE)
	
cleandoc:
	rm -Rf doc/*
	
jar: $(JARFILE)


$(JARFILE): JarHeader.txt $(wildcard $(PACKAGE)/*.class) $(wildcard icons/*.png)  
	jar -cmf JarHeader.txt $(PACKAGE).jar $(PACKAGE)/*.class icons
#	jar -tf $(PACKAGE).jar
	mv $(PACKAGE).jar $(JARDIR)
	
	
doc: doc/index.html
doc/index.html : \
	 $(SOURCEDIR)/xiDesktop.java \
	 $(SOURCEDIR)/xiDimParameter.java \
	 $(SOURCEDIR)/xiUserPanel.java \
	 $(SOURCEDIR)/xiUserCommand.java \
	 $(SOURCEDIR)/xiUserInfoHandler.java \
	 $(SOURCEDIR)/xiPanelItem.java \
	 $(SOURCEDIR)/xRecord.java \
	 $(SOURCEDIR)/xRecordMeter.java \
	 $(SOURCEDIR)/xRecordState.java \
	 $(SOURCEDIR)/xRecordInfo.java \
	 $(SOURCEDIR)/xHisto.java \
	 $(SOURCEDIR)/xRate.java \
	 $(SOURCEDIR)/xMeter.java \
	 $(SOURCEDIR)/xState.java \
	 $(SOURCEDIR)/xInfo.java \
	 $(SOURCEDIR)/xXmlParser.java \
	 $(SOURCEDIR)/xParser.java \
	 $(SOURCEDIR)/xParTable.java \
	 $(SOURCEDIR)/xLogger.java \
	 $(SOURCEDIR)/xLayout.java \
	 $(SOURCEDIR)/xSet.java \
	 $(SOURCEDIR)/xPanelLogger.java \
	 $(SOURCEDIR)/xPanelPrompt.java \
	 $(SOURCEDIR)/xDimNameInfo.java \
	 $(SOURCEDIR)/xPanelDabc.java \
	 $(SOURCEDIR)/xPanelDabcMbs.java \
	 $(SOURCEDIR)/xPanelMbs.java \
	 $(SOURCEDIR)/xPanelMeter.java \
	 $(SOURCEDIR)/xPanelState.java \
	 $(SOURCEDIR)/xPanelInfo.java \
	 $(SOURCEDIR)/xDimCommand.java \
	 $(SOURCEDIR)/xDimParameter.java \
	 $(SOURCEDIR)/xDimBrowser.java \
	 $(SOURCEDIR)/xPanelCommand.java \
	 $(SOURCEDIR)/xPanelParameter.java \
	 $(SOURCEDIR)/xPanelHisto.java \
	 $(SOURCEDIR)/xPanelState.java \
	 $(SOURCEDIR)/xPanelInfo.java \
	 $(SOURCEDIR)/xPanelGraphics.java \
	 $(SOURCEDIR)/xInternalFrame.java \
	 $(SOURCEDIR)/xInternalCompound.java \
	 $(SOURCEDIR)/xGui.java \
	 $(SOURCEDIR)/xDesktop.java
	javadoc -d doc -author -version -doctitle "DABC Generic Java GUI" $(SOURCEDIR)/x*.java
    javadoc -author -noinherited H.G.Essel -title "DABC Generic Java GUI programming" -doclet org.wonderly.doclets.TexDoclet -docletpath $(JARDIR)/texdoclet.jar -output doc/xgui.tex $(SOURCEDIR)/x*.java
