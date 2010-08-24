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

// Could also extend JPanel
public class RocPanel  extends xPanelPrompt            // provides some simpple formatting
                        implements  xiUserPanel,        // needed by GUI
                                    xiUserInfoHandler,  // neewded by xDimParameter DIM client
                                    xiUserCommand,      // optional to specify command formats
                                    ActionListener      // for panel input
{
private String name, p1name;
private String tooltip;
private ActionListener action;
private JTextField prompt1;
private JCheckBox check1;
private xiDesktop desk;
private xiDimBrowser brows;
private xInternalCompound frame; 
private int dividersize=4;
private xPanelGraphics metpan, stapan, infpan;
private xFormDabc formDabc;
private xiDimParameter paraServer;;
private xRate rate;
private xMeter meter;
private xState state, stateServer;
private xInfo info;
private xRecordMeter meterrec;
private xRecordState staterec;
private xRecordInfo inforec;
private Vector<xRecordMeter> meterrecs;
private Vector<xRecordState> staterecs;
private Vector<xRecordInfo> inforecs;
private Vector<xRate> rates; // alternative to meters
private Vector<xMeter> meters; // alternative to rates
private Vector<xState> states;
private Vector<xInfo> infos;
private Color back;
private xLayout panlo, p1lo;
private Point panpnt, p1pnt;
private boolean paramOK=false;
private int counter=0;
private ImageIcon   menuIcon, 
                    launchIcon, 
                    killIcon,  
                    configIcon, 
                    startIcon, 
                    stopIcon, 
                    graphIcon, 
                    enableIcon, 
                    haltIcon, 
                    exitIcon;
private ActionListener DabcListener;

// Must have empty argument constructor!
public RocPanel(){
// Head line (title) inside window
super("RocSetup");
// Icon for the main menu and window decoration
menuIcon=xSet.getIcon("icons/usericonblue.png");
// name of window. This name will be used in layout XML file
name=new String("RocPanel");
// name for graphics windows. These names will be used in layout XML file
p1name=new String("RocMonitor");
back = new Color(0.0f, 0.2f, 0.0f); // overall background
tooltip=new String("Launch ROC panel");
}

//----- xiUserPanel interface methods -----------------------
// tool tip for main window menu (xiUserPanel)
public  String getToolTip(){return tooltip;}
// Lettering for window decoration (xiUserPanel)
public  String getHeader(){return name;}
// Icon for the main menu and window decoration (xiUserPanel)
public  ImageIcon getIcon(){return menuIcon;}
// Layout
public xLayout checkLayout(){return panlo;}

// Command handler (xiUserPanel)
// xiUserCommand must implement getArgumentStyleXml (see below))
// With a returned xiUserCommand we can steer the command formatting
// Here we do not change it:
public xiUserCommand getUserCommand(){return null;} // this or xiUserCommand class

// Build panel (xiUserPanel)
public void init(xiDesktop desktop, ActionListener actionlistener){
    desk=desktop;
    action=actionlistener;    // external DABC GUI action listener 
    formDabc=(xFormDabc)xSet.getObject("xgui.xFormDabc");
    DabcListener=formDabc.getActionListener(); // DABC launch action listener
    // Create layout for this panel
    xLayout panlo=xSet.getLayout(name); // if not loaded from setup file, create new one
    // Point gives the position, Dimension the size, 1 column, boolean to pop up or not
    if(panlo == null) panlo=xSet.createLayout(name,new Point(100,100), new Dimension(100,75),1,true);
// Add prompter lines
    prompt1=addPrompt("ROC Command: ","","ROC",20,this);
// Add text buttons
    addTextButton("This is a test button","command1","whatever it does",this);
// Add checkboxes
    check1=addCheckBox("Data server","check1",this);
// Build button line
    launchIcon  = xSet.getIcon("icons/connprm.png");
    killIcon    = xSet.getIcon("icons/disconn.png");
    configIcon  = xSet.getIcon("icons/dabcconfig.png");
    enableIcon  = xSet.getIcon("icons/dabcenable.png");
    startIcon   = xSet.getIcon("icons/dabcstart.png");
    stopIcon    = xSet.getIcon("icons/dabcstop.png");
    haltIcon    = xSet.getIcon("icons/dabchalt.png");
    exitIcon    = xSet.getIcon("icons/exitall.png");
    graphIcon   = xSet.getIcon("icons/windowred.png");
    addButton("dabcLaunch","Launch DABC",launchIcon,this);
    addButton("dabcConfig","Configure DABC",configIcon,this);
    addButton("dabcEnable","Enable DABC",enableIcon,this);
    addButton("dabcStart","Start",startIcon,this);
    addButton("dabcStop","Stop",stopIcon,this);
    addButton("dabcHalt","Halt",haltIcon,this);
    addButton("dabcExit","Exit and shut down",killIcon,this);
    addButton("dabcCleanup","Cleanup DABC",exitIcon,this);
    addButton("userDisp","Display",graphIcon,this);
    stateServer = new xState("DataServer", xState.XSIZE,xState.YSIZE);
    stateServer.setColorBack(back);

// create graphics panels, will be placed into compound frame p1
    stapan=new xPanelGraphics(new Dimension(160,50),1); // one column of states
    metpan=new xPanelGraphics(new Dimension(410,14),1); // one columns of meters
    infpan=new xPanelGraphics(new Dimension(410,14),1); // one column of info lines
    stapan.setColorBack(back);
    infpan.setColorBack(back);
    metpan.setColorBack(back);
    // specify layout of frame. Is controlled by topdivisions and number of panels in rebuild():
    // topdivisions = 0:
    // 2 panels: left p1 right p2 
    // 3 panels: left p1, right top p2 right bottom p3 
    // topdivisions = 1:
    // 2 panels: top p1 bottom p2 
    // 3 panels: top p1, bottom left p2 bottom right p3 
    // topdivisions = 2:
    // 2 panels: top p1 bottom p2 
    // 3 panels: top left p1, top right p2 bottom p3 
    int topdivisions=0;
    p1pnt = new Point(topdivisions,0); // layout for compound
// store position and size of graphics frame
    p1lo=xSet.getLayout(p1name); // if not loaded from setup file create new one
    if(p1lo == null)p1lo=xSet.createLayout(p1name,new Point(400,400), new Dimension(100,75),1,true);
// create the compound for the graphics panels
    if(p1lo.show()){
        frame=new xInternalCompound(p1name,graphIcon, 0, p1lo, back);
        dividersize=frame.getDividerSize();
        desk.addFrame(frame);
    }
    }

// Release local references to DIM parameters and commands (xiUserPanel)
// otherwise we would get memory leaks!
public void releaseDimServices(){
    paramOK=false; // deactivate infoHandler
    System.out.println("ROC release services");
    metpan.cleanup();
    stapan.cleanup();
    infpan.cleanup();
    meters=null;
    states=null;
    infos=null;
    rates=null;
    meterrecs=null;
    inforecs=null;
    staterecs=null;
    paraServer=null;
}
// Setup references to DIM parameters and commands (xiUserPanel)
// Called after releaseDimServices() after every change in DIM services
public void setDimServices(xiDimBrowser browser){
int len1, len2;
xiParser p;
brows=browser;
// paramOK ist false when we enter here
boolean OK=true;
// graphics
rates=new Vector<xRate>();
meters=new Vector<xMeter>();
states=new Vector<xState>();
infos=new Vector<xInfo>();
// and data objects
meterrecs=new Vector<xRecordMeter>();
staterecs=new Vector<xRecordState>();
inforecs=new Vector<xRecordInfo>();
String full;
String last=new String("*");
String head;
String name;
// get list of commands, look for generic MBS command
Vector<xiDimCommand> vicom=browser.getCommands();
// get list of parameters
Vector<xiDimParameter> vipar=browser.getParameters();
// Get reference to parameters
for(int i=0;i<vipar.size();i++){
    p=vipar.get(i).getParserInfo();
    full=p.getFull();
    if(full.indexOf("/DumpMode.RocPlugin")>0) paraServer=vipar.get(i);
    // if(full.indexOf("CfgNodeId.BnetPlugin")>0) paraServer=vipar.get(i);
    if(p.isRate()){ // with this, we take all meters
        head = new String(p.getNode()+"/"+p.getApplication());
        name = new String(p.getName()+"["+vipar.get(i).getMeter().getUnits()+"]");
        rate=new xRate(head, name, !head.equals(last),
            xRate.XSIZE,xRate.YSIZE,
            vipar.get(i).getMeter().getLower(),
            vipar.get(i).getMeter().getUpper()
            ,"Green");
        rate.setColorBack(back);
        last = new String(head);
        rates.add(rate);
        metpan.addGraphics(rate,false); // do not update immediately, better after all has been added
        meterrecs.add(vipar.get(i).getMeter());
        infoHandler(vipar.get(i)); // to get current parameter values
        brows.addInfoHandler(vipar.get(i),this); // register this xiUserInfoHandler
    }
    if(p.isState()){ // with this, we take all states
        if(p.getName().equals("RunStatus"))
             state=new xState(new String(p.getNodeName()+":"+p.getApplicationName()),xState.XSIZE,xState.YSIZE);
        else state=new xState(new String(p.getNodeName()+":"+p.getName()),xState.XSIZE,xState.YSIZE);
        states.add(state); // add to local list
        state.setColorBack(back);
        // add reference to record data to local list
        staterecs.add(vipar.get(i).getState());
        stapan.addGraphics(state,false); // add to graphics panel
        infoHandler(vipar.get(i)); // to get current parameter values
        brows.addInfoHandler(vipar.get(i),this); // register this xiUserInfoHandler
    }
    } // list of parameters
stapan.addGraphics(stateServer,false); // add to graphics panel
// now update all graphics
infpan.updateAll();
stapan.updateAll();
metpan.updateAll();
// in this example we used Rates instead of Meters
//if(meters.size() != meterrecs.size()){OK=false;System.out.println("Error meters");}
//if(infos.size() != inforecs.size()){OK=false;System.out.println("Error infos");}
if(states.size() != staterecs.size()){OK=false;System.out.println("ROC Error states");}
//if(meters.size() == 0){OK=false;System.out.println("No meters");}
//if(infos.size() == 0){OK=false;System.out.println("No infos");}
if(states.size() == 0){OK=false;System.out.println("ROC No states");}
if(rates.size() == 0){OK=false;System.out.println("ROC No rates");}
// Build up the frame with the graphics panels  
if(frame != null) frame.rebuild(stapan, metpan);
paramOK=OK;
System.out.println("ROC services OK: "+paramOK);
}
//----- end of xiUserPanel interface methods -----------------------

// ---- xiUserInfoHandler interface methods ----------------------------
public String getName(){return name;}
// call back for parameter update (xiUserInfoHandler))
//NOTE that this infoHandler is invoked in the Java event loop.
//This means, that graphics actions can be done, but be sure not to
//block by waiting for something which is provided by another event function (dead lock)
public void infoHandler(xiDimParameter param){
// System.out.println("Param: "+par.getParserInfo().getFull()+", value "+par.getRecordMeter().getValue());

// look which meter to update
if((param.getMeter() != null)&&(meterrecs!=null)&&(rates!=null))
{ // has it a meter object?
for(int i=0;i < meterrecs.size(); i++){
    if(meterrecs.get(i)==param.getMeter()){
    //System.out.println("Rate: "+meterrecs.get(i).getName()+" = "+meterrecs.get(i).getValue());
    rates.get(i).redraw((double)meterrecs.get(i).getValue(), true);
    }}}
// look which state to update
else if((param.getState() != null)&&(staterecs!=null)&&(states!=null))
{ // has it a state object?
for(int i=0;i < staterecs.size(); i++){
    if(staterecs.get(i)==param.getState()){
    //System.out.println("State: "+staterecs.get(i).getName()+" = "+staterecs.get(i).getValue());
    states.get(i).redraw(staterecs.get(i).getSeverity(),
                                staterecs.get(i).getColor(),
                                staterecs.get(i).getValue(), true);
    }}}
}
// ---- end of xiUserInfoHandler interface methods ----------------------------

// ---- xiUserCommand interface methods -----------------------------
// return the command style (xiUserCommand)
public boolean getArgumentStyleXml(String scope, String command)
{
//System.out.println("User: Scope="+scope+" Command="+command);
return false; // do not format commands in XML, might be true only for MBS commands
}
// ---- end of xiUserCommand interface methods

// ---- Handle the menu actions ---------------------------------------
public void actionPerformed(ActionEvent e) {
// Text input
if ("command1".equals(e.getActionCommand())) {
System.out.println(e.getActionCommand()+":"+prompt1.getText()+" "+check1.isSelected());
}
else if ("ROC".equals(e.getActionCommand())) {
System.out.println(e.getActionCommand()+":"+prompt1.getText()+" "+check1.isSelected());
}
else if ("check1".equals(e.getActionCommand())) {
System.out.println("Data server "+check1.isSelected());
counter++;
if(check1.isSelected()){
if(paraServer != null)paraServer.setParameter("0");
stateServer.redraw(0,"Green","Transport",true);
} else {
if(paraServer != null)paraServer.setParameter("1");
stateServer.redraw(0,"Gray","None",true);
}
}
// open display
else if ("userDisp".equals(e.getActionCommand())) {
if(!desk.findFrame(p1name)){
    frame=new xInternalCompound(p1name,graphIcon, 0, xSet.getLayout(p1name), back);
    frame.rebuild(stapan, metpan); 
    desk.addFrame(frame); // old frame will be deleted
}}
//-----------
else { // route to PanelDabc
DabcListener.actionPerformed(e);
}
}
}
