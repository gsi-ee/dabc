package xgui;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.ImageIcon;
import javax.swing.JTextField;
import javax.swing.JCheckBox;
import javax.swing.JPanel;
import javax.swing.JDesktopPane;
import javax.swing.JSplitPane;
import org.w3c.dom.*;
import dim.*;
/**
 * Panel for display of selected list of parameters.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xPanelSelect  extends xPanelPrompt         // provides some simple formatting
                        implements  xiUserInfoHandler,  // needed by xDimParameter DIM client
                                    ActionListener      // for panel input
{
private xRemoteShell mbsshell;
private String Name, p1name;
private String tooltip;
private ActionListener action;
private JTextField wildfull, wildnode, wildapp, wildname;
private JCheckBox checkfull, checkonly, checknode, checkapp, checkname, checkrate, checkstate, checkinfo;
private xiDesktop desk;
private xInternalCompound frame; 
private xiDimBrowser brows;
private xiDimCommand mbsCommand=null;
private xPanelPrompt parpan;
private Vector<xiDimParameter> parlist;
private Vector<myParameter> valist;
private xTimer tim;
private Color back;
private xLayout panlo, p1lo;
private Point panpnt;
private boolean paramOK=false, select=false;
private ActionListener MbsListener;
private ImageIcon storeIcon, graphIcon, menuIcon; 
private String sfull, snode, sapp, sname;
private boolean only, full, node, app, name, rate, state, info, show;
/**
 * Creates panel to specify filters for parameters and display a filtered parameter list.
 * @param file XML file to restore values (Selection.xml).
 * @param title Title of panel.
 * @param browser DIM browser interface. 
 * @param desktop Desktop interface to open the windows.
 * @param actionlistener Events can be passed to Desktop action listener.
 */
public xPanelSelect(String file, String title, xiDimBrowser browser, xiDesktop desktop, ActionListener actionlistener){
// Head line (title) inside window
super(title);
brows=browser; // store browser
// Icon for the main menu and window decoration
menuIcon=xSet.getIcon("icons/usericon.png");
// name of window. This name will be used in layout XML file
Name=new String(title);
// name for graphics windows. These names will be used in layout XML file
p1name=new String("ParameterList");
back = new Color(0.0f, 0.2f, 0.0f); // overall background
tooltip=new String("Launch parameter selection panel");
restoreSetup(file);
init(desktop,actionlistener);
}
/**
 * Called by constructor.
 * @param desktop Desktop interface to open the windows.
 * @param actionlistener Events can be passed to Desktop action listener.
 */
private void init(xiDesktop desktop, ActionListener actionlistener){
    xSet.setColorBack(back);
    desk=desktop;
    action=actionlistener;    // external DABC action listener 
// Add prompter lines
int width=14;
    wildfull=addPrompt("Full name: ",sfull,"filter",width,this);
    wildnode=addPrompt("Node: ",snode,"filter",width,this);
    wildapp=addPrompt("Application: ",sapp,"filter",width,this);
    wildname=addPrompt("Name: ",sname,"filter",width,this);
// Add checkboxes
    checkfull=addCheckBox("Check full: ","filter",this);checkfull.setSelected(full);
    checknode=addCheckBox("Check nodes: ","filter",this);checknode.setSelected(node);
    checkapp=addCheckBox("Check appl.: ","filter",this);checkapp.setSelected(app);
    checkname=addCheckBox("Check names: ","filter",this);checkname.setSelected(name);
    checkonly=addCheckBox("Below only: ","filter",this);checkonly.setSelected(only);
    checkrate=addCheckBox("Rates: ","filter",this);checkrate.setSelected(rate);
    checkstate=addCheckBox("States: ","filter",this);checkstate.setSelected(state);
    checkinfo=addCheckBox("Infos: ","filter",this);checkinfo.setSelected(info);
// Build button line
    graphIcon   = xSet.getIcon("icons/usergraphics.png");
    storeIcon   = xSet.getIcon("icons/savewin.png");
    addButton("Store","Store selection panel content",storeIcon,this);
    addButton("Display","Display parameter list",menuIcon,this);
// store position and size of graphics frame
    p1lo=xSet.getLayout(p1name); // if not loaded from setup file create new one
    if(p1lo == null)p1lo=xSet.createLayout(p1name,new Point(100,400), new Dimension(800,200),1,true);
// create the compound for the graphics panels
    if(p1lo.show()){
        int topdivisions=0;
        frame=new xInternalCompound(p1name, menuIcon, topdivisions, p1lo, back);
        desk.addFrame(frame);
    }
}
/**
 * Release local references to DIM parameters and commands (xiUserPanel)
 * otherwise we would get memory leaks!
 */
public void releaseDimServices(){
//System.out.println("Selection releaseDimServices");
if((parlist != null)&(valist != null)){
if(valist.size() > 0)
    for(int i=0;i<parlist.size();i++)
        brows.removeInfoHandler(parlist.get(i),valist.get(0)); // unregister xiUserInfoHandler
}}
/**
 * Setup references to DIM parameters and commands (xiUserPanel)
 * Called after releaseDimServices() after every change in DIM services
 */
public void setDimServices(){
parlist=brows.getParameters(); // get parameter list
// create graphics panels, will be placed into compound frame
parpan=new xPanelPrompt("Selected parameter list, type RET on white fields to enter value.", new Dimension(800,400));
// Get references to selected parameter and build parpan
buildList();
//System.out.println("Selection setDimServices");
// Build up the frame with the graphics panels  
if(frame != null) frame.rebuild(parpan); 
}
/**
 * Call back for parameter update (xiUserInfoHandler).
 */
public void infoHandler(xiDimParameter param){
}

/**
 * Build parameter panel from parameter list with filters.
 */
private void buildList(){
xiParser xip;
myParameter mypar;
String fulnam;
boolean isRecord;
    releaseDimServices();
    valist = new Vector<myParameter>(); // vector of selected parameters
    sfull=new String(wildfull.getText());
    snode=new String(wildnode.getText());
    sapp=new String(wildapp.getText());
    sname=new String(wildname.getText());
    only=checkonly.isSelected();
    full=checkfull.isSelected();
    node=checknode.isSelected();
    app=checkapp.isSelected();
    name=checkname.isSelected();
    rate=checkrate.isSelected();
    state=checkstate.isSelected();
    info=checkinfo.isSelected();
    JTextField nf=parpan.addPrompt("Selected ","0","X",30,this);
    int num=0;
    for(int i=0;i<parlist.size();i++){
        xip=parlist.get(i).getParserInfo();
        if(xip.isCommandDescriptor())continue;
        isRecord=xip.isRate()|xip.isState()|xip.isInfo();
        if(only & !isRecord) continue;
        if(xip.isRate() & !rate) continue;
        if(xip.isState() & !state)continue;
        if(xip.isInfo() & !info)continue;
        fulnam=xip.getFull();
        if(full & !fulnam.contains(sfull)) continue;
        if(node & !xip.getNode().contains(snode)) continue;
        if(app & !xip.getApplication().contains(sapp)) continue;
        if(name & !xip.getName().contains(sname)) continue;
        if(parlist.get(i).getValue().length()>50)continue;
        num++;
        infoHandler(parlist.get(i)); // to get current parameter values
        mypar = new myParameter("ParaList",parpan); // will handle infoHandler
        if(xip.isChangable()) // handle events by actionPerformed of myParameter
            mypar.setField(parpan.addPrompt(new String(fulnam+" "),parlist.get(i).getValue(),fulnam,30,mypar),true);
        else // handle events by actionPerformed of xPanelSelect (and ignore)
            mypar.setField(parpan.addPrompt(new String(fulnam+" "),parlist.get(i).getValue(),fulnam,30,this),false);
        brows.addInfoHandler(parlist.get(i),mypar); // register this xiUserInfoHandler
        valist.add(mypar);
    } // list of parameters
    nf.setText(new Integer(num).toString());
    nf.setEditable(false);
}
/**
 * Reads Xml file and restores filter setup values.
 * @param file Xml file name.
 */
public void restoreSetup(String file){
NodeList li;
Element el;
String att;
System.out.println("Restore selections from "+file);
    Element root=xXml.read(file);
    if(root != null){
        li=root.getElementsByTagName("Records");
        el=(Element)li.item(0);
        only=new Boolean(el.getAttribute("Only")).booleanValue();
        rate=new Boolean(el.getAttribute("Rates")).booleanValue();
        state=new Boolean(el.getAttribute("States")).booleanValue();
        info=new Boolean(el.getAttribute("Infos")).booleanValue();
        li=root.getElementsByTagName("Full");
        el=(Element)li.item(0);
        sfull=el.getAttribute("contains");
        full=new Boolean(el.getAttribute("filter")).booleanValue();
        li=root.getElementsByTagName("Node");
        el=(Element)li.item(0);
        snode=el.getAttribute("contains");
        node=new Boolean(el.getAttribute("filter")).booleanValue();
        li=root.getElementsByTagName("Application");
        el=(Element)li.item(0);
        sapp=el.getAttribute("contains");
        app=new Boolean(el.getAttribute("filter")).booleanValue();
        li=root.getElementsByTagName("Name");
        el=(Element)li.item(0);
        sname=el.getAttribute("contains");
        name=new Boolean(el.getAttribute("filter")).booleanValue();
    }
}
/**
 * Writes Xml file with filter setup values.
 * @param file Xml file name.
 */
public void saveSetup(String file){
System.out.println("Store selections to "+file);
    StringBuffer str=new StringBuffer();
    str.append(xXml.header());
    str.append(xXml.tag("Selection",xXml.OPEN));
    str.append("<Full "+xXml.attr("contains",wildfull.getText())+xXml.attr("filter",checkfull.isSelected())+"/>\n");
    str.append("<Node "+xXml.attr("contains",wildnode.getText())+xXml.attr("filter",checknode.isSelected())+"/>\n");
    str.append("<Application "+xXml.attr("contains",wildapp.getText())+xXml.attr("filter",checkapp.isSelected())+"/>\n");
    str.append("<Name "+xXml.attr("contains",wildname.getText())+xXml.attr("filter",checkname.isSelected())+"/>\n");
    str.append("<Records "+xXml.attr("Only",checkonly.isSelected()));
    str.append(xXml.attr("Rates",checkrate.isSelected()));
    str.append(xXml.attr("States",checkstate.isSelected()));
    str.append(xXml.attr("Infos",checkinfo.isSelected())+"/>\n");
    str.append(xXml.tag("Selection",xXml.CLOSE));
    xXml.write(file,str.toString());
}
//---- Handle the menu actions ---------------------------------------
public void actionPerformed(ActionEvent e) {
String cmd = new String(e.getActionCommand());

if ("Store".equals(cmd)) saveSetup("Selection.xml");

else if ("Display".equals(cmd)) {
if(!desk.findFrame(p1name)){
    xSet.setLayout(p1name,null,null,0, true);
    frame=new xInternalCompound(p1name,menuIcon, 0, xSet.getLayout(p1name), back);
    frame.rebuild(parpan);
    desk.addFrame(frame); // old frame will be deleted
}}
else if ("filter".equals(cmd)){
    parpan=new xPanelPrompt("Selected parameter list", new Dimension(800,400));
    buildList();
    if(frame != null) frame.rebuild(parpan); 
}
else System.out.println("Ignore "+cmd);
}
/**
 * These objects are created for each parameter to be shown in the panel.
 * They keep mainly the references to the text fields of the parameters.
 * They are registered as info handlers. The text fields are updated in infoHandler. 
 */
private class myParameter implements xiUserInfoHandler, ActionListener{
private JTextField field;
private xPanelPrompt prompt;
private String Name;
/**
 * @param name Name of handler. 
 */
private myParameter(String name, xPanelPrompt prompter){
prompt=prompter;
Name = new String(name);
}
/**
 * @param Field Text field of parameter to be updated in infoHandler call back.
 */
private void setField(JTextField Field, boolean edit){
field=Field;
field.setEditable(edit);
}
// return unique name for identification of handler in xDimParameter.
public  String getName(){return Name;}
/**
 * @param parameter Parameter returns value for text field.
 */
public void infoHandler(xiDimParameter parameter){
field.setText(parameter.getValue());
}
public void actionPerformed(ActionEvent e){
xiParser xip;
String cmd = new String(e.getActionCommand());
String value = prompt.askInput(cmd,((JTextField)e.getSource()).getText());
if(value != null){
// look for parameter object
for(int i=0;i<parlist.size();i++){
    xip=parlist.get(i).getParserInfo();
    if(xip.getFull().equals(cmd)){
        parlist.get(i).setParameter(value);
        break;
        }
    }
}}
} // class myParameter
} // class xPanelSelect
