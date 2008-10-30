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
public class xPanelInfo extends JPanel implements ActionListener {
private int i,indent;
private int elements=0,columns=1;
private Object obj;
private xInfo info;
private GridBagConstraints c;
private JScrollPane metpan;
private JPanel pan;
private Vector<Object> list;
private Dimension panSize;
private ActionListener action=null;
// private GroupLayout.SequentialGroup vGroup;
// private GroupLayout.ParallelGroup pGroup;
/**
 * DIM GUI class. Uses JScrollPanes with GridBagLayout.
 * Creates the table for the parameters and the tree for the commands.
 * Creates rate meters for dataBW, dataLatency and dataRate
 */
public xPanelInfo(Dimension dim) {
    super(new GridLayout(1,0));
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
// panSize.setSize(xs+20, ys+20);
// metpan.setPreferredSize(panSize);
// setPreferredSize(panSize);
    add(metpan);
//setViewportView(pan);
}

public void setColumns(int col){
    columns=col;
    updateAll();
    action.actionPerformed(null);
}
public void removeInfo(xInfo info){
    list.remove(info);
    elements--;
    updateAll();
}

public void cleanup(){
    list=new Vector<Object>();
    elements=0;
    pan.removeAll();
}
public void updateAll(){
    pan.removeAll();
    adjustSize();
    for(int i=1;i<=elements;i++){
        if((i/columns)*columns == i) c.gridwidth = GridBagConstraints.REMAINDER;
        else c.gridwidth = 1;
        pan.add((xInfo)list.elementAt(i-1),c); //,c
    }
    pan.revalidate();
    if(action!=null)action.actionPerformed(null);
}

public void adjustSize(){
    int rows=1;
    if(elements>0){
        rows=(elements-1)/columns+1;
        panSize=((xInfo)list.elementAt(0)).getPreferredSize();
    }
    rows=rows*((int)panSize.getHeight());
    int cols=columns*((int)panSize.getWidth());
    pan.setPreferredSize(new Dimension(cols,rows));
}

public void addInfo(xInfo info, boolean update){
addInfo(info);
if(update)updateAll();
}

public void addInfo(xInfo info){
    info.setSizeXY();
// only 1.6
// if((elements/3)*3 == elements) {
    // pGroup=layout.createParallelGroup();
    // vGroup.addGroup(pGroup);
    // }
// pGroup.addComponent(info);
//
    elements += 1;
// just adding the new info would put all in same row!
// if((elements/columns)*columns == elements) c.gridwidth = GridBagConstraints.REMAINDER;
// else c.gridwidth = 1;
// pan.add(info);
    list.add(info);
    
    //updateAll();
}
public JMenuBar createMenuBar() {
    // JMenuBar menuBar = new JMenuBar();
    // JMenu cmenu = new JMenu("Columns");
    // cmenu.setMnemonic(KeyEvent.VK_C);
    // menuBar.add(cmenu);

    // JMenuItem menuItem=new JMenuItem("Fit");
    // menuItem.addActionListener(this);
    // cmenu.add(menuItem);
    // for(int i =1;i<9;i++){
        // menuItem=new JMenuItem(Integer.toString(i));
        // menuItem.addActionListener(this);
        // cmenu.add(menuItem);
    // }
    // return menuBar;
    return null;
}
public void setListener(ActionListener al){
    action=al;
}
//React to menu selections.
public void actionPerformed(ActionEvent e) {
    if ("Fit".equals(e.getActionCommand())) action.actionPerformed(e);
    if ("1".equals(e.getActionCommand())) setColumns(1);
    if ("2".equals(e.getActionCommand())) setColumns(2);
    if ("3".equals(e.getActionCommand())) setColumns(3);
    if ("4".equals(e.getActionCommand())) setColumns(4);
    if ("5".equals(e.getActionCommand())) setColumns(5);
    if ("6".equals(e.getActionCommand())) setColumns(6);
    if ("7".equals(e.getActionCommand())) setColumns(7);
    if ("8".equals(e.getActionCommand())) setColumns(8);
}
}



