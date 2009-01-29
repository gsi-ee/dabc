package xgui;
import javax.swing.JLabel;
import javax.swing.JButton;
import javax.swing.JTextField;
import javax.swing.JPasswordField;
import javax.swing.JCheckBox;
import javax.swing.JOptionPane;
import javax.swing.ImageIcon;
import java.awt.Dimension;
import java.awt.Point;
//import javax.swing.GroupLayout;

import java.io.IOException;
import java.awt.event.*;
import java.awt.Color;
import java.util.*;

/**
 * Form panel to control MBS.
 * @author Hans G. Essel
 * @version 1.0
 * @see xForm
 * @see xFormMbs
 */
public class xPanelMbs extends xPanelPrompt implements ActionListener, Runnable
{
private xRemoteShell mbsshell;
private ImageIcon closeIcon ;
private ImageIcon winIcon;
private ImageIcon workIcon,mworkIcon;
private ImageIcon launchIcon;
private ImageIcon killIcon;
private ImageIcon storeIcon, mbsIcon, configIcon, startIcon, stopIcon, dabcIcon, disIcon, infoIcon;
private JTextField DimName, MbsNode, MbsServers, Username, MbsUserpath, MbsPath, MbsScript, MbsCommand, MbsLaunchFile;
private JPasswordField Password;
private JCheckBox boxTest;
private String MbsMaster;
private String Action;
private String Access;
private xDimBrowser browser;
private xiDesktop desk;
private xInternalFrame progress;
private xState progressState;
private xFormMbs formMbs;
private ActionListener action;
private JOptionPane confirm;
private int nServers, nLoggers, nRates;
private xDimCommand mbsCommand=null;
private xDimParameter mbsNodeList=null;
private Vector<xDimParameter> runMsgLog;
private Vector<xDimParameter> runRate;
private Thread threxe;
private ActionEvent ae;
private xTimer etime;
private boolean threadRunning=false;

/**
 * Constructor of MBS launch panel.
 * @param title Title of window.
 * @param diminfo DIM browser
 * @param desktop Interface to desktop
 * @param al Event handler of desktop. Handles events from xTimer.<br>
 * Passed actions are: Update, DisplayFrame, RemoveFrame.
 */
public xPanelMbs(String title, xDimBrowser diminfo, xiDesktop desktop, ActionListener al) {
    super(title);
    runMsgLog=new Vector<xDimParameter>(0);
    runRate=new Vector<xDimParameter>(0);
    // get icons
    action=al;
    desk=desktop;
    browser=diminfo;
    storeIcon   = xSet.getIcon("icons/savewin.png");
    launchIcon  = xSet.getIcon("icons/connprm.png");
    killIcon    = xSet.getIcon("icons/dabchalt.png");
    mbsIcon     = xSet.getIcon("icons/conndsp.png");
    disIcon     = xSet.getIcon("icons/disconn.png");
    infoIcon    = xSet.getIcon("icons/info.png");
    dabcIcon    = xSet.getIcon("icons/dabcicon.png");
    winIcon     = xSet.getIcon("icons/rshmbs.png");
    workIcon    = xSet.getIcon("icons/control.png");
    mworkIcon   = xSet.getIcon("icons/controlmbs.png");
    closeIcon   = xSet.getIcon("icons/fileclose.png");
    configIcon  = xSet.getIcon("icons/mbsconfig.png");
    startIcon   = xSet.getIcon("icons/mbsstart.png");
    stopIcon    = xSet.getIcon("icons/dabcstop.png");
    // add buttons
    addButton("mbsQuit","Close window",closeIcon,this);
    addButton("mbsSave","Save form to file",storeIcon,this);
    addButton("prmLaunch","Launch MBS multi node servers",launchIcon,this);
    addButton("mbsLaunch","Launch MBS single node servers",mbsIcon,this);
    addButton("mbsConfig","Configure MBS (@startup)",configIcon,this);
    // addButton("mbsDabc","Set DABC mode",dabcIcon,this);
    addButton("mbsStart","Start acquisition",startIcon,this);
    addButton("mbsStop","Stop acquisition",stopIcon,this);
    addButton("mbsShut","Shutdown tasks (@shutdown)",killIcon,this);
    addButton("mbsCleanup","Shut down servers and cleanup MBS",disIcon,this);
    addButton("mbsShow","Show acquisition",infoIcon,this);
    addButton("mbsTest","Rsh Node -l Username SystemPath/Script SystemPath Userpath Command",winIcon,this);
    addButton("mbsShell","Rsh Node -l Username Command",mworkIcon,this);
// Text input fields
    // read defaults from setup file
    if(System.getenv("DABC_LAUNCH_MBS")!=null)
         formMbs=new xFormMbs(System.getenv("DABC_LAUNCH_MBS"));
    else formMbs=new xFormMbs("MbsLaunch.xml");
    formMbs.addActionListener(this);
    Object o=xSet.addObject(formMbs);
    // formMbs=(xFormMbs)xSet.getObject("xgui.xFormMbs"); // how to retrieve

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
    MbsNode=addPrompt("Master node: ",formMbs.getMaster(),"set",width,this);
    MbsServers=addPrompt("Servers: ",formMbs.getServers(),"set",width,this);
    MbsPath=addPrompt("System path: ",formMbs.getSystemPath(),"set",width,this);
    MbsUserpath=addPrompt("User path: ",formMbs.getUserPath(),"set",width,this);
    MbsScript=addPrompt("Script: ",formMbs.getScript(),"set",width,this);
    MbsCommand=addPrompt("Command: ",formMbs.getCommand(),"set",width,this);
    MbsLaunchFile=addPrompt("Launch file: ",formMbs.getLaunchFile(),"set",width,this);
    // addCheckBox("Test box",new JCheckBox());
    nServers=1+Integer.parseInt(formMbs.getServers());// add DNS
    System.out.println("MBS   servers needed: DNS + "+(nServers-1));
    
    mbsshell = new xRemoteShell("rsh");
    String service = new String(MbsNode.getText().toUpperCase()+":PRM");
    String servlist = browser.getServers();
    if(servlist.indexOf(service)>=0) setProgress("MBS servers ready",xSet.greenD());
    service = new String(MbsNode.getText().toUpperCase()+":DSP");
    if(servlist.indexOf(service)>=0) setProgress("MBS servers ready",xSet.greenD());
    setDimServices();
    etime = new xTimer(al, false); // fire only once
}

private void setLaunch(){
xSet.setAccess(Password.getPassword());
formMbs.setMaster(MbsNode.getText());
formMbs.setServers(MbsServers.getText());
formMbs.setSystemPath(MbsPath.getText());
formMbs.setUserPath(MbsUserpath.getText());
formMbs.setScript(MbsScript.getText());
formMbs.setLaunchFile(MbsLaunchFile.getText());
formMbs.setCommand(MbsCommand.getText());
//formMbs.printForm();
}
/**
 * Called in xDesktop to rebuild references to DIM services.
 */
public void setDimServices(){
int i;
releaseDimServices();
System.out.println("Mbs setDimServices");
Vector<xDimCommand> list=browser.getCommandList();
for(i=0;i<list.size();i++){
if(list.get(i).getParser().getFull().indexOf("/MbsCommand")>0) {mbsCommand=list.get(i);break;}
}
Vector<xDimParameter> para=browser.getParameterList();
if(para != null)for(i=0;i<para.size();i++){
if(para.get(i).getParser().getFull().indexOf("MSG/MsgLog")>0) runMsgLog.add(para.get(i));
if(para.get(i).getParser().getFull().indexOf("MSG/Rate")>0) runRate.add(para.get(i));
}
}
/**
 * Called in xDesktop to release references to DIM services.
 */
public void releaseDimServices(){
System.out.println("Mbs releaseDimServices");
    mbsCommand=null;
    runMsgLog.removeAllElements();
    runRate.removeAllElements();
}

// Start internal frame with an xState panel through timer.
// Timer events are handled by desktop event handler passed to constructor.
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
// Fire event handler of desktop through timer.
private void setProgress(String info, Color color){
setTitle(info,color);
if(threadRunning) progressState.redraw(-1,xSet.blueL(),info, true);
}
// Fire event handler of desktop through timer.
private void stopProgress(){
//etime.setActionCommand(progress.getTitle()); // Java 1.6
    etime.action(new ActionEvent(progress,1,"RemoveFrame"));
}

// wait until all parameters from vector match serv (parameters exist).
// or wait until the number of *MSG/serv parameters reaches the requested number.  
private boolean waitMbs(int timeout, Vector<xDimParameter> param, String serv){
int t=0,i;
// param is supposed to be Vector of message logger or rate task parameters.
// If not there, wait for parameters to be created,
// otherwise wait that their values match the task names
String wild = new String("*MSG/"+serv);
if(param.size()>0){
    setProgress(new String("Wait for "+(nServers-2)+" "+serv+" tasks ..."),xSet.blueD());
    System.out.print("Wait for "+(nServers-2)+" "+serv);
    while(t < timeout){
        nLoggers=0;
        for(i=0;i<param.size();i++)
            if(param.elementAt(i).getValue().equals(serv))nLoggers++;
        if(nLoggers == (nServers-2)) return true;
        System.out.print(".");
        browser.sleep(1);
        t++;
}} else {
    setProgress(new String("Wait for "+(nServers-2)+" "+serv+" tasks ..."),xSet.blueD());
    System.out.print("Wait for "+(nServers-2)+" "+wild);
    while(t < timeout){
        String[] xx = browser.getServices(wild);
        if(xx.length == (nServers-2)) return true;
        System.out.print(".");
        browser.sleep(1);
        t++;
    }
    }
    return false;
}

private void launch(String cmd, String service){
int time=0;
int num=0;
    setProgress("Launch MBS",xSet.blueD());
    System.out.print("Wait sockets free ");
    String s=mbsshell.rshout(MbsMaster,Username.getText(),"netstat|grep 600|grep TIME");
    while(s.indexOf("TIME")>=0){
        System.out.print(".");
        browser.sleep(1);
        time++;
        setProgress("Wait sockets free "+time+"["+20+"]",xSet.blueD());
        s=mbsshell.rshout(MbsMaster,Username.getText(),"netstat|grep 600|grep TIME");
    }
    time=0;
    setProgress("Launch MBS ...",xSet.blueD());
    System.out.println("");
    if(mbsshell.rsh(MbsMaster,Username.getText(),cmd,0L)){
    setProgress("Wait for MBS servers ready ...",xSet.blueD());
        boolean w = waitMbs(20,runMsgLog,"MsgLog");
        // if(w && (browser.getServers().indexOf(service)!=-1)){
        browser.sleep(2); // get message loggers the chance to announce prompter
        System.out.println("\nServers "+browser.getServers());
        num=browser.getServers().indexOf(service);
        while(num == -1){
            System.out.print("x");
            browser.sleep(1);
            time++;
            if(time > 20) break;
            num=browser.getServers().indexOf(service);
        }
        if(time > 0)System.out.println("");
        if(num >= 0){
            System.out.println("MBS connnected");
            setProgress("MBS servers ready, update parameters ...",xSet.blueD());
            xSet.setSuccess(false);
            etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);
            if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);}
            if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
            else setProgress("OK: MBS servers ready",xSet.greenD());
            //setDimServices();
        }
        else {
            System.out.println("MBS Failed ");
            setProgress("Failed: Launch",xSet.redD());
            //tellError("MBS servers missing!");
        }
    } else {
        System.out.println("\nMBS startup script Failed ");
        setProgress("Failed: Launch script",xSet.redD());
        //tellError("MBS startup script failed");
        }
    }

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
return;
}
if ("setpwd".equals(e.getActionCommand())) {
setLaunch();
return;
}

if ("mbsSave".equals(e.getActionCommand())) {
    xLogger.print(1,Action);
    setLaunch();
    formMbs.saveSetup(MbsLaunchFile.getText());
    String msg=new String("Mbs launch: "+MbsLaunchFile.getText());
    tellInfo(msg);
    return;
}

    if(!threadRunning){
    Action = new String(e.getActionCommand());
    // must do confirm here, because in thread it would block forever
    if ("mbsCleanup".equals(Action)) doit=askQuestion("Confirmation","Shut down and cleanup MBS?");
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
// CAUTION: Do not use tellInfo or askQuestion here: Thread will never continue!
/**
 * Thread handling events.<br>
 * If an update of DIM parameter list is necessary, Update event is launched through timer
 * and handled by desktop action listener.
 */
public void run(){
//xSet.setWaitCursor();
    MbsMaster = MbsNode.getText();
    if ("prmLaunch".equals(Action)) {
        String cmd = new String(MbsPath.getText()+
                                "/script/prmstartup.sc "+MbsPath.getText()+" "+
                                MbsUserpath.getText()+" "+xSet.getDimDns()+" "+xSet.getGuiNode()+" "+xSet.getAccess());
        String service = new String(MbsNode.getText().toUpperCase()+":PRM");
        launch(cmd,service);
    }
    else if ("mbsLaunch".equals(Action)) {
        String cmd = new String(MbsPath.getText()+
                                "/script/dimstartup.sc "+MbsPath.getText()+" "+
                                MbsUserpath.getText()+" "+xSet.getDimDns()+" "+xSet.getAccess());
        String service = new String(MbsNode.getText().toUpperCase()+":DSP");
        launch(cmd,service);
    }
    else if ("mbsTest".equals(Action)) {
        String cmd = new String(MbsPath.getText()+"/"+
                                MbsScript.getText()+" "+
                                MbsPath.getText()+" "+
                                MbsUserpath.getText()+" "+
                                MbsCommand.getText()
                                );
        //xLogger.print(0,MbsNode+": "+cmd);
        // if(mbsshell.rsh(MbsNode.getText(),Username.getText(),cmd,0L)){}
        cmd = new String(MbsPath.getText()+"/"+
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
    else if ("mbsShell".equals(Action)) {
        xLogger.print(1,"Shell: "+MbsCommand.getText());
        //xLogger.print(1,MbsCommand);
        if(mbsshell.rsh(MbsNode.getText(),Username.getText(),MbsCommand.getText(),0L)){}
   }
    else if ("mbsQuit".equals(Action)) {
        xLogger.print(1,Action);
    }
    else if ("mbsCleanup".equals(Action)) {
        String cmd = new String(MbsPath.getText()+
                                "/script/prmshutdown.sc "+
                                MbsPath.getText()+" "+
                                MbsUserpath.getText());
        //xLogger.print(0,MbsMaster+": "+cmd);
        setProgress("Shut down MBS, update parameters ...",xSet.blueD());
        if(mbsshell.rsh(MbsMaster,Username.getText(),cmd,0L)){
            xSet.setSuccess(false);
            etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);
            if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);}
            if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
            else setProgress("OK: MBS shut down",xSet.greenD());
            //setDimServices();
            System.out.println("Run down finished");
    }}
    else {

// all others need mbscommand
    if(mbsCommand == null) setDimServices();
    if(mbsCommand == null) {
        setProgress("MBS commands not available! Launch first.",xSet.redD());
        //tellError("MBS commands not available!");
        threadRunning=false;
        stopProgress();
        System.out.println("Thread done!!!");
//xSet.setDefaultCursor();
        return;
    }
    if ("mbsShut".equals(Action)) {
        xLogger.print(1,"MBS: @shutdown.scom");
        setProgress("MBS shutdown",xSet.blueD());
        mbsCommand.exec(xSet.getAccess()+" @shutdown.scom");
        browser.sleep(5);
        setProgress("OK: MBS servers ready",xSet.greenD());
    }
    else if ("mbsConfig".equals(Action)) {
        xLogger.print(1,"MBS: @startup.scom");
        mbsCommand.exec(xSet.getAccess()+" @startup.scom");
        setProgress("Start up and configure MBS tasks",xSet.blueD());
        if(waitMbs(20,runRate,"Rate")){
            System.out.println(" ");
            browser.sleep(2); // get message loggers the chance to announce new tasks
            xSet.setSuccess(false);
            setProgress("Update parameters ...",xSet.blueD());
            etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);
            if(!xSet.isSuccess()) {etime.action(new ActionEvent(ae.getSource(),ae.getID(),"Update"));
            browser.sleep(1);}
            if(!xSet.isSuccess()) setProgress(xSet.getMessage(),xSet.redD());
            else setProgress("OK: MBS tasks ready",xSet.greenD());
            //setDimServices();
        } else {
            System.out.println("\nMBS startup failed ");
            setProgress("Fail: Configure",xSet.redD());
            //tellError("MBS rate meter missing!");
        }
    }
    else if ("mbsStart".equals(Action)) {
        xLogger.print(1,"MBS: *::Start acquisition");
        mbsCommand.exec(xSet.getAccess()+" *::Start acquisition");
     }
    else if ("mbsStop".equals(Action)) {
        xLogger.print(1,"MBS: *::Stop acquisition");
        mbsCommand.exec(xSet.getAccess()+" *::Stop acquisition");
    }
    else if ("mbsShow".equals(Action)) {
        xLogger.print(1,"MBS: *::Show acqisition");
        mbsCommand.exec(xSet.getAccess()+" *::Show acquisition");
    }
    else if ("mbsDabc".equals(Action)) {
        xLogger.print(1,"MBS: *::enable dabc");
        mbsCommand.exec(xSet.getAccess()+" *::enable dabc");
    }
}
//xSet.setDefaultCursor();
threadRunning=false;
stopProgress();
System.out.println("Thread done!!!");
}
}



