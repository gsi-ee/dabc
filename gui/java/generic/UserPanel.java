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
public class UserPanel  extends xPanelPrompt            // provides some simpple formatting
                        implements  xiUserPanel,        // needed by GUI
                                    xiUserInfoHandler,  // neewded by xDimParameter DIM client
                                    xiUserCommand,      // optional to specify command formats
                                    ActionListener      // for panel input
{
private ImageIcon storeIcon, menuIcon, closeIcon, startIcon, stopIcon, graphIcon, colorIcon;
private String name, p1name, p2name;
private String tooltip;
private ActionListener action;
private JTextField prompt1,prompt2,prompt3;
private JCheckBox check1,check2;
private xiDesktop desk;
private xiDimBrowser brows;
private xInternalCompound frame, color; 
private int dividersize=4;
private xiDimCommand mbsCommand=null;
private xPanelGraphics metpan, stapan, infpan;
private JPanel colpan;
private xRate rate;
private xMeter meter;
private xState state;
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
private xLayout panlo, p1lo, p2lo;
private Point panpnt, p1pnt, p2pnt;
private boolean paramOK=false;

// Must have empty argument constructor!
public UserPanel(){
// Head line (title) inside window
super("UserSetup");
// Icon for the main menu and window decoration
menuIcon=xSet.getIcon("icons/usericon.png");
// name of window. This name will be used in layout XML file
name=new String("UserPanel");
// name for graphics windows. These names will be used in layout XML file
p1name=new String("UserMonitor");
p2name=new String("UserColor");
back = new Color(0.0f, 0.2f, 0.0f); // overall background
tooltip=new String("Launch user panel");
}

//----- xiUserPanel interface methods -----------------------
// tool tip for main window menu (xiUserPanel)
public  String getToolTip(){return tooltip;}
// Lettering for window decoration (xiUserPanel)
public  String getHeader(){return name;}
// Icon for the main menu and window decoration (xiUserPanel)
public  ImageIcon getIcon(){return menuIcon;}

// Command handler (xiUserPanel)
public xiUserCommand getUserCommand(){return this;} // or xiUserCommand class

// Build panel (xiUserPanel)
public void init(xiDesktop desktop, ActionListener actionlistener){
    desk=desktop;
    action=actionlistener;    // external DABC action listener 
    xLayout panlo=xSet.getLayout(name); // if not loaded from setup file, create new one
    // Point gives the position, Dimension the size, 1 column, boolean to pop up or not
    if(panlo == null) panlo=xSet.createLayout(name,new Point(100,100), new Dimension(100,75),1,true);
// Add prompter lines
    prompt1=new JTextField(20);
    prompt1.addActionListener(this);
    addPrompt("MBS Command: ",prompt1);
// Add checkboxes
    check1 = new JCheckBox();
    addCheckBox("Startup on start",check1);
    check2 = new JCheckBox();
    addCheckBox("Shutdown on stop",check2);
// Build button line
    startIcon   = xSet.getIcon("icons/mbsstart.png");
    stopIcon    = xSet.getIcon("icons/mbsstop.png");
    closeIcon   = xSet.getIcon("icons/fileclose.png");
    storeIcon   = xSet.getIcon("icons/savewin.png");
    graphIcon   = xSet.getIcon("icons/usergraphics.png");
    colorIcon   = xSet.getIcon("icons/colormap.png");
    addButton("userQuit","Close window",closeIcon,this);
    addButton("userStore","Store",storeIcon,this);
    addButton("userDisp","Display",graphIcon,this);
    addButton("userColor","Color map",colorIcon,this);
    addButton("userStart","Start event receiver",startIcon,this);
    addButton("userStop","Stop event receiver",stopIcon,this);
// create graphics panels, will be placed into compound frame p1
    stapan=new xPanelGraphics(new Dimension(160,50),1); // one column of states
    metpan=new xPanelGraphics(new Dimension(410,14),1); // one columns of meters
    infpan=new xPanelGraphics(new Dimension(410,14),1); // one column of info lines
    stapan.setColorBack(back);
    infpan.setColorBack(back);
    metpan.setColorBack(back);
    int upperdivisions=0;
    int lowerdivisions=1;
    p1pnt = new Point(0,1); // layout for compound: upper, lower
// store position and size of graphics frame
    p1lo=xSet.getLayout(p1name); // if not loaded from setup file create new one
    if(p1lo == null)p1lo=xSet.createLayout(p1name,new Point(400,400), new Dimension(100,75),1,true);
// create the compound for the graphics panels
    if(p1lo.show()){
        frame=new xInternalCompound(p1name,graphIcon, p1pnt, p1lo, back);
        dividersize=frame.getDividerSize();
        desk.addFrame(frame);
        frame.moveToFront();
    }
// create graphics panels, will be placed into compound frame p2
    colpan=new ColorTest();
    p2pnt = new Point(0,1); // layout for compound
// store position and size of graphics frame
    p2lo=xSet.getLayout(p2name); // if loaded from setup file or create new one
    if(p2lo == null)p2lo=xSet.createLayout(p2name,new Point(400,500), new Dimension(100,75),1,true);
// create the compound for the graphics panels
    if(p2lo.show()){
        color=new xInternalCompound(p2name,colorIcon, p2pnt, p2lo, back);
        color.rebuild(colpan); // top 2, bottom 1
        dividersize=color.getDividerSize();
        desk.addFrame(color);
        color.moveToFront();
    }
    }

// Release local references to DIM parameters and commands (xiUserPanel)
// otherwise we would get memory leaks!
public void releaseDimServices(){
    paramOK=false; // deactivate infoHandler
    System.out.println("User release services");
    mbsCommand=null;
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
for(int i=0;i<vicom.size();i++){
    if(vicom.get(i).getParserInfo().getFull().indexOf("/MbsCommand")>0) {mbsCommand=vicom.get(i);break;}
    }
// get list of parameters
Vector<xiDimParameter> vipar=browser.getParameters();
// Get reference to parameters
for(int i=0;i<vipar.size();i++){
    p=vipar.get(i).getParserInfo();
    full=p.getFull();
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
    }
    if(p.isState()){ // with this, we take all states
        state=new xState(new String(p.getNodeName()+":"+p.getApplicationName()),xState.XSIZE,xState.YSIZE);
        states.add(state); // add to local list
        state.setColorBack(back);
        // add reference to record data to local list
        if(vipar.get(i).getState()!=null) staterecs.add(vipar.get(i).getState());
        stapan.addGraphics(state,false); // add to graphics panel
    }
//    if((full.indexOf("/Eventbuilder:42/Events.")>0)||(full.indexOf("/EventRate")>0)) {
    if((full.indexOf("xx")>0)) { // here we could select by name
        len1=p.getNodeName().indexOf(".");
        if(len1 == -1)len1=p.getNodeName().length();
        len2=p.getName().indexOf(".");
        if(len2 == -1)len2=p.getName().length();
        meter=new xMeter(xMeter.ARC,
                new String(p.getNodeName().substring(0,len1)+":"+p.getName().substring(0,len2)),
                            0.0,10.0,xMeter.XSIZE,xMeter.YSIZE,new Color(1.0f,0.0f,0.0f));
        meter.setColorBack(back);
        meters.add(meter); // add to local list
        // add reference to record data to local list
        if(vipar.get(i).getMeter()!=null) {
            meter.setMode(vipar.get(i).getMeter().getMode()); // set style from parameter
            meter.setLimits((double)vipar.get(i).getMeter().getLower(), (double)vipar.get(i).getMeter().getUpper());
            meterrecs.add(vipar.get(i).getMeter());
            }
        metpan.addGraphics(meter,false); // add to graphics panel
        // System.out.println("Found Rate: "+full);
        }
    //else if(full.indexOf("/Acquisition/")>0) {
    else if(full.indexOf("xxx")>0) { // here we could select by name
        state=new xState(new String(p.getNodeName()+":"+p.getApplicationName()),xState.XSIZE,xState.YSIZE);
        state.setColorBack(back);
        states.add(state); // add to local list
        // add reference to record data to local list
        if(vipar.get(i).getState()!=null) staterecs.add(vipar.get(i).getState());
        stapan.addGraphics(state,false); // add to graphics panel
        // System.out.println("Found State: "+full);
        }
    else if((full.indexOf("/eFile")>0)||(full.indexOf("/ePerform")>0)) { // here we could select by name
        info=new xInfo(new String(p.getNodeName()+":"),
                                    dividersize+xInfo.XSIZE,xInfo.YSIZE);
        info.setColorBack(back);
        infos.add(info); // add to local list
        // add reference to record data to local list
        if(vipar.get(i).getInfo()!=null) inforecs.add(vipar.get(i).getInfo());
        infpan.addGraphics(info,false); // add to graphics panel
        // System.out.println("Found Info: "+full);
        }
    infoHandler(vipar.get(i)); // to get current parameter values
    brows.addInfoHandler(vipar.get(i),this); // register this xiUserInfoHandler
    } // list of parameters
// now update all graphics
infpan.updateAll();
stapan.updateAll();
metpan.updateAll();
// in this example we used Rates instead of Meters
//if(meters.size() != meterrecs.size()){OK=false;System.out.println("Error meters");}
if(infos.size() != inforecs.size()){OK=false;System.out.println("Error infos");}
if(states.size() != staterecs.size()){OK=false;System.out.println("Error states");}
//if(meters.size() == 0){OK=false;System.out.println("No meters");}
if(infos.size() == 0){OK=false;System.out.println("No infos");}
if(states.size() == 0){OK=false;System.out.println("No states");}
if(rates.size() == 0){OK=false;System.out.println("No rates");}
// Build up the frame with the graphics panels  
if(frame != null) frame.rebuild(stapan, metpan, infpan); // top 2, bottom 1
// frame.rebuild(infpan, stapan, metpan); // top 1, bottom 2
// frame.rebuild(stapan, metpan); // top 1, bottom 1
paramOK=OK;
System.out.println("User services OK: "+paramOK);
}
//----- end of xiUserPanel interface methods -----------------------

// ---- xiUserInfoHandler interface methods ----------------------------
// call back for parameter update (xiUserInfoHandler))
public void infoHandler(xiDimParameter param){
// System.out.println("Param: "+par.getParserInfo().getFull()+", value "+par.getRecordMeter().getValue());

// if(paramOK){
// look which meter to update
if((param.getMeter() != null)&&(meterrecs!=null)&&(rates!=null))
{ // has it a meter object?
for(int i=0;i < meterrecs.size(); i++){
    if(meterrecs.get(i)==param.getMeter()){
    //System.out.println("Rate: "+meterrecs.get(i).getName()+" = "+meterrecs.get(i).getValue());
    //meters.get(i).redraw((double)meterrecs.get(i).getValue());
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
// look which info to update
else if((param.getInfo() != null)&&(inforecs!=null)&&(infos!=null))
{ // has it an info object?
for(int i=0;i < inforecs.size(); i++){
    if(inforecs.get(i)==param.getInfo()){
    //System.out.println("Info: "+inforecs.get(i).getName()+" = "+inforecs.get(i).getValue());
    infos.get(i).redraw(inforecs.get(i).getSeverity(),
                                inforecs.get(i).getColor(),
                                inforecs.get(i).getValue(), true);
    }}}
//} 
//else System.out.println("User update failed");
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
//-----------
if ("userQuit".equals(e.getActionCommand())) {
}
//-----------
else if ("userStore".equals(e.getActionCommand())) {
String s = new String("Hello, Test only");
tellInfo(s);
}
//-----------
else if ("userStart".equals(e.getActionCommand())) {
if(mbsCommand != null) {
    if(check1.isSelected()) {
        mbsCommand.exec("@startup");
        mbsCommand.exec(
        "rsh x86g-4 -l goofy /daq/usr/goofy/mbswork/v51/script/remote_exe.sc /daq/usr/goofy/mbswork/v51/ v50/x86/newmbs m_rising v50/x86/newmbs . x86g-4 x86-7");
        }
    else mbsCommand.exec("*::Start rirec");
    }
else tellError("Command not available!");
}
//-----------
else if ("userStop".equals(e.getActionCommand())) {
if(mbsCommand != null) {
    if(check2.isSelected()) mbsCommand.exec("@shutdown");
    else mbsCommand.exec("*::Stop rirec");
    }
else tellError("Command not available!");
}
//-----------
else if ("userDisp".equals(e.getActionCommand())) {
    frame=new xInternalCompound(p1name,graphIcon, p1pnt, xSet.getLayout(p1name), back);
    frame.rebuild(stapan, metpan, infpan); // top 2, bottom 1
    desk.addFrame(frame); // old frame will be deleted
    frame.moveToFront();
}
else if ("userColor".equals(e.getActionCommand())) {
    color=new xInternalCompound(p2name,colorIcon, p2pnt, xSet.getLayout(p2name),back);
    color.rebuild(colpan); // top 2, bottom 1
    colpan.repaint();
    desk.addFrame(color); // old frame will be deleted
    color.moveToFront();
}
//-----------
else {
System.out.println(prompt1.getText());
if(mbsCommand != null) mbsCommand.exec(prompt1.getText());
}
}
}
