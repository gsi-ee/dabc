package xgui;
/**
* CLASSPATH=.:/misc/goofy/cbm/daq/controls/smi/dim_v16r10/jdim/classes
*/
// import javax.swing.JInternalFrame;
// import javax.swing.JDesktopPane;
// import javax.swing.JMenu;
// import javax.swing.JMenuItem;
// import javax.swing.JMenuBar;
// import javax.swing.JFrame;
// import java.lang.Object.*;
// import jv.object.PsActionSupport; // JavaView 3D package
import java.lang.*;
import javax.swing.*;
import java.awt.event.*;
import java.awt.*;
import java.util.*;
import java.io.*;
import dim.*;
import java.beans.PropertyVetoException;

public class xGui extends JFrame implements xiDesktop, ActionListener {
private JTextArea bottomState;
private JPanel mainpanel;
private JDesktopPane desktop;
private xPanelMeter     metpan;
private xPanelCommand   compan;
private xPanelParameter parpan;
private xPanelHistogram hispan;
private xPanelState     stapan;
private xPanelInfo      infpan;
private xPanelLogger    logpan;
private xPanelDabc      dabcpan;
private xPanelMbs       mbspan;
private xPanelDabcMbs   dbspan;
private static xiUserPanel     usrpan;
private ImageIcon browserIcon, histoIcon, stateIcon , infoIcon, paramIcon, commIcon, meterIcon, loggerIcon,
        closeIcon, storeIcon, dabcIcon, winIcon, workIcon, launchIcon, killIcon, mbsIcon, dabcmbsIcon;
private xInternalFrame frCommands, frParameters, frMeters, frHistograms, frState, frInfos, 
        frLogger, frDabcLauncher, frMbsLauncher, frDabcMbsLauncher, frUserLauncher;
private Vector<JInternalFrame> frUserFrame;
private boolean showCommands, showParameters, showMeters, showHistograms, showState, showInfos, showLogger,
        showDabcLauncher, showMbsLauncher, showDabcMbsLauncher, showUserLauncher;
private xDimBrowser browser;
private menuAction maLogger, maCommands, maParameters, maMeters, maHistograms, maState, maBrowser, maQuit,
        maDabcLauncher, maMbsLauncher, maDabcMbsLauncher, maUserLauncher, maInfos, maSave, maWork;
private String usrHeader, usrGraphics;
private boolean clearOnUpdate=false;
private static boolean enableControl=true;
private String Layout;

// private PsActionSupport actionHandler;
public xGui() {
    super("DABC Controls");
    if(usrpan == null){
    if(System.getenv("DABC_USER_PANEL")!=null){
        String up=System.getenv("DABC_USER_PANEL");
        try{
         usrpan = (xiUserPanel) Class.forName(up).newInstance();
        }   catch(ClassNotFoundException ee){System.out.println("NotFound: Error creating "+up);}
            catch(InstantiationException x){System.out.println("Instant: Error creating "+up);}
            catch(IllegalAccessException xx){System.out.println("IllAccess: Error creating "+up);}
    }}
    // actionHandler=new PsActionSupport(); // can fire action events to registered listeners
    // actionHandler.addActionListener(this);
    // actionHandler.fireAction(ActionEvent e);
    //setJMenuBar(createTopMenuBar()); // JFrame, pull down menu bar in frame
    desktop = new JDesktopPane(); //a specialized layered pane for the internal frames
    xSet.setDesktop((JFrame)this,desktop);
// load icons
    storeIcon   = xSet.getIcon("icons/savewin.png");
    loggerIcon  = xSet.getIcon("icons/logwin.png");
    stateIcon   = xSet.getIcon("icons/statewin.png");
    infoIcon    = xSet.getIcon("icons/infowin.png");
    browserIcon = xSet.getIcon("icons/browser.png");
    histoIcon   = xSet.getIcon("icons/histowin.png");
    paramIcon   = xSet.getIcon("icons/paramwin.png");
    commIcon    = xSet.getIcon("icons/comicon.png");
    meterIcon   = xSet.getIcon("icons/meterwin.png");
    launchIcon  = xSet.getIcon("icons/dabclaunch.png");
    killIcon    = xSet.getIcon("icons/dabckill.png");
    mbsIcon     = xSet.getIcon("icons/mbsicon.png");
    dabcmbsIcon = xSet.getIcon("icons/dabcmbsicon.png");
    winIcon     = xSet.getIcon("icons/user.png");
    workIcon    = xSet.getIcon("icons/control.png");
    closeIcon   = xSet.getIcon("icons/fileclose.png");
    dabcIcon    = xSet.getIcon("icons/dabcicon.png");
    if (dabcIcon != null) setIconImage(dabcIcon.getImage());
    
// specify default layouts: name, position, size, columns, to show
    Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
    int xs=screenSize.width  - 100;
    int ys=screenSize.height - 100;
    showCommands=false;
    showParameters=false;
    showMeters=false;
    showHistograms=false;
    showState=false;
    showInfos=false;
    showLogger=false;
    showDabcLauncher=false;
    showMbsLauncher=true;
    showDabcMbsLauncher=false;
    xSet.createLayout("Main",        new Point(50,50),   new Dimension(xs,ys),    0, true);
    xSet.createLayout("Command",     new Point(0,230),   new Dimension(650,200),  0, showCommands);
    xSet.createLayout("Parameter",   new Point(0,465),   new Dimension(680,150),  0, showParameters);
    xSet.createLayout("Logger",      new Point(0,650),   new Dimension(680,150),  0, showLogger);
    xSet.createLayout("Meter",       new Point(880,0),   new Dimension(100,75),   2, showMeters);
    xSet.createLayout("State",       new Point(710,0),   new Dimension(100,75),   1, showState);
    xSet.createLayout("Info",        new Point(710,410), new Dimension(100,75),   0, showInfos);
    xSet.createLayout("Histogram",   new Point(710,600), new Dimension(200,150),  3, showHistograms);
    xSet.createLayout("DabcLauncher",new Point(0,0),     new Dimension(100,100),  0, showDabcLauncher);
    xSet.createLayout("MbsLauncher", new Point(100,0),   new Dimension(100,100),  0, showMbsLauncher);
    xSet.createLayout("DabcMbsLauncher", new Point(0,0), new Dimension(100,100),  0, showDabcMbsLauncher);
    // columns width of parameter table
    // ID, name, appl, param, value, input, show
    int w[]={50,80,50,100,150,20,50,30};
    xSet.setParTableWidth(w);
    // if layout file there, restore
    if(System.getenv("DABC_GUI_LAYOUT")!=null) Layout=System.getenv("DABC_GUI_LAYOUT");
    else Layout=new String("Layout.xml");
    xSet.Restore(Layout);   
    // overwrite appearance for monitor only
    xSet.getLayout("DabcMbsLauncher").set(null,null,0,enableControl&xSet.getLayout("DabcMbsLauncher").show());    
    xSet.getLayout("MbsLauncher").set(null,null,0,enableControl&xSet.getLayout("MbsLauncher").show());    
    xSet.getLayout("DabcLauncher").set(null,null,0,enableControl&xSet.getLayout("DabcLauncher").show());    
    xSet.getLayout("Command").set(null,null,0,enableControl&xSet.getLayout("Command").show());    
    xSet.getLayout("Logger").set(null,null,0,enableControl&xSet.getLayout("Logger").show());    
    xSet.getLayout("Parameter").set(null,null,0,enableControl&xSet.getLayout("Parameter").show());    
    // create the panels
    logpan=new xPanelLogger(xSet.getLayout("Logger").getSize());
    hispan=new xPanelHistogram(xSet.getLayout("Histogram").getSize(),xSet.getLayout("Histogram").getColumns());
    metpan=new xPanelMeter(xSet.getLayout("Meter").getSize(),xSet.getLayout("Meter").getColumns());
    stapan=new xPanelState(xSet.getLayout("State").getSize(),xSet.getLayout("State").getColumns());
    infpan=new xPanelInfo(xSet.getLayout("Info").getSize());
    xLogger.setLoggerPanel(logpan); // Logger must know the panel to write messages to it
    // Get list of services from DIM server. Handover the graphics panels.
    browser = new xDimBrowser(hispan,metpan,stapan,infpan);
    browser.releaseServices(clearOnUpdate); // remove all parameters
    browser.initServices("*");
    // Register DIM service to get changes in server list. Bottom line displays server list:
    bottomState=new JTextArea("DIM servers: "+browser.getServers());
    bottomState.setPreferredSize(new Dimension(100,20));
    bottomState.setMargin(new Insets(2,2,2,2));
    bottomState.setForeground(xSet.greenL());
    bottomState.setBackground(xSet.blueD());
    xDimNameInfo dimservers = new xDimNameInfo("DIS_DNS/SERVER_LIST",bottomState);
    // launch panels MBS and DABC and combined
    mbspan=new xPanelMbs("Login",browser,this,this);
    dabcpan=new xPanelDabc("Login",browser,this,this);
    dbspan=new xPanelDabcMbs("Login",browser,this,this);
 // parpan waits until all parameters are registered and updated (quality != -1):
    parpan=new xPanelParameter(browser,xSet.getLayout("Parameter").getSize());
    browser.enableServices();
    compan=new xPanelCommand(browser,xSet.getLayout("Command").getSize());
    // put list of command descriptors from parameters to commands:
    compan.setCommandDescriptors(parpan.getCommandDescriptors()); 
    infpan.updateAll();
    stapan.updateAll();
    metpan.updateAll();
    hispan.updateAll();
    frUserFrame = new Vector<JInternalFrame>(0);
// user panel
    // if user panel was in layout file, but is not instantiated:
    if(usrpan == null){
        showUserLauncher=false;
        usrHeader = new String("x");
    } else {
        usrpan.init(this,this);
        usrHeader=usrpan.getHeader();
        usrpan.setDimServices((xiDimBrowser) browser);
        compan.setUserCommand(usrpan.getUserCommand());
    }
    
// create the actions for buttons and menu items
    Integer vk_Q = new Integer(KeyEvent.VK_Q);
    Integer vk_O = new Integer(KeyEvent.VK_O);
    Integer vk_B = new Integer(KeyEvent.VK_B);
    Integer vk_H = new Integer(KeyEvent.VK_H);
    Integer vk_C = new Integer(KeyEvent.VK_C);
    Integer vk_P = new Integer(KeyEvent.VK_P);
    Integer vk_M = new Integer(KeyEvent.VK_M);
    Integer vk_S = new Integer(KeyEvent.VK_S);
    Integer vk_T = new Integer(KeyEvent.VK_T);
    Integer vk_L = new Integer(KeyEvent.VK_L);
    Integer vk_D = new Integer(KeyEvent.VK_D);
    Integer vk_A = new Integer(KeyEvent.VK_A);
    Integer vk_G = new Integer(KeyEvent.VK_G);
    Integer vk_W = new Integer(KeyEvent.VK_W);
    maQuit           =new menuAction("Quit",           closeIcon,"Quit GUI.",vk_Q,this);
    maWork           =new menuAction("Test",           workIcon,"Test.",vk_W,this);
    maSave           =new menuAction("Save",           storeIcon,"Store windows layout.",vk_O,this);
    maBrowser        =new menuAction("Browser",        browserIcon,"Browse DIM server.",vk_B,this);
    maHistograms     =new menuAction("Histograms",     histoIcon,"Open histogram window.",vk_H,this);
    maCommands       =new menuAction("Commands",       commIcon,"Open command window.",vk_C,this);
    maParameters     =new menuAction("Parameters",     paramIcon,"Open parameter table window.",vk_P,this);
    maMeters         =new menuAction("Meters",         meterIcon,"Open meter window.",vk_M,this);
    maState          =new menuAction("States",         stateIcon,"Open state window.",vk_S,this);
    maInfos          =new menuAction("Infos",          infoIcon,"Open info window.",vk_T,this);
    maLogger         =new menuAction("Logger",         loggerIcon,"Open logger window.",vk_L,this);
    if(usrpan != null) 
    maUserLauncher   =new menuAction(usrHeader,usrpan.getIcon(),usrpan.getToolTip(),vk_D,this);
    maDabcLauncher   =new menuAction("DabcLauncher",   dabcIcon,"Open DABC launcher window.",vk_D,this);
    maMbsLauncher    =new menuAction("MbsLauncher",    mbsIcon,"Open MBS launcher window.",vk_A,this);
    maDabcMbsLauncher=new menuAction("DabcMbsLauncher",dabcmbsIcon,"Open DABC-MBS launcher window.",vk_G,this);

// Position main window 
    setLocation(xSet.getLayout("Main").getPosition());
    setSize(xSet.getLayout("Main").getSize());

//Set up the GUI.
    mainpanel=new JPanel(new BorderLayout()); // for tool bar, desktop
    mainpanel.add(createToolBar(),BorderLayout.NORTH);
    mainpanel.add(desktop,BorderLayout.CENTER);
    mainpanel.add(bottomState,BorderLayout.SOUTH);
    mainpanel.setVisible(true);
    add(mainpanel); // panel to frame

    // create the panel frames which shall be visible:
    if(xSet.getLayout("Command").show()) frCommands=
        createFrame("Commands",commIcon,compan,xSet.getLayout("Command"),null, true); 
    if(xSet.getLayout("Parameter").show()) frParameters=
        createFrame("Parameters",paramIcon,parpan,xSet.getLayout("Parameter"),null, true); 
    if(xSet.getLayout("DabcLauncher").show()) frDabcLauncher=
        createFrame("DabcLauncher",dabcIcon,dabcpan,xSet.getLayout("DabcLauncher"),null, false);
    if(xSet.getLayout("DabcMbsLauncher").show()) frDabcMbsLauncher=
        createFrame("DabcMbsLauncher",dabcmbsIcon,dbspan,xSet.getLayout("DabcMbsLauncher"),null, false);
    if(xSet.getLayout("MbsLauncher").show()) frMbsLauncher=
        createFrame("MbsLauncher",mbsIcon,mbspan,xSet.getLayout("MbsLauncher"),null, false);
    if(xSet.getLayout(usrHeader)!= null){
        if(xSet.getLayout(usrHeader).show()) frUserLauncher=
           createFrame(usrHeader,usrpan.getIcon(),(JPanel)usrpan,xSet.getLayout(usrHeader),null, false);
        }
    if(xSet.getLayout("Meter").show()) metpan.setListener(frMeters=
        createFrame("RateMeters",meterIcon,metpan,xSet.getLayout("Meter"),metpan.createMenuBar(), false));
    if(xSet.getLayout("Histogram").show()) hispan.setListener(frHistograms=
        createFrame("Histograms",histoIcon,hispan,xSet.getLayout("Histogram"),hispan.createMenuBar(), false));
    if(xSet.getLayout("State").show()) stapan.setListener(frState=
        createFrame("States",stateIcon,stapan,xSet.getLayout("State"),stapan.createMenuBar(), false));
    if(xSet.getLayout("Info").show()) infpan.setListener(frInfos=
        createFrame("Infos",infoIcon,infpan,xSet.getLayout("Info"),infpan.createMenuBar(), true));
    if(xSet.getLayout("Logger").show()) logpan.setListener(frLogger=
        createFrame("Logger",loggerIcon,logpan,xSet.getLayout("Logger"),logpan.createMenuBar(), true));

//Make dragging a little faster but perhaps uglier.
    desktop.setDragMode(JDesktopPane.OUTLINE_DRAG_MODE);
//----------------------- startup finished -----------------------
}

protected JToolBar createToolBar() {
//Create the toolbar.
    JButton button;
    JToolBar toolBar = new JToolBar();
    toolBar.setMargin(new Insets(-4,0,-6,0));
    toolBar.add(new toolButton(maQuit));
    toolBar.add(new toolButton(maWork));
    toolBar.add(new toolButton(maSave));
    toolBar.addSeparator();
    if(enableControl) toolBar.add(new toolButton(maDabcMbsLauncher));
    if(enableControl) toolBar.add(new toolButton(maDabcLauncher));
    if(enableControl) toolBar.add(new toolButton(maMbsLauncher));
    if(usrpan != null)toolBar.add(new toolButton(maUserLauncher));
    toolBar.add(new toolButton(maBrowser));
    toolBar.addSeparator();
    if(enableControl) toolBar.add(new toolButton(maCommands));
    if(enableControl) toolBar.add(new toolButton(maParameters));
    toolBar.add(new toolButton(maMeters));
    toolBar.add(new toolButton(maHistograms));
    toolBar.add(new toolButton(maState));
    toolBar.add(new toolButton(maInfos));
    if(enableControl) toolBar.add(new toolButton(maLogger));
    return toolBar;
}

// needed for pulldown menus:
// protected JMenuBar createTopMenuBar() {
    // JMenuItem menuItem;
    // JMenuBar menuBar = new JMenuBar();
    // JMenu wmenu = new JMenu("File");
    // wmenu.setMnemonic(KeyEvent.VK_F);
    // menuBar.add(wmenu);
    // wmenu.add(new JMenuItem(maDabcLauncher));
    // wmenu.add(new JMenuItem(maMbsLauncher));
    // wmenu.add(new JMenuItem(maQuit));
    // JMenu omenu = new JMenu("Windows");
    // omenu.setMnemonic(KeyEvent.VK_W);
    // menuBar.add(omenu);
    // omenu.add(new JMenuItem(maCommands));
    // omenu.add(new JMenuItem(maParameters));
    // omenu.add(new JMenuItem(maMeters));
    // omenu.add(new JMenuItem(maHistograms));
    // omenu.add(new JMenuItem(maState));
    // omenu.add(new JMenuItem(maInfos));
    // omenu.add(new JMenuItem(maLogger));
    // return menuBar;
// }
// xiDesktop
public boolean frameExists(String title){
    JInternalFrame[] fr=desktop.getAllFrames();
    for(int i=0;i<fr.length;i++)
        if(fr[i].getTitle().equals(title)) return true;
    return false;
}

// xiDesktop
public void addDesktop(JInternalFrame frame){
addDesktop(frame, true);
}
// xiDesktop
public void addDesktop(JInternalFrame frame, boolean manage){
boolean create=true;
if(!frameExists(frame.getTitle())){
//System.out.println("Add frame "+frame.getTitle()+" to desktop");
desktop.add(frame); 
frame.moveToFront();
if(manage){
for(int i=0;i<frUserFrame.size();i++)if(frUserFrame.get(i).getTitle().equals(frame.getTitle())) create=false;
if(create) {
//System.out.println("Add frame "+frame.getTitle()+" to list");
frUserFrame.add(frame); 
}}}}

// xiDesktop
public void removeDesktop(String title){
JInternalFrame[] fr=desktop.getAllFrames();
for(int i=fr.length-1;i>=0;i--) {
//System.out.println("        frame "+fr[i].getTitle());
if(fr[i].getTitle().equals(title)) {
//System.out.println("Dispose frame "+title+" from desktop");
fr[i].dispose();
}}
for(int i=frUserFrame.size()-1;i>=0;i--)if(frUserFrame.get(i).getTitle().equals(title)){
//System.out.println("Remove frame "+title+" from list");
 frUserFrame.remove(i);
 }
}

// xiDesktop
public void frameSelect(String title, boolean select){
JInternalFrame[] fr=desktop.getAllFrames();
try{
for(int i=0;i<fr.length;i++) if(fr[i].getTitle().equals(title)) fr[i].setSelected(select);
} catch(PropertyVetoException e){}
}

// xiDesktop
public void frameToFront(String title){
JInternalFrame[] fr=desktop.getAllFrames();
for(int i=0;i<fr.length;i++) if(fr[i].getTitle().equals(title)) fr[i].toFront();
}

// React to menu selections of main window.
public void actionPerformed(ActionEvent e) {
    if ("DabcMbsLauncher".equals(e.getActionCommand())) {frDabcMbsLauncher=
        createFrame("DabcMbsLauncher",dabcmbsIcon,dbspan,xSet.getLayout("DabcMbsLauncher"),null, false);
        xSet.setLayout("DabcMbsLauncher",null,null,0, true);
        }
    else if ("DabcLauncher".equals(e.getActionCommand())) {frDabcLauncher=
        createFrame("DabcLauncher",dabcIcon,dabcpan,xSet.getLayout("DabcLauncher"),null, false);
        xSet.setLayout("DabcLauncher",null,null,0, true);
        }
    else if (usrHeader.equals(e.getActionCommand())) {frUserLauncher=
        createFrame(usrHeader,usrpan.getIcon(),(JPanel)usrpan,xSet.getLayout(usrHeader),null, false);
        xSet.setLayout(usrHeader,null,null,0, true);
        }
    else if ("MbsLauncher".equals(e.getActionCommand())) {frMbsLauncher=
        createFrame("MbsLauncher",mbsIcon,mbspan,xSet.getLayout("MbsLauncher"),null, false); 
        xSet.setLayout("MbsLauncher",null,null,0, true);
        }
    else if ("Commands".equals(e.getActionCommand())) {frCommands=
        createFrame("Commands",commIcon,compan,xSet.getLayout("Command"),null, true);
        xSet.setLayout("Command",null,null,0, true);
        }
    else if ("Parameters".equals(e.getActionCommand())) {frParameters=
        createFrame("Parameters",paramIcon,parpan,xSet.getLayout("Parameter"),null, true);
        xSet.setLayout("Parameter",null,null,0, true);
        }
    else if ("Meters".equals(e.getActionCommand())) {metpan.setListener(frMeters=
        createFrame("RateMeters",meterIcon,metpan,xSet.getLayout("Meter"),metpan.createMenuBar(), false));
        xSet.setLayout("Meter",null,null,0, true);
        }
    else if ("Histograms".equals(e.getActionCommand())) {hispan.setListener(frHistograms=
        createFrame("Histograms",histoIcon,hispan,xSet.getLayout("Histogram"),hispan.createMenuBar(), false));
        xSet.setLayout("Histogram",null,null,0, true);
        }
    else if ("States".equals(e.getActionCommand())) {stapan.setListener(frState=
        createFrame("State",stateIcon,stapan,xSet.getLayout("State"),stapan.createMenuBar(), false));
        xSet.setLayout("State",null,null,0, true);
        }
    else if ("Infos".equals(e.getActionCommand())) {infpan.setListener(frInfos=
        createFrame("Infos",infoIcon,infpan,xSet.getLayout("Info"),infpan.createMenuBar(), true));
        xSet.setLayout("Info",null,null,0, true);
        }
    else if ("Logger".equals(e.getActionCommand())) {logpan.setListener(frLogger=
        createFrame("Logger",loggerIcon,logpan,xSet.getLayout("Logger"),logpan.createMenuBar(), true));
        xSet.setLayout("Logger",null,null,0, true);
        }
    else if ("Test".equals(e.getActionCommand())) {
        browser.listServices(false); // exclude command definitions
    }
    else if ("Quit".equals(e.getActionCommand())) {
        // String[] srvcs;
        // int i=0;
        // xDimParameter di;
       // for (i=0;i<1000;i++){
        // System.out.print(".");
        // System.gc(); // garbage collect now to save time later
       // srvcs = DimBrowser.getServices("*");
       // di = new xDimParameter("XDAQ/X86G-4/MSG/M","C",1,"%BrokenLink%",0);
       // di.releaseService();
       // }
        quit();
    }
    else if ("Save".equals(e.getActionCommand())) {
    
        if(frCommands     != null) 
        xSet.setLayout("Command",  
            frCommands.getLocation(),  frCommands.getSize()    , 0, xSet.getLayout("Command").show());
        if(frParameters   != null) 
        xSet.setLayout("Parameter",
            frParameters.getLocation(),frParameters.getSize() ,  0, xSet.getLayout("Parameter").show());
        if(frLogger       != null) 
        xSet.setLayout("Logger",   
            frLogger.getLocation(),    frLogger.getSize(),       0, xSet.getLayout("Logger").show());
        if(frMeters       != null) 
        xSet.setLayout("Meter",    
            frMeters.getLocation(),    null, metpan.getColumns()  , xSet.getLayout("Meter").show());
        if(frState       != null) 
        xSet.setLayout("State",   
            frState.getLocation(),    null, stapan.getColumns()  , xSet.getLayout("State").show());
        if(frInfos       != null) 
        xSet.setLayout("Info",    
            frInfos.getLocation(),    frInfos.getSize()      , 1, xSet.getLayout("Info").show());
        if(frHistograms   != null) 
        xSet.setLayout("Histogram",
            frHistograms.getLocation(),null, hispan.getColumns()  , xSet.getLayout("Histogram").show());
        if(frDabcLauncher != null) 
        xSet.setLayout("DabcLauncher",
            frDabcLauncher.getLocation(),frDabcLauncher.getSize(),0,xSet.getLayout("DabcLauncher").show());
        if(frUserLauncher  != null) 
        xSet.setLayout(usrHeader,
            frUserLauncher.getLocation(),frUserLauncher.getSize(),0,xSet.getLayout(usrHeader).show());
        if(frUserFrame  != null) for(int i = 0; i < frUserFrame.size();i++)
        xSet.setLayout(frUserFrame.get(i).getTitle(),
                frUserFrame.get(i).getLocation(),
                frUserFrame.get(i).getSize(),0,
                xSet.getLayout(frUserFrame.get(i).getTitle()).show());
        if(frMbsLauncher  != null) 
        xSet.setLayout("MbsLauncher",
            frMbsLauncher.getLocation(),frMbsLauncher.getSize(),0, xSet.getLayout("MbsLauncher").show());
        if(frDabcMbsLauncher  != null) 
        xSet.setLayout("DabcMbsLauncher",
            frDabcMbsLauncher.getLocation(),frDabcMbsLauncher.getSize(),0,xSet.getLayout("DabcMbsLauncher").show());
        xSet.setLayout("Main", getLocation(), getSize() , 0, true);
        try{
        int ncols=parpan.getTable().getColumnModel().getColumnCount();
        if(ncols > 0){
            int[] cols = new int[ncols];
            for(int i=0;i<ncols;i++)
                cols[i]=parpan.getTable().getColumnModel().getColumn(i).getWidth()-1;// substract increment
            xSet.setParTableWidth(cols);
        }} catch(NullPointerException x){}
        xSet.Save(Layout);
        JOptionPane.showInternalMessageDialog(
            desktop, "Layout saved: "+Layout, "Information",JOptionPane.INFORMATION_MESSAGE);
    }
    else if (("Update".equals(e.getActionCommand())) ||
        ("Browser".equals(e.getActionCommand()))) {
        //if("Browser".equals(e.getActionCommand()))clearOnUpdate=true;
        // clear all references to DIM services
        mbspan.releaseDimServices();
        dabcpan.releaseDimServices();
        dbspan.releaseDimServices();
        if(usrpan != null)usrpan.releaseDimServices();
        parpan=null;
        compan=null;
        System.gc(); // garbage collect now to save time later
        browser.releaseServices(clearOnUpdate); // remove all parameters
        clearOnUpdate=false;
        browser.initServices("*"); 
     // waits until all parameters are registered and updated:
        parpan=new xPanelParameter(browser,xSet.getLayout("Parameter").getSize());
        compan=new xPanelCommand(browser,xSet.getLayout("Command").getSize());
        infpan.updateAll();
        stapan.updateAll();
        metpan.updateAll();
        hispan.updateAll();
        mbspan.setDimServices();
        dabcpan.setDimServices();
        dbspan.setDimServices();
        if(usrpan != null){
            usrpan.setDimServices(browser);
            compan.setUserCommand(usrpan.getUserCommand());
            }
        browser.enableServices();
        
// put list of command descriptors from parameters to commands:
        compan.setCommandDescriptors(parpan.getCommandDescriptors()); 
        if(frParameters != null) frParameters.addWindow(parpan);
        if(frCommands   != null) frCommands.addWindow(compan);
        if(frInfos      != null) frInfos.addWindow(infpan);
        // if(frInfos      != null) {
            // infpan.setListener(frInfos=
            // createFrame("Infos",infoIcon,infpan,xSet.getLayout("Info"),infpan.createMenuBar(), true));
            // xSet.setLayout("Info",null,null,0, true);
            // frInfos.addWindow(infpan);
        // }
    }
    else if ("DisplayFrame".equals(e.getActionCommand())) {
        addDesktop((xInternalFrame)e.getSource(), false); // do not manage
    }
    else if ("RemoveFrame".equals(e.getActionCommand())) {
        removeDesktop(((xInternalFrame)e.getSource()).getTitle());
    }
    else {
    System.out.println("Unsolicited event:"+e.getActionCommand());
    }
} // actionPerformed


//Quit the application.
protected void quit() {
    System.exit(0);
}

private xInternalFrame  createFrame(String title, ImageIcon icon, JPanel panel, xLayout la, JMenuBar menu, boolean resize){
    if(!frameExists(title)){
        xInternalFrame frame = new xInternalFrame(title,la);
        frame.setupFrame(icon, menu, panel, resize);  // true for resizable
        desktop.add(frame);
        frame.moveToFront();
        return frame;
    } else return null;
}

public class menuAction extends AbstractAction {
    private ActionListener action;
    public menuAction(String text, ImageIcon icon,
            String desc, Integer mnemonic, ActionListener al) {
        super(text, icon);
        putValue(ACTION_COMMAND_KEY, text);
        putValue(SHORT_DESCRIPTION, desc);
        putValue(MNEMONIC_KEY, mnemonic);
        action=al; // store the calling object
    }
    public void actionPerformed(ActionEvent e) {
// action events are handled in top class listener
        action.actionPerformed(e);
    }
}
public class toolButton extends JButton {
public toolButton(menuAction ma){
    super(ma);
    setText("");
    setBorderPainted(false);
}}

/**
 * Create the GUI and show it.  For thread safety,
 * this method should be invoked from the
 * event-dispatching thread.
 */
private static void createAndShowGUI() {
xGui frame;
//Make sure we have nice window decorations.
    JFrame.setDefaultLookAndFeelDecorated(true);

//Create and set up the window.
    frame = new xGui();
    frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

//Display the window.
    frame.setVisible(true);
}

public static void main(String[] args) {
System.out.println("Using Java: "+System.getProperties().getProperty("java.version"));
System.out.println("From      : "+System.getProperties().getProperty("java.home"));
// do we have to create user panel
int i=0;

// Without DIM name server nothing can go
if(System.getenv("DIM_DNS_NODE")==null){
System.out.println("No DIM name server defined, set DIM_DNS_NODE!");
return;
}

if(args.length > i){
    if(args[i].startsWith("-m")){
        enableControl=false;
        i++;
        }
    else enableControl=true;
}
if(args.length > i){
    try{
     usrpan = (xiUserPanel) Class.forName(args[i]).newInstance();
    }   catch(ClassNotFoundException ee){System.out.println("NotFound: Error creating "+args[i]);}
        catch(InstantiationException x){System.out.println("Instant: Error creating "+args[i]);}
        catch(IllegalAccessException xx){System.out.println("IllAccess: Error creating "+args[i]);}
    }
javax.swing.SwingUtilities.invokeLater(
new Runnable() {
public void run() {createAndShowGUI();}
});
}
}
