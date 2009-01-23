import xgui.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.ImageIcon;
import javax.swing.JTextField;
import javax.swing.JCheckBox;

public class MiniPanel  extends xPanelPrompt
                      implements  xiUserPanel,
                                  ActionListener {
private String name,fname;
private ImageIcon menuIcon,graphIcon;
private String tooltip;
private JTextField prompt;
private JCheckBox check;
private xiDesktop desk;
private xInternalCompound frame; 
private xPanelGraphics metpan, stapan;
private xState state;
private xiDimParameter param;
private xLayout panelLayout, frameLayout;
//----------------------------------------------------------
private class myInfoHandler implements xiUserInfoHandler{
private xMeter meter;
private xState state;
private String name;

private myInfoHandler(String Name, xMeter Meter, xState State){
	name = new String(Name); // store
	meter=Meter; // store
	state=State; // store
}
public String getName(){return name;
}
public void infoHandler(xiDimParameter P){
	if(meter != null) meter.redraw(
		P.getMeter().getValue(),true, true);
	if(state != null) state.redraw(
		P.getState().getSeverity(),
		P.getState().getColor(),
		P.getState().getValue(),true);
}
}
//=========================================================
// Constructor without arguments
public MiniPanel(){
   super("MyPanel");
   menuIcon=xSet.getIcon("icons/usericon.png");
   graphIcon=xSet.getIcon("icons/usergraphics.png");
   name=new String("MyPanel"); // name for prompter panel
   panelLayout=xSet.getLayout(name); // restored from setup?
   if(panelLayout==null) // no, create
   panelLayout=xSet.createLayout(name,new Point(100,200), new Dimension(100,75),1,true);
   tooltip=new String("Launch my panel");
}
// xiUserPanel interface functions
public String getToolTip(){return tooltip;
}
public String getHeader(){return name;
}
public ImageIcon getIcon(){return menuIcon;
}
public xLayout checkLayout(){return panelLayout;
}
public xiUserCommand getUserCommand(){return null;
}
public void init(xiDesktop desktop, ActionListener al){
	desk=desktop; // save
	prompt=addPrompt("My Command: ","Defaultvalue","prompt",20,this);
	addTextButton("This is a test button","button","Tool tip, whatever it does",this);
	check=addCheckBox("Data server on/off","check",this);
	addButton("Display","Display info",graphIcon,this);
	state  = new xState("ServerState", xState.XSIZE,xState.YSIZE);
	stapan = new xPanelGraphics(new Dimension(160,50),1); // one column of states
	metpan = new xPanelGraphics(new Dimension(410,14),3); // one columns of meters
	fname=new String("MyGraphics"); // name for display frame
	frameLayout = xSet.getLayout(fname);
	if(frameLayout == null)
	frameLayout = xSet.createLayout(fname,new Point(200,200), new Dimension(100,75),1,true);
	frame  = new xInternalCompound(fname,graphIcon,0,frameLayout,xSet.blueD());
	desk.addFrame(frame); // display
}
public void setDimServices(xiDimBrowser browser){
	Vector<xiDimParameter> vipar=browser.getParameters();
	for(int i=0;i<vipar.size();i++){
	  xiParser p=vipar.get(i).getParserInfo();
	  String metername=new String(p.getNode()+":"+p.getName());
	  String statename=new String(p.getNode()+":"+p.getApplication());
	  if(p.isRate()){ 
		  if(p.getFull().indexOf("Rate")>0){ 
	      xMeter meter=new xMeter(xMeter.ARC,
	        metername,0.0,10.0,xMeter.XSIZE,xMeter.YSIZE,xSet.blueL());
	      meter.setLettering(p.getNode(),p.getName(),vipar.get(i).getMeter().getUnits(),"");
	      metpan.addGraphics(meter,false); 
	      browser.addInfoHandler(vipar.get(i),new myInfoHandler(metername,meter,null));
	  }} else if(p.isState()){ 
		  if((p.getFull().indexOf("Acquis")>0)| 
		    (p.getFull().indexOf("RunMode")>0)){ 
	     xState state=new xState(statename,xState.XSIZE,xState.YSIZE);
	     stapan.addGraphics(state,false);
	     browser.addInfoHandler(vipar.get(i),new myInfoHandler(statename,null,state));
	  }} else if(p.getFull().indexOf("Setup_File")>0) param=vipar.get(i);
	} // end list of parameters
	stapan.addGraphics(state,false);
	stapan.updateAll();
	metpan.updateAll();
	if(frame != null) frame.rebuild(stapan, metpan);
}
public void releaseDimServices(){
    metpan.cleanup();
    stapan.cleanup();
    param=null;
}
private void print(String s){
	System.out.println(s);
}
public void actionPerformed(ActionEvent e) {
	String cmd=e.getActionCommand();
	if ("prompt".equals(cmd)) {
	  print(cmd+":"+prompt.getText()+" "+check.isSelected());
	} else if ("button".equals(cmd)) {
	  print(cmd+":"+prompt.getText()+" "+check.isSelected());
	} else if ("check".equals(cmd)) {
	  print("Data server "+check.isSelected());
	  if(check.isSelected()){
	    if(param != null)param.setParameter("0");
	    state.redraw(0,"Green","Active",true);
	  } else {
	    if(param != null)param.setParameter("1");
	    state.redraw(0,"Gray","Dead",true);
	}} else if ("Display".equals(cmd)) {
	  print(cmd+" "+fname);
	  if(!desk.findFrame(fname)){
	    frame=new xInternalCompound(fname,graphIcon,0,frameLayout,xSet.blueD());
	    frame.rebuild(stapan, metpan); 
	    desk.addFrame(frame); 
	}}
	else print("Unknown "+cmd);
}
}
