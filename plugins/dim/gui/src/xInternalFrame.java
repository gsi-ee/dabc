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
import javax.swing.JInternalFrame;
import javax.swing.JPanel;
import javax.swing.JMenuBar;
import javax.swing.ImageIcon;

import javax.swing.event.*;
import java.awt.event.*;
import java.awt.*;

/**
 * Frame for one panel. To be added by caller to a JDesktopPane.
 * Provides functions to add one panel.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xInternalFrame extends JInternalFrame
                            implements ActionListener, InternalFrameListener, ComponentListener
{
private String name;
private xLayout layout; 
private JPanel mypan;
private boolean hasMenu=false;
private boolean resizable=true;
/**
 * Create the frame. Set background and layout.
 * @param title Title of frame.
 * @param la Frame layout (position and size).
 */
public xInternalFrame(String title, xLayout la) {
    super(title,
            true, //resizable
            true, //closable
            true, //maximizable
            true);//iconifiable
    pack();
    setBackground(xSet.getColorBack());
    //Set the window's location.
    layout=la;
    setLocation(la.getPosition());
    name=title;
    addInternalFrameListener(this);
    addComponentListener(this);
    mypan=null;
    }
/**
 * Set up frame and add panel.
 * @param icon Give it an icon.
 * @param menu Optional menu bar. Event handler defined from caller.
 * @param panel Panel to be displayed.
 * @param resize Make frame resizable.
 */
public void setupFrame(ImageIcon icon, JMenuBar menu, JPanel panel, boolean resize) {
    if(icon != null)setFrameIcon(icon);
    if(menu != null){setJMenuBar(menu);hasMenu=true;}
    setResizable(resize);
    resizable=resize;
    addWindow(panel);
    setVisible(true);
    try { setSelected(true);
    } catch (java.beans.PropertyVetoException pe) {System.out.println("Frame select failed");}
}   
/**
 * Remove all panels, add this one, and pack.
 * @param panel Panel to display.
 */
public void addWindow(JPanel panel){
    getContentPane().removeAll();
    getContentPane().add(panel);
    getContentPane().setBackground(xSet.getColorBack());
    mypan=panel;
    pack();
}
public JPanel getPanel(){return mypan;}
public xLayout getFrameLayout(){return layout;}

// Action listener
//React to menu selections from panels
public void actionPerformed(ActionEvent e) {
Dimension nd;
// if(hasMenu)
     // nd=xSet.addDimension(mypan.getSize(), new Dimension(10,53));
// else nd=xSet.addDimension(mypan.getSize(), new Dimension(10,32));
    // setPreferredSize(nd);
// System.out.println("intfram "+mypan.getClass()+" "+nd);
    // setResizable(true);
if(e == null) {
    mypan.revalidate();
    pack();
} else if ("Zoom".equals(e.getActionCommand())) pack();
else if ("MyResize".equals(e.getActionCommand())){
    mypan.revalidate();
    pack();
}
}
/**
 * Store layout (position and size)
 */
public void internalFrameClosing(InternalFrameEvent e) {
//System.out.println("Frame closing : "+getTitle()+" "+layout.getName()+" false");
	layout.set(getLocation(),getSize(),0,false);
}
/**
 * Store layout (position and size)
 */
public void internalFrameClosed(InternalFrameEvent e){
//System.out.println("Frame closed : "+getTitle()+" "+layout.getName()+" false");
	layout.set(getLocation(),getSize(),0,false);
}
public void internalFrameActivated(InternalFrameEvent e){}
public void internalFrameDeactivated(InternalFrameEvent e){}
public void internalFrameDeiconified(InternalFrameEvent e){}
public void internalFrameIconified(InternalFrameEvent e){}
public void internalFrameOpened(InternalFrameEvent e){}
// Component listener
public void componentHidden(ComponentEvent e) {}
public void componentMoved(ComponentEvent e) {
	//System.out.println("Frame moved : "+getTitle()+" "+layout.getName());
	layout.set(getLocation(),getSize(),0);
}
public void componentShown(ComponentEvent e) {}
/**
 * Set new preferred size when resizable was enabled, otherwise noop.
 */
public void componentResized(ComponentEvent e) {
Dimension nd;
// panel must implement ComponentListener, and resize itself if necessary
if(resizable){ // size of mypan must follow size of this frame
    if(hasMenu)
         nd=xSet.addDimension(getSize(), new Dimension(-10,-53));
    else nd=xSet.addDimension(getSize(), new Dimension(-10,-32));
    mypan.setPreferredSize(nd);
	layout.set(getLocation(),getSize(),0);
    //System.out.println("intfram "+mypan.getClass()+" "+nd);
}
else { // Frame size determined by mypan size
    // System.out.println("intfram "+mypan.getClass()+" pref "+mypan.getPreferredSize());
    // System.out.println("intfram "+mypan.getClass()+" get  "+mypan.getSize());
}
}
}
