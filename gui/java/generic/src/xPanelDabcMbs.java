/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
package xgui;
import javax.swing.JLabel;
import javax.swing.JButton;
import javax.swing.JTextField;
import javax.swing.JPasswordField;
import javax.swing.ImageIcon;
import java.awt.Dimension;
import java.awt.Point;
//import javax.swing.GroupLayout;
import java.io.IOException;
import java.awt.event.*;
import java.awt.Color;
import java.util.*;

/**
 * Form panel to control DABC.
 * @author Hans G. Essel
 * @version 1.0
 * @see xForm
 * @see xFormDabc
 */
public class xPanelDabcMbs extends xPanelPrompt implements ActionListener , Runnable
{
private xRemoteShell mbsshell, dabcshell;
private ImageIcon storeIcon, closeIcon, exitIcon, winIcon, workIcon, dworkIcon, mworkIcon, launchIcon,  killIcon, shrinkIcon, enlargeIcon;
private ImageIcon mbsIcon, configIcon, enableIcon, startIcon, stopIcon, haltIcon, dabcIcon, disIcon, infoIcon;
private JTextField MbsNode, MbsServers, Username, MbsUserpath, MbsPath, MbsStart, MbsShut, MbsCommand, MbsLaunchFile;
private JTextField DimName, DabcNode, DabcServers, DabcName, DabcUserpath, DabcPath, DabcScript, DabcSetup, DabcLaunchFile;
private JPasswordField Password;
private JButton shrinkButton;
private String MbsMaster;
private String dabcMaster;
private String Action;
private xDimBrowser browser;
private xiDesktop desk;
private xInternalFrame progress;
private xFormMbs formMbs;
private xFormDabc formDabc;
private xState progressState;
private ActionListener action;
private xTimer etime;
private int nMbsServers, nMbsNodes;
private int nDabcServers;
private int nServers;
private xDimCommand mbsCommand;
private xDimCommand doConfig, doEnable, doStart, doStop, doHalt;
private Vector<xDimCommand> doExit;
private Vector<xDimParameter> runState;
private Vector<xDimParameter> mbsRunning;
private Vector<xDimParameter> runMode;
private Vector<xDimParameter> mbsTaskList;
private Thread threxe;
private ActionEvent ae;
private boolean threadRunning=false;
private int width=25;
private int mini=25;
private Point progressP;

/**
 * Constructor of MBS+DABC launch panel.
 * @param title Title of window.
 * @param diminfo DIM browser
 * @param desktop Interface to desktop
 * @param al Event handler of desktop. Handles events from xTimer.<br>
 * Passed actions are: Update, DisplayFrame, RemoveFrame.
 * @see xTimer
 */
public xPanelDabcMbs(String title, xDimBrowser diminfo, xiDesktop desktop, ActionListener al) {
super(title);
mbsTaskList=new Vector<xDimParameter>(0);
// get icons
action=al;
desk=desktop;
browser=diminfo;
storeIcon   = xSet.getIcon("icons/savewin.png");
launchIcon  = xSet.getIcon("icons/connprm.png");
killIcon    = xSet.getIcon("icons/exitall.png");
mbsIcon     = xSet.getIcon("icons/conndsp.png");
disIcon     = xSet.getIcon("icons/disconn.png");
infoIcon    = xSet.getIcon("icons/info.png");
dabcIcon    = xSet.getIcon("icons/dabcicon.png");
enableIcon  = xSet.getIcon("icons/dabcenable.png");
winIcon     = xSet.getIcon("icons/rshmbs.png");
workIcon    = xSet.getIcon("icons/control.png");
dworkIcon   = xSet.getIcon("icons/controldabc.png");
mworkIcon   = xSet.getIcon("icons/controlmbs.png");
closeIcon   = xSet.getIcon("icons/fileclose.png");
configIcon  = xSet.getIcon("icons/mbsconfig.png");
startIcon   = xSet.getIcon("icons/mbsstart.png");
stopIcon    = xSet.getIcon("icons/dabcstop.png");
haltIcon    = xSet.getIcon("icons/dabchalt.png");
exitIcon    = xSet.getIcon("icons/exitall.png");
shrinkIcon  = xSet.getIcon("icons/shrink.png");
enlargeIcon = xSet.getIcon("icons/enlarge.png");
// add buttons
//    addButton("mbsQuit","Close window",closeIcon,this);
shrinkButton=addButton("shrink","Minimize/maximize panel",shrinkIcon,enlargeIcon,this);
addButton("Save","Save forms to files",storeIcon,this);
addButton("prmLaunch","Launch DABC and MBS servers",launchIcon,this);
// addButton("mbsLaunch","Launch DABC and MBS dispatcher",mbsIcon,this);
addButton("mbsConfig","Configure MBS and DABC",configIcon,this);
// addButton("dabcEnable","Enable DABC",enableIcon,this);
addButton("mbsStart","Start acquisition",startIcon,this);
addButton("mbsStop","Stop acquisition",stopIcon,this);
addButton("dabcHalt","Stop DABC threads and MBS tasks",haltIcon,this);
addButton("exitCleanup","Shutdown all servers",disIcon,this);
addButton("shellCleanup","Cleanup all by shell",exitIcon,this);
//addButton("mbsCleanup","Reset and cleanup",disIcon,this);
addButton("mbsShow","Show acquisition",infoIcon,this);
addButton("mbsShell","Rsh MbsNode -l Username Command",mworkIcon,this);
addButton("dabcShell","ssh DabcNode -l Username Script",dworkIcon,this);
// Text input fields
if(System.getenv("DABC_CONTROL_MBS")!=null)
    formMbs=new xFormMbs(System.getenv("DABC_CONTROL_MBS"),this);
else formMbs=new xFormMbs("MbsControl.xml",this);
formMbs.addActionListener(this);
Object o=xSet.addObject(formMbs);
if(System.getenv("DABC_CONTROL_DABC")!=null)
    formDabc=new xFormDabc(System.getenv("DABC_CONTROL_DABC"),this);
else formDabc=new xFormDabc("DabcControl.xml",this);
formDabc.addActionListener(this);
o=xSet.addObject(formDabc);

DimName=new JTextField(xSet.getDimDns(),width);
DimName.setEditable(false);
Username=new JTextField(xSet.getUserName(),width);
Username.setEditable(false);
Password=new JPasswordField(width);
Password.setEchoChar('*');
Password.addActionListener(this);
Password.setActionCommand("setpwd");
// add prompt/input fields
addPromptLines();
// read defaults from setup file
nMbsServers=Integer.parseInt(formMbs.getServers()); // all without DNS
nMbsNodes=nMbsServers-1;
nDabcServers=Integer.parseInt(formDabc.getServers()); // all without DNS
nServers=nMbsServers+nDabcServers; // without DNS
System.out.println("Total servers needed: DNS + "+nServers);

mbsshell = new xRemoteShell("rsh");
dabcshell = new xRemoteShell("ssh");
if(xSet.isDabs())checkDir();
String service = new String(MbsNode.getText().toUpperCase()+":PRM");
String servlist = browser.getServers();
if(servlist.indexOf(service)>=0) setProgress("MBS connected",xSet.greenD());
service = new String(MbsNode.getText().toUpperCase()+":DSP");
if(servlist.indexOf(service)>=0) setProgress("MBS connected",xSet.greenD());
setDimServices();
etime = new xTimer(al, false); // fire only once
dabcMaster = DabcNode.getText();
MbsMaster = MbsNode.getText();
}
//----------------------------------------
private void addPromptLines(){
mini=width;
if(formDabc.isShrink()){
	mini=0; // do not show
	shrinkButton.setSelected(true);
}
if(mini > 0)addPrompt("Name server: ",DimName);
if(mini > 0)addPrompt("User name: ",Username);
addPrompt("Password [RET]: ",Password);
MbsNode=addPrompt("MBS master node: ",formMbs.getMaster(),"set",mini,this);
MbsServers=addPrompt("MBS servers: ",formMbs.getServers(),"set",mini,this);
DabcNode=addPrompt("DABC master node: ",formDabc.getMaster(),"set",mini,this);
DabcName=addPrompt("DABC master name: ",formDabc.getName(),"set",mini,this);
DabcServers=addPrompt("DABC servers: ",formDabc.getServers(),"set",mini,this);
MbsPath=addPrompt("MBS system path: ",formMbs.getSystemPath(),"set",mini,this);
MbsUserpath=addPrompt("MBS user path: ",formMbs.getUserPath(),"set",mini,this);
MbsStart=addPrompt("MBS Startup: ",formMbs.getStart(),"set",mini,this);
MbsShut=addPrompt("MBS Shutdown: ",formMbs.getShut(),"set",mini,this);
DabcPath=addPrompt("DABC system path: ",formDabc.getSystemPath(),"set",mini,this);
DabcUserpath=addPrompt("DABC user path: ",formDabc.getUserPath(),"set",mini,this);
DabcSetup=addPrompt("DABC setup file: ",formDabc.getSetup(),"set",mini,this);
MbsLaunchFile=addPrompt("MBS launch file: ",formMbs.getLaunchFile(),"set",mini,this);
DabcLaunchFile=addPrompt("DABC launch file: ",formDabc.getLaunchFile(),"set",mini,this);
MbsCommand=addPrompt("MBS command: ",formMbs.getCommand(),"mbsCommand",width,this);
DabcScript=addPrompt("DABC script: ",formDabc.getScript(),"dabcShell",width,this);
}
//----------------------------------------
private void checkDir(){
String check, result;
nMbsServers=Integer.parseInt(formMbs.getServers());
nMbsNodes=nMbsServers-1;
nDabcServers=Integer.parseInt(formDabc.getServers());
nServers=nMbsServers+nDabcServers; // without DNS
System.out.println("DABC and MBS +++++ check directories");
if(!formDabc.getUserPath().contains("%")){
	check = new String(formDabc.getUserPath()+"/"+formDabc.getSetup());
	result = dabcshell.rshout(formDabc.getMaster(),xSet.getUserName(),"ls "+check);
	if(result.indexOf(formDabc.getSetup()) < 0){
		tellError(xSet.getDesktop(),"Not found: "+check);
		System.out.println("Not found: "+check);
	}
	check = new String(formDabc.getSystemPath()+"/Makefile");
	result = dabcshell.rshout(formDabc.getMaster(),xSet.getUserName(),"ls "+check);
	if(result.indexOf("Makefile") < 0){
		tellError(xSet.getDesktop(),"Not found: "+check);
		System.out.println("Not found: "+check);
	}}
if(!formMbs.getUserPath().contains("%")){
	check = new String(formMbs.getUserPath()+"/"+formMbs.getStart());
    result = mbsshell.rshout(formMbs.getMaster(),xSet.getUserName(),"ls "+check);
    if(result.indexOf(formMbs.getStart()) < 0){
    	tellError(xSet.getDesktop(),"Not found: "+check);
    	System.out.println("Not found: "+check);
    }
    check = new String(formMbs.getUserPath()+"/"+formMbs.getShut());
    result = mbsshell.rshout(formMbs.getMaster(),xSet.getUserName(),"ls "+check);
    if(result.indexOf(formMbs.getShut()) < 0){
    	tellError(xSet.getDesktop(),"Not found: "+check);
    	System.out.println("Not found: "+check);
    }
    check = new String(formMbs.getSystemPath()+"/alias.com");
    result = mbsshell.rshout(formMbs.getMaster(),xSet.getUserName(),"ls "+check);
    if(result.indexOf("alias.com") < 0){
    	tellError(xSet.getDesktop(),"Not found: "+check);
    	System.out.println("Not found: "+check);
    }}
}
//----------------------------------------
private void setLaunch(){
xSet.setAccess(Password.getPassword());
formMbs.setMaster(MbsNode.getText());
formMbs.setServers(MbsServers.getText());
formMbs.setSystemPath(MbsPath.getText());
formMbs.setUserPath(MbsUserpath.getText());
formMbs.setStart(MbsStart.getText());
formMbs.setShut(MbsShut.getText());
formMbs.setLaunchFile(MbsLaunchFile.getText());
formMbs.setCommand(MbsCommand.getText());
//formMbs.printForm();

formDabc.setMaster(DabcNode.getText());
formDabc.setServers(DabcServers.getText());
formDabc.setSystemPath(DabcPath.getText());
formDabc.setUserPath(DabcUserpath.getText());
formDabc.setScript(DabcScript.getText());
formDabc.setLaunchFile(DabcLaunchFile.getText());
formDabc.setName(DabcName.getText());
formDabc.setSetup(DabcSetup.getText());

//formDabc.printForm();
}
//----------------------------------------
// get from command list special commands for buttons.
/**
 * Called in xDesktop to rebuild references to DIM services.
 */
public void setDimServices(){
int i;
releaseDimServices();
System.out.println("DabcMbs setDimServices");
doExit=new Vector<xDimCommand>(0);
runState=new Vector<xDimParameter>(0);
mbsRunning=new Vector<xDimParameter>(0);
runMode=new Vector<xDimParameter>(0);
// Scan command list for some commands
Vector<xDimCommand> list=browser.getCommandList();
for(i=0;i<list.size();i++){
if(list.get(i).getParser().getFull().indexOf("/MbsCommand")>0) {
    mbsCommand=list.get(i);}
    // DABC name is name of controller
else if(list.get(i).getParser().getFull().indexOf(DabcName.getText()+"/DoConfigure")>0) {
    doConfig=list.get(i);}
else if(list.get(i).getParser().getFull().indexOf(DabcName.getText()+"/DoEnable")>0) {
    doEnable=list.get(i);}
else if(list.get(i).getParser().getFull().indexOf(DabcName.getText()+"/DoStart")>0) {
    doStart=list.get(i);}
else if(list.get(i).getParser().getFull().indexOf(DabcName.getText()+"/DoStop")>0) {
    doStop=list.get(i);}
else if(list.get(i).getParser().getFull().indexOf(DabcName.getText()+"/DoHalt")>0) {
    doHalt=list.get(i);}
else if(list.get(i).getParser().getFull().indexOf("/EXIT")>0) {
    doExit.add(list.get(i));}
}
// Scan parameter list
Vector<xDimParameter> para=browser.getParameterList();
if(para != null)for(i=0;i<para.size();i++){
if(para.get(i).getParser().getFull().indexOf("/RunStatus")>0) {
    runState.add(para.get(i));}
else if(para.get(i).getParser().getFull().indexOf("MSG/TaskList")>0) 
	mbsTaskList.add(para.get(i));
else if(para.get(i).getParser().getFull().indexOf("/Acquisition")>0) 
	mbsRunning.add(para.get(i));
else if(para.get(i).getParser().getFull().indexOf("/RunMode")>0) {
    runMode.add(para.get(i));}
}
// if(mbsCommand==null)System.out.println("missing Command MbsCommand");
// if(doConfig==null)System.out.println("missing Command DoConfigure");
// if(doEnable==null)System.out.println("missing Command DoEnable");
// if(doStart==null)System.out.println("missing Command DoStart");
// if(doStop==null)System.out.println("missing Command DoStop");
// if(doHalt==null)System.out.println("missing Command DoHalt");
// if(doExit==null)System.out.println("missing Command doExit");
// if(runState==null)System.out.println("missing Parameter runState");
// if(runMsgLog==null)System.out.println("missing Parameter runMsgLog ");
// if(runRate==null)System.out.println("missing Parameter runRate");
}
//----------------------------------------
/**
 * Called in xDesktop to release references to DIM services.
 */
public void releaseDimServices(){
System.out.println("DabcMbs releaseDimServices");
mbsCommand=null;
doConfig=null;
doEnable=null;
doStart=null;
doStop=null;
doHalt=null;
if(doExit != null) doExit.removeAllElements();
if(mbsTaskList != null) mbsTaskList.removeAllElements();
if(runMode != null) runMode.removeAllElements();
if(runState != null) runState.removeAllElements();
if(mbsRunning != null) mbsRunning.removeAllElements();
doExit=null;
runState=null;
runMode=null;
mbsRunning=null;
}
//----------------------------------------
// Start internal frame with an xState panel through timer.
// Timer events are handled by desktop event handler passed to constructor.
// Note that etime.action calls handler directly. To run handler through
// timer event, use etime.start(). Because the timer is set up to
// fire only once, it stops after that.
private void startProgress(){
progressP=xSet.getLayout("DabcMbsController").getPosition();
xLayout la= new xLayout("progress");
la.set(progressP, new Dimension(300,100),0,true);
progress=new xInternalFrame("Work in progress, please wait", la);
progressState=new xState("Current action:",350,30);
progressState.redraw(-1,"Green","Starting", true);
progressState.setSizeXY();
progress.setupFrame(workIcon, null, progressState, true);
etime.action(new ActionEvent(progress,1,"DisplayFrame"));
}
//----------------------------------------
// Fire event handler of desktop through timer.
private void setProgress(String info, Color color){
setTitle(info,color);
if(threadRunning) progressState.redraw(-1,xSet.blueL(),info, true);
}
//----------------------------------------
// Fire event handler of desktop through timer.
private void stopProgress(){
//etime.setInitialDelay(1000);
//etime.setDelay(1000);
//System.out.println("=== Timer action stop");
// etime.setActionCommand("RemoveFrame"); // Java 6 only
xSet.setActionCommand("RemoveFrame"); // Java < 6 
etime.start(); // fires event with ActionCommand=null, then stop
}
//----------------------------------------
private boolean waitMbs(int timeout, String serv, boolean running){
int t=0,i,n;
boolean ok=false;
if(running){ // wait for task to run
// If not there, wait for parameters to be created,
// otherwise wait that their values match the task names
if(mbsTaskList.size() < nMbsNodes){
    setProgress(new String("Wait for "+nMbsNodes+" MBS Info servers ..."),xSet.blueD());
    System.out.print("Wait for "+nMbsNodes+" MBS Info servers ");
    while(t < timeout){
        String[] xx = browser.getServices("*MSG/TaskList");
        if(xx.length == nMbsNodes) {ok=true; break;}
        System.out.print(".");
        browser.sleep(1);
        t++;
    }
    if(ok){
    	t=0;
	    setProgress(new String("Wait for MBS Command server ..."),xSet.blueD());
	    System.out.print(" Wait for MBS Command server ");
        String server = new String(MbsNode.getText().toUpperCase()+":PRM");
	    while(t < timeout){	    	
	        System.out.print(".");
	        browser.sleep(1);
	        n=browser.getServers().indexOf(server);
	        if(n >= 0) return true;
	        t++;
	    }	    	
    }
    return false;
} // less task lists than nodes
if(mbsTaskList.size() == nMbsNodes){
    setProgress(new String("Wait for "+nMbsNodes+" "+serv+" tasks ..."),xSet.blueD());
    System.out.print("Wait for "+nMbsNodes+" "+serv);
    while(t < timeout){
        n=0;
        for(i=0;i<mbsTaskList.size();i++)
            if(mbsTaskList.elementAt(i).getValue().contains(serv))n++;
        if(n == nMbsNodes) return true;
        System.out.print(".");
        browser.sleep(1);
        t++;
    }} // equal task lists as nodes
    return false;
} // running
else { // check if task is gone
    setProgress(new String("Wait for "+nMbsNodes+" "+serv+" tasks stop ..."),xSet.blueD());
    System.out.print("Wait for "+nMbsNodes+" "+serv+" stop");
    while(t < timeout){
        n=0;
        for(i=0;i<mbsTaskList.size();i++){
            if(mbsTaskList.elementAt(i).getValue().contains(serv))n++;
        }
        if(n == 0) return true;
        System.out.print(".");
        browser.sleep(1);
        t++;
    }} // wait for gone
    return false;
}
//----------------------------------------
// wait until all runState parameters have the value state.
private boolean waitState(int timeout, String state){
int t=0;
boolean ok;
System.out.print("Wait for DABC state "+state);
    while(t < timeout){
    ok=true;
    for(int i=0;i<runState.size();i++){
        if(!runState.get(i).getValue().equals(state)) {ok=false;break;}
    }
        if(ok) return true;
        setProgress(new String("Wait for DABC state "+state+" "+t+" ["+timeout+"]"),xSet.blueD());
        if(t>0)System.out.print(".");
        browser.sleep(1);
        t++;
    }
    return false;
}
//----------------------------------------
// wait until all runMode parameters match mode.
private boolean waitMbsMode(int timeout, String mode){
int t=0;
boolean ok;
System.out.print("Wait for MBS run mode "+mode);
    while(t < timeout){
    ok=true;
    for(int i=0;i<runMode.size();i++){
        if(runMode.get(i).getValue().indexOf(mode) <0) {ok=false;break;}
    }
        if(ok) return true;
        setProgress(new String("Wait for MBS run mode "+mode+" "+t+" ["+timeout+"]"),xSet.blueD());
        System.out.print(".");
        browser.sleep(1);
        t++;
    }
    return false;
}
//----------------------------------------
//wait until all runMode parameters match mode.
private boolean waitRun(int timeout, String mode){
int t=0;
boolean ok;
System.out.print("Wait for acquisition mode "+mode);
    while(t < timeout){
    ok=true;
    for(int i=0;i<mbsRunning.size();i++){
        if(mbsRunning.get(i).getValue().indexOf(mode) <0) {ok=false;break;}
    }
        if(ok) return true;
        setProgress(new String("Wait for MBS acquisition mode "+mode+" "+t+" ["+timeout+"]"),xSet.blueD());
        System.out.print(".");
        browser.sleep(1);
        t++;
    }
    return false;
}
//----------------------------------------
private boolean waitServers(int servers, int time){
int t=0;
while(xSet.getNofServers()!=servers){
	t++;
	browser.sleep(1);
	if(t == time) break;
}
return(!(t==time));
}
//----------------------------------------
private void waitSockets(boolean all){
	int time=0;
	System.out.print("Wait sockets free ");
	String s="WAIT";
	while(s.indexOf("WAIT")>=0){
	    System.out.print(".");
	    browser.sleep(1);
	    time++;
	    setProgress("Wait sockets free "+time+"["+20+"]",xSet.blueD());
	    s=mbsshell.rshout(MbsMaster,Username.getText(),
		"netstat|grep \"\\.600. \"|grep -v \"\\.6.. \"|grep -v 6004|grep WAIT");
	}
	s="WAIT";
	while(s.indexOf("WAIT")>=0){
	    System.out.print(".");
	    browser.sleep(1);
	    time++;
	    setProgress("Wait sockets free "+time+"["+20+"]",xSet.blueD());
	    s=mbsshell.rshout(MbsMaster,Username.getText(),
		"netstat|grep \"\\.61.. \"|grep WAIT");
	}	
}
//----------------------------------------
//React to menu selections.
/**
 * Handle events.
 * @param e Event. Some events are handled directly. Others are handled in a thread. 
 * If an update of DIM parameter list is necessary, Update event is launched through timer
 * and handled by desktop action listener.
 */
public void actionPerformed(ActionEvent e) {
boolean doit=true;
if ("set".equals(e.getActionCommand())) {
setLaunch();
checkDir();
return;
}
if ("setpwd".equals(e.getActionCommand())) {
setLaunch();
return;
}
if ("shrink".equals(e.getActionCommand())) {
	shrinkButton.setSelected(!shrinkButton.isSelected());
	formDabc.setShrink(shrinkButton.isSelected());
	formMbs.setShrink(shrinkButton.isSelected());
	cleanupPanel();
	addPromptLines();
	refreshPanel();
	return;
	}

if ("Save".equals(e.getActionCommand())) {
    xLogger.print(1,Action);
    setLaunch();
    formDabc.saveSetup(DabcLaunchFile.getText());
    formMbs.saveSetup(MbsLaunchFile.getText());
    String msg=new String("Dabc launch: "+DabcLaunchFile.getText()+"\nMbs  launch: "+MbsLaunchFile.getText());
    tellInfo(msg);
    return;
}

if(!threadRunning){
Action = new String(e.getActionCommand());
// must do confirm here, because in thread it would block forever
doit=true;
if ((("exitCleanup".equals(Action))||("shellCleanup".equals(Action))) && (!xSet.isGuru())){
doit=askQuestion("Confirmation","Shut down and cleanup MBS/DABC?");
}
if(doit){
    startProgress();
    ae=e;
    threxe = new Thread(this);
    xSet.setProcessing(true);
    threadRunning=true;
    threxe.start();
}
} else tellError(xSet.getDesktop(),"Execution thread not yet finished!");
}
// start thread by threxe.start()
// CAUTION: Do not use tellInfo or askQuestion here: Thread will never continue!
/**
 * Thread handling events.<br>
 * If an update of DIM parameter list is necessary, Update event is launched through timer
 * and handled by desktop action listener.
 */
public void run(){
int time=0;
    MbsMaster = MbsNode.getText();
    dabcMaster = DabcNode.getText();
    
    if ("mbsQuit".equals(Action)) {
        xLogger.print(1,Action);
    }
    else if ("mbsShell".equals(Action)) {
        setLaunch();
        xLogger.print(1,"Shell: "+MbsCommand.getText());
        //xLogger.print(1,MbsCommand);
        if(mbsshell.rsh(MbsNode.getText(),Username.getText(),MbsCommand.getText(),0L)){}
    }
    else if ("dabcShell".equals(Action)) {
        setLaunch();
        xLogger.print(1,"Shell: "+DabcScript.getText());
        //xLogger.print(1,MbsCommand);
        if(dabcshell.rsh(DabcNode.getText(),Username.getText(),DabcScript.getText(),0L)){}
    }
    else if (("exitCleanup".equals(Action))||("shellCleanup".equals(Action))){
        setProgress("DABC cleanup ...",xSet.blueD());
        String cmd;
    	if ("exitCleanup".equals(Action)) {
            cmd = new String(DabcPath.getText()+
                                "/script/dabcshutdown.sh "+DabcPath.getText()+" "+
                                DabcUserpath.getText()+" "+DabcSetup.getText()+" "+
                                xSet.getDimDns()+" "+dabcMaster+" stop &");
    	}
    	else {
            cmd = new String(DabcPath.getText()+
                                "/script/dabcshutdown.sh "+DabcPath.getText()+" "+
                                DabcUserpath.getText()+" "+DabcSetup.getText()+" "+
                                xSet.getDimDns()+" "+dabcMaster+" kill &");
		}
        xLogger.print(1,cmd);
        dabcshell.rsh(dabcMaster,Username.getText(),cmd,0L);    		
		browser.sleep(2);
        setProgress("Shut down MBS ...",xSet.blueD());
        if(mbsCommand != null) mbsCommand.exec(xSet.getAccess()+" *::exit");
        cmd = new String(MbsPath.getText()+
                                "/script/prmshutdown.sc "+
                                MbsPath.getText()+" "+
                                MbsUserpath.getText());
        //xLogger.print(0,MbsMaster+": "+cmd);
        if(mbsshell.rsh(MbsMaster,Username.getText(),cmd,0L)){
            setProgress("OK: MBS and DABC shut down, update parameters ...",xSet.blueD());
            xSet.setSuccess(false);
            etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);}
            if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
            else setProgress("OK: MBS and DABC servers down",xSet.greenD());
            System.out.println("Run down finished");
        }
        else setProgress("MBS rundown failure!",xSet.redD());
    }
    else if ("prmLaunch".equals(Action)) {
    	if(nServers == xSet.getNofServers())
            setProgress("DABC already launched!",xSet.greenD());
    	else {
        setProgress("Launching DABC ...",xSet.blueD());
        String cmd = new String(DabcPath.getText()+
                                "/script/dabcstartup.sh "+DabcPath.getText()+" "+
                                DabcUserpath.getText()+" "+DabcSetup.getText()+" "+
                                xSet.getDimDns()+" "+dabcMaster+" "+xSet.getAccess()+" &");
        xLogger.print(1,cmd);
        time=0;
        dabcshell.rsh(dabcMaster,Username.getText(),cmd,0L);
        setProgress("Launching MBS ...",xSet.blueD());
        waitSockets(true);
        cmd = new String(MbsPath.getText()+
                                "/script/prmstartup.sc "+MbsPath.getText()+" "+
                                MbsUserpath.getText()+" "+
                                xSet.getDimDns()+" "+xSet.getGuiNode()+" "+xSet.getAccess());
        xLogger.print(0,MbsMaster+": "+cmd);
        boolean launchMbs=false;
        if(mbsshell.rsh(MbsMaster,Username.getText(),cmd,0L)){
        	setProgress("Wait for MBS servers ready ...",xSet.blueD());
        	if(waitServers(nMbsServers,10)){
                System.out.println("\nMbs connnected");
                setProgress("MBS servers ready, update parameters ...",xSet.blueD());
                xSet.setSuccess(false);
                etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
                if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
                browser.sleep(1);}
                if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
                else {
                	setProgress("OK: MBS servers ready",xSet.greenD());
                	launchMbs=true;
                }
                //setDimServices();
            } else {
                System.out.println("Mbs Failed ");
                setProgress("Failed: Launch",xSet.redD());
            }
        } else {
            System.out.println("\nMbs startup script Failed ");
            setProgress("Failed: Launch script",xSet.redD());
            }

        if(launchMbs){
	        time=0;
	        int nserv=0;
	        System.out.print("Dabc wait ");
	        nserv=xSet.getNofServers();
	        setProgress(new String("Wait for "+nServers+" servers ..."),xSet.blueD());
	        while(nserv < nServers){
	            System.out.print(".");
	            browser.sleep(1);
	            time++;
	            if(time > 10) break;
	            nserv=xSet.getNofServers();
	        }
	        if(xSet.getNofServers() >= nServers){
	            System.out.println("\nDabc connnected "+nServers+" servers");
	            setProgress("OK: MBS and DABC servers ready, update parameters ...",xSet.blueD());
	            xSet.setSuccess(false);
	            etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
	            if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
	            browser.sleep(1);}
	            if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
	            else setProgress("OK: MBS and DABC servers ready",xSet.greenD());
	            //setDimServices();        
	        }
	        else {
	            System.out.println("\nDabc Failed ");
	            setProgress("Servers missing: "+(nServers-nserv)+" from "+nServers+", update parameters ...",xSet.redD());
	            xSet.setSuccess(false);
	            etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
	            if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
	            browser.sleep(1);}
	            if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
	            else setProgress("Failure: DABC servers not ready",xSet.redD());
	            //setDimServices();        
	        }
    }}}
    // from here we need commands
    else {
    if(mbsCommand == null) setDimServices();
    if(mbsCommand == null) {
        setProgress("MBS commands not available!",xSet.redD());
        threadRunning=false;
        xSet.setProcessing(false);
        stopProgress();
        System.out.println("Thread done!!!");
        return;
    }
    if(doHalt == null) setDimServices();
    if(doHalt == null) {
        setProgress("DABC doHalt not available!",xSet.redD());
        threadRunning=false;
        xSet.setProcessing(false);
        stopProgress();
        System.out.println("Thread done!!!");
        return;
    }
    if(doEnable == null) setDimServices();
    if(doEnable == null) {
        setProgress("DABC doEnable not available!",xSet.redD());
        threadRunning=false;
        xSet.setProcessing(false);
        stopProgress();
        System.out.println("Thread done!!!");
        return;
    }
    if(doConfig == null) setDimServices();
    if(doConfig == null) {
        setProgress("DABC doConfig not available!",xSet.redD());
        threadRunning=false;
        xSet.setProcessing(false);
        stopProgress();
        System.out.println("Thread done!!!");
        return;
    }
    else if ("mbsConfig".equals(Action)) {
        if(waitState(1,"Ready")) {
            xLogger.print(1,doHalt.getParser().getFull());
            setProgress("Before configure, Halt DABC",xSet.blueD());
            doHalt.exec(xSet.getAccess());
            waitState(10,"Halted");        
        }else if(waitState(1,"Running")){
            xLogger.print(1,doHalt.getParser().getFull());
            setProgress("Before configure, Halt DABC",xSet.blueD());
            doHalt.exec(xSet.getAccess());        	
            waitState(10,"Halted");
        }
        setProgress("Start up and configure MBS tasks",xSet.blueD());
        waitSockets(false); // wait for MBS
        xLogger.print(1,"MBS: @"+MbsStart.getText());
        mbsCommand.exec(xSet.getAccess()+" @"+MbsStart.getText());
        // first wait for MBS
        if(!waitMbs(5+5*nMbsNodes,"Daq_rate",true)){
            System.out.println("\nMbs startup failed ");
            setProgress("MBS configure failed",xSet.redD());
        } else {
            System.out.println(" ");
            xLogger.print(1,"MBS: *::enable dabc");
            mbsCommand.exec(xSet.getAccess()+" *::enable dabc");
            waitMbsMode(5+5*nMbsNodes,"DABC");
            setProgress("Start up and configure DABC tasks",xSet.blueD());
            System.out.println(" ");
            xLogger.print(1,doConfig.getParser().getFull());
            doConfig.exec(xSet.getAccess());
            if(waitState(10,"Configured")){
                System.out.println(" ");
                setProgress("Enable DABC ...",xSet.blueD());
                xLogger.print(1,doEnable.getParser().getFull());
                doEnable.exec(xSet.getAccess());
                if(waitState(10,"Ready")){
                    System.out.println(" ");
                    setProgress("OK: DABC ready, MBS servers ready, update parameters ...",xSet.blueD());
                    xSet.setSuccess(false);
                    etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
                    browser.sleep(1);
                    if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
                    browser.sleep(1);}
                    if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
                    else setProgress("OK: DABC ready",xSet.greenD());
                } else setProgress("Failure: DABC not ready",xSet.redD());
            } else {
                System.out.println("\nDabc configure failed ");
                setProgress("DABC configure failed, update parameters ...",xSet.redD());
                xSet.setSuccess(false);
                etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
                browser.sleep(1);
                if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
                browser.sleep(1);}
                if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
                else setProgress("DABC configure failed",xSet.redD());
                //setDimServices();
            }
        }
    }
//     else if ("dabcEnable".equals(Action)) {
//         xLogger.print(1,doEnable.getParser().getFull());
//         doEnable.exec(xSet.getAccess());
//         setProgress("DABC enabled",xSet.greenD());
//     }
    else if ("mbsStart".equals(Action)) {
        xLogger.print(1,"MBS: *::Start acquisition");
        mbsCommand.exec(xSet.getAccess()+" *::Start acquisition");
        if(waitRun(5+3*nMbsNodes,"Running"))
        setProgress("OK: all running",xSet.greenD());
        else setProgress("LOOK: not all running",xSet.redD());
        xLogger.print(1,doStart.getParser().getFull());
        doStart.exec(xSet.getAccess());
    }
    else if ("mbsStop".equals(Action)) {
        xLogger.print(1,"MBS: *::Stop acquisition");
        mbsCommand.exec(xSet.getAccess()+" *::Stop acquisition");
        if(waitRun(5+5*nMbsNodes,"Stopped"))
        setProgress("OK: all stopped",xSet.greenD());
        else setProgress("LOOK: not all stopped",xSet.redD());
        xLogger.print(1,doStop.getParser().getFull());
        doStop.exec(xSet.getAccess());
    }
    else if ("mbsCommand".equals(Action)) {
        setLaunch();
        xLogger.print(1,MbsCommand.getText());
    	mbsCommand.exec(xSet.getAccess()+" "+MbsCommand.getText());
    	}
   else if ("mbsShow".equals(Action)) {
        xLogger.print(1,"MBS: *::Show acqisition");
        mbsCommand.exec(xSet.getAccess()+" *::Show acquisition");
    }
    else if ("dabcHalt".equals(Action)) {
        xLogger.print(1,doHalt.getParser().getFull());
        setProgress("Halt DABC, shutdown MBS",xSet.blueD());
        doHalt.exec(xSet.getAccess());
        xLogger.print(1,"MBS: @"+MbsShut.getText());
        mbsCommand.exec(xSet.getAccess()+" @"+MbsShut.getText());
        waitMbs(5+5*nMbsNodes,"Daq_rate ",false);
        if(waitState(10,"Halted")){
            System.out.println(" ");
            setProgress("OK: DABC halted, MBS servers ready, update parameters ...",xSet.blueD());
            xSet.setSuccess(false);
            etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);}
            if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
            else setProgress("OK: DABC halted, MBS servers ready",xSet.greenD());
            //setDimServices();
        }else setProgress("Failure: DABC halted, MBS servers ready",xSet.redD());
    }
}
threadRunning=false;
xSet.setProcessing(false);
stopProgress();
System.out.println("Thread done!!!");
}
}



