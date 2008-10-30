package xgui;
/**
* @author goofy
*/
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
import java.io.*;
import org.w3c.dom.*;
import org.xml.sax.*;
import javax.xml.parsers.*;
/**
* DIM GUI class
*/
public class xPanelDabcMbs extends xPanelPrompt implements ActionListener , Runnable
{
private xRemoteShell mbsshell, dabcshell;
private ImageIcon storeIcon, closeIcon, winIcon, workIcon, launchIcon,  killIcon;
private ImageIcon mbsIcon, configIcon, enableIcon, startIcon, stopIcon, haltIcon, dabcIcon, disIcon, infoIcon;
private JTextField MbsNode, MbsServers, Username, MbsUserpath, MbsPath, MbsScript, MbsCommand, MbsLaunchFile;
private JTextField DimName, DabcNode, DabcServers, DabcName, DabcUserpath, DabcPath, DabcScript, DabcSetup, DabcLaunchFile;
private JPasswordField Password;
private String MbsMaster;
private String dabcMaster;
private String Action;
private xDimBrowser browser;
private xiDesktop desk;
private xInternalFrame progress;
private xFormMbs setupMbs;
private xFormDabc setupDabc;
private xState progressState;
private ActionListener action;
private xTimer etime;
private DocumentBuilderFactory factory;
private DocumentBuilder builder;
private Document document;
private Element root, elem;
private int nMbsServers;
private int nDabcServers;
private int nServers;
private xDimCommand mbsCommand;
private xDimCommand doConfig, doEnable, doStart, doStop, doHalt;
private Vector<Object> doExit;
private Vector<xDimParameter> runState;
private Vector<xDimParameter> runMsgLog;
private Vector<xDimParameter> runRate;
private Vector<xDimParameter> runMode;
private Thread threxe;
private ActionEvent ae;
private boolean threadRunning=false;

public xPanelDabcMbs(String title, xDimBrowser diminfo, xiDesktop desktop, ActionListener al) {
    super(title);
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
    closeIcon   = xSet.getIcon("icons/fileclose.png");
    configIcon  = xSet.getIcon("icons/mbsconfig.png");
    startIcon   = xSet.getIcon("icons/mbsstart.png");
    stopIcon    = xSet.getIcon("icons/dabcstop.png");
    haltIcon    = xSet.getIcon("icons/dabchalt.png");
    // add buttons
    addButton("mbsQuit","Close window",closeIcon,this);
    addButton("Save","Save forms to files",storeIcon,this);
    addButton("prmLaunch","Launch DABC and MBS servers",launchIcon,this);
    // addButton("mbsLaunch","Launch DABC and MBS dispatcher",mbsIcon,this);
    addButton("mbsConfig","Configure MBS and DABC",configIcon,this);
    // addButton("dabcEnable","Enable DABC",enableIcon,this);
    addButton("mbsStart","Start acquisition",startIcon,this);
    addButton("mbsStop","Stop acquisition",stopIcon,this);
    addButton("dabcHalt","Stop DABC threads and MBS tasks",haltIcon,this);
    addButton("mbsCleanup","Shutdown all servers",disIcon,this);
    //addButton("mbsCleanup","Reset and cleanup",disIcon,this);
    addButton("mbsShow","Show acquisition",infoIcon,this);
    addButton("mbsTest","Rsh Node -l Username MbsPath/MbsScript MbsPath MbsUserpath Command",winIcon,this);
    addButton("mbsShell","Rsh MbsNode -l Username Command",workIcon,this);
    addButton("dabcShell","ssh DabcNode -l Username Script",workIcon,this);
// Text input fields
    if(System.getenv("DABC_LAUNCH_MBS")!=null)
         setupMbs=new xFormMbs(System.getenv("DABC_LAUNCH_MBS"),this);
    else setupMbs=new xFormMbs("MbsLaunch.xml",this);
    if(System.getenv("DABC_LAUNCH_DABC")!=null)
        setupDabc=new xFormDabc(System.getenv("DABC_LAUNCH_DABC"),this);
    else setupDabc=new xFormDabc("DabcLaunch.xml",this);
    int width=25;
    DimName=new JTextField(xSet.getDimDns(),width);
    DimName.setEditable(false);
    Username=new JTextField(xSet.getUserName(),width);
    Username.setEditable(false);
    Password=new JPasswordField(width);
    Password.setEchoChar('*');
    Password.addActionListener(this);
    Password.setActionCommand("setpwd");
// add prompt/input fields
    addPrompt("Name server: ",DimName);
    addPrompt("User name: ",Username);
    addPrompt("Password [RET]: ",Password);
    MbsNode=addPrompt("MBS master node: ",setupMbs.getMaster(),"set",width,this);
    MbsServers=addPrompt("MBS servers: ",setupMbs.getServers(),"set",width,this);
    DabcNode=addPrompt("DABC master node: ",setupDabc.getMaster(),"set",width,this);
    DabcName=addPrompt("DABC master name: ",setupDabc.getName(),"set",width,this);
    DabcServers=addPrompt("DABC servers: ",setupDabc.getServers(),"set",width,this);
    MbsPath=addPrompt("MBS system path: ",setupMbs.getSystemPath(),"set",width,this);
    MbsUserpath=addPrompt("MBS user path: ",setupMbs.getUserPath(),"set",width,this);
    MbsScript=addPrompt("MBS script: ",setupMbs.getScript(),"set",width,this);
    MbsCommand=addPrompt("MBS command: ",setupMbs.getCommand(),"set",width,this);
    DabcPath=addPrompt("DABC system path: ",setupDabc.getSystemPath(),"set",width,this);
    DabcUserpath=addPrompt("DABC user path: ",setupDabc.getUserPath(),"set",width,this);
    DabcSetup=addPrompt("DABC setup file: ",setupDabc.getSetup(),"set",width,this);
    DabcScript=addPrompt("DABC script: ",setupDabc.getScript(),"set",width,this);
    MbsLaunchFile=addPrompt("MBS launch file: ",setupMbs.getLaunchFile(),"set",width,this);
    DabcLaunchFile=addPrompt("DABC launch file: ",setupDabc.getLaunchFile(),"set",width,this);
    // read defaults from setup file
    nMbsServers=Integer.parseInt(setupMbs.getServers());
    nDabcServers=Integer.parseInt(setupDabc.getServers());
    nServers=nMbsServers+nDabcServers+1; // add DNS
    System.out.println("Total servers needed: DNS + "+(nServers-1));
    
    mbsshell = new xRemoteShell("rsh");
    dabcshell = new xRemoteShell("ssh");
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

private void setLaunch(){
xSet.setAccess(Password.getPassword());
setupMbs.setMaster(MbsNode.getText());
setupMbs.setServers(MbsServers.getText());
setupMbs.setSystemPath(MbsPath.getText());
setupMbs.setUserPath(MbsUserpath.getText());
setupMbs.setScript(MbsScript.getText());
setupMbs.setLaunchFile(MbsLaunchFile.getText());
setupMbs.setCommand(MbsCommand.getText());
//setupMbs.printForm();

setupDabc.setMaster(DabcNode.getText());
setupDabc.setServers(DabcServers.getText());
setupDabc.setSystemPath(DabcPath.getText());
setupDabc.setUserPath(DabcUserpath.getText());
setupDabc.setScript(DabcScript.getText());
setupDabc.setLaunchFile(DabcLaunchFile.getText());
setupDabc.setName(DabcName.getText());
setupDabc.setSetup(DabcSetup.getText());
//setupDabc.printForm();
}
// get from command list special commands for buttons.
public void setDimServices(){
int i;
releaseDimServices();
System.out.println("DabcMbs setDimServices");
doExit=new Vector<Object>(0);
runMsgLog=new Vector<xDimParameter>(0);
runRate=new Vector<xDimParameter>(0);
runState=new Vector<xDimParameter>(0);
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
else if(para.get(i).getParser().getFull().indexOf("MSG/MsgLog")>0) {
    runMsgLog.add(para.get(i));}
else if(para.get(i).getParser().getFull().indexOf("MSG/Rate")>0) {
    runRate.add(para.get(i));}
else if(para.get(i).getParser().getFull().indexOf("/RunMode/State")>0) {
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

public void releaseDimServices(){
System.out.println("DabcMbs releaseDimServices");
mbsCommand=null;
doConfig=null;
doEnable=null;
doStart=null;
doStop=null;
if(doExit != null) doExit.removeAllElements();
if(runState != null) runState.removeAllElements();
if(runMsgLog != null) runMsgLog.removeAllElements();
if(runRate != null) runRate.removeAllElements();
doHalt=null;
doExit=null;
runState=null;
runMsgLog=null;
runRate=null;
}

private void setProgress(String info, Color color){
setTitle(info,color);
if(threadRunning) progressState.redraw(-1,xSet.blueL(),info, true);
}
private void startProgress(){
    xLayout la= new xLayout("progress");
    la.set(new Point(50,200), new Dimension(300,100),0,true);
    progress=new xInternalFrame("Work in progress, please wait", la);
    progressState=new xState("Current action:",350,30);
    progressState.redraw(-1,"Green","Starting", true);
    progressState.setSizeXY();
    progress.setupFrame(workIcon, null, progressState, true);
    etime.action(new ActionEvent(progress,1,"DisplayFrame"));
}
private void stopProgress(){
    etime.action(new ActionEvent(progress,1,"RemoveFrame"));
}

private boolean waitMbs(int timeout, Vector<xDimParameter> param, String serv){
int t=0,i;
int nLoggers;
if(param == null) setDimServices();
if(param == null) return false;
String wild = new String("*MSG/"+serv);
if(param.size()>0){
    setProgress(new String("Wait for "+(nMbsServers-1)+" MBS "+serv+" tasks ..."),xSet.blueD());
    System.out.print("Wait for "+(nMbsServers-1)+" "+serv);
    while(t < timeout){
        nLoggers=0;
        for(i=0;i<param.size();i++)
            if(param.get(i).getValue().equals(serv))nLoggers++;
        if(nLoggers == (nMbsServers-1)) return true;
        System.out.print(".");
        browser.sleep(1);
        t++;
}} else {
    setProgress(new String("Wait for "+(nMbsServers-1)+" MBS "+serv+" tasks ..."),xSet.blueD());
    System.out.print("Wait for "+(nMbsServers-1)+" "+wild);
    while(t < timeout){
        String[] xx = browser.getServices(wild);
        if(xx.length == (nMbsServers-1)) return true;
        System.out.print(".");
        browser.sleep(1);
        t++;
    }
    }
    return false;
}
private boolean waitState(int timeout, String state){
int t=0;
boolean ok;
System.out.print("Wait for state "+state);
    while(t < timeout){
    ok=true;
    for(int i=0;i<runState.size();i++){
        if(!runState.get(i).getValue().equals(state)) ok=false;
    }
        if(ok) return true;
        setProgress(new String("Wait for DABC state "+state+" "+t+" ["+timeout+"]"),xSet.blueD());
        System.out.print(".");
        browser.sleep(1);
        t++;
    }
    return false;
}
private boolean waitMode(int timeout, String mode){
int t=0;
boolean ok;
System.out.print("Wait for run mode "+mode);
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
//React to menu selections.
public void actionPerformed(ActionEvent e) {
boolean doit=true;
if ("set".equals(e.getActionCommand())) {
setLaunch();
return;
}
if ("setpwd".equals(e.getActionCommand())) {
setLaunch();
return;
}

if ("Save".equals(e.getActionCommand())) {
    xLogger.print(1,Action);
    setLaunch();
    setupDabc.saveSetup(DabcLaunchFile.getText());
    setupMbs.saveSetup(MbsLaunchFile.getText());
    String msg=new String("Dabc launch: "+DabcLaunchFile.getText()+"\nMbs  launch: "+MbsLaunchFile.getText());
    tellInfo(msg);
    return;
}

    if(!threadRunning){
    Action = new String(e.getActionCommand());
    // must do confirm here, because in thread it would block forever
    if ("mbsCleanup".equals(Action)) doit=choose("Shut down and cleanup MBS/DABC?");
    if(doit){
        startProgress();
        ae=e;
        threxe = new Thread(this);
        threadRunning=true;
        threxe.start();
    }
    } else tellError("Execution thread not yet finished!");
}
// start thread by threxe.start()
// CAUTION: Do not use tell or choose here: Thread will never continue!
public void run(){
int time=0;
    MbsMaster = MbsNode.getText();
    dabcMaster = DabcNode.getText();
    
    if ("mbsQuit".equals(Action)) {
        xLogger.print(1,Action);
    }
    else if ("mbsShell".equals(Action)) {
        xLogger.print(1,"Shell: "+MbsCommand.getText());
        //xLogger.print(1,MbsCommand);
        if(mbsshell.rsh(MbsNode.getText(),Username.getText(),MbsCommand.getText(),0L)){}
    }
    else if ("dabcShell".equals(Action)) {
        xLogger.print(1,"Shell: "+DabcScript.getText());
        //xLogger.print(1,MbsCommand);
        if(dabcshell.rsh(DabcNode.getText(),Username.getText(),DabcScript.getText(),0L)){}
    }
    else if ("mbsCleanup".equals(Action)) {
        if(doExit == null) setDimServices();
        if(doExit != null){
        setProgress("Exit DABC ...",xSet.blueD());
        for(int i=0;i<doExit.size();i++){
            xLogger.print(1,((xDimCommand)doExit.get(i)).getParser().getFull());
            ((xDimCommand)doExit.get(i)).exec(xSet.getAccess());
        }
        }  else {setProgress("No DABC exit commands available!",xSet.redD());browser.sleep(2);}
        setProgress("Shut down MBS ...",xSet.blueD());
        if(mbsCommand != null) mbsCommand.exec(xSet.getAccess()+" *::Stop rirec");
        if(mbsCommand != null) mbsCommand.exec(xSet.getAccess()+" *::exit");
        String cmd = new String(MbsPath.getText()+
                                "/script/prmshutdown.sc "+
                                MbsPath.getText()+" "+
                                MbsUserpath.getText());
        //xLogger.print(0,MbsMaster+": "+cmd);
        if(mbsshell.rsh(MbsMaster,Username.getText(),cmd,0L)){
            setProgress("OK: MBS and DABC shut down, update parameters ...",xSet.blueD());
            xSet.setSuccess(false);
            etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);
            if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);}
            if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
            else setProgress("OK: MBS and DABC servers down",xSet.greenD());
            System.out.println("Run down finished");
        }
        else setProgress("MBS rundown failure!",xSet.redD());
    }
    else if ("prmLaunch".equals(Action)) {
        setProgress("Launching DABC, wait 5 sec ...",xSet.blueD());
        String cmd = new String(DabcPath.getText()+
                                "/script/dabcstartup.sh "+DabcPath.getText()+" "+
                                DabcUserpath.getText()+" "+DabcSetup.getText()+" "+
                                xSet.getDimDns()+" "+dabcMaster+" "+xSet.getAccess()+" &");
        xLogger.print(1,cmd);
        time=0;
        dabcshell.rsh(dabcMaster,Username.getText(),cmd,5L);
        System.out.print("Wait MBS sockets free ");
        String s=mbsshell.rshout(MbsMaster,Username.getText(),"netstat|grep 600|grep TIME");
        while(s.indexOf("TIME")>=0){
            System.out.print(".");
            browser.sleep(1);
            time++;
            setProgress("Wait MBS sockets free "+time+"["+20+"]",xSet.blueD());
            s=mbsshell.rshout(MbsMaster,Username.getText(),"netstat|grep 600|grep TIME");
        }
        System.out.println("");
        
        setProgress("Launching MBS ...",xSet.blueD());
        cmd = new String(MbsPath.getText()+
                                "/script/prmstartup.sc "+MbsPath.getText()+" "+
                                MbsUserpath.getText()+" "+
                                xSet.getDimDns()+" "+xSet.getGuiNode()+" "+xSet.getAccess());
        xLogger.print(0,MbsMaster+": "+cmd);
        if(mbsshell.rsh(MbsMaster,Username.getText(),cmd,0L)){
        setProgress("Wait for MBS servers ready ...",xSet.blueD());
        boolean w = waitMbs(20,runMsgLog,"MsgLog");
        // if(w && (browser.getServers().indexOf(service)!=-1)){
        browser.sleep(2); // get message loggers the chance to announce prompter
        System.out.println("\nServers "+browser.getServers());
        String service = new String(MbsNode.getText().toUpperCase()+":PRM");
        System.out.print("MBS wait ");
        time=0;
        while(browser.getServers().indexOf(service)==-1){
            System.out.print(".");
            browser.sleep(1);
            time++;
            if(time > 10) break;
        }
        if(browser.getServers().indexOf(service)!=-1){
            System.out.println("\nMBS connnected");
            setProgress("MBS servers ready",xSet.greenD());
        } else {
            System.out.println("\nMBS Failed ");
            setProgress("Failed: MBS launch",xSet.redD());
            //tellError("MBS servers missing!");
        }
        } else{
           setProgress("Failed: MBS launch script",xSet.redD());
           //tellError("MBS startup script failed");
	}
        time=0;
        int nserv=0;
        System.out.print("DABC wait ");
        nserv=browser.getNofServers();
        setProgress(new String("Wait for "+(nServers-1)+" servers ..."),xSet.blueD());
        while(nserv < nServers){
            System.out.print(".");
            browser.sleep(1);
            time++;
            if(time > 10) break;
            nserv=browser.getNofServers();
        }
        if(browser.getNofServers() >= nServers){
            System.out.println("\nDABC connnected "+nServers+" servers");
            setProgress("OK: MBS and DABC servers ready, update parameters ...",xSet.blueD());
            xSet.setSuccess(false);
            etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);
            if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);}
            if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
            else setProgress("OK: MBS and DABC servers ready",xSet.greenD());
            //setDimServices();        
        }
        else {
            System.out.println("\nDABC Failed ");
            setProgress("Servers missing: "+(nServers-nserv)+" from "+nServers+", update parameters ...",xSet.redD());
            xSet.setSuccess(false);
            etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);
            if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);}
            if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
            else setProgress("Failure: DABC servers not ready",xSet.redD());
            //setDimServices();        
        }
    }
    else if ("mbsTest".equals(Action)) {
        // String cmd = new String(MbsPath.getText()+"/"+
                                // MbsScript.getText()+" "+
                                // MbsPath.getText()+" "+
                                // MbsUserpath.getText()+" "+
                                // MbsCommand.getText()
                                // );
        // if(mbsshell.rsh(MbsNode.getText(),Username.getText(),cmd,0L)){}
        String cmd = new String(MbsPath.getText()+"/"+
                                MbsScript.getText()+" "+
                                MbsPath.getText()+" "+
                                "v50/rio3/runmbs "+
                                "m_evgen_RIO2 v50/rio3/runmbs . x86g-4"
                                );
        if(mbsshell.rsh("R3g-2",Username.getText(),cmd,0L)){}
        cmd = new String(MbsPath.getText()+"/"+
                                MbsScript.getText()+" "+
                                MbsPath.getText()+" "+
                                "v50/rio3/runmbs "+
                                "m_evgen_RIO3 v50/rio3/runmbs . x86-7"
                                );
        if(mbsshell.rsh("R2f-2",Username.getText(),cmd,0L)){}
    }
    // else if ("mbsShut".equals(Action)) {
        // if(doExit == null) setDimServices();
        // if(doExit != null){
        // for(int i=0;i<doExit.size();i++){
            // xLogger.print(1,((xDimCommand)doExit.get(i)).getParser().getFull());
            // ((xDimCommand)doExit.get(i)).exec(xSet.getAccess());
        // }}  else setProgress("No DABC exit commands available!",xSet.redD());
        // if(mbsCommand == null) setDimServices();
        // if(mbsCommand != null){
        // xLogger.print(1,"MBS: @shutdown.scom");
        // mbsCommand.exec(xSet.getAccess()+" *::Stop rirec");
        // mbsCommand.exec(xSet.getAccess()+" @shutdown.scom");
        // setProgress("OK: Launch DABC, configure!",xSet.greenD());
	// } else setProgress("No MBS commands available!",xSet.redD());
    // }

    // from here we need commands
    else {
    if(mbsCommand == null) setDimServices();
    if(mbsCommand == null) {
        setProgress("MBS commands not available!",xSet.redD());
        threadRunning=false;
        stopProgress();
        System.out.println("Thread done!!!");
        return;
    }
    if(doHalt == null) setDimServices();
    if(doHalt == null) {
        setProgress("DABC doHalt not available!",xSet.redD());
        threadRunning=false;
        stopProgress();
        System.out.println("Thread done!!!");
        return;
    }
    if(doEnable == null) setDimServices();
    if(doEnable == null) {
        setProgress("DABC doEnable not available!",xSet.redD());
        threadRunning=false;
        stopProgress();
        System.out.println("Thread done!!!");
        return;
    }
    if(doConfig == null) setDimServices();
    if(doConfig == null) {
        setProgress("DABC doConfig not available!",xSet.redD());
        threadRunning=false;
        stopProgress();
        System.out.println("Thread done!!!");
        return;
    }
    else if ("mbsConfig".equals(Action)) {
        setProgress("Start up and configure MBS tasks",xSet.blueD());
        System.out.print("Wait MBS transport socket free ");
        String s=mbsshell.rshout(MbsMaster,Username.getText(),"netstat|grep 6000|grep WAIT");
        time=0;
        while(s.indexOf("WAIT")>=0){
            System.out.print(".");
            browser.sleep(1);
            time++;
            setProgress("Wait MBS transport socket free "+time+"["+10+"]",xSet.blueD());
            s=mbsshell.rshout(MbsMaster,Username.getText(),"netstat|grep 6000|grep WAIT");
            if(time > 10)break;
        }
        System.out.println(" ");
        xLogger.print(1,"MBS: @startup.scom");
        mbsCommand.exec(xSet.getAccess()+" @startup.scom");
        // first wait for MBS
        if(!waitMbs(10,runRate,"Rate")){
            System.out.println("\nMBS startup failed ");
            setProgress("MBS configure failed",xSet.redD());
            //tellError("MBS tasks missing!");
        } else {
            System.out.println(" ");
            xLogger.print(1,"MBS: *::enable dabc");
            mbsCommand.exec(xSet.getAccess()+" *::enable dabc");
            waitMode(10,"DABC");
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
                System.out.println("\nDABC configure failed ");
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
    else if ("dabcEnable".equals(Action)) {
        xLogger.print(1,doEnable.getParser().getFull());
        doEnable.exec(xSet.getAccess());
        setProgress("DABC enabled",xSet.greenD());
    }
    else if ("mbsStart".equals(Action)) {
        xLogger.print(1,"MBS: *::Start acquisition");
        mbsCommand.exec(xSet.getAccess()+" *::Start acquisition");
        xLogger.print(1,doStart.getParser().getFull());
        doStart.exec(xSet.getAccess());
    }
    else if ("mbsStop".equals(Action)) {
        xLogger.print(1,"MBS: *::Stop acquisition");
        mbsCommand.exec(xSet.getAccess()+" *::Stop acquisition");
        xLogger.print(1,doStop.getParser().getFull());
        doStop.exec(xSet.getAccess());
    }
    else if ("mbsShow".equals(Action)) {
        xLogger.print(1,"MBS: *::Show acqisition");
        mbsCommand.exec(xSet.getAccess()+" *::Show acquisition");
    }
    else if ("dabcHalt".equals(Action)) {
        xLogger.print(1,doHalt.getParser().getFull());
        setProgress("Halt DABC, shutdown MBS",xSet.blueD());
        doHalt.exec(xSet.getAccess());
        xLogger.print(1,"MBS: @shutdown.scom");
        mbsCommand.exec(xSet.getAccess()+" @shutdown.scom");
        browser.sleep(5);
        if(waitState(10,"Halted")){
            System.out.println(" ");
            setProgress("OK: DABC halted, MBS servers ready, update parameters ...",xSet.blueD());
            xSet.setSuccess(false);
            etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);
            if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);}
            if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
            else setProgress("OK: DABC halted, MBS servers ready",xSet.greenD());
            //setDimServices();
        }else setProgress("Failure: DABC halted, MBS servers ready",xSet.redD());
    }
}
threadRunning=false;
stopProgress();
System.out.println("Thread done!!!");
}
}



