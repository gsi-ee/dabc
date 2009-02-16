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
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JOptionPane;
//import javax.swing.JSplitPane;
import javax.swing.UIManager;
import javax.swing.ImageIcon;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JMenuBar;
//import javax.swing.GroupLayout;

import java.net.URL;
import java.io.IOException;
//import java.awt.*;
import java.awt.Color;
import java.awt.Point;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import java.awt.event.*;
import java.awt.Insets;
//import java.awt.event.ActionListener;
//import java.awt.event.ActionEvent;
//import java.awt.event.MouseAdapter;
import java.util.*;
/**
 * Panel for set of histogram panels.
 * @author Hans G. Essel
 * @version 1.0
 * @see xDesktop
 * @see xHisto
 */
public class xPanelHisto extends JPanel implements ActionListener {
private int i,indent;
private int elements=0,columns=3;
private Object obj;
private xHisto histo;
private int[] histobuffer=new int[1000];
private GridBagConstraints c;
private JScrollPane metpan;
private JPanel pan;
private Vector<Object> list;
private Dimension panSize;
private ActionListener action=null;
// private GroupLayout.SequentialGroup vGroup;
// private GroupLayout.ParallelGroup pGroup;
/**
 * Uses JPanel with GridBagLayout. Assumes that all items have same size.
 * Items are ordered in lines, each line has same number of columns (except last one).
 * Number of columns define number of lines.
 * @param dim Dimension
 * @param col Number of columns
 */
public xPanelHisto(Dimension dim, int col) {
    super(new GridLayout(1,0));
    columns=col;
    list=new Vector<Object>();
    c = new GridBagConstraints();
    c.fill = GridBagConstraints.HORIZONTAL;
    c.weightx = 0.0;
    c.weighty = 0.0;
    c.gridwidth = 1;
    c.gridheight = 1;
    c.insets = new Insets(0,0,0,0);
    setBackground(xSet.getColorBack());
    pan=new JPanel(new GridBagLayout());
    pan.setBackground(xSet.getColorBack());
// Only with 1.6
// GroupLayout layout = new GroupLayout(pan);
// pan.setLayout(layout);
// layout.setAutoCreateGaps(true);
// layout.setAutoCreateContainerGaps(true);
// vGroup = layout.createSequentialGroup();
// layout.setVerticalGroup(vGroup);
    metpan=new JScrollPane(pan);
    metpan.setBackground(xSet.getColorBack());
    panSize = new Dimension(dim);
    pan.setPreferredSize(panSize);
// panSize.setSize(xs+30, ys+30);
// metpan.setPreferredSize(panSize);
// setPreferredSize(panSize);
    add(metpan);
}
/**
 * @param col New number of columns. Calls updateAll().
 */
public void setColumns(int col){
    columns=col;
    updateAll();
}
public int getColumns(){
    return columns;
}
/**
 * @param histo Removes histogram. Calls updateAll().
 */
public void removeHistogram(xHisto histo){
    list.remove(histo);
    elements--;
    updateAll();
}

/**
 * Cleanup item list and panel.
 */
public void cleanup(){
    list=new Vector<Object>();
    elements=0;
    pan.removeAll();
}
/**
 * Removes all items from panel, resize and rebuild all items.
 */
public void updateAll(){
    pan.removeAll();
    adjustSize();
    for(int i=1;i<=elements;i++){
        if((i/columns)*columns == i) c.gridwidth = GridBagConstraints.REMAINDER;
        else c.gridwidth = 1;
        pan.add((xHisto)list.elementAt(i-1),c); //,c
    }
    pan.revalidate();
    if(action!=null)action.actionPerformed(null);
}

private void adjustSize(){
    int rows=1;
    if(elements>0){
        rows=(elements-1)/columns+1;
        panSize=((xHisto)list.elementAt(0)).getPreferredSize();
    }
    rows=rows*((int)panSize.getHeight());
    int cols=columns*((int)panSize.getWidth());
    pan.setPreferredSize(new Dimension(cols,rows));
    setPreferredSize(new Dimension(cols+3,rows+3));
}
/**
 * Add histogram item to internal table.
 * @param histo Histogram.
 * @param update True: updateAll, false: no graphics update. Several histograms can be added without
 * redrawing the panel each time. The last one should. 
 */
public void addHistogram(xHisto histo, boolean update){
addHistogram(histo);
if(update)updateAll();
}
/**
 * Add histogram item to internal table. No redraw.
 * @param histo Histogram.
*/
public void addHistogram(xHisto histo){
    histo.setActionListener(this);
    histo.setSizeXY();
// only 1.6
// if((elements/3)*3 == elements) {
    // pGroup=layout.createParallelGroup();
    // vGroup.addGroup(pGroup);
    // }
// pGroup.addComponent(histo);
//
    histo.setID(elements);
    elements += 1;
// just adding the new histo would put all in same row!
// if((elements/columns)*columns == elements) c.gridwidth = GridBagConstraints.REMAINDER;
// else c.gridwidth = 1;
// pan.add(histo);
    list.add(histo);
    //updateAll();
}
// Called by "Size" menu item."
private void askSize(){
int ix,iy;
    histo=(xHisto)list.elementAt(0);
    String str=JOptionPane.showInputDialog("Enter size: x,y",
        new String(Integer.toString(histo.getSizeX())+","+Integer.toString(histo.getSizeY())));
    if(str != null){
    String f[]=str.split(",");
    ix=Integer.parseInt(f[0]);
    iy=Integer.parseInt(f[1]);
    for(int i=0;i<elements;i++){
        ((xHisto)list.elementAt(i)).setSizeXY(new Dimension(ix,iy));
    }
    updateAll();
    pan.revalidate();
    if(action!=null)action.actionPerformed(null); // xInternalFrame will pack.
    }
}
public JMenuBar createMenuBar() {
JMenuItem menuItem;
    JMenuBar menuBar = new JMenuBar();
    JMenu cmenu = new JMenu("Layout");
    cmenu.setMnemonic(KeyEvent.VK_C);
    menuBar.add(cmenu);
    menuItem=new JMenuItem("Size");
    cmenu.add(menuItem);
    menuItem.addActionListener(this);
    for(int i =1;i<9;i++){
        menuItem=new JMenuItem(Integer.toString(i)+" x n");
        menuItem.addActionListener(this);
        cmenu.add(menuItem);
    }
    return menuBar;
}
/**
 * This is called directly after creating the internal frame
 * where this is in. Listener is xInternalFrame.
 * After resizing the items by "Size" event this action listener is
 * called with null event to pack the frame.
 * @param actionlistener Frame containing this panel.
 */
public void setListener(ActionListener actionlistener){
    action=actionlistener;
}
//React to menu selections.
/**
 * Event handler.<br>
 * <b>Large</b>: Called through xHisto handler which wants to be displayed in an extra frame.
 * Histogram is cloned and a new frame is created. This frame will be action handler for
 * that histogram.
 */
public void actionPerformed(ActionEvent e) {
if("Large".equals(e.getActionCommand())){
    int index=((xHisto)e.getSource()).getID();
    histo=(xHisto)list.elementAt(index);
    String title=new String(histo.getName());
    xLayout la = new xLayout(title);
    la.set(new Point(index*30,index*30), null, 0, true); // position
    xInternalFrame extFrame = new xInternalFrame(title,la);
    xHisto extHisto=histo.getClone(title);
    extHisto.setSizeXY(new Dimension(xHisto.XSIZE_LARGE,xHisto.YSIZE_LARGE));
    extHisto.hasParent();
    extFrame.setupFrame(null, null, extHisto, false);  // true for resizable
    xSet.getDesktop().add(extFrame);
    xSet.getDesktop().setSelectedFrame(extFrame);
    extFrame.moveToFront();
    histo.setExternHisto(extHisto,extFrame);
    // add the frame as action listener! Frame must resize when size of histogram panel changes.
    // This action listener is called with "MyResize"
    extHisto.setActionListener(extFrame);
}
else if ("Size".equals(e.getActionCommand())){
askSize();
}
else if ("Fit".equals(e.getActionCommand())) action.actionPerformed(e);
else if ("1 x n".equals(e.getActionCommand())) setColumns(1);
else if ("2 x n".equals(e.getActionCommand())) setColumns(2);
else if ("3 x n".equals(e.getActionCommand())) setColumns(3);
else if ("4 x n".equals(e.getActionCommand())) setColumns(4);
else if ("5 x n".equals(e.getActionCommand())) setColumns(5);
else if ("6 x n".equals(e.getActionCommand())) setColumns(6);
else if ("7 x n".equals(e.getActionCommand())) setColumns(7);
else if ("8 x n".equals(e.getActionCommand())) setColumns(8);
}
}



