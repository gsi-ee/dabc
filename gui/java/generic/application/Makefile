# Must set CLASSPATH to xgui.jar and DIM:
#FLAGSA=-d . -classpath xgui.jar:$(DIMDIR)/jdim/classes
  
all: classes

classes: \
	ColorTest.class \
	MiniPanel.class \
	MbsPanel.class \
	MbsEvGen.class \
	RocPanel.class \
	UserPanel.class \
	CbmPanel.class \
	testclient.class \
	testclient
	
ColorTest.class: ColorTest.java
	javac $(FLAGSA) ColorTest.java
	
CbmPanel.class: CbmPanel.java
	javac $(FLAGSA) CbmPanel.java
	
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
	
testclient.class: testclient.java
	javac testclient.java
	
testclient: testclient.cxx
	gcc -g testclient.cxx -o testclient -I$(DIMDIR)/dim -L$(DIMDIR)/linux -ldim -Llinux -lpthread
	
clean:
	rm -f *.class *.o testclient
    