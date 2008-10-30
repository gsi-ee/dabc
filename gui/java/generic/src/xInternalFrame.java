package xgui;
import javax.swing.JInternalFrame;
import javax.swing.JPanel;
import javax.swing.JMenuBar;
import javax.swing.ImageIcon;

import javax.swing.event.*;
import java.awt.event.*;
import java.awt.*;

public class xInternalFrame extends JInternalFrame
                            implements ActionListener, InternalFrameListener, ComponentListener
{
private String name;
private xLayout layout; 
private JPanel mypan;
private boolean hasMenu=false;
private boolean resizable=true;
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
    
public void addWindow(JPanel pan){
    getContentPane().removeAll();
    getContentPane().add(pan);
    getContentPane().setBackground(xSet.getColorBack());
    mypan=pan;
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
    if(e == null) pack();
    else if ("Zoom".equals(e.getActionCommand())) pack();
    // setResizable(resizable);
}
// Frame listener
public void internalFrameClosing(InternalFrameEvent e) {
//System.out.println("Frame closing: "+getTitle());
layout.set(getLocation(),getSize(),0,false);
}
public void internalFrameClosed(InternalFrameEvent e){
//System.out.println("Frame closed : "+getTitle());
layout.set(getLocation(),getSize(),0,false);
}
public void internalFrameActivated(InternalFrameEvent e){}
public void internalFrameDeactivated(InternalFrameEvent e){}
public void internalFrameDeiconified(InternalFrameEvent e){}
public void internalFrameIconified(InternalFrameEvent e){}
public void internalFrameOpened(InternalFrameEvent e){}
// Component listener
public void componentHidden(ComponentEvent e) {}
public void componentMoved(ComponentEvent e) {}
public void componentShown(ComponentEvent e) {}
public void componentResized(ComponentEvent e) {
Dimension nd;
// panel must implement ComponentListener, and resize itself if necessary
if(resizable){ // size of mypan must follow size of this frame
    if(hasMenu)
         nd=xSet.addDimension(getSize(), new Dimension(-10,-53));
    else nd=xSet.addDimension(getSize(), new Dimension(-10,-32));
    mypan.setPreferredSize(nd);
    // System.out.println("intfram "+mypan.getClass()+" "+nd);
}
else { // Frame size determined by mypan size
    // System.out.println("intfram "+mypan.getClass()+" pref "+mypan.getPreferredSize());
    // System.out.println("intfram "+mypan.getClass()+" get  "+mypan.getSize());
}
}
}