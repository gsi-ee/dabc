package xgui;
import javax.swing.JEditorPane;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JLabel;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.UIManager;
import javax.swing.ImageIcon;
import javax.swing.table.*;

import javax.swing.JTable;
import javax.swing.table.TableColumn;
import java.net.URL;
import java.io.IOException;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.GridLayout;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import java.awt.event.MouseListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseAdapter;
import java.util.*;
import org.w3c.dom.*;
import javax.xml.parsers.*;

/**
 * Handles parameter table. On parameter update, the table is updated.
 * @author Hans G. Essel
 * @version 1.0
 * @see xDimParameter
 */
public class xPanelParameter extends JPanel implements TableModelListener {
private static xParser pars=new xParser();
private static final int COLS=8;
private xPanelMeter metpan;
private xPanelState stapan;
private xPanelInfo infpan;
private xPanelHisto hispan;
private xDimBrowser dimbrowser;
private JTable partab;
private xParTable partabmod;
private int i,ii,indent,version=0;
private int ipar,icmd;
private Object obj;
private Vector<xDimParameter> vpar;
private Vector<xXmlParser> descriptors;
private boolean statusOK=true;

/**
 * Panel for parameter table. Gets xDimParameter list from browser and calls initPanel.
 * Polls over all parameters until all have been updated at least once (quality valid).
 * Cleans all graphics panels and build new table.
 * Graphics panels must be rebuilt from caller.
 * Keeps a list of command descriptors (xXmlParser).
 * @param browser DIM browser.
 * @param dim Size and position of window.
 */
public xPanelParameter(xDimBrowser browser, Dimension dim) {
    super(new GridLayout(1,0));
    dimbrowser=browser;
// xDimParameter list of parameters
    vpar=browser.getParameterList();
    ipar=vpar.size(); // number of parameters
    xSet.setMessage("Update parameter list done.");
    xSet.setSuccess(true);
    initPanel(dim);
}
// protected void finalize(){
// System.out.println("finalize panel param");
// descriptors=null;
// vpar=null;
// }

private void initPanel(Dimension dim){
    metpan=dimbrowser.getPanelMeter();
    hispan=dimbrowser.getPanelHistogram();
    stapan=dimbrowser.getPanelState();
    infpan=dimbrowser.getPanelInfo();
    descriptors=new Vector<xXmlParser>();
//-------------------------------------------------------------------------------------------------------
// before we fill the table, we must be sure that the first
// parameter update has been done. Otherwise we would not get correct quality.
// Before we create a new table, we must be sure that
// infoHandler does not use wrong table row index.
// Therefore we set all indices to -1, will be set below (addRow)
// Parameters are inactive at this point.
    for(i=0;i<ipar;i++) {
        vpar.get(i).setTableIndex(-1);
    }
    dimbrowser.sleep(2);
    System.out.print("Wait for update "+ipar);
    int timeout=0;
    boolean ok=false;
    int noq=0;
    while(!ok){
        ok=true;
        for(i=0;i<ipar;i++) if(vpar.get(i).getDimQuality() == -1){
        ok=false;
        noq++;
        // vpar.get(i).releaseService();
        // vpar.get(i).setParameterActiv(false);
        // System.out.println("Replace  "+vpar.get(i).getParser().getFull()+ " "+vpar.get(i).getParser().getFormat());
        // xDimParameter p=newParameter(vpar.get(i).getParser().getFull(),vpar.get(i).getParser().getFormat());
        // vpar.set(i,p);
        }
        if(ok)break;
        System.out.print("."+noq);
        dimbrowser.sleep(2);
        timeout++;
        if(timeout > 3)break;
    }
    if(ok) {
        System.out.println(" done");
    } else {
        System.out.println("\nfailed");
        xSet.setMessage("Update parameter list failed!");
        for(i=0;i<ipar;i++) 
        	if(vpar.get(i).getDimQuality() == -1)
        		vpar.get(i).printParameter(true);
        xSet.setSuccess(false);
        statusOK=false;
        return;
    }
    // now all records and graphical objects of the parameters should have been created.
    // Because the table is not yet built, none is added to the graphics panels.    
    // See if we must update some values from XML file.
    NodeList list = xSet.getRecordXml();
    if(list != null){
    for(ii=0;ii<ipar;ii++) {
        xRecordMeter recmet=vpar.get(ii).getMeter();
        xRecordHisto rechis=vpar.get(ii).getHisto();
        for(int i=0;i<list.getLength();i++){
            Element el=(Element)list.item(i);
            if(el.getAttribute("name").toString().equals(vpar.get(ii).getName())){
            if(recmet != null){
                recmet.restoreRecord(el);
                vpar.get(ii).setAttributeMeter(); // from recmet
                }
            else if(rechis != null){
                rechis.restoreRecord(el);
                vpar.get(ii).setAttributeHisto(); // from rechis
                }
            break;
            }
        }
    }}
    
    // cleanup graphics panels.
    hispan.cleanup();
    metpan.cleanup();
    stapan.cleanup();
    infpan.cleanup();
//----------------------------------------------------------
// Create the table for the parameters, specify editable columns
    xParTable partabmod = new xParTable(COLS-2,COLS-1); // column 6 editable, 7 meter button
    partab= new JTable(partabmod);
    partabmod.addMouseListenerToHeaderInTable(partab);
    partabmod.addTableModelListener(this);
    partabmod.addColumn(new String("ID"));
    partabmod.addColumn(new String("Node"));
    partabmod.addColumn(new String("Application"));
    partabmod.addColumn(new String("Parameter"));
    partabmod.addColumn(new String("Type"));
    partabmod.addColumn(new String("Current"));
    partabmod.addColumn(new String("Set value"));
    partabmod.addColumn(new String("Show"));
// Fill parameters into table
// this is used to access the display panels.
// partabmod must be passed explicitely, because getTable would return null.
    int irow=0;
    for(i=0;i<ipar;i++) {
        vpar.get(i).setIndex(i); // sets paraShown false to trigger addition of graphic elements to graphic panel 
        if(vpar.get(i).addRow(partabmod,irow)) irow++; // create/add graphics to panels
        if(vpar.get(i).isCommandDescriptor()) descriptors.add(vpar.get(i).getXmlParser());
    }// parlist
//partab.setAutoResizeMode(JTable.AUTO_RESIZE_ALL_COLUMNS);
    partab.setEnabled(true);
    partab.setRowSelectionAllowed(true);
    TableColumn column = null;
    int[] w=xSet.getParTableWidth();
    for(i=0;i<COLS;i++){
        column = partab.getColumnModel().getColumn(i);
        column.setPreferredWidth(w[i]); 
    }
//-------------------------------------------------------------------------
//Create the parameter viewing pane.
    JScrollPane paraView = new JScrollPane(partab);
    // substract difference between inner and outer frame
    paraView.setMinimumSize(xSet.addDimension(dim, new Dimension(-10,-32))); 
    paraView.setPreferredSize(xSet.addDimension(dim, new Dimension(-10,-32)));
//Add frame to panel
    add(paraView);
// Rebuild graphics panels
    infpan.updateAll();
    stapan.updateAll();
    metpan.updateAll();
    hispan.updateAll();
}
private xDimParameter newParameter(String name, String format){
xDimParameter dimpar;
         if(format.startsWith("C")) dimpar= new xDimParameter(name,format,"%BrokenLink%",version);
    else if(format.startsWith("F")) dimpar= new xDimParameter(name,format,(float)-1.,version);
    else                            dimpar= new xDimParameter(name,format,-1,version);
    dimpar.setPanels(hispan,metpan,stapan,infpan);
    return dimpar;
}
//----------------------------------------------------------
/**
 * Called by desktop before saving layout.
 * stores column widths in xSet 
 */
public void saveColWidth()
{
int[] cols = new int[COLS];
try{
    TableColumnModel tm=partab.getColumnModel();
    for(int i=0;i<COLS;i++) cols[i]=tm.getColumn(i).getWidth()-1;// substract increment
} catch(NullPointerException x){
    System.out.println("xPanelParameter: Error saving table column widths");
    return;}
xSet.setParTableWidth(cols);
return;
}
/**
 * Called from desktop, the list is passed to xPanelCommand.
 * @return List of command descriptors (xXmlParser)
 */
public Vector<xXmlParser> getCommandDescriptors(){return descriptors;}
/**
 * If the parameter is not changable, value is "-"
 * Therefore we must ignore value "-". Effect is that
 * a value of "-" cannot be set to a parameter.
 */
 public void tableChanged(TableModelEvent e) {
    xDimParameter dp;
    int row = e.getFirstRow();
    int last = e.getLastRow();
    int column = e.getColumn();
//	System.out.println("row "+row+" last "+last+" col "+column+" ");
// when table has been ordered by columns all rows changed
    if(last > row){
        xParTable model = (xParTable)e.getSource();
        if(model == null)System.out.println("model NULL");
        // the model must tell all parameter objects their new row index
        else {
            for (int i = 0;i<model.getRowCount();i++){
                dp = (xDimParameter) model.getValueAt(i, 0);
                dp.setTableIndex(i);
            }}
    } else {
        // we get events each time any column changes
        // therefore we must filter input column
        if(column == 6){ // value column
            // for some reasons, partab is available, but partabmod is NULL
            // to be sure, get model from event
            xParTable model = (xParTable)e.getSource();
            if(model == null)System.out.println("model NULL");
            else{   // if the parameter is not changable, value is set to "-"
                    // in setParameter and tableChanged is called again.
                    // Therefore we must ignore value "-". Effect is that
                    // a value of "-" cannot be set to a parameter.
                dp = (xDimParameter) model.getValueAt(row, 0);
                if(!model.getValueAt(row, column).toString().equals("-"))
                dp.setParameter(model.getValueAt(row, column).toString());
            }}
        if(column == 7){ // Visibility column
            // for some reasons, partab is available, but partabmod is NULL
            // to be sure, get model from event
            xParTable model = (xParTable)e.getSource();
            if(model == null)System.out.println("model NULL");
            else{
                dp = (xDimParameter) model.getValueAt(row, 0);
                Boolean b = (Boolean)model.getValueAt(row,column);
                if(dp.getParser().isRate())     dp.createMeter(b); {if(b)metpan.updateAll();}
                if(dp.getParser().isHistogram())dp.createHisto(b); {if(b)hispan.updateAll();}
                if(dp.getParser().isState())    dp.createState(b); {if(b)stapan.updateAll();}
                if(dp.getParser().isInfo())     dp.createInfo(b); {if(b)infpan.updateAll();}
            }}}}
}



