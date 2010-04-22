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
import java.util.*;
import java.io.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.ImageIcon;
import javax.swing.JTextField;
import javax.swing.JTextArea;
import javax.swing.JCheckBox;
import javax.swing.JPanel;
import javax.swing.JDesktopPane;
import javax.swing.JSplitPane;
import javax.swing.JScrollPane;
import org.w3c.dom.*;
import dim.*;
/**
 * Panel for display of selected list of parameters.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xPanelRunInfo  extends xPanelPrompt         // provides some simple formatting
						implements  xiUserPanel,        // needed by GUI
						ActionListener      // for panel input
{
private xRemoteShell mbsshell;
private String name, p1name;
private String tooltip;
private ActionListener action;
private JTextField run, exper, line;
private JCheckBox checkfull;
private JTextArea comar;
private xiDesktop desk;
private xiDimBrowser brows;
private xiDimCommand mbsSetHead=null;
private xFormRunInfo formRun;
private Color back;
private xLayout panelLayout;
private Point panpnt;
private ImageIcon menuIcon; 
private String srun, sexp, sline, file;
private boolean only;
/**
 * Creates panel to specify run information.
 */
public xPanelRunInfo(){
// Head line (title) inside window
	   super("Run Information");
	   menuIcon=xSet.getIcon("icons/filenew.png");
	   name=new String("RunInfo"); // name for prompter panel
	   panelLayout=xSet.getLayout(name); // restored from setup?
	   if(panelLayout==null) // no, create
	   panelLayout=xSet.createLayout(name,new Point(200,200), new Dimension(200,75),1,false);
	   tooltip=new String("Launch run information panel");
	}
	// xiUserPanel interface functions
	public String getToolTip(){return tooltip;}
	public String getHeader(){return name;}
	public ImageIcon getIcon(){return menuIcon;}
	public xLayout checkLayout(){return panelLayout;}
	public xiUserCommand getUserCommand(){return null;}
/**
 * Called by constructor.
 * @param desktop Desktop interface to open the windows.
 * @param actionlistener Events can be passed to Desktop action listener.
 */
public void init(xiDesktop desktop, ActionListener actionlistener){
    desk=desktop;
    action=actionlistener;    // external DABC action listener 
// Text input fields
    // read defaults from setup file
    if(System.getenv("DABC_RUN_INFO")!=null)
         file = new String(System.getenv("DABC_RUN_INFO"));
    else file = new String("RunInformation.xml");
    formRun=new xFormRunInfo(file);
    formRun.addActionListener(this);
    Object o=xSet.addObject(formRun);
// Add prompter lines
    int width=30;
    run=addPrompt("Run [64]: ",formRun.getRun(),"set",width,this);
    exper=addPrompt("Experiment [64]: ",formRun.getExperiment(),"set",width,this);
//    line=addPrompt("Comment [76]: ",formRun.getComment(),"set",width,this);
    comar=addTextArea(formRun.getComment(),"set",5,width,this);
    comar.setFont(new Font("Monospaced",0,12));
    addTextButton("Save run information for file header","set","Use RET in text field to separate lines.",this);
}
/**
 * Release local references to DIM parameters and commands (xiUserPanel).
 * otherwise we would get memory leaks!
 */
public void releaseDimServices(){
 System.out.println("RunInfo releaseDimServices");
 mbsSetHead=null;
}
/**
 * Setup references to DIM parameters and commands (xiUserPanel).
 * Called after releaseDimServices() after every change in DIM services
 * @param browser DIM browser interface.
 */
public void setDimServices(xiDimBrowser browser){
 System.out.println("RunInfo setDimServices");
//get list of commands, look for generic MBS command
Vector<xiDimCommand> vicom=browser.getCommands();
for(int i=0;i<vicom.size();i++){
 if(vicom.get(i).getParserInfo().getFull().indexOf("/SetFileheader/MbsTransport")>0){
	 mbsSetHead=vicom.get(i);
	 System.out.println("RunInfo found command");
   break;
   }
}}
//---- Handle the menu actions ---------------------------------------
public void actionPerformed(ActionEvent e) {
String arg;
String line;
String subline;
int len, offset,chars;
String cmd = new String(e.getActionCommand());

if ("set".equals(cmd)) {
arg=new String(run.getText());
if(arg.length()>64)arg=arg.substring(0,64);
formRun.setRun(arg);
arg=new String(exper.getText());
if(arg.length()>64)arg=arg.substring(0,64);
formRun.setExperiment(arg);
//arg=new String(line.getText());
//if(arg.length()>76)arg=arg.substring(0,76);
arg=new String(comar.getText());
formRun.setComment(arg);
formRun.saveSetup(file);
if(mbsSetHead != null){
xLogger.print(1,"MBS: SetFileHeader -clear");
xLogger.print(1,"MBS: SetFileHeader -run \""+formRun.getRun()+"\"");
xLogger.print(1,"MBS: SetFileHeader -exp \""+formRun.getExperiment()+"\"");
mbsSetHead.exec(xSet.getAccess()+" x -clear");
mbsSetHead.exec(xSet.getAccess()+" x -file"); // dummy file name, will be ovewrwritten
mbsSetHead.exec(xSet.getAccess()+" \""+formRun.getRun()+"\" -run");
mbsSetHead.exec(xSet.getAccess()+" \""+formRun.getExperiment()+"\" -exp");
// break comment into lines
try{
StringTokenizer st = new StringTokenizer(arg,"\n\b\r");
int tokens=st.countTokens();
	for(int i=0;i<tokens;i++){
	line= st.nextToken();
	len=line.length();
	offset=0;
	chars=74;
	while(len > 0){
		if(chars>len)chars=len;
		subline=line.substring(offset,offset+chars);
		xLogger.print(1,"MBS: SetFileHeader -comm \""+subline+"\"");
		mbsSetHead.exec(xSet.getAccess()+" \""+subline+"\" -comm");
		len -= chars;
		offset += chars;
	}
	}}
catch(NoSuchElementException ee){System.out.println("EOF");}
}}
else System.out.println("Ignore "+cmd);
}
} // class xPanelRunInfo
