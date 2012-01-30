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
import javax.swing.JLabel;
import javax.swing.JButton;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JToolBar;
import javax.swing.border.TitledBorder;
import javax.swing.BorderFactory;
import javax.swing.JOptionPane;
import javax.swing.JTextField;
import javax.swing.JTextArea;
import javax.swing.JCheckBox;
//import javax.swing.GroupLayout;

import java.net.URL;
import java.io.IOException;
import java.awt.Color;
import java.awt.Component;
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
 * Base class for prompt panels.
 * Provides functions to build forms from text input fields (one column)
 * and buttons (tool bar).
 * @author Hans G. Essel
 * @version 1.0
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
private ActionListener action=null;
// private GroupLayout.SequentialGroup vGroup;
// private GroupLayout.ParallelGroup pGroup;
/**
 * Create panel with title.
 * @param title Title
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
/**
 * Create panel with title.
 * @param title Title
 * @param dim Window size.
 */
public xPanelPrompt(String title, Dimension dim) {
    super(new BorderLayout());
    gridconst = new GridBagConstraints();
    gridconst.anchor = GridBagConstraints.EAST;
    pan=new JPanel(new GridBagLayout());
    scrollpan=new JScrollPane(pan);
    titleborder=BorderFactory.createTitledBorder(title);
    titleborder.setTitleColor(xSet.greenD());
    pan.setBorder(titleborder);
    scrollpan.setPreferredSize(dim);
    // panSize = new Dimension(xs, ys);
    // pan.setPreferredSize(panSize);
    add(scrollpan,BorderLayout.CENTER);
    tool=new JToolBar();
    tool.setMargin(new Insets(-4,0,-6,0));
    add(tool,BorderLayout.SOUTH);
}
public void setListener(ActionListener al){
    action=al;
}
public void cleanupPanel(){
	pan.removeAll();
}
public void refreshPanel(){
if(action!=null)action.actionPerformed(null);
}
/**
 * Add prompter line in column.
 * @param label Label for prompt.
 * @param input Input text field. Action listener must be set from caller.
 */
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
/**
 * Add prompter line in column and return text field.
 * @param label Label for prompt.
 * @param value Default value.
 * @param cmd Action command.
 * @param width Size of text field.
 * @param al Action listener to handle action command.
 * @return Text field.
 */
public JTextField addPrompt(String label, String value, String cmd, int width, ActionListener al){
    JTextField tf=new JTextField(value,width);
    if(width > 0){
    tf.setActionCommand(cmd);
    tf.addActionListener(al);
    addPrompt(label,tf);
    }
    return tf;
}
/**
 * Add check box in column and return it.
 * @param label Label for check box.
 * @param cmd Action command.
 * @param al Action listener to handle check box command.
 * @return Check box.
 */
public JCheckBox addCheckBox(String label, String cmd, ActionListener al){
    JCheckBox check1 = new JCheckBox();
    check1.addActionListener(al);
    check1.setActionCommand(cmd);
    addCheckBox(label,check1);
    return check1;
}
/**
 * Add check box in column.
 * @param label Label for check box.
 * @param check Check box. Action listener must be set from caller.
 */
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
/**
 * Add text area in column.
 * @param text Text of area.
 * @param cmd command. 
 * @param rows rows (height)
 * @param cols columns (width)
 * @param al Action listener handling command.
 */
public JTextArea addTextArea(String text, String cmd, int rows, int cols, ActionListener al){
    JTextArea ta = new JTextArea(text,rows,cols);
    ta.setLineWrap(true);
    //ta.setActionCommand(cmd);
    //ta.addActionListener(al);
    JScrollPane textpan=new JScrollPane(ta);
    textpan.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
    textpan.setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
    gridconst.gridwidth = GridBagConstraints.REMAINDER;     //end row
    gridconst.fill = GridBagConstraints.HORIZONTAL;
    gridconst.weightx = 1.0;
    pan.add(textpan, gridconst);
    return ta;
}
/**
 * Add text button in column.
 * @param label Text of button.
 * @param cmd Button command. 
 * @param tt Tool tip text.
 * @param al Action listener handling button command.
 */
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
/**
 * Add tool bar button.
 * @param cmd Button command. 
 * @param icon Button icon.
 * @param tt Tool tip text.
 * @param al Action listener handling button command.
 */
public void addButton(String cmd, String tt, ImageIcon icon, ActionListener al){
    JButton button = new JButton(icon);
    button.setActionCommand(cmd);
    button.addActionListener(al);
    button.setText("");
    button.setToolTipText(tt);
    button.setBorderPainted(false);
    tool.add(button);
}
/**
 * Add tool bar button.
 * @param cmd Button command. 
 * @param icon Button icon.
 * @param selected icon Button pressed icon
 * @param tt Tool tip text.
 * @param al Action listener handling button command.
 */
public JButton addButton(String cmd, String tt, ImageIcon icon, ImageIcon selected, ActionListener al){
    JButton button = new JButton(icon);
    button.setSelectedIcon(selected);
    button.setRolloverSelectedIcon(selected);
    button.setActionCommand(cmd);
    button.addActionListener(al);
    button.setText("");
    button.setToolTipText(tt);
    button.setBorderPainted(false);
    tool.add(button);
    return button;
}
/**
 * Set title and color of title boarder.
 * @param title Title
 * @param color Color
 */
public void setTitle(String title, Color color){
titleborder.setTitle(title);
titleborder.setTitleColor(color);
repaint();
}
// public TitledBorder getTitle(){return titleborder;}

/**
 * Modal pop up window with error message.
 * @param msg Error message
 */
public void tellError(Component parent, String msg){
// first argument must be a component which is displayed. 
// Therefore we use desktop. 
JOptionPane.showInternalMessageDialog(parent, msg, "Error",JOptionPane.ERROR_MESSAGE);
}
/**
 * Modal pop up window with info message.
 * @param msg Info message
 */
public void tellInfo(String msg){
JOptionPane.showInternalMessageDialog(this, msg, "Information",JOptionPane.INFORMATION_MESSAGE);
}
/**
 * Modal pop up window with question.
 * @param title Text.
 * @param question Question.
 * @return True, if answer was yes.
 */
public boolean askQuestion(String title, String question){
    int i=JOptionPane.showInternalConfirmDialog(this,question,title, JOptionPane.YES_NO_OPTION); 
    return (i == 0);
}
/**
 * Modal pop up window to enter text.
 * @param title Text describing input.
 * @param Default Default text.
 */
public String askInput(String title, String Default){
 return JOptionPane.showInputDialog(this,title,Default);
}/**
 * Modal pop up window to enter text.
 * @param title Text describing input.
 */
public String askInput(String title){
 return JOptionPane.showInputDialog(this,title);
}}



