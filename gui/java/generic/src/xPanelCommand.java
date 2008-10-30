package xgui;
/*
This client expects server tDABCserver from eclipse DIM project.
*/


/**
* @author goofy
*/
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

/**
* DIM GUI class
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
private String treepath;
private Vector<xDimCommand> vcom;
private Vector<JTextField> inputs;
private Vector<JCheckBox> checkers;
private String[] argnames;
private String[] argtypes;
private ImageIcon execIcon;
private xiUserCommand userCmd;

public void actionPerformed(ActionEvent e) {
executeCommands();
}
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
        setPromptPanel(cmd.getXmlParser(),cmd.getParser().getName(), cmd.getType());
        addPromptButton("Execute");
        Args.revalidate();
        }
    else System.out.println("null");
}
//----------------------------------------------------------
// called by mouseClicked, nodesel set by previously called valueChanged
public void executeCommands(){
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
public void executeCommand(xDimCommand cmd){
boolean done=false;
boolean xmlargs=false;
StringBuffer sb=null;
int in=0;
xXmlParser x =cmd.getXmlParser();
sb=new StringBuffer();
// No XML command description,
// build simple command string, i.e. only one text field
if(inputs.size() == 1){
if(argnames[0].equals("*")){
    sb.append(inputs.elementAt(0).getText());
    cmd.exec(xSet.getAccess()+" "+sb.toString());
    done=true;
}}
// we have an XML description
if(!done && (x != null)){ 
// System.out.println(x.getName());
// System.out.println(x.getXmlString());
// application can specify if this command needs XML arguments or not
    if(userCmd != null) xmlargs=userCmd.getArgumentStyleXml(x.getCommandScope(),cmd.getParser().getFull());
// Store values in xml description without header
    x.newCommand(x.getCommandName(),false,x.getCommandScope(),true); // values have changed
    in=0;
    for(int i=0;i<inputs.size();i++){
        x.addArgument(argnames[in],
            x.getArgumentType(in),
            inputs.elementAt(i).getText(),
            x.getArgumentRequired(in));
            in++;
    }
    for(int i=0;i<checkers.size();i++){
        if(checkers.elementAt(i).isSelected())
            x.addArgument(argnames[in],
                x.getArgumentType(in),
                "true",
                x.getArgumentRequired(in));
                else
            x.addArgument(argnames[in],
                x.getArgumentType(in),
                "false",
                x.getArgumentRequired(in));
        in++;
    }
// finalize XML string
    x.parseXmlString(x.getCommandName(),x.getXmlString());
// build arg=value arg=value string
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
        else if(x.getArgumentRequired(in).startsWith("R")){
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
    }
    if(!done){
        if(xmlargs) cmd.exec(xSet.getAccess()+" "+x.getXmlString());
        else        cmd.exec(xSet.getAccess()+" "+sb.toString());
        done=true;
    }
}
xLogger.print(1,cmd.getParser().getFull()+" "+sb.toString());
}
//----------------------------------------------------------
/**
 * DIM GUI class. Uses JScrollPanes with GridBagLayout.
 */
public xPanelCommand(xDimBrowser browser, Dimension dim) {
    super(new GridLayout(1,0));
// xDimCommand list of commands
    dimbrowser=browser;
    vcom=browser.getCommandList();
    icmd=vcom.size(); // number of commands
    if(icmd>0)initPanel(dim);
}

public void setUserCommand(xiUserCommand uc){userCmd=uc;}

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
        }
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
private void setPromptPanel(xXmlParser xparser, String command, String type){
int iInputs=0, iCheckers=0;
JCheckBox cb;
inputs=new Vector<JTextField>();
checkers=new Vector<JCheckBox>();
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
        }
        else {
            inputs.add(new JTextField(xparser.getArgumentValue(ii),18));
            addPrompt(" "+
                new String(argnames[ii]+" ( "+
                argtypes[ii]+" , "+
                xparser.getArgumentRequired(ii)+" ) : "),
                inputs.elementAt(iInputs));
            iInputs++;
        }
    }
} else {
    inputs.add(new JTextField("",18));
    argnames=new String[1];
    argtypes=new String[1];
    argtypes[0]="C";
    argnames[0]="*"; // these are returned as value only, without name=
    addPromptHeader(command,"Common");
    addPrompt(new String(" * ( "+type+" ) : "),inputs.elementAt(0));
}
}
// called by xGui, descriptors from PanelParameter
public void setCommandDescriptors(Vector<Object> desc){
// look through command definitions to find the commands
    for(int i=0;i<vcom.size();i++){
    for(int ii=0;ii<desc.size();ii++){
        xXmlParser x=(xXmlParser)desc.elementAt(ii);
        if(vcom.get(i).getParser().getFull().equals(x.getName())){
            vcom.get(i).setXmlParser(x);
            break;
    }}}
}
}



