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
import javax.swing.JEditorPane;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JLabel;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.UIManager;
import javax.swing.ImageIcon;
import javax.swing.border.TitledBorder;
import javax.swing.BorderFactory;

import javax.swing.JTextField;
import javax.swing.JTree;
import javax.swing.JTable;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.TreeSelectionModel;
import javax.swing.event.TreeSelectionEvent;
import javax.swing.event.TreeSelectionListener;
import javax.swing.tree.DefaultTreeCellRenderer;
import javax.swing.tree.TreePath;
import javax.swing.table.TableColumn;
import java.net.URL;
import java.io.*;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.BorderLayout;
import java.awt.GridLayout;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import java.awt.event.*;
import java.awt.event.MouseListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseAdapter;
import java.util.*;
import org.w3c.dom.*;
/**
 * Panel for command tree
 * @see xDimCommand
 * @author Hans G. Essel
 * @version 1.0
 */
public class xPanelCommand extends JPanel implements TreeSelectionListener, ActionListener {
private JTree comtree;
private JTextField compar;
private JPanel Args, ArgLabels, ArgFields;
private JScrollPane argView, comView;
private GridBagConstraints gridconst;
private int i,indent;
private int ipar,icmd;
private xDimCommand cmd;
private xDimBrowser dimbrowser;
private DefaultMutableTreeNode node1,node2,nodesel;
private DefaultMutableTreeNode[] nodepath;
private Object obj;
private String[] treestr = new String[4];
private String treepath, filter;
private Vector<xDimCommand> vcom;
private Vector<JTextField> inputs;
private Vector<JCheckBox> checkers;
private String[] argnames;
private String[] argtypes;
private ImageIcon execIcon;
private xiUserCommand userCmd;

/**
 * Opens panel for command tree. Gets list of xDimCommand objects
 * @param browser DIM browser.
 * @param dim Size and position of window.
 */
public xPanelCommand(xDimBrowser browser, Dimension dim, String Filter) {
    super(new GridLayout(1,0));
// xDimCommand list of commands
    filter=Filter;
    dimbrowser=browser;
    vcom=browser.getCommandList();
    icmd=vcom.size(); // number of commands
    if(icmd>0)initPanel(dim);
}

private void initPanel(Dimension dim){
    execIcon  = new ImageIcon("icons/execute.png");
// -------------------------------------------------------------------------------------------------------
// create the tree of commands
    DefaultMutableTreeNode[] treenode=new DefaultMutableTreeNode[4];
//Create the root node.
    treestr[0]=vcom.get(0).toString(0);
    treenode[0] = new DefaultMutableTreeNode(treestr[0]);
//Create a tree that allows one selection at a time.
    comtree = new JTree(treenode[0]);
    i=0;
    indent=1;
    Boolean sibl;
    while(i<icmd) {
    if(!vcom.get(i).toString(1).startsWith("_")){
    	if((!vcom.get(i).getParser().getName().startsWith("Mbs"))|
    	   (filter.contains(vcom.get(i).getParser().getName().substring(3)))){
        if(indent==1) sibl=treestr[indent-1].equals(vcom.get(i).toString(indent-1));
        else
            sibl=treestr[indent-1].equals(vcom.get(i).toString(indent-1))
            && treestr[indent-2].equals(vcom.get(i).toString(indent-2));
        if(sibl){
            treestr[indent]=vcom.get(i).toString(indent);
            if(indent==3)treenode[indent] = new DefaultMutableTreeNode(vcom.get(i));
            else		 treenode[indent] = new DefaultMutableTreeNode(treestr[indent]);
            //System.out.println("i "+i+" t "+indent+" "+treestr[indent]);
            treenode[indent-1].add(treenode[indent]);
            if(indent<3){indent++;i--;}
        } else {indent--;if(indent>0)i--;}
        }}
        i++;
    }
    comtree.getSelectionModel().setSelectionMode(TreeSelectionModel.SINGLE_TREE_SELECTION);
    comtree.setToggleClickCount(3); // needs 3 clicks to expand/close
// set icon for command nodes
    ImageIcon leafIcon = new ImageIcon("icons/shiftright.png");
    if (leafIcon != null) {
        DefaultTreeCellRenderer renderer = new DefaultTreeCellRenderer();
        renderer.setLeafIcon(leafIcon);
        comtree.setCellRenderer(renderer);
    }
    comtree.setRootVisible(true);
    comtree.expandRow(0);
//Listen for when the selection changes. Calls valueChanged
//-------------------------------------------------------------------------
    comtree.addTreeSelectionListener(this);
//- check for double click------------------------------------------------------------------------
    comtree.addMouseListener(new MouseAdapter() {
        public void mouseClicked(MouseEvent e) {
            if(e.getClickCount()==2){
                if(nodesel==null){System.out.println("NO node selected");return;}
                if(comtree.getPathForLocation(e.getX(), e.getY())==null){System.out.println("NO location");return;}
                if(comtree.getSelectionPath()==null){System.out.println("NO selection");return;}
                String treepath1=comtree.getSelectionPath().toString();
                String treepath2=comtree.getPathForLocation(e.getX(), e.getY()).toString();
                if(treepath1.equals(treepath2))	executeCommands();
            }}}
    );
    gridconst = new GridBagConstraints();
    JSplitPane splitpane=new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
    comView = new JScrollPane(comtree);
    comView.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
    splitpane.setLeftComponent(comView);
    Args=new JPanel(new GridBagLayout());
    TitledBorder bord=BorderFactory.createTitledBorder("Command arguments");
    bord.setTitleColor(Color.red);
    Args.setBorder(bord);
    argView = new JScrollPane(Args);
    splitpane.setRightComponent(argView);
    splitpane.setDividerLocation(-1); // adjust according left
    Dimension cdim=xSet.addDimension(dim, new Dimension(-21,-35));
    comView.setPreferredSize(new Dimension((int)(cdim.getWidth()*0.4), (int)cdim.getHeight()));
    argView.setPreferredSize(new Dimension((int)(cdim.getWidth()*0.6), (int)cdim.getHeight()));
    add(splitpane);
}
/**
 * Handles mainly RET to execute commands.
 */
public void actionPerformed(ActionEvent e) {executeCommands();}

//----------------------------------------------------------
// Required by TreeSelectionListener interface.
public void valueChanged(TreeSelectionEvent e) {
// here we store the treenode to be processed in the MouseListener(MouseClicked->executeCommand)
// mouseClicked is called after valueChanged.
// mouseClicked reacts only on double click
// tree reacts only on triple clicks or click on handles
    nodesel = (DefaultMutableTreeNode) comtree.getLastSelectedPathComponent();
    // look down to find the corresponding command
    cmd=null;
    if (nodesel==null) return;
    if (nodesel.getLevel() == 0)return;
    if (nodesel.isLeaf()) {
        cmd = (xDimCommand)nodesel.getUserObject();
    } else{ // no leaf, get childs. At least one child must be there!
        node1 = (DefaultMutableTreeNode)nodesel.getFirstChild();
        if(node1.isLeaf()){
               cmd = (xDimCommand)node1.getUserObject();
             } else{ // no leaf, must have children
                node2 = (DefaultMutableTreeNode)node1.getFirstChild();
                cmd = (xDimCommand)node2.getUserObject();
                }
            }
    if(cmd != null) {
        setPromptPanel(cmd);
        addPromptButton("Execute");
        Args.revalidate();
        }
    else System.out.println("null");
}
//----------------------------------------------------------
// called by mouseClicked, nodesel set by previously called valueChanged
private void executeCommands(){
// check if we can do
    if (nodesel==null) return;
    if (nodesel.getLevel() == 0)return;
//process single leaf (execution node)
    if (nodesel.isLeaf()) {
        executeCommand((xDimCommand)nodesel.getUserObject());
    } else{ // no leaf, get childs. At least one child must be there!
        node1 = (DefaultMutableTreeNode)nodesel.getFirstChild();
        if(node1.isLeaf()){
            while(node1!=null){
                executeCommand((xDimCommand)node1.getUserObject());
                node1= node1.getNextSibling();
            }} else{ // no leaf, must have children
            while(node1!=null){
                node2 = (DefaultMutableTreeNode)node1.getFirstChild();
                while(node2!=null){
                    executeCommand((xDimCommand)node2.getUserObject());
                    node2 = node2.getNextSibling();
                }
                node1 = node1.getNextSibling();
            }}}
}
//----------------------------------------------------------
// called by executeCommands
private void executeCommand(xDimCommand cmd){
boolean done=false;
boolean xmlargs=false;
boolean hasargs=false;
StringBuffer sb=null;
int in=0;
xXmlParser xmlp =cmd.getXmlParser();
sb=new StringBuffer();
// we have an XML description
if(xmlp != null){ 
// System.out.println(xmlp.getName());
// System.out.println(xmlp.getXmlString());
// application can specify if this command needs XML arguments or not
    if(userCmd != null) xmlargs=userCmd.getArgumentStyleXml(xmlp.getCommandScope(),cmd.getParser().getFull());
// Store values in xml description without header
//    System.out.println(xmlp.getXmlString());
    hasargs=xmlp.hasArgs();
    xmlp.newCommand(xmlp.getCommandName(),false,xmlp.getCommandScope(),true); // values have changed
    if(hasargs){
    in=0;
    for(int i=0;i<inputs.size();i++){
        xmlp.addArgument(argnames[in],
            xmlp.getArgumentType(in),
            inputs.elementAt(i).getText(),
            xmlp.getArgumentRequired(in));
            in++;
    }
    for(int i=0;i<checkers.size();i++){
        if(checkers.elementAt(i).isSelected())
            xmlp.addArgument(argnames[in],
                xmlp.getArgumentType(in),
                "true",
                xmlp.getArgumentRequired(in));
                else
            xmlp.addArgument(argnames[in],
                xmlp.getArgumentType(in),
                "false",
                xmlp.getArgumentRequired(in));
        in++;
    }} //has args
// finalize XML string
    xmlp.parseXmlString(xmlp.getName(),xmlp.getXmlString());
//    System.out.println(xmlp.getXmlString());
    // handle simple command with only one string argument
    // separately. Just give string as argument
    if(hasargs){
        if(argnames[0].equals("*")){
        sb.append(inputs.elementAt(0).getText());
        // two spaces because in MBS the scom scripts
        // have no space at the end of command. If this is fixed,
        // single spce would do.
        cmd.exec(xSet.getAccess()+"  "+sb.toString());
        done=true;
        } else { // build arg=value arg=value string
    in=0;
    for(int i=0;i<inputs.size();i++){
        if(inputs.elementAt(i).getText().length() > 0){
            sb.append(argnames[in]);
            if(argtypes[in].equals("C"))sb.append("=\"");
            else sb.append("=");
            sb.append(inputs.elementAt(i).getText());
            if(argtypes[in].equals("C"))sb.append("\" ");
            else sb.append(" ");
        }
        else if(xmlp.getArgumentRequired(in).startsWith("R")){
            sb.append(argnames[in]);
            sb.append(" is REQUIRED! ");
            done=true; // mark error, do not execute
        }
        in++;
    }
    // append switches just by names
    for(int i=0;i<checkers.size();i++){
        if(checkers.elementAt(i).isSelected()){
        sb.append(argnames[in]);
        sb.append(" ");
        }
        in++;
    }}} // has args
    if(!done){
        if(xmlargs) cmd.exec(xSet.getAccess()+" "+xmlp.getXmlString());
        else        cmd.exec(xSet.getAccess()+" "+sb.toString());
        done=true;
    }
} // has XML
xLogger.print(1,cmd.getParser().getFull()+" "+sb.toString());
}
private void addPrompt(String label, JTextField input){
    gridconst.gridwidth = GridBagConstraints.RELATIVE; //next-to-last
    gridconst.fill = GridBagConstraints.NONE;      //reset to default
    gridconst.weightx = 0.0;                       //reset to default
    JLabel lab=new JLabel(label);
    lab.setForeground(Color.blue);
    Args.add(lab, gridconst);
    gridconst.gridwidth = GridBagConstraints.REMAINDER;     //end row
    gridconst.fill = GridBagConstraints.HORIZONTAL;
    gridconst.weightx = 1.0;
    Args.add(input, gridconst);
}
private void addCheckBox(String label, JCheckBox check){
    gridconst.gridwidth = GridBagConstraints.RELATIVE; //next-to-last
    gridconst.fill = GridBagConstraints.NONE;      //reset to default
    gridconst.weightx = 0.0;                       //reset to default
    JLabel lab=new JLabel(label);
    lab.setForeground(Color.blue);
    Args.add(lab, gridconst);
    gridconst.gridwidth = GridBagConstraints.REMAINDER;     //end row
    gridconst.fill = GridBagConstraints.HORIZONTAL;
    gridconst.weightx = 1.0;
    Args.add(check, gridconst);
}
private void addPromptButton(String command){
    gridconst.gridwidth = GridBagConstraints.REMAINDER;     //end row
    gridconst.fill = GridBagConstraints.HORIZONTAL;
    gridconst.weightx = 0.0;
    JLabel lab=new JLabel(" ");
    lab.setForeground(Color.red);
    Args.add(lab, gridconst);

    gridconst.gridwidth = GridBagConstraints.REMAINDER; //next-to-last
    gridconst.fill = GridBagConstraints.HORIZONTAL;      //reset to default
    gridconst.weightx = 0.0;                       //reset to default  
    JButton exec = new JButton(execIcon); 
    exec.setBorderPainted(false);
    exec.setContentAreaFilled(false);
    getRootPane().setDefaultButton(exec);
    exec.addActionListener(this);
    Args.add(exec, gridconst);
}
private void addPromptHeader(String command, String scope){
    Args.removeAll();
    TitledBorder bord=BorderFactory.createTitledBorder(new String("Command "+command+", scope "+scope));
    bord.setTitleColor(Color.red);
    Args.setBorder(bord);
    gridconst.anchor = GridBagConstraints.EAST;
}
private void setPromptPanel(xDimCommand cmd){
int iInputs=0, iCheckers=0;
xXmlParser xparser=cmd.getXmlParser();
String command=cmd.getParser().getName();
String type=cmd.getType();
JCheckBox cb;
inputs=new Vector<JTextField>();
checkers=new Vector<JCheckBox>();
if(xparser==null){ // create default one
	xparser=new xXmlParser();
	xparser.standard(cmd.getParser().getFull(),command,"MBS");
	cmd.setXmlParser(xparser);
}
if(xparser!=null){
    int nargs=xparser.getNargs();
    argnames=new String[nargs];
    argtypes=new String[nargs];
    addPromptHeader(xparser.getCommandName(),xparser.getCommandScope());
    for(int ii=0;ii<nargs;ii++){
        argnames[ii]=xparser.getArgumentName(ii);
        argtypes[ii]=xparser.getArgumentType(ii);
        if(argtypes[ii].startsWith("B")){
            cb = new JCheckBox();
            if(xparser.getArgumentValue(ii).equals("true"))cb.setSelected(true);
            checkers.add(cb);
            addCheckBox(argnames[ii],checkers.elementAt(iCheckers));
            iCheckers++;
        } else {
            inputs.add(new JTextField(xparser.getArgumentValue(ii),18));
            addPrompt(" "+
                new String(argnames[ii]+" ( "+
                argtypes[ii]+" , "+
                xparser.getArgumentRequired(ii)+" ) : "),
                inputs.elementAt(iInputs));
            iInputs++;
        }
    }
}}
/**
 * Called by desktop, format from xiUserPanel.getUserCommand()
 * @param format format.getArgumentStyleXml(...) function is called
 * before command argument composing
 * to check if arguments should be formatted in XML or not.
 * @see xiUserPanel
 */
protected void setUserCommand(xiUserCommand format){userCmd=format;}
/**
 * Called by desktop, descriptors from PanelParameter
 * @param desc Descriptor list as returned from xPanelParameter.getCommandDescriptors()
 * @see xPanelParameter
 */
protected void setCommandDescriptors(Vector<xXmlParser> desc){
	Element root,com,arg;
	NodeList comlist, arglist;
	xXmlParser xmlp;
	StringBuffer str;
	System.out.println("Set command definitions");
// look if we have saved and restored definitions with values
	Vector<String> comdefs=new Vector<String>();
	comlist = xSet.getCommandXml();
	if(comlist != null){ // get definitions
		// build vector with definitions
		for(int i=0;i<comlist.getLength();i++){
			com=(Element)comlist.item(i);
			str=new StringBuffer();
			str.append("<command ");
			str.append(xXml.attr("name",com.getAttribute("name")));
			str.append(xXml.attr("dim",com.getAttribute("dim")));
			str.append(xXml.attr("scope",com.getAttribute("scope")));
			str.append(xXml.attr("content",com.getAttribute("content"),">\n"));
			arglist=com.getElementsByTagName("*");
			for(int ii=0;ii<arglist.getLength();ii++){
				arg=(Element)arglist.item(ii);
				str.append("<argument ");
				str.append(xXml.attr("name",arg.getAttribute("name")));
				str.append(xXml.attr("type",arg.getAttribute("type")));
				str.append(xXml.attr("value",arg.getAttribute("value")));
				str.append(xXml.attr("required",arg.getAttribute("required"),"/>\n"));
			}
			str.append(xXml.tag("command",xXml.CLOSE));
			comdefs.add(str.toString());
		}
	}
	// look through command definitions to find the commands
	for(int i=0;i<vcom.size();i++){
// look for definitions for this command
		for(int ii=0;ii<comdefs.size();ii++){
			if(comdefs.get(ii).indexOf(vcom.get(i).getParser().getFull())>0){
			System.out.println("Restore arguments of "+vcom.get(i).getParser().getFull());
			xmlp=new xXmlParser();
			xmlp.parseXmlString(vcom.get(i).getParser().getFull(),comdefs.get(ii));
			xmlp.isChanged(true); // otherwise this would not be stored
			vcom.get(i).setXmlParser(xmlp); // will not be overwritten
		}}
		// no definitions yet, take initial ones
			for(int ii=0;ii<desc.size();ii++){
			xmlp=desc.elementAt(ii);
			if(vcom.get(i).getParser().getFull().equals(xmlp.getName())){
				vcom.get(i).setXmlParser(xmlp);
				break;
			}}
		}
}
}



