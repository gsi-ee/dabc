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
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import java.awt.event.*;
import java.awt.Insets;
import java.util.*;
/**
 * Container panel for graphic panels.
 * Provides functions to add graphic panels in columns.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xPanelGraphics extends JPanel implements ActionListener {
private int i,indent;
private int elements=0,columns=2;
private Object obj;
private JPanel panel;
private GridBagConstraints c;
private JScrollPane metpan;
private JPanel pan;
private Vector<JPanel> list;
private Dimension panSize;
private ActionListener action=null;
// private GroupLayout.SequentialGroup vGroup;
// private GroupLayout.ParallelGroup pGroup;
/**
 * Create panel for number of columns.
 * @param dim Dimension
 * @param col Columns
 */
public xPanelGraphics(Dimension dim, int col) {
    super(new GridLayout(1,0));
    columns=col;
    list=new Vector<JPanel>();
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

public void setColorBack(Color back){
    setBackground(back);
    pan.setBackground(back);
    metpan.setBackground(back);
}
private void setColumns(int col){
    columns=col;
    updateAll();
}
public int getColumns(){
    return columns;
}
/**
 * Remove a panel from list and update all.
 * @param panel Panel to be removed.
 */
public void removeGraphics(JPanel panel){
    list.remove(panel);
    elements--;
    updateAll();
}
/**
 * Remove all panels from list and panel.
 */
public void cleanup(){
    list=new Vector<JPanel>();
    elements=0;
    pan.removeAll();
}
/**
 * Remove all, adjust size, add to panel, revalidate, call action handler (null).
 */
public void updateAll(){
    pan.removeAll();
    adjustSize();
    for(int i=1;i<=elements;i++){
        if((i/columns)*columns == i) c.gridwidth = GridBagConstraints.REMAINDER;
        else c.gridwidth = 1;
        pan.add((JPanel)list.elementAt(i-1),c); //,c
    }
    pan.revalidate();
 // System.out.println("grapan pref "+pan.getPreferredSize());
 // System.out.println("grapan get "+pan.getSize());
    if(action!=null)action.actionPerformed(null);
}
/**
 * Recalculate sizes.
 */
private void adjustSize(){
    int rows=1;
    int cols=0;
    if(columns > 1){ // equal height for all
        if(elements>0){
            rows=(elements-1)/columns+1;
            panSize=((JPanel)list.elementAt(0)).getPreferredSize();
        }
        rows=rows*((int)panSize.getHeight());
    } else { // equal width for all, variant height, sum up
        rows=0;
        for(int i=0;i<elements;i++){
            panSize=((JPanel)list.elementAt(i)).getPreferredSize();
            rows=rows+(int)panSize.getHeight();
    }}
    cols=columns*(int)panSize.getWidth();
    pan.setPreferredSize(new Dimension(cols,rows));
    setPreferredSize(new Dimension(cols+3,rows+3));
}

/**
 * Add graphic panel to list.
 * @param panelItem Interface of panel to be added. setSizeXY and setID is called.
 * @param update True: update all, false to only add to list.
 */
public void addGraphics(xiPanelItem panelItem, boolean update){
    panelItem.setSizeXY();
// only 1.6
// if((elements/3)*3 == elements) {
    // pGroup=layout.createParallelGroup();
    // vGroup.addGroup(pGroup);
    // }
// pGroup.addComponent(panel);
//
    panelItem.setID(elements); // gives unique number to panel
    elements += 1;
// just adding the new panel would put all in same row!
// if((elements/columns)*columns == elements) c.gridwidth = GridBagConstraints.REMAINDER;
// else c.gridwidth = 1;
// pan.add(panel);
    list.add(panelItem.getPanel());
    if(update) updateAll();
}
public JMenuBar createMenuBar() {
    return null;
}
/**
 * Attach an action listener. 
 * This is called directly after creating the internal frame
 * where this is in. Listener is internal frame.
 * @param al Action listener.
 * @see xInternalFrame
 */
public void setListener(ActionListener al){
    action=al;
}
// React to menu selections.
public void actionPerformed(ActionEvent e) {
if(action!=null)action.actionPerformed(e);
}
}



