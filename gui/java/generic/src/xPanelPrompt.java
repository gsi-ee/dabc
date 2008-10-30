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
import javax.swing.JLabel;
import javax.swing.JButton;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JToolBar;
import javax.swing.border.TitledBorder;
import javax.swing.BorderFactory;
import javax.swing.JOptionPane;
import javax.swing.JTextField;
import javax.swing.JCheckBox;
//import javax.swing.GroupLayout;

import java.net.URL;
import java.io.IOException;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.BorderLayout;
import java.awt.GridLayout;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import java.awt.event.*;
import java.awt.Insets;
import java.util.*;

/**
* DIM GUI class
*/
public class xPanelPrompt extends JPanel 
{
private GridBagConstraints gridconst;
private JScrollPane scrollpan;
private JPanel pan;
private Dimension panSize;
private JToolBar tool;
private JButton button;
private ImageIcon Icon;
private TitledBorder titleborder;
private JOptionPane confirm;
// private GroupLayout.SequentialGroup vGroup;
// private GroupLayout.ParallelGroup pGroup;
/**
 * DIM GUI class. Uses JScrollPanes with GridBagLayout.
 * Creates the table for the parameters and the tree for the commands.
 * Creates rate meters for dataBW, dataLatency and dataRate
 */
public xPanelPrompt(String title) {
    super(new BorderLayout());
    gridconst = new GridBagConstraints();
    gridconst.anchor = GridBagConstraints.EAST;
    pan=new JPanel(new GridBagLayout());
    scrollpan=new JScrollPane(pan);
    titleborder=BorderFactory.createTitledBorder(title);
    titleborder.setTitleColor(xSet.greenD());
    pan.setBorder(titleborder);
    // panSize = new Dimension(xs, ys);
    // pan.setPreferredSize(panSize);
    add(scrollpan,BorderLayout.CENTER);
    tool=new JToolBar();
    tool.setMargin(new Insets(-4,0,-6,0));
    add(tool,BorderLayout.SOUTH);
}
public void addPrompt(String label, JTextField input){
    gridconst.gridwidth = GridBagConstraints.RELATIVE; //next-to-last
    gridconst.fill = GridBagConstraints.NONE;      //reset to default
    gridconst.weightx = 0.0;                       //reset to default
    JLabel lab=new JLabel(label);
    lab.setForeground(xSet.blueD());
    pan.add(lab, gridconst);
    gridconst.gridwidth = GridBagConstraints.REMAINDER;     //end row
    gridconst.fill = GridBagConstraints.HORIZONTAL;
    gridconst.weightx = 1.0;
    pan.add(input, gridconst);
}
public JTextField addPrompt(String label, String value, String cmd, int width, ActionListener al){
    JTextField tf=new JTextField(value,width);
    tf.setActionCommand(cmd);
    tf.addActionListener(al);
    addPrompt(label,tf);
    return tf;
}
public JCheckBox addCheckBox(String label, String cmd, ActionListener al){
    JCheckBox check1 = new JCheckBox();
    check1.addActionListener(al);
    check1.setActionCommand(cmd);
    addCheckBox(label,check1);
    return check1;
}
public void addCheckBox(String label, JCheckBox check){
    gridconst.gridwidth = GridBagConstraints.RELATIVE; //next-to-last
    gridconst.fill = GridBagConstraints.NONE;      //reset to default
    gridconst.weightx = 0.0;                       //reset to default
    JLabel lab=new JLabel(label);
    lab.setForeground(xSet.blueD());
    pan.add(lab, gridconst);
    gridconst.gridwidth = GridBagConstraints.REMAINDER;     //end row
    gridconst.fill = GridBagConstraints.HORIZONTAL;
    gridconst.weightx = 1.0;
    pan.add(check, gridconst);
}
public void addTextButton(String label, String cmd, String tt, ActionListener al){
    JButton button = new JButton();
    button.setActionCommand(cmd);
    button.addActionListener(al);
    button.setText(label);
    button.setToolTipText(tt);
    button.setBorderPainted(true);
    gridconst.gridwidth = GridBagConstraints.REMAINDER;     //end row
    gridconst.fill = GridBagConstraints.HORIZONTAL;
    gridconst.weightx = 1.0;
    pan.add(button, gridconst);
}
public void addButton(String cmd, String tt, ImageIcon icon, ActionListener a){
    JButton button = new JButton(icon);
    button.setActionCommand(cmd);
    button.addActionListener(a);
    button.setText("");
    button.setToolTipText(tt);
    button.setBorderPainted(false);
    tool.add(button);
}
public void addButton(String cmd, String tt, ImageIcon icon, ActionListener a, boolean def){
    JButton button = new JButton(icon);
    button.setActionCommand(cmd);
    button.addActionListener(a);
    button.setText("");
    button.setToolTipText(tt);
    button.setBorderPainted(false);
    button.setContentAreaFilled(false);
    getRootPane().setDefaultButton(button);
    tool.add(button);
}
public void setTitle(String t, Color c){
titleborder.setTitle(t);
titleborder.setTitleColor(c);
repaint();
}
// public TitledBorder getTitle(){return titleborder;}

public void tellError(String msg){
// first argument must be a component which is displayed. Could be PanelDabc, but this may be closed.
// therefore we use desktop. 
    JOptionPane.showInternalMessageDialog(xSet.getDesktop(), msg, "Error",JOptionPane.ERROR_MESSAGE);
}
public void tellInfo(String msg){
    JOptionPane.showInternalMessageDialog(xSet.getDesktop(), msg, "Information",JOptionPane.INFORMATION_MESSAGE);
}
public boolean choose(String msg){
    int i=JOptionPane.showInternalConfirmDialog(xSet.getDesktop(),msg, "Really continue?", JOptionPane.YES_NO_OPTION); 
    return (i == 0);
}
}



