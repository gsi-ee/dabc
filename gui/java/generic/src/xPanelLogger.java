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
/*
This client expects server tDABCserver from eclipse DIM project.
*/


/**
* @author goofy
*/
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.UIManager;
import javax.swing.ImageIcon;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JMenuBar;
import javax.swing.JTextArea;
import javax.swing.JTextPane;
import javax.swing.event.*;
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
* DIM GUI class
*/
public class xPanelLogger extends JPanel implements ActionListener {
private int i,indent;
private Object obj;
private GridBagConstraints c;
private JScrollPane textpan;
private JPanel pan;
private JTextArea textar;
private ActionListener action=null;
// private GroupLayout.SequentialGroup vGroup;
// private GroupLayout.ParallelGroup pGroup;
/**
 * DIM GUI class. Uses JScrollPanes with GridBagLayout.
 * Creates the table for the parameters and the tree for the commands.
 * Creates rate meters for dataBW, dataLatency and dataRate
 */
public xPanelLogger(Dimension dim) {
    super(new GridLayout(1,0));
    c = new GridBagConstraints();
    c.fill = GridBagConstraints.HORIZONTAL;
    c.weightx = 0.0;
    c.weighty = 0.0;
    c.gridwidth = 1;
    c.gridheight = 1;
    c.insets = new Insets(0,0,0,0);
    textar=new JTextArea();
    textpan=new JScrollPane(textar);
    textpan.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
    textpan.setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_ALWAYS);
    textpan.setPreferredSize(xSet.addDimension(dim, new Dimension(-10,-53)));
    add(textpan);
}

public void print(String s){
textar.append(s);
textpan.getVerticalScrollBar().setValue(1000000); // to see the last line
}

// public void updateAll(){
    // if(action!=null)action.actionPerformed(null);
// }


public JMenuBar createMenuBar() {
    JMenuBar menuBar = new JMenuBar();
    JMenu cmenu = new JMenu("Logging");
    cmenu.setMnemonic(KeyEvent.VK_L);
    menuBar.add(cmenu);

    JMenuItem menuItem=new JMenuItem("Save");
    menuItem.addActionListener(this);
    cmenu.add(menuItem);
    return menuBar;
}
// this is called directly after creating the internal frame
// where this is in. Listener is internal frame.
public void setListener(ActionListener al){
    action=al;
}
//React to menu selections.
public void actionPerformed(ActionEvent e) {
    if ("Save".equals(e.getActionCommand())) System.out.println("Save");
}
public void internalFrameClosed(InternalFrameEvent e) {
System.out.println("Frame logger closed");
}
}



