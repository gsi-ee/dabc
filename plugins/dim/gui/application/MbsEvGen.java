import xgui.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.ImageIcon;
import javax.swing.JTextField;
import javax.swing.JCheckBox;
import javax.swing.JPanel;
import javax.swing.JDesktopPane;
import javax.swing.JSplitPane;
import dim.*;

// To start event generators for MBS
public class MbsEvGen  extends xPanelPrompt            // provides some simple formatting
                        implements  xiUserPanel,        // needed by GUI
                                    ActionListener      // for panel input
{
private xRemoteShell mbsshell;
private String name, p1name;
private String tooltip;
private ActionListener action;
private JTextField send1,send2,send3;
private JTextField recv1,recv2,recv3,mbsComm, sysComm, userDir;
private JCheckBox single;
private xiDesktop desk;
private xiDimBrowser brows;
private xiDimCommand mbsCommand=null;
private xFormMbs formMbs;
private Color back;
private xLayout panlo;
private boolean paramOK=false;
private ActionListener MbsListener;
private ImageIcon winIcon, workIcon, menuIcon;

// Must have empty argument constructor!
public MbsEvGen(){
// Head line (title) inside window
super("MbsEventGenerator");
// Icon for the main menu and window decoration
menuIcon=xSet.getIcon("icons/usericongreen.png");
// name of window. This name will be used in layout XML file
name=new String("MbsEvGen");
// name for graphics windows. These names will be used in layout XML file
tooltip=new String("Launch event generator panel");
mbsshell = new xRemoteShell("rsh");
}

//----- xiUserPanel interface methods -----------------------
// tool tip for main window menu (xiUserPanel)
public  String getToolTip(){return tooltip;}
// Lettering for window decoration (xiUserPanel)
public  String getHeader(){return name;}
// Icon for the main menu and window decoration (xiUserPanel)
public  ImageIcon getIcon(){return menuIcon;}
//Layout
public xLayout checkLayout(){return panlo;}
//Command handler (xiUserPanel)
public xiUserCommand getUserCommand(){return null;} // or xiUserCommand class

// Build panel (xiUserPanel)
public void init(xiDesktop desktop, ActionListener actionlistener){
    desk=desktop;
    action=actionlistener;    // external DABC action listener 
    formMbs=(xFormMbs)xSet.getObject("xgui.xFormMbs");
    MbsListener=formMbs.getActionListener();
// Add prompter lines
	send1=addPrompt("Sender 1: ","R3G-2","send1",20,this);
	recv1=addPrompt("Receiver 1: ","X86-7","recv1",20,this);
	send2=addPrompt("Sender 2: ","R2f-2","send2",20,this);
	recv2=addPrompt("Receiver 2: ","X86G-4","recv2",20,this);
	send3=addPrompt("Sender 3: ","","send3",20,this);
	recv3=addPrompt("Receiver 3: ","","recv3",20,this);
	single=addCheckBox("One sender?","single",this);
	userDir=addPrompt("User path: ","v50/rio3/runmbs","user",20,this);
    mbsComm=addPrompt("MBS Command: ","","mbsComm",20,this);
    sysComm=addPrompt("System Command: ","","sysComm",20,this);
// Build button line
    winIcon     = xSet.getIcon("icons/rshmbs.png");
    workIcon    = xSet.getIcon("icons/controlmbs.png");
    addButton("mbsTest","Start event generators",winIcon,this);
    xLayout panlo=xSet.getLayout(name); // if not loaded from setup file, create new one
    // Point gives the position, Dimension the size, 1 column, boolean to pop up or not
    if(panlo == null) panlo=xSet.createLayout(name,new Point(100,100), new Dimension(100,75),1,true);
}

// Release local references to DIM parameters and commands (xiUserPanel)
// otherwise we would get memory leaks!
public void releaseDimServices(){
    mbsCommand=null;
}
// Setup references to DIM parameters and commands (xiUserPanel)
// Called after releaseDimServices() after every change in DIM services
public void setDimServices(xiDimBrowser browser){
// get list of commands, look for generic MBS command
Vector<xiDimCommand> vicom=browser.getCommands();
for(int i=0;i<vicom.size();i++){
    if(vicom.get(i).getParserInfo().getFull().indexOf("/MbsCommand")>0) {
    	mbsCommand=vicom.get(i);break;
    }}
}
//----- end of xiUserPanel interface methods -----------------------

// ---- Handle the menu actions ---------------------------------------
public void actionPerformed(ActionEvent e) {
System.out.println("MbsEvGen: "+e.getActionCommand());
boolean sin=single.isSelected();
//-----------
if ("mbsTest".equals(e.getActionCommand())) {
        String cmd = new String(formMbs.getSystemPath()+
                                "/script/remote_exe.sc "+
                                formMbs.getSystemPath()+" "+
                                userDir.getText()+
                                " m_evgen_RIO2 "+userDir.getText()+" . "+recv1.getText()
                                );
        mbsshell.rsh(send1.getText(),xSet.getUserName(),cmd,0L);
        cmd = new String(formMbs.getSystemPath()+
                                "/script/remote_exe.sc "+
                                formMbs.getSystemPath()+" "+
                                userDir.getText()+
                                " m_evgen_RIO2 "+userDir.getText()+" . "+recv2.getText()
                                );
        mbsshell.rsh(send2.getText(),xSet.getUserName(),cmd,0L);
}
else if ("sysComm".equals(e.getActionCommand())) {
    mbsshell.rsh(formMbs.getMaster(),xSet.getUserName(),sysComm.getText(),0L);
}
else if ("mbsComm".equals(e.getActionCommand())) {
	if(mbsCommand != null) 
	    mbsCommand.exec(xSet.getAccess()+" "+mbsComm.getText());
}
 else {
	System.out.println("Move");
	MbsListener.actionPerformed(e);
} }
}
