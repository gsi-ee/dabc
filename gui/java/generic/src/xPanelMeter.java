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
import javax.swing.JScrollPane;
import javax.swing.JOptionPane;
import javax.swing.UIManager;
import javax.swing.ImageIcon;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JMenuBar;
import java.awt.event.*;
import java.awt.Insets;
//import javax.swing.GroupLayout;

import java.net.URL;
import java.io.IOException;
import java.awt.Color;
import java.awt.Point;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import java.awt.event.*;
import java.awt.Insets;
import java.util.*;
/**
 * Panel for set of meter panels.
 * @author Hans G. Essel
 * @version 1.0
 * @see xDesktop
 * @see xMeter
 */
public class xPanelMeter extends JPanel implements ActionListener {
private int i,indent;
private int elements=0,columns=2;
private Object obj;
private xMeter Meter;
private GridBagConstraints c;
private JScrollPane metpan;
private JPanel pan;
private Vector<xMeter> list;
private Dimension panSize;
private int meterSize=0;
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
public xPanelMeter(Dimension dim, int col) {
    super(new GridLayout(1,0));
    columns=col;
    list=new Vector<xMeter>();
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
// panSize.setSize(xs+20, ys+20);
// metpan.setPreferredSize(panSize);
// setPreferredSize(panSize);
    add(metpan);
}

private void setDefaults(){
for(int i=0;i<elements;i++) list.elementAt(i).setDefaults();
}
private void setLog(boolean log){
for(int i=0;i<elements;i++) list.elementAt(i).setLogScale(log);
}
private void setMode(int mode){
for(int i=0;i<elements;i++) list.elementAt(i).setMode(mode);
}
private void setAutoScale(boolean on){
for(int i=0;i<elements;i++) list.elementAt(i).setAutoScale(on);
}
// from pull down menu.
private void askLimits(){
double low=0.0,high=100.0;
    String str=JOptionPane.showInputDialog(this,"Enter limits: low,high",
        new String(Double.toString(low)+","+Double.toString(high)));
    if(str != null){
    String f[]=str.split(",");
    if(f.length == 1)high=Double.parseDouble(f[0]);
    if(f.length == 2){low=Double.parseDouble(f[0]);high=Double.parseDouble(f[1]);}
    for(int i=0;i<elements;i++) list.elementAt(i).setLimits(low,high);
}}
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
 * @param meter Removes meter. Calls updateAll().
 */
public void removeMeter(xMeter meter){
    list.remove(meter);
    elements--;
    updateAll();
}

/**
 * Cleanup item list and panel.
 */
public void cleanup(){
    list=new Vector<xMeter>();
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
        pan.add(list.elementAt(i-1),c); //,c
    }
    pan.revalidate();
// System.out.println("metpan pref "+pan.getPreferredSize());
// System.out.println("metpan get "+pan.getSize());
    if(action!=null)action.actionPerformed(null);
}

private void adjustSize(){
    int rows=1;
    if(elements>0){
        rows=(elements-1)/columns+1;
        panSize=list.elementAt(0).getPreferredSize();
    }
    meterSize=(int)panSize.getWidth();
    rows=rows*((int)panSize.getHeight());
    int cols=columns*meterSize;
    pan.setPreferredSize(new Dimension(cols,rows));
    setPreferredSize(new Dimension(cols+3,rows+3));
}

/**
 * Add meter item to internal table.
 * @param meter Meter.
 * @param update True: updateAll, false: no graphics update. Several meters can be added without
 * redrawing the panel each time. The last one should. 
 */
public void addMeter(xMeter meter, boolean update){
addMeter(meter);
if(update)updateAll();
}

/**
 * Add meter item to internal table. No redraw, calls setSizeXY and setID.
 * @param meter Meter.
*/
public void addMeter(xMeter meter){
    meter.setActionListener(this);
    meter.setSizeXY();
// only 1.6
// if((elements/3)*3 == elements) {
    // pGroup=layout.createParallelGroup();
    // vGroup.addGroup(pGroup);
    // }
// pGroup.addComponent(meter);
//
    meter.setID(elements); // gives unique number to meter
    elements += 1;
// just adding the new meter would put all in same row!
// if((elements/columns)*columns == elements) c.gridwidth = GridBagConstraints.REMAINDER;
// else c.gridwidth = 1;
// pan.add(meter);
    list.add(meter);
    //updateAll();
}
public JMenuBar createMenuBar() {
    JMenuBar menuBar = new JMenuBar();
    JMenu cmenu = new JMenu("Settings");
    cmenu.setMnemonic(KeyEvent.VK_C);
    menuBar.add(cmenu);

    JMenuItem menuItem=new JMenuItem("Zoom");
    menuItem.addActionListener(this);
    cmenu.add(menuItem);
    cmenu.addSeparator();
    JMenu colmen=new JMenu("Columns");
    for(int i =1;i<13;i++){
        menuItem=new JMenuItem(Integer.toString(i)+" x n");
        menuItem.addActionListener(this);
        colmen.add(menuItem);
    }
    cmenu.add(colmen);
    cmenu.addSeparator();
    colmen=new JMenu("Mode");
    menuItem=new JMenuItem("Meter");
    colmen.add(menuItem);
    menuItem.addActionListener(this);
    menuItem=new JMenuItem("Bar");
    colmen.add(menuItem);
    menuItem.addActionListener(this);
    menuItem=new JMenuItem("Trend");
    colmen.add(menuItem);
    menuItem.addActionListener(this);
    menuItem=new JMenuItem("Histo");
    colmen.add(menuItem);
    menuItem.addActionListener(this);
    cmenu.add(colmen);
    cmenu.addSeparator();
    colmen=new JMenu("Scale");
    menuItem=new JMenuItem("Auto On");
    colmen.add(menuItem);
    menuItem.addActionListener(this);
    menuItem=new JMenuItem("Auto Off");
    colmen.add(menuItem);
    menuItem.addActionListener(this);
    menuItem=new JMenuItem("Log scale");
    colmen.add(menuItem);
    menuItem.addActionListener(this);
    menuItem=new JMenuItem("Lin scale");
    colmen.add(menuItem);
    menuItem.addActionListener(this);
    cmenu.add(colmen);
    cmenu.addSeparator();
    menuItem=new JMenuItem("Limits");
    menuItem.addActionListener(this);
    cmenu.add(menuItem);
    menuItem=new JMenuItem("Defaults");
    menuItem.addActionListener(this);
    cmenu.add(menuItem);
    return menuBar;
}
/**
 * This is called directly after creating the internal frame
 * where this is in. Listener is xInternalFrame.
 * After resizing the items by "Zoom" event this action listener is
 * called with same event to pack the frame.
 * @param actionlistener Frame containing this panel.
 */
public void setListener(ActionListener actionlistener){
    action=actionlistener;
}
/***
 * React to menu selections. If this is set as action listener of the xMeters,
 * it processes event "Large" from the xMeter.<br>
 * Event "Zoom" is fired by the menu bar.
 */
public void actionPerformed(ActionEvent e) {
Dimension nd=null;
if ("Zoom".equals(e.getActionCommand())){
    if(meterSize==xMeter.XSIZE_LARGE)nd=new Dimension(xMeter.XSIZE,xMeter.YSIZE);
    if(meterSize==xMeter.XSIZE)      nd=new Dimension(xMeter.XSIZE_LARGE,xMeter.YSIZE_LARGE);
    if(nd != null){
    for(int i=0;i<elements;i++) list.elementAt(i).setSizeXY(nd); 
    updateAll();
    action.actionPerformed(e); // xInternalFrame will pack()
    }
}
else if("Large".equals(e.getActionCommand())){
    int index=((xMeter)e.getSource()).getID();
    Meter=list.elementAt(index);
    String title=new String(Meter.getHead1()+Meter.getHead2()+Meter.getHead3());
    xLayout la = new xLayout(title);
    la.set(new Point(index*30,index*30), null, 0, true); // position
    xInternalFrame extFrame = new xInternalFrame(title,la);
    xMeter extMeter=Meter.getClone(title);
    extMeter.setSizeXY(new Dimension(xMeter.XSIZE_LARGE,xMeter.YSIZE_LARGE));
    extMeter.hasParent();
    extFrame.setupFrame(null, null, extMeter, false);  // true for resizable
    xSet.getDesktop().add(extFrame);
    xSet.getDesktop().setSelectedFrame(extFrame);
    extFrame.moveToFront();
    Meter.setExternMeter(extMeter,extFrame);
}
else if("Defaults".equals(e.getActionCommand())) setDefaults();
else if("Log scale".equals(e.getActionCommand())) setLog(true);
else if("Lin scale".equals(e.getActionCommand())) setLog(false);
else if("Limits".equals(e.getActionCommand())) askLimits();
else if("Auto On".equals(e.getActionCommand())) setAutoScale(true);
else if("Auto Off".equals(e.getActionCommand())) setAutoScale(false);
else if("Meter".equals(e.getActionCommand())) setMode(xMeter.ARC);
else if("Bar".equals(e.getActionCommand())) setMode(xMeter.BAR);
else if("Trend".equals(e.getActionCommand())) setMode(xMeter.TREND);
else if("Histo".equals(e.getActionCommand())) setMode(xMeter.STAT);
else if ("1 x n".equals(e.getActionCommand())) setColumns(1);
else if ("2 x n".equals(e.getActionCommand())) setColumns(2);
else if ("3 x n".equals(e.getActionCommand())) setColumns(3);
else if ("4 x n".equals(e.getActionCommand())) setColumns(4);
else if ("5 x n".equals(e.getActionCommand())) setColumns(5);
else if ("6 x n".equals(e.getActionCommand())) setColumns(6);
else if ("7 x n".equals(e.getActionCommand())) setColumns(7);
else if ("8 x n".equals(e.getActionCommand())) setColumns(8);
else if ("9 x n".equals(e.getActionCommand())) setColumns(9);
else if ("10 x n".equals(e.getActionCommand())) setColumns(10);
else if ("11 x n".equals(e.getActionCommand())) setColumns(11);
else if ("12 x n".equals(e.getActionCommand())) setColumns(12);
}
}



