#FLAGSA=-d . -classpath xgui.jar:$(DIMDIR)/jdim/classes
  
all: classes

classes: \
	ColorTest.class \
	MiniPanel.class \
	MbsPanel.class \
	MbsEvGen.class \
	RocPanel.class \
	UserPanel.class
	
ColorTest.class: ColorTest.java
	javac $(FLAGSA) ColorTest.java
	
MiniPanel.class: MiniPanel.java
	javac $(FLAGSA) MiniPanel.java
	
MbsPanel.class: MbsPanel.java
	javac $(FLAGSA) MbsPanel.java
	
MbsEvGen.class: MbsEvGen.java
	javac $(FLAGSA) MbsEvGen.java
	
RocPanel.class: RocPanel.java
	javac $(FLAGSA) RocPanel.java
	
UserPanel.class: UserPanel.java
	javac $(FLAGSA) UserPanel.java
	
clean:
	rm -f *.class
    