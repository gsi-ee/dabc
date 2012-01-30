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
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.ImageIcon;
import javax.swing.JTextField;
import javax.swing.JCheckBox;
import javax.swing.JPanel;
import javax.swing.JDesktopPane;
import javax.swing.JSplitPane;
import dim.*;
/**
 * Panel to display context from Xdaq XML file as editable textfields.
 * @author Hans G. Essel
 * @version 1.0
 * @see xPanelDabc
 */
public class xPanelSetup extends xPanelPrompt implements ActionListener 
{
private ImageIcon storeIcon, menuIcon;
private String name, p1name, p2name;
private String tooltip;
private ActionListener action;
private JTextField prompt1,prompt2,prompt3;
private JCheckBox check1,check2;
private Vector<String> Values;
private Vector<JTextField> Fields;
private int off, num;
/**
 * Create panel for one context. In the lists there are all contexts. Therefore
 * an offset and length must be passed.
 * @param title Title of frame.
 * @param names Reference to name list.
 * @param types Reference to type list.
 * @param values Reference to value list.
 * @param offset Offset in the lists.
 * @param number Items in the lists to be used (behind offset).
 * @see xPanelDabc
 */
protected xPanelSetup(String title, Vector<String> names, Vector<String> types, Vector<String> values, int offset, int number){
    super(title);
    boolean show;
    off=offset;
    num=number;
    Values=values;
    Fields = new Vector<JTextField>(0);
    for(int i=off;i<(off+number);i++){
    // Add prompter lines
        prompt1=new JTextField(Values.get(i));
        Fields.add(prompt1);
        show=true;
        if(names.get(i).equals("Module"))show=false;
        else if(names.get(i).equals("debugLevel"))show=false;
        else if(names.get(i).equals("nodeId"))show=false;
        else if(names.get(i).equals("CfgNodeId.BnetPlugin"))show=false;
        else if(names.get(i).equals("workerTid"))show=false;
        else if(names.get(i).equals("controllerTid"))show=false;
        else if(names.get(i).equals("Property"))show=false;
        else if(names.get(i).equals("Application ID"))show=false;
        else if(names.get(i).equals("Application instance"))show=false;
        else if(names.get(i).equals("nodeDNS"))show=false;
        else if(names.get(i).equals("portDNS"))show=false;
        if(show)addPrompt(names.get(i)+" ["+types.get(i)+"]",prompt1);
    }
}
/**
 * Update value list from text input fields.
 * @see xPanelDabc
 */
protected void updateList(){
    for(int i=0;i<Fields.size();i++){
        if(!Values.get(off+i).equals(Fields.get(i).getText())){
            System.out.print(Values.get(off+i)+" => ");
            Values.set(off+i,Fields.get(i).getText());
            System.out.println(Values.get(off+i));
        }
    }
}
// ---- Handle the menu actions ---------------------------------------
public void actionPerformed(ActionEvent e) {

}
}
