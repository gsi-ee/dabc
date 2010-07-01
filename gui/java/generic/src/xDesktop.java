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
import java.lang.*;
import javax.swing.*;
import java.awt.event.*;
import java.awt.*;
import java.util.*;
import java.io.*;
import dim.*;
import java.beans.PropertyVetoException;

/**
 * Top desktop class.
 * This object manages the main window including the main toolbar
 * and a JDesktopPane in which all windows (JPanel) are opened as JInternalFrame.
 * It also creates the DIM browser object which manages the list of parameters and commands.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xDesktop extends JFrame implements Runnable, xiDesktop, ActionListener {
private JTextArea bottomState;
private JToolBar toolBar;
private JPanel mainpanel;
private JDesktopPane    desktop;
private xPanelSelect    selpan;
private xPanelMeter     metpan;
private xPanelCommand   compan;
private xPanelParameter parpan;
private xPanelHisto     hispan;
private xPanelState     stapan;
private xPanelInfo      infpan;
private xPanelLogger    logpan;
private xPanelDabc      dabcpan;
private xPanelMbs       mbspan;
private xPanelDabcMbs   dbspan;
private Vector<xiUserPanel>     usrpan;
private ImageIcon browserIcon, histoIcon, stateIcon , infoIcon, paramIcon, commIcon, meterIcon, loggerIcon,
        closeIcon, storeIcon, dabcIcon, winIcon, workIcon, selIcon, launchIcon, killIcon, mbsIcon, dabcmbsIcon;
private xInternalFrame frSelect, frCommands, frParameters, frMeters, frHistograms, frState, frInfos, 
        frLogger, frDabcController, frMbsController, frDabcMbsController;
private Vector<xInternalFrame> frUserController;
private Vector<JInternalFrame> frUserFrame;
private boolean showSelect, showCommands, showParameters, showMeters, showHistograms, showState, showInfos, showLogger,
        showDabcController, showMbsController, showDabcMbsController;
private boolean showUserController[];
private xDimBrowser browser;
private menuAction maSelect, maLogger, maCommands, maParameters, maMeters, maHistograms, maState, maBrowser, maQuit,
        maDabcController, maMbsController, maDabcMbsController, maInfos, maSave, maWork;
private Vector<menuAction> maUserController;
private String usrHeader, usrGraphics, usrPanels;
private boolean clearOnUpdate=false;
private String LayoutFile, RecordFile, CommandFile, SelectionFile;
private String ActionCommand;
private String lastframe;
private String mbsTaskList;
private int dostep;
private xMeter progmet;
private JFrame progframe;

// private PsActionSupport actionHandler;
/**
 * Creates the top level GUI.
 * usrPanels are optional user panel names. 
 * If null, user panel class name could alternatively be specified
 * as DABC_USER_PANEL and will be instantiated.
 */
public xDesktop(xMeter promet, JFrame proframe) {
    super("DABC Controls and Monitoring");
    progmet=promet;
    progframe=proframe;
}
public void run(){
	switch (dostep) {
	case 0: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": Desktop","");
	progmet.redraw((double)(dostep*10));
    if(xSet.isMbs())      setTitle("MBS Controls and Monitoring");
    if(xSet.isDabs())     setTitle("MBS+DABC Controls and Monitoring");
    if(!xSet.isControl()) setTitle("Generic Monitoring");
    if(xSet.isGuru())     setTitle("Guru Controls and Monitoring");
	usrpan=new Vector<xiUserPanel>(0); // create here or below
	usrpan.add(new xPanelRunInfo());
	usrPanels=xSet.getUserPanels(); // specified by shell call
	if(usrPanels==null) usrPanels=System.getenv("DABC_APPLICATION_PANELS");
	if(usrPanels!=null){
    	//usrpan=new Vector<xiUserPanel>(0); // only if not created above
        String[] upl=usrPanels.split(",");
        for(int ii=0;ii<upl.length;ii++){
        try{
    	System.out.println("Instance "+upl[ii]);
    	usrpan.add((xiUserPanel) Class.forName(upl[ii]).newInstance());
	    }   catch(ClassNotFoundException ee){System.out.println("NotFound: Error creating "+upl[ii]);}
	        catch(InstantiationException x){System.out.println("Instant: Error creating "+upl[ii]);}
	        catch(IllegalAccessException xx){System.out.println("IllAccess: Error creating "+upl[ii]);}
    }}
    if(usrpan != null){
        frUserController = new Vector<xInternalFrame>(0);
        maUserController = new Vector<menuAction>(0);
        showUserController = new boolean[usrpan.size()];
    }
    // user frames can be added here
    frUserFrame = new Vector<JInternalFrame>(0);
    // actionHandler=new PsActionSupport(); // can fire action events to registered listeners
    // actionHandler.addActionListener(this);
    // actionHandler.fireAction(ActionEvent e);
    //setJMenuBar(createTopMenuBar()); // JFrame, pull down menu bar in frame
    desktop = new JDesktopPane(); //a specialized layered pane for the internal frames
    xSet.setDesktop((JFrame)this,desktop);
    javax.swing.SwingUtilities.invokeLater(this);
	break;
	
	case 1: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": Layouts","");
	progmet.redraw((double)(dostep*10));
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
    selIcon     = xSet.getIcon("icons/usericon.png");
    workIcon    = xSet.getIcon("icons/control.png");
    closeIcon   = xSet.getIcon("icons/fileclose.png");
    dabcIcon    = xSet.getIcon("icons/dabcicon.png");
    if (dabcIcon != null) setIconImage(dabcIcon.getImage());
    
// specify default layouts: name, position, size, columns, to show
    Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
    int xs=screenSize.width  - 100;
    int ys=screenSize.height - 100;
    showSelect=false;
    showCommands=false;
    showParameters=false;
    showMeters=true;
    showHistograms=false;
    showState=true;
    showInfos=true;
    showLogger=false;
    showDabcController=false;
    showMbsController=false;
    showDabcMbsController=false;
    xSet.createLayout("Main",        new Point(50,50),   new Dimension(xs,ys),    0, true);
    xSet.createLayout("Command",     new Point(0,230),   new Dimension(650,200),  0, showCommands);
    xSet.createLayout("Parameter",   new Point(0,465),   new Dimension(680,150),  0, showParameters);
    xSet.createLayout("Logger",      new Point(0,650),   new Dimension(680,150),  0, showLogger);
    xSet.createLayout("Meter",       new Point(200,0),   new Dimension(100,75),   2, showMeters);
    xSet.createLayout("State",       new Point(420,0),   new Dimension(100,75),   1, showState);
    xSet.createLayout("Info",        new Point(0,410), new Dimension(100,75),   0, showInfos);
    xSet.createLayout("Histogram",   new Point(0,600), new Dimension(200,150),  3, showHistograms);
    xSet.createLayout("DabcController",new Point(0,0),     new Dimension(100,100),  0, showDabcController);
    xSet.createLayout("MbsController", new Point(100,0),   new Dimension(100,100),  0, showMbsController);
    xSet.createLayout("DabcMbsController", new Point(0,0), new Dimension(100,100),  0, showDabcMbsController);
    xSet.createLayout("ParameterSelect", new Point(300,0), new Dimension(100,100),  0, showSelect);
    // columns width of parameter table
    // ID, name, appl, param, value, input, show
    int w[]={50,80,50,100,150,20,50,30};
    xSet.setParTableWidth(w);
    // if layout file there, restore
    if(System.getenv("DABC_GUI_LAYOUT")!=null) LayoutFile=System.getenv("DABC_GUI_LAYOUT");
    else LayoutFile=new String("Layout.xml");
    xSaveRestore.restoreLayouts(LayoutFile);   
    if(System.getenv("DABC_RECORD_ATTRIBUTES")!=null) RecordFile=System.getenv("DABC_RECORD_ATTRIBUTES");
    else RecordFile=new String("Records.xml");
    xSaveRestore.restoreRecords(RecordFile);
    if(System.getenv("DABC_COMMAND_ATTRIBUTES")!=null) CommandFile=System.getenv("DABC_COMMAND_ATTRIBUTES");
    else CommandFile=new String("Commands.xml");
    xSaveRestore.restoreCommands(CommandFile);
    if(System.getenv("DABC_PARAMETER_FILTER")!=null) SelectionFile=System.getenv("DABC_PARAMETER_FILTER");
    else SelectionFile=new String("Selection.xml");
    javax.swing.SwingUtilities.invokeLater(this);
	break;
	
	case 2: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": Panels","");
	progmet.redraw((double)(dostep*10));
    // create the panels
    logpan=new xPanelLogger(xSet.getLayout("Logger").getSize());
    hispan=new xPanelHisto(xSet.getLayout("Histogram").getSize(),xSet.getLayout("Histogram").getColumns());
    metpan=new xPanelMeter(xSet.getLayout("Meter").getSize(),xSet.getLayout("Meter").getColumns());
    stapan=new xPanelState(xSet.getLayout("State").getSize(),xSet.getLayout("State").getColumns());
    infpan=new xPanelInfo(xSet.getLayout("Info").getSize());
    xLogger.setLoggerPanel(logpan); // Logger must know the panel to write messages to it
    // Get list of services from DIM server. Handover the graphics panels.
    // The panels are then passed to the parameter objects for update.
    javax.swing.SwingUtilities.invokeLater(this);
	break;
	
	case 3: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": DIM services","");
	progmet.redraw((double)(dostep*10));
    browser = new xDimBrowser(hispan,metpan,stapan,infpan);
    browser.initServices("*");
    javax.swing.SwingUtilities.invokeLater(this);
	break;
	
	case 4: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": DIM servers","");
	progmet.redraw((double)(dostep*10));
    // Register DIM service to get changes in server list. Bottom line displays server list:
    bottomState=new JTextArea("DIM servers: "+browser.getServers());
    bottomState.setPreferredSize(new Dimension(100,20));
    bottomState.setMargin(new Insets(2,2,2,2));
    bottomState.setForeground(xSet.greenL());
    bottomState.setBackground(xSet.blueD());
    xDimNameInfo dimservers = new xDimNameInfo("DIS_DNS/SERVER_LIST",bottomState,this);
    // launch panels MBS and DABC and combined
    javax.swing.SwingUtilities.invokeLater(this);
	break;
	
	case 5: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": Parameter","");
	progmet.redraw((double)(dostep*10));
    selpan=new xPanelSelect(SelectionFile,"Parameter selection",browser,this,this);
    javax.swing.SwingUtilities.invokeLater(this);
	break;
	
	case 6: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": MBS panel","");
	progmet.redraw((double)(dostep*10));
    mbspan=new xPanelMbs("Login",browser,this,this);
    javax.swing.SwingUtilities.invokeLater(this);
	break;
	
	case 7: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": DABC panel","");
	progmet.redraw((double)(dostep*10));
    dabcpan=new xPanelDabc("Login",browser,this,this);
    javax.swing.SwingUtilities.invokeLater(this);
	break;
	
	case 8: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": DABCMBS panel","");
	progmet.redraw((double)(dostep*10));
    dbspan=new xPanelDabcMbs("Login",browser,this,this);
    javax.swing.SwingUtilities.invokeLater(this);
	break;
	
	case 9: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": Parameter panel","");
	progmet.redraw((double)(dostep*10));
 // parpan waits until all parameters are registered and updated (quality != -1):
    parpan=new xPanelParameter(browser,xSet.getLayout("Parameter").getSize());
    javax.swing.SwingUtilities.invokeLater(this);
	break;
	
	case 10: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": Enable services","");
	progmet.redraw((double)(dostep*10));
    browser.enableServices();
    // This task list is used for a command filter
    mbsTaskList=mbspan.getTaskList();
    javax.swing.SwingUtilities.invokeLater(this);
	break;
	
	case 11: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": Commands","");
	progmet.redraw((double)(dostep*10));
    compan=new xPanelCommand(browser,xSet.getLayout("Command").getSize(),mbsTaskList);
    // put list of command descriptors from parameters to commands:
    compan.setCommandDescriptors(parpan.getCommandDescriptors()); 
    selpan.setDimServices();
// user panels
    if(usrpan == null){
        usrHeader = new String("x");
    } else {
    	for(int ii=0;ii<usrpan.size();ii++){
	        usrpan.get(ii).init(this,this);
	        showUserController[ii]=true;
	        usrpan.get(ii).setDimServices((xiDimBrowser) browser);
    	}
        // Note that first panel in list defines command format!
    	// 0 is RunInfo panel
        if(usrpan.size()>1)compan.setUserCommand(usrpan.get(1).getUserCommand());
    }
    
    javax.swing.SwingUtilities.invokeLater(this);
	break;
	
	case 12: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": Main panels","");
	progmet.redraw((double)(dostep*10));
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
    if(usrpan != null) for(int ii=0;ii<usrpan.size();ii++)
    maUserController.add(new menuAction(usrpan.get(ii).getHeader(),usrpan.get(ii).getIcon(),usrpan.get(ii).getToolTip(),vk_D,this));
    maSelect         =new menuAction("ParameterSelect",   selIcon,"Open parameter selection window.",vk_D,this);
    maDabcController   =new menuAction("DabcController",   dabcIcon,"Open DABC controller window.",vk_D,this);
    maMbsController    =new menuAction("MbsController",    mbsIcon,"Open MBS controller window.",vk_A,this);
    maDabcMbsController=new menuAction("DabcMbsController",dabcmbsIcon,"Open DABC-MBS controller window.",vk_G,this);

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

    javax.swing.SwingUtilities.invokeLater(this);
	break;
	
	case 13: dostep++;	
    System.out.println(xLogger.getDate()+"========== Step "+dostep);
    progmet.setLettering("Progress "," Startup",Integer.toString(dostep)+": Create frames","");
	progmet.redraw((double)(dostep*10));
    // create the panel frames which shall be visible:
    if(xSet.isControl()){
    if(xSet.getLayout("Logger").show()) logpan.setListener(frLogger=
        createFrame("Logger",loggerIcon,logpan,xSet.getLayout("Logger"),logpan.createMenuBar(), true));
    if(xSet.getLayout("Command").show()) frCommands=
        createFrame("Commands",commIcon,compan,xSet.getLayout("Command"),null, true); 
    if(xSet.isDabc()&xSet.getLayout("DabcController").show()) dabcpan.setListener(frDabcController=
        createFrame("DabcController",dabcIcon,dabcpan,xSet.getLayout("DabcController"),null, false));
    if(xSet.isDabs()&xSet.getLayout("DabcMbsController").show()) dbspan.setListener(frDabcMbsController=
        createFrame("DabcMbsController",dabcmbsIcon,dbspan,xSet.getLayout("DabcMbsController"),null, false));
    if(xSet.isMbs()&xSet.getLayout("MbsController").show()) mbspan.setListener(frMbsController=
        createFrame("MbsController",mbsIcon,mbspan,xSet.getLayout("MbsController"),null, false));
    if(usrpan != null){
    	if(xSet.getLayout(usrpan.get(0).getHeader())!= null){//RunInfo
        if(xSet.getLayout(usrpan.get(0).getHeader()).show()) 
        	frUserController.add(createFrame(
        		usrpan.get(0).getHeader(),usrpan.get(0).getIcon(),(JPanel)usrpan.get(0),xSet.getLayout(usrpan.get(0).getHeader()),null, false));
        }}
    } // control mode
    if(usrpan != null){
    	for(int ii=1;ii<usrpan.size();ii++)
    	if(xSet.getLayout(usrpan.get(ii).getHeader())!= null){
        if(xSet.getLayout(usrpan.get(ii).getHeader()).show()) frUserController.add
           (createFrame(usrpan.get(ii).getHeader(),usrpan.get(ii).getIcon(),(JPanel)usrpan.get(ii),xSet.getLayout(usrpan.get(ii).getHeader()),null, false));
        }}
    if(xSet.getLayout("ParameterSelect").show()) frSelect=
        createFrame("ParameterSelect",selIcon,selpan,xSet.getLayout("ParameterSelect"),null, false);
    if(xSet.getLayout("Parameter").show()) frParameters=
        createFrame("Parameters",paramIcon,parpan,xSet.getLayout("Parameter"),null, true); 
    if(xSet.getLayout("Meter").show()) metpan.setListener(frMeters=
        createFrame("RateMeters",meterIcon,metpan,xSet.getLayout("Meter"),metpan.createMenuBar(), false));
    if(xSet.getLayout("Histogram").show()) hispan.setListener(frHistograms=
        createFrame("Histograms",histoIcon,hispan,xSet.getLayout("Histogram"),hispan.createMenuBar(), false));
    if(xSet.getLayout("State").show()) stapan.setListener(frState=
        createFrame("States",stateIcon,stapan,xSet.getLayout("State"),stapan.createMenuBar(), false));
    if(xSet.getLayout("Info").show()) infpan.setListener(frInfos=
        createFrame("Infos",infoIcon,infpan,xSet.getLayout("Info"),infpan.createMenuBar(), true));

// Make dragging a little faster but perhaps uglier.
    desktop.setDragMode(JDesktopPane.OUTLINE_DRAG_MODE);
	//Display the window.
    default:
    setVisible(true);
    progframe.dispose();
}} // run and switch
//----------------------- startup finished -----------------------

private JToolBar createToolBar() {
//Create the toolbar.
    JButton button;
    toolBar = new JToolBar();
    toolBar.setMargin(new Insets(-4,0,-6,0));
    toolBar.add(new toolButton(maQuit));
    if(xSet.isGuru()) toolBar.add(new toolButton(maWork));
    toolBar.add(new toolButton(maSave));
    toolBar.addSeparator();
    if(xSet.isDabs()) toolBar.add(new toolButton(maDabcMbsController));
    if(xSet.isDabc()) toolBar.add(new toolButton(maDabcController));
    if(xSet.isMbs())  toolBar.add(new toolButton(maMbsController));
    toolBar.add(new toolButton(maBrowser));
    toolBar.addSeparator();
    if(xSet.isControl()) toolBar.add(new toolButton(maCommands));
    toolBar.add(new toolButton(maParameters));
    toolBar.add(new toolButton(maSelect));
    toolBar.add(new toolButton(maMeters));
    toolBar.add(new toolButton(maHistograms));
    toolBar.add(new toolButton(maState));
    toolBar.add(new toolButton(maInfos));
    if(xSet.isControl()) toolBar.add(new toolButton(maLogger));
    if(usrpan != null){
    	if(xSet.isControl()) toolBar.add(new toolButton(maUserController.get(0))); // RunInfo
        toolBar.addSeparator();
    	for(int ii=1;ii<usrpan.size();ii++)
     	toolBar.add(new toolButton(maUserController.get(ii)));
    }
    return toolBar;
}

// needed for pulldown menus:
// protected JMenuBar createTopMenuBar() {
    // JMenuItem menuItem;
    // JMenuBar menuBar = new JMenuBar();
    // JMenu wmenu = new JMenu("File");
    // wmenu.setMnemonic(KeyEvent.VK_F);
    // menuBar.add(wmenu);
    // wmenu.add(new JMenuItem(maDabcController));
    // wmenu.add(new JMenuItem(maMbsController));
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

private void printFrames(){
    JInternalFrame[] fr=desktop.getAllFrames();
    for(int i=0;i<fr.length;i++)
      System.out.println("Frame "+fr[i].getTitle());
}
// xiDesktop implementation
/**
 * Checks if a frame exists on the desktop .
 * @param title Title of the frame to searched for.
 * @return true if frame with specified title exists, or false.
 */
public boolean findFrame(String title){
    JInternalFrame[] fr=desktop.getAllFrames();
    for(int i=0;i<fr.length;i++){
//    System.out.println("Look for "+title+" Found "+fr[i].getTitle());
        if(fr[i].getTitle().equals(title)) return true;
        }
    return false;
}

//xiDesktop implementation
/**
 * Update command list
 */
public void rebuildCommands(){
    String str=mbspan.getTaskList();
    compan=new xPanelCommand(browser,xSet.getLayout("Command").getSize(),str);
    if(usrpan != null){
    	if(usrpan.size()>1)compan.setUserCommand(usrpan.get(1).getUserCommand());
    }
    compan.setCommandDescriptors(parpan.getCommandDescriptors()); 
    if(frCommands   != null) frCommands.addWindow(compan);
	System.out.println("----- rebuild commands finished");
}
/**
 * Update DIM parameter list
 * @param cleanup remove all services first
 */
public void updateDim(boolean cleanup){
	updateParameters(true, cleanup);
}
// xiDesktop implementation
/**
 * Adds a frame to desktop if a frame with same title does not exist.
 * @param frame Frame to put on desktop. Frame will be managed.
 */
public void addFrame(JInternalFrame frame){
//System.out.println("Add frame "+frame.getTitle()+" to desktop ...");
addFrame(frame, true);
}
// xiDesktop implementation
/**
 * Adds a frame to desktop.
 * @param frame Frame to put on desktop. 
 * @param manage If true, frame will be managed by GUI: layout is saved and restored.
 */
public void addFrame(JInternalFrame frame, boolean manage){
boolean create=true;
//System.out.println("Add frame \""+frame.getTitle()+"\" to desktop");
if(!findFrame(frame.getTitle())){
	//JInternalFrame[] fr=desktop.getAllFrames(); // length is next index
    //System.out.println("Added frame "+fr.length+" \""+frame.getTitle()+"\" to desktop");
    desktop.add(frame); 
//    fr=desktop.getAllFrames();
//    for(int i=fr.length-1;i>=0;i--)
//    	System.out.println("        frame "+i+" \""+fr[i].getTitle()+"\"");
    frame.moveToFront();
    if(manage){
        for(int i=0;i<frUserFrame.size();i++)
            if(frUserFrame.get(i).getTitle().equals(frame.getTitle())) create=false;
        if(create) {
        //System.out.println("Added frame "+frUserFrame.size()+" \""+frame.getTitle()+"\" to list");
        frUserFrame.add(frame); 
}}}}

// xiDesktop implementation
/**
 * Remove (dispose) a frame from the desktop and list of managed frames.
 * @param title Title of the frame to be removed.
 */
public void removeFrame(String title){
	//System.out.println("Frame \""+title+"\" to remove");
	JInternalFrame[] fr=desktop.getAllFrames();
for(int i=fr.length-1;i>=0;i--) {
//System.out.println("        frame "+i+" \""+fr[i].getTitle()+"\"");
    if(fr[i].getTitle().equals(title)) {
        fr[i].dispose();
        //System.out.println("Disposed frame \""+title+"\" from desktop");
    }}
for(int i=frUserFrame.size()-1;i>=0;i--)
    if(frUserFrame.get(i).getTitle().equals(title)){
        frUserFrame.remove(i);
        //System.out.println("Removed frame "+i+" \""+title+"\" from list");
}
}

// xiDesktop implementation
/**
 * Switch a frames selection state (setSelected).
 * @param title Title of the frame to be selected.
 * @param select passed to setSelected method of frame.
 */
public void setFrameSelected(String title, boolean select){
JInternalFrame[] fr=desktop.getAllFrames();
try{
    for(int i=0;i<fr.length;i++) 
        if(fr[i].getTitle().equals(title)) fr[i].setSelected(select);
    } catch(PropertyVetoException e){}
}

// xiDesktop implementation
/**
 * Set frames to front.
 * @param title Title of the frame.
 */
public void toFront(String title){
JInternalFrame[] fr=desktop.getAllFrames();
try{
for(int i=0;i<fr.length;i++) 
        if(fr[i].getTitle().equals(title)) fr[i].toFront();
}catch(ArrayIndexOutOfBoundsException x){System.out.println("Frame \""+title+"\" tofront exc");}
}

public void updateParameters(boolean doit, boolean cleanup){
if(doit){
    clearOnUpdate=cleanup;
    clearOnUpdate=false; // never cleanup for now
    // clear all references to DIM services
	System.out.println("----- update, clear: "+clearOnUpdate);
    mbspan.releaseDimServices();
    dabcpan.releaseDimServices();
    dbspan.releaseDimServices();
    selpan.releaseDimServices();
    if(usrpan != null){
    	for(int ii=0;ii<usrpan.size();ii++)
    		usrpan.get(ii).releaseDimServices();
    }
    parpan=null;
    compan=null;
    System.gc(); // garbage collect now to save time later
    browser.releaseServices(clearOnUpdate); // remove all parameters
    clearOnUpdate=false;
    browser.initServices("*"); 
 // waits until all parameters are registered and updated:
    parpan=new xPanelParameter(browser,xSet.getLayout("Parameter").getSize());
    String str=mbspan.getTaskList();
    compan=new xPanelCommand(browser,xSet.getLayout("Command").getSize(),str);
    mbspan.setDimServices();
    dabcpan.setDimServices();
    dbspan.setDimServices();
    selpan.setDimServices();
    if(usrpan != null){
    	for(int ii=0;ii<usrpan.size();ii++)
    		usrpan.get(ii).setDimServices(browser);
    	if(usrpan.size()>1)compan.setUserCommand(usrpan.get(1).getUserCommand());
    }
    browser.enableServices();
    
//put list of command descriptors from parameters to commands:
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
	System.out.println("----- update finished");
}
return;
}

// React to menu selections of main window.
/**
 * Central action switch.<br>
 * <ul>
 * <li>Update<br>
 * releaseDimServices() of all form panels.<br>
 * Browser releaseServices (deactivate all parameters and remove user handlers).<br>
 * Browser initServices (get new list of services). Merge new ones into existing list.<br>
 * Create new parameter panel (Wait for update of all parameters, 
 * cleanup graphics panels, build new table, mark all parameters as not shown to rebuild graphics,
 * build new command definition list).<br>
 * Create new command panel (build command tree)<br>
 * updateAll graphical panels.<br>
 * setDimServices of all form panels<br>
 * enableServices (activate all parameters)<br>
 * Pass command descriptors to command panel<br>
 * Replace parameter, command, and info panel in their window frames.
 * </ul>
 * @param e event. Switch on action command:<br>
 */
public void actionPerformed(ActionEvent e) {
	if(e.getActionCommand() == null) ActionCommand=xSet.getActionCommand();
	else                          ActionCommand=e.getActionCommand();
    if ("DabcMbsController".equals(ActionCommand)) {dbspan.setListener(frDabcMbsController=
        createFrame("DabcMbsController",dabcmbsIcon,dbspan,xSet.getLayout("DabcMbsController"),null, false));
        xSet.setLayout("DabcMbsController",null,null,0, true);
        }
    else if ("DabcController".equals(ActionCommand)) {dabcpan.setListener(frDabcController=
        createFrame("DabcController",dabcIcon,dabcpan,xSet.getLayout("DabcController"),null, false));
        xSet.setLayout("DabcController",null,null,0, true);
        }
    else if ("MbsController".equals(ActionCommand)) {mbspan.setListener(frMbsController=
        createFrame("MbsController",mbsIcon,mbspan,xSet.getLayout("MbsController"),null, false)); 
        xSet.setLayout("MbsController",null,null,0, true);
        }
    else if ("ParameterSelect".equals(ActionCommand)) {frSelect=
        createFrame("ParameterSelect",selIcon,selpan,xSet.getLayout("ParameterSelect"),null, false); 
        xSet.setLayout("ParameterSelect",null,null,0, true);
        }
    else if ("Commands".equals(ActionCommand)) {frCommands=
        createFrame("Commands",commIcon,compan,xSet.getLayout("Command"),null, true);
        xSet.setLayout("Command",null,null,0, true);
        }
    else if ("Parameters".equals(ActionCommand)) {frParameters=
        createFrame("Parameters",paramIcon,parpan,xSet.getLayout("Parameter"),null, true);
        xSet.setLayout("Parameter",null,null,0, true);
        }
    else if ("Meters".equals(ActionCommand)) {metpan.setListener(frMeters=
        createFrame("RateMeters",meterIcon,metpan,xSet.getLayout("Meter"),metpan.createMenuBar(), false));
        xSet.setLayout("Meter",null,null,0, true);
        }
    else if ("Histograms".equals(ActionCommand)) {hispan.setListener(frHistograms=
        createFrame("Histograms",histoIcon,hispan,xSet.getLayout("Histogram"),hispan.createMenuBar(), false));
        xSet.setLayout("Histogram",null,null,0, true);
        }
    else if ("States".equals(ActionCommand)) {stapan.setListener(frState=
        createFrame("State",stateIcon,stapan,xSet.getLayout("State"),stapan.createMenuBar(), false));
        xSet.setLayout("State",null,null,0, true);
        }
    else if ("Infos".equals(ActionCommand)) {infpan.setListener(frInfos=
        createFrame("Infos",infoIcon,infpan,xSet.getLayout("Info"),infpan.createMenuBar(), true));
        xSet.setLayout("Info",null,null,0, true);
        }
    else if ("Logger".equals(ActionCommand)) {logpan.setListener(frLogger=
        createFrame("Logger",loggerIcon,logpan,xSet.getLayout("Logger"),logpan.createMenuBar(), true));
        xSet.setLayout("Logger",null,null,0, true);
        }
    else if ("Test".equals(ActionCommand)) {
    	printFrames();
    	String filter=JOptionPane.showInputDialog("Filter","*");
        browser.listServices(false, filter); // exclude command definitions
    }
    else if ("Quit".equals(ActionCommand)) {
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
    else if ("Save".equals(ActionCommand)) {
    
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
        if(frDabcController != null) 
        xSet.setLayout("DabcController",
            frDabcController.getLocation(),frDabcController.getSize(),0,xSet.getLayout("DabcController").show());
        if(frMbsController  != null) 
        xSet.setLayout("MbsController",
            frMbsController.getLocation(),frMbsController.getSize(),0, xSet.getLayout("MbsController").show());
        if(frDabcMbsController  != null) 
            xSet.setLayout("DabcMbsController",
                frDabcMbsController.getLocation(),frDabcMbsController.getSize(),0,xSet.getLayout("DabcMbsController").show());
        if(frSelect  != null) 
            xSet.setLayout("ParameterSelect",
                frSelect.getLocation(),frSelect.getSize(),0,xSet.getLayout("ParameterSelect").show());
        if(frUserController  != null) 
        	for(int ii=0;ii<usrpan.size()&ii<frUserController.size();ii++)
        	xSet.setLayout(usrpan.get(ii).getHeader(),
            frUserController.get(ii).getLocation(),frUserController.get(ii).getSize(),0,xSet.getLayout(usrpan.get(ii).getHeader()).show());

        if(frUserFrame  != null) for(int i = 0; i < frUserFrame.size();i++)
        xSet.setLayout(frUserFrame.get(i).getTitle(),
                frUserFrame.get(i).getLocation(),
                frUserFrame.get(i).getSize(),0,
                xSet.getLayout(frUserFrame.get(i).getTitle()).show());
        xSet.setLayout("Main", getLocation(), getSize() , 0, true);
        parpan.saveColWidth();
        xSaveRestore.saveLayouts(LayoutFile);
        xSaveRestore.saveCommands(browser.getCommandList(),CommandFile);
        xSaveRestore.saveRecords(browser.getParameters(),RecordFile);
        selpan.saveSetup(SelectionFile);
        JOptionPane.showInternalMessageDialog(
            desktop, "Layout saved: "+RecordFile+" "+SelectionFile+" "+LayoutFile, "Information",JOptionPane.INFORMATION_MESSAGE);
    }
    else if (("Update".equals(ActionCommand)) ||
            ("Rebuild".equals(ActionCommand)) ||
            ("Browser".equals(ActionCommand))) {
        if("Browser".equals(ActionCommand)){
        	if(xSet.isProcessing()) { // MBS or DABC command thread running
        		System.out.println("Busy, no update");
        		return;
        	}
        	clearOnUpdate=true;
            updateParameters(true,true);
            return;
        }
        if("Rebuild".equals(ActionCommand))	updateParameters(true,true);
        else updateParameters(true,false);
    }
    else if ("RebuildCommands".equals(ActionCommand)) {
    	rebuildCommands();
    }
    else if ("DisplayFrame".equals(ActionCommand)) { //used for progress window
    	lastframe=((xInternalFrame)e.getSource()).getTitle();
        addFrame((xInternalFrame)e.getSource(), false); // do not manage
    }
    else if ("RemoveFrame".equals(ActionCommand)) { //used for progress window
        //removeFrame(((xInternalFrame)e.getSource()).getTitle()); // Java 6
        removeFrame(lastframe);
    }
    else if(usrpan != null){ 
    	for(int ii=0;ii<usrpan.size();ii++)
    	if(usrpan.get(ii).getHeader().equals(ActionCommand)) {frUserController.add(
        createFrame(usrpan.get(ii).getHeader(),usrpan.get(ii).getIcon(),(JPanel)usrpan.get(ii),xSet.getLayout(usrpan.get(ii).getHeader()),null, false));
        xSet.setLayout(usrpan.get(ii).getHeader(),null,null,0, true);
        }}
    else {
    System.out.println("Unsolicited event:"+ActionCommand);
    }
} // actionPerformed


//Quit the application.
protected void quit() {
    xSaveRestore.saveCommands(browser.getCommandList(),CommandFile);
	if(xSet.isGuru())System.exit(0);
    int i=JOptionPane.showInternalConfirmDialog(desktop,"Quit GUI?","GUI can be restarted.", JOptionPane.YES_NO_OPTION); 
    if(i == 0) System.exit(0);
}

private xInternalFrame  createFrame(String title, ImageIcon icon, JPanel panel, xLayout la, JMenuBar menu, boolean resize){
    if(!findFrame(title)){
        xInternalFrame frame = new xInternalFrame(title,la);
        frame.setupFrame(icon, menu, panel, resize);  // true for resizable
        desktop.add(frame);
        frame.moveToFront();
        return frame;
    } else return null;
}

private class menuAction extends AbstractAction {
    private ActionListener action;
    private menuAction(String text, ImageIcon icon,
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
private class toolButton extends JButton {
private toolButton(menuAction ma){
    super(ma);
    setText("");
    setBorderPainted(false);
}}
}

