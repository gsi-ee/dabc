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
private xTimer etime;
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
etime = new xTimer(al, false); // fire only once
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
nServers=Integer.parseInt(formDabc.getServers()); // all without DNS
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
//formDabc.printForm();
}
//----------------------------------------
/**
 * Called in xDesktop to rebuild references to DIM services.
 */
public void setDimServices(){
int i;
releaseDimServices();
System.out.println("Dabc setDimServices");
doExit=new Vector<xDimCommand>();
runState=new Vector<xDimParameter>();
Vector<xDimCommand> list=browser.getCommandList();
String pref=new String(DabcName.getText());
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
	if(para.get(i).getParser().getFull().indexOf("/RunStatus")>0) runState.add(para.get(i));
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
runState=null;
}
//----------------------------------------
// Start internal frame with an xState panel through timer.
// Timer events are handled by desktop event handler passed to constructor.
// Note that etime.action calls handler directly. To run handler through
// timer event, use etime.start(). Because the timer is set up to
// fire only once, it stops after that.
private void startProgress(){
progressP=xSet.getLayout("DabcController").getPosition();
xLayout la= new xLayout("progress");
la.set(progressP, new Dimension(300,100),0,true);
progress=new xInternalFrame("Work in progress, please wait", la);
progressState=new xState("Current action:",350,30);
progressState.redraw(-1,"Green","Starting", true);
progressState.setSizeXY();
progress.setupFrame(workIcon, null, progressState, true);
//System.out.println("=== Timer action start");
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
// wait until all runState parameters have the value state.
private boolean waitState(int timeout, String state){
int t=0;
boolean ok;
System.out.println("Wait for "+state);
while(t < timeout){
    ok=true;
    for(int i=0;i<runState.size();i++){
        if(!runState.elementAt(i).getValue().equals(state)) ok=false;
    }
    setProgress(new String("Wait for "+state+" "+t+" ["+timeout+"]"),xSet.blueD());
    if(ok) return true;
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
	cleanupPanel();
	addPromptLines();
	refreshPanel();
	return;
}
//if ("ReadSetup".equals(e.getActionCommand())) {
//int off[]=new int[100],len[]=new int[100],ind,i;
//    String name,header;
//    if(getnew.isSelected()){
//        setup=null;
//        if(titles != null) for(i=0;i<titles.size();i++) desk.removeFrame(titles.get(i));
//        titles=null;
//    }
//    getnew.setSelected(false);
//    if(setup == null){
//        setup=new xSetup();
//        if(!setup.parseSetup(DabcUserpath.getText()+"/"+DabcSetup.getText())) {
//            tellError(this,"Setup parse failed");
//            System.out.println("Setup parse failed: "+DabcUserpath.getText()+"/"+DabcSetup.getText());
//            getnew.setSelected(true);
//            setup=null;
//            titles=null;
//            return;
//        }
//        names=setup.getNames();
//        types=setup.getTypes();
//        values=setup.getValues();
//        int number=setup.getContextNumber();// number of nodes
//        PanelSetupList=new Vector<xPanelSetup>(0);
//        titles=new Vector<String>(0);
//        ind=1;
//        off[0]=0; // first offset is 0
//        for(i=1;i<names.size();i++){
//            if(names.get(i).equals("Context")) {
//                off[ind]=i;
//                len[ind-1]=i-off[ind-1];
//                ind++;
//            }
//        }    
//        len[ind-1]=names.size()-off[ind-1];
//        for(i=0;i<ind;i++){ // create the panels
//            header=new String(values.get(off[i]+1));
//            titles.add(values.get(off[i]));
//            PanelSetupList.add(new xPanelSetup(header,names,types,values,off[i],len[i]));
//        }
//    }
//    System.out.println("Setup parse: "+DabcUserpath.getText()+"/"+DabcSetup.getText());
//    if(titles != null) for(i=0;i<titles.size();i++) desk.removeFrame(titles.get(i));
//    for(i=0;i<PanelSetupList.size();i++){ // display panels
//        name=new String(titles.get(i));
//        xLayout panlo=xSet.getLayout(name); // if not loaded from setup file, create new one
//        panlo=xSet.getLayout(name);
//        if(panlo==null)panlo=xSet.createLayout(name,new Point(100+100*i,200+10*i), new Dimension(100,75),1,true);
//        xInternalCompound ic =new xInternalCompound(name,setupIcon, 0, panlo,null);
//        ic.rebuild(PanelSetupList.get(i)); 
//        desk.addFrame(ic); // old frame will be deleted
//        ic.moveToFront();
//    }
//    desk.toFront("DabcLauncher");
//    return;
//}
//----------------------------------------
else if ("dabcSave".equals(e.getActionCommand())) {
    xLogger.print(1,Action+" "+DabcLaunchFile.getText());
    setLaunch();
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
        etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
        if(!xSet.isSuccess()) {
        	etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Rebuild"));
        	browser.sleep(1);
        }
        if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
        else setProgress("OK: DABC servers ready",xSet.greenD());
    }
    else {
        System.out.println("\nFailed ");
        setProgress("Servers missing: "+(nServers-nserv)+" from "+nServers,xSet.redD());
    }
    }
    else setProgress("Failed: DABC startup script",xSet.redD());
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
    setProgress("DABC cleanup ...",xSet.blueD());
    String cmd = new String(DabcPath.getText()+
                        "/script/dabcshutdown.sh "+DabcPath.getText()+" "+
                        DabcUserpath.getText()+" "+DabcSetup.getText()+" "+
                        xSet.getDimDns()+" "+dabcMaster+" kill &");
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
    etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
    if(!xSet.isSuccess()) {
    	etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Rebuild"));
    }
    if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
    else setProgress("OK: DABC down",xSet.greenD());
 }
else if ("dabcExit".equals(Action)) {
    setProgress("DABC Exit ...",xSet.blueD());
    String cmd = new String(DabcPath.getText()+
                        "/script/dabcshutdown.sh "+DabcPath.getText()+" "+
                        DabcUserpath.getText()+" "+DabcSetup.getText()+" "+
                        xSet.getDimDns()+" "+dabcMaster+" stop &");
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
    etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
    if(!xSet.isSuccess()) {
    	etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Rebuild"));
    }
    if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
    else setProgress("OK: DABC down",xSet.greenD());
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
	System.out.println(xSet.getAccess());
    if(waitState(1,"Ready")) setProgress("DABC already Ready",xSet.greenD());
    else if(waitState(1,"Running")) setProgress("DABC already Running",xSet.greenD());
    else {
    setProgress("Configure DABC ...",xSet.blueD());
    xLogger.print(1,doConfig.getParser().getFull());
    doConfig.exec(xSet.getAccess());
    if(waitState(5,"Configured")){
        setProgress("OK: DABC configured, enable ...",xSet.blueD());
    } else { // retry update
    	setProgress("Retry: DABC configure ...",xSet.redD());
    	System.out.println("Retry configure");
        xSet.setSuccess(false);
        etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Rebuild"));
        if(!xSet.isSuccess()) setProgress("Retry: DABC configure failed",xSet.redD());
        else {
        	if(waitState(5,"Configured"))setProgress("Retry: DABC configured, enable ...",xSet.greenD());
        	else {
                xSet.setSuccess(false);
        		setProgress("Retry: DABC configure failed",xSet.redD());
        	}
        }
    }
    if(xSet.isSuccess()){
	    xLogger.print(1,doEnable.getParser().getFull());
	    doEnable.exec(xSet.getAccess());
	    if(waitState(5,"Ready")){
	        xSet.setSuccess(false);
	        setProgress("OK: DABC enabled, update parameters ...",xSet.blueD());
	        etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
	        if(!xSet.isSuccess()) { // retry
	        	etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Rebuild"));
	        }
	        if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
	        else setProgress("OK: DABC configured and enabled",xSet.greenD());
	    } else { // retry
	    	setProgress("Retry: DABC enable ...",xSet.redD());
	    	System.out.println("Retry enable");
	        xSet.setSuccess(false);
	        etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Rebuild"));
	        if(!xSet.isSuccess()) setProgress("Retry: DABC enable failed",xSet.redD());
	        else {
	        	if(waitState(5,"Ready"))setProgress("Retry: DABC configured and enabled",xSet.greenD());
	        	else setProgress("Retry: DABC enable failed",xSet.redD());
	        }
	    }
    }
    }// state was "halted"
    }
else if ("dabcHalt".equals(Action)) {
    setProgress("Halt DABC ...",xSet.blueD());
    xLogger.print(1,doHalt.getParser().getFull());
    doHalt.exec(xSet.getAccess());
    if(waitState(5,"Halted")){
        setProgress("OK: DABC halted, update parameters ...",xSet.blueD());
        xSet.setSuccess(false);
        etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
        if(!xSet.isSuccess()) {
        	System.out.println("Retry update");
        	etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Rebuild"));
       }
        if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
        else setProgress("OK: DABC halted, servers ready",xSet.greenD());
    } else {
    	setProgress("Retry: DABC halt ...",xSet.redD());
    	System.out.println("Retry halt");
        xSet.setSuccess(false);
        etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Rebuild"));
        if(!xSet.isSuccess()) setProgress("Retry: DABC halt failed",xSet.redD());
        else {
        	if(waitState(5,"Halted"))setProgress("Retry: DABC halted, servers ready",xSet.greenD());
        	else setProgress("Retry: DABC halt failed",xSet.redD());
        }
    }
}
// else if ("dabcEnable".equals(Action)) {
//     xLogger.print(1,doEnable.getParser().getFull());
//     doEnable.exec(xSet.getAccess());
//     waitState(5,"Ready");
//     setProgress("OK: DABC enabled",xSet.greenD());
// }
else if ("dabcStart".equals(Action)) {
    xLogger.print(1,doStart.getParser().getFull());
    doStart.exec(xSet.getAccess());
    if(waitState(5,"Running"))
    	 setProgress("OK: DABC started",xSet.greenD());
    else setProgress("DABC not started",xSet.redD());
}
else if ("dabcStop".equals(Action)) {
    xLogger.print(1,doStop.getParser().getFull());
    doStop.exec(xSet.getAccess());
    if(waitState(5,"Ready"))
         setProgress("OK: DABC stopped",xSet.greenD());
    else setProgress("DABC not stopped",xSet.redD());
}
}
threadRunning=false;
xSet.setProcessing(false);
stopProgress();
System.out.println("Thread done!!!");
}
}



