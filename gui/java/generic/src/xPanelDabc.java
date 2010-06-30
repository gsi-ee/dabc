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
import javax.swing.JCheckBox;
import javax.swing.ImageIcon;
import java.awt.Dimension;
import java.awt.Point;
//import javax.swing.GroupLayout;

import java.io.IOException;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.awt.Color;

/**
 * Form panel to control DABC.
 * @author Hans G. Essel
 * @version 1.0
 * @see xForm
 * @see xFormDabc
 */
public class xPanelDabc extends xPanelPrompt implements ActionListener, Runnable 
{
private xRemoteShell dabcshell;
private ImageIcon storeIcon, closeIcon, dabcIcon, launchIcon, killIcon, workIcon, dworkIcon, shrinkIcon, enlargeIcon;
private ImageIcon configIcon, enableIcon, startIcon, stopIcon, haltIcon, exitIcon, setupIcon, wcloseIcon;
private JTextField DimName, DabcNode, DabcServers, DabcName, Username, DabcUserpath, DabcPath, DabcScript, DabcSetup, DabcLaunchFile;
private JButton shrinkButton;
private JPasswordField Password;
private JCheckBox getnew;
private xDimBrowser browser;
private xiDesktop desk;
private xInternalFrame progress;
private xState progressState;
private xFormDabc formDabc;
private ActionListener action;
private String dabcMaster;
private String Action;
private int nServers;
private xDimCommand doConfig, doEnable, doStart, doStop, doHalt;
private Vector<xDimParameter> runState;
private Vector<xDimCommand> doExit;
private Thread threxe;
private ActionEvent ae;
private boolean threadRunning=false;
private xSetup setup;
private Vector<xPanelSetup> PanelSetupList;
private Vector<String> names;
private Vector<String> types;
private Vector<String> values;
private Vector<String> titles;
private int width=25;
private int mini=25;
private Point progressP;
private xDesktopAction dAction;

/**
 * Constructor of DABC launch panel.
 * @param title Title of window.
 * @param diminfo DIM browser
 * @param desktop Interface to desktop
 * @param al Event handler of desktop. Handles events from xTimer.<br>
 * Passed actions are: Update, DisplayFrame, RemoveFrame.
 * @see xTimer
 */
public xPanelDabc(String title, xDimBrowser diminfo, xiDesktop desktop, ActionListener al) {
super(title);
action=al;
desk=desktop;
browser=diminfo;
dabcIcon    = xSet.getIcon("icons/DABClogo1small.png");
closeIcon   = xSet.getIcon("icons/fileclose.png");
workIcon    = xSet.getIcon("icons/control.png");
dworkIcon   = xSet.getIcon("icons/controldabc.png");
setupIcon   = xSet.getIcon("icons/usericon.png");
wcloseIcon  = xSet.getIcon("icons/windowclose.png");
storeIcon   = xSet.getIcon("icons/savewin.png");
launchIcon  = xSet.getIcon("icons/connprm.png");
killIcon    = xSet.getIcon("icons/disconn.png");
configIcon  = xSet.getIcon("icons/dabcconfig.png");
enableIcon  = xSet.getIcon("icons/dabcenable.png");
startIcon   = xSet.getIcon("icons/dabcstart.png");
stopIcon    = xSet.getIcon("icons/dabcstop.png");
haltIcon    = xSet.getIcon("icons/dabchalt.png");
exitIcon    = xSet.getIcon("icons/exitall.png");
shrinkIcon  = xSet.getIcon("icons/shrink.png");
enlargeIcon = xSet.getIcon("icons/enlarge.png");
//    addButton("dabcQuit","Close window",closeIcon,this);
//    addButton("ReadSetup","Read setup file from user path",setupIcon,this);
//    addButton("CloseWindows","Close setup windows",wcloseIcon,this);
shrinkButton=addButton("shrink","Minimize/maximize panel",shrinkIcon,enlargeIcon,this);
addButton("dabcSave","Save this form and setup to files",storeIcon,this);
addButton("dabcLaunch","Launch DABC",launchIcon,this);
addButton("dabcConfig","Configure DABC",configIcon,this);
// addButton("dabcEnable","Enable DABC",enableIcon,this);
addButton("dabcStart","Start",startIcon,this);
addButton("dabcStop","Stop",stopIcon,this);
addButton("dabcHalt","Halt",haltIcon,this);
addButton("dabcExit","Exit all DABC tasks",killIcon,this);
addButton("dabcCleanup","Kill all DABC tasks",exitIcon,this);
addButton("dabcShell","ssh Node -l Username Script",dworkIcon,this);
int width=25;
// read dabc setup from file
if(System.getenv("DABC_CONTROL_DABC")!=null)
    formDabc=new xFormDabc(System.getenv("DABC_CONTROL_DABC"));
else formDabc=new xFormDabc("DabcControl.xml");
formDabc.addActionListener(this);
Object o=xSet.addObject(formDabc);
// formDabc=(xFormDabc)xSet.getObject("xgui.xFormDabc"); // how to retrieve
DimName=new JTextField(xSet.getDimDns(),width);
DimName.setEditable(false);
Username=new JTextField(xSet.getUserName(),width);
Username.setEditable(false);
Password=new JPasswordField(width);
Password.setEchoChar('*');
Password.addActionListener(this);
Password.setActionCommand("setpwd");
addPromptLines();
// Add checkboxes
//    getnew = new JCheckBox();
//    getnew.setSelected(true);
//    addCheckBox("Get new setup",getnew);
dabcshell = new xRemoteShell("ssh");
if(xSet.isDabc())checkDir();
nServers=Integer.parseInt(formDabc.getServers()); // all without DNS
setDimServices();
System.out.println("Dabc  servers needed: DNS + "+nServers);
dAction = new xDesktopAction(desktop);
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
DabcNode=addPrompt("Master node: ",formDabc.getMaster(),"set",mini,this);
DabcName=addPrompt("Master name: ",formDabc.getName(),"set",mini,this);
DabcServers=addPrompt("Servers: ",formDabc.getServers(),"set",mini,this);
DabcPath=addPrompt("System path: ",formDabc.getSystemPath(),"set",mini,this);
DabcUserpath=addPrompt("User path: ",formDabc.getUserPath(),"set",mini,this);
DabcSetup=addPrompt("Setup file: ",formDabc.getSetup(),"set",mini,this);
DabcLaunchFile=addPrompt("Launch file: ",formDabc.getLaunchFile(),"set",mini,this);
DabcScript=addPrompt("Script: ",formDabc.getScript(),"dabcScript",width,this);
}
//----------------------------------------
private void checkDir(){
String check, result;
System.out.println("DABC +++++ check directories");
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
	}
}}
//----------------------------------------
private void setLaunch(){
xSet.setAccess(Password.getPassword());
formDabc.setMaster(DabcNode.getText());
formDabc.setServers(DabcServers.getText());
formDabc.setSystemPath(DabcPath.getText());
formDabc.setUserPath(DabcUserpath.getText());
formDabc.setScript(DabcScript.getText());
formDabc.setLaunchFile(DabcLaunchFile.getText());
formDabc.setName(DabcName.getText());
formDabc.setSetup(DabcSetup.getText());
nServers=Integer.parseInt(formDabc.getServers()); // all without DNS
//formDabc.printForm();
}
//----------------------------------------
/**
 * Called in xDesktop to rebuild references to DIM services.
 */
public void setDimServices(){
int i;
if(doExit != null)releaseDimServices();
System.out.println("Dabc setDimServices");
doExit=new Vector<xDimCommand>();
runState=new Vector<xDimParameter>();
Vector<xDimCommand> list=browser.getCommandList();
String pref=new String(DabcName.getText()); // controller name
for(i=0;i<list.size();i++){
	if(list.get(i).getParser().getFull().indexOf(pref+"/DoConfigure")>0) doConfig=list.get(i);
	else if(list.get(i).getParser().getFull().indexOf(pref+"/DoEnable")>0) doEnable=list.get(i);
	else if(list.get(i).getParser().getFull().indexOf(pref+"/DoStart")>0) doStart=list.get(i);
	else if(list.get(i).getParser().getFull().indexOf(pref+"/DoStop")>0) doStop=list.get(i);
	else if(list.get(i).getParser().getFull().indexOf(pref+"/DoHalt")>0) doHalt=list.get(i);
	else if(list.get(i).getParser().getFull().indexOf("/EXIT")>0) doExit.add(list.get(i));
}
Vector<xDimParameter> para=browser.getParameterList();
if(para != null)for(i=0;i<para.size();i++){
	// select state of master controller on master node
	if(para.get(i).getParser().getFull().indexOf(pref+"/RunStatus")>0) {
		if(para.get(i).getParser().getFull().contains(formDabc.getMaster())){
			runState.add(para.get(i));
		}}
}}
//----------------------------------------
/**
 * Called in xDesktop to release references to DIM services.
 */
public void releaseDimServices(){
System.out.println("Dabc releaseDimServices");
doConfig=null;
doEnable=null;
doStart=null;
doStop=null;
doHalt=null;
if(doExit != null) doExit.removeAllElements();
if(runState != null) runState.removeAllElements();
doExit=null;
//runState=null;
}
//----------------------------------------
// Called in actionPerformed: Start internal frame directly.
private void startProgress(){
progressP=xSet.getLayout("DabcController").getPosition();
xLayout la= new xLayout("progress");
la.set(progressP, new Dimension(300,100),0,true);
progress=new xInternalFrame("Work in progress, please wait", la);
progressState=new xState("Current action:",350,30);
progressState.redraw(-1,"Green","Starting", true);
progressState.setSizeXY();
progress.setupFrame(workIcon, null, progressState, true);
desk.addFrame(progress, false);
}
//----------------------------------------
private void setProgress(String info, Color color){
setTitle(info,color);
if(threadRunning) progressState.redraw(-1,xSet.blueL(),info, true);
}
//----------------------------------------
// Called in this thread: Remove frame in event thread.
private void stopProgress(){
dAction.execute("RemoveFrame",progress.getTitle()); // execute in event thread
while(dAction.isRunning())browser.sleep(1);
}
//----------------------------------------
private void shutdown(String mode){
    setProgress("DABC cleanup ...",xSet.blueD());
    String cmd = new String(DabcPath.getText()+
                        "/script/dabcshutdown.sh "+DabcPath.getText()+" "+
                        DabcUserpath.getText()+" "+DabcSetup.getText()+" "+
                        xSet.getDimDns()+" "+dabcMaster+" "+mode+" &");
    xLogger.print(1,cmd);
    dabcshell.rsh(dabcMaster,Username.getText(),cmd,0L);
    int nserv=xSet.getNofServers();
    int timeout=0;
    while(nserv > 0){
        setProgress(new String("Wait "+timeout+" [10] for server shutdown, found "+nserv),xSet.blueD());
        System.out.print("."+nserv);
        browser.sleep(1);
        timeout++;
        if(timeout > 10) break;
        nserv=xSet.getNofServers();
    }
    setProgress("OK: DABC down, update parameters ...",xSet.blueD());
    xSet.setSuccess(false);
    dAction.execute("Update"); // execute in event thread
    while(dAction.isRunning())browser.sleep(1);
    if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
    else setProgress("OK: DABC down",xSet.greenD());

}
//----------------------------------------
// wait until all runState parameters have the value state.
private boolean waitState(int timeout, String state){
int t=0;
int statesOK=0;
System.out.println("Wait for "+state);
while(t <= timeout){
    statesOK=0;
    for(int i=0;i<runState.size();i++){
        if(runState.elementAt(i).getValue().equals(state)) statesOK++;
    }
    if(statesOK==runState.size()) return true;
    if(t == timeout) return(statesOK==runState.size());
    setProgress(new String("Wait for "+state+" "+t+" ["+timeout+"]"),xSet.blueD());
    if(t>0)System.out.print(".");
    browser.sleep(1);
    t++;
}
return false;
}
//----------------------------------------
//React to menu selections.
/**
 * Handle events.
 * @param e Event. Some events are handled directly. Others are handled in a thread. 
 * If an update of DIM parameter list is necessary, Update event is launched through timer
 * and handled by desktop action listener.<p>
 * "ReadSetup":<br>
 * Creates a new xSetup object, read Xdaq XML setup file, get references to name/type/value lists.
 * Create for each context a xPanelSetup passing the list references, and display in a separate frame. <br>
 * "DabcSave":<br>
 * Save content of form to file and contents of context panels to XML file. 
 */
public void actionPerformed(ActionEvent e) {
boolean doit=true;
setLaunch();
if ("set".equals(e.getActionCommand())) {
	checkDir();
	return;
}
if ("setpwd".equals(e.getActionCommand())) {
	return;
}
if ("shrink".equals(e.getActionCommand())) {
	shrinkButton.setSelected(!shrinkButton.isSelected());
	formDabc.setShrink(shrinkButton.isSelected());
	cleanupPanel();
	addPromptLines();
	refreshPanel();
	return;
}
//----------------------------------------
if ("dabcSave".equals(e.getActionCommand())) {
    xLogger.print(1,Action+" "+DabcLaunchFile.getText());
    formDabc.saveSetup(DabcLaunchFile.getText());
    String msg=new String("Dabc launch: "+DabcLaunchFile.getText());
    if(setup != null){
        for(int i=0;i<PanelSetupList.size();i++)PanelSetupList.get(i).updateList();
        if(!setup.updateSetup()) tellError(xSet.getDesktop(),"Setup update failed");
        else { 
        if(!setup.writeSetup(DabcUserpath.getText()+"/"+DabcSetup.getText())) tellError(xSet.getDesktop(),"Write setup failed");
        else {
            String mes=new String(msg+"\nDabc setup: "+DabcUserpath.getText()+"/"+DabcSetup.getText());
            tellInfo(mes);
        }
        }
    } else  tellInfo(msg);
    return;
}
//else if ("CloseWindows".equals(e.getActionCommand())) {
//    if(titles != null) for(int i=0;i<titles.size();i++) desk.removeFrame(titles.get(i));
//    return;
//}
    
if(!threadRunning){
    Action = new String(e.getActionCommand());
    // must do confirm here, because in thread it would block forever
    doit=true;
    if (("dabcExit".equals(Action)) && (!xSet.isGuru())) doit=askQuestion("Confirmation","Exit, shut down and cleanup DABC?");
    if (("dabcCleanup".equals(Action)) && (!xSet.isGuru())) doit=askQuestion("Confirmation","Kill DABC tasks?");
    if(doit){
        startProgress();
        ae=e;
        threxe = new Thread(this);
        threadRunning=true;
        xSet.setProcessing(true);
        threxe.start();
    }
    } else tellError(xSet.getDesktop(),"Execution thread not yet finished!");
return;
}
// start thread by threxe.start()
// CAUTION: Do not use tellInfo or askQuestion here: Thread will never continue!
/**
 * Thread handling events.<br>
 * If an update of DIM parameter list is necessary, Update event is launched through timer
 * and handled by desktop action listener.
 */
public void run(){
dabcMaster = DabcNode.getText();

if ("dabcQuit".equals(Action)) {
    xLogger.print(1,Action);
}
else if ("dabcLaunch".equals(Action)) {
	if(nServers == xSet.getNofServers()) // without DNS
        setProgress("DABC already launched!",xSet.greenD());
	else {
    // to avoid unneccessary automatic updates when new services are created:
    xSet.setAutoUpdate(false); // must be enabled after we made the update
    setProgress("Launch DABC servers ...",xSet.blueD());
    String cmd = new String(DabcPath.getText()+
                            "/script/dabcstartup.sh "+DabcPath.getText()+" "+
                            DabcUserpath.getText()+" "+DabcSetup.getText()+" "+
                            xSet.getDimDns()+" "+dabcMaster+" "+xSet.getAccess()+" &");
    xLogger.print(1,cmd);
    int timeout=0;
    int nserv=0;
    if(dabcshell.rsh(dabcMaster,Username.getText(),cmd,0L)){
        System.out.print("Dabc wait "+nServers);
        nserv=xSet.getNofServers();
        while(nserv < nServers){
            setProgress(new String("Wait "+timeout+" [10] for "+nServers+" servers, found "+nserv),xSet.blueD());
            System.out.print("."+nserv);
            browser.sleep(1);
            timeout++;
            if(timeout > 10) break;
            nserv=xSet.getNofServers();
        }
    if(nserv >= nServers){
        System.out.println("\nDabc connnected "+nServers+" servers");
        setProgress("OK: DABC servers ready, update parameters ...",xSet.blueD());
        xSet.setSuccess(false);
        dAction.execute("Update"); // execute in event thread
        while(dAction.isRunning())browser.sleep(1);
        if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
        else setProgress("OK: DABC servers ready",xSet.greenD());
    }
    else {
        System.out.println("\nFailed ");
        setProgress("Servers missing: "+(nServers-nserv)+" from "+nServers,xSet.redD());
    }
    }
    else setProgress("Failed: DABC startup script",xSet.redD());
    xSet.setAutoUpdate(true);
}}
else if ("dabcScript".equals(Action)) {
    //xLogger.print(1,Command);
	setLaunch();
    xLogger.print(1,"dabcShell: "+DabcScript.getText());
    dabcshell.rsh(DabcNode.getText(),Username.getText(),DabcScript.getText(),0L);
}
else if ("dabcShell".equals(Action)) {
    //xLogger.print(1,Command);
    xLogger.print(1,"dabcShell: "+DabcScript.getText());
    dabcshell.rsh(DabcNode.getText(),Username.getText(),DabcScript.getText(),0L);
}
else if ("dabcCleanup".equals(Action)) {
	shutdown("kill");
 }
else if ("dabcExit".equals(Action)) {
	shutdown("stop");
}
else {
if(doHalt == null) setDimServices();
if(doHalt == null) {
    setProgress("No DABC commands available!",xSet.redD());
    threadRunning=false;
    xSet.setProcessing(false);
    stopProgress();
    System.out.println("Thread done!!!");
    return;
}
if ("dabcConfig".equals(Action)) {
	System.out.println(doConfig.getParser().getFull()+" Check state");
    if(waitState(0,"Ready")) setProgress("DABC already Ready",xSet.greenD());
    else if(waitState(0,"Running")) setProgress("DABC already Running",xSet.greenD());
    else {
    setProgress("Configure DABC ...",xSet.blueD());
    xLogger.print(1,doConfig.getParser().getFull());
    // to avoid unneccessary automatic updates when new services are created:
    xSet.setAutoUpdate(false); // must be enabled after we made the update
    doConfig.exec(xSet.getAccess());
    //browser.sleep(xSet.getNofServers());
    if(waitState(5,"Configured")){
        setProgress("OK: DABC configured, enable ...",xSet.blueD());
    } else { 
                xSet.setSuccess(false);
        		setProgress("DABC configure failed",xSet.redD());
    }
    if(xSet.isSuccess()){
	    xLogger.print(1,doEnable.getParser().getFull());
	    doEnable.exec(xSet.getAccess());
	    if(waitState(5,"Ready")){
	        xSet.setSuccess(false);
	        setProgress("OK: DABC enabled, update parameters ...",xSet.blueD());
	        dAction.execute("Update"); // execute in event thread
	        while(dAction.isRunning())browser.sleep(1);
	        if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
	        else setProgress("OK: DABC configured and enabled",xSet.greenD());
	    } else { 
	    	setProgress("DABC enable failed",xSet.redD());
	    }
    }
    }// state was "halted"
    xSet.setAutoUpdate(true); 
    }
else if ("dabcHalt".equals(Action)) {
    setProgress("Halt DABC ...",xSet.blueD());
    xLogger.print(1,doHalt.getParser().getFull());
    doHalt.exec(xSet.getAccess());
    if(waitState(5,"Halted")){
    	setProgress("OK: DABC halted, servers ready",xSet.greenD());
    } else {
    	setProgress("Retry: DABC halt failed",xSet.redD());
    }
}
else if ("dabcStart".equals(Action)) {
    xLogger.print(1,doStart.getParser().getFull());
    doStart.exec(xSet.getAccess());
    if(waitState(5,"Running"))
    	 setProgress("OK: DABC started",xSet.greenD());
    else {
    	setProgress("DABC not started",xSet.redD());
    }
}
else if ("dabcStop".equals(Action)) {
    xLogger.print(1,doStop.getParser().getFull());
    doStop.exec(xSet.getAccess());
    if(waitState(5,"Ready"))
         setProgress("OK: DABC stopped",xSet.greenD());
    else {
    	System.out.println("Probably not stopped");
    	setProgress("DABC not stopped",xSet.redD());
    }
}
}
threadRunning=false;
xSet.setProcessing(false);
stopProgress();
System.out.println("Thread done!!!");
}
}



