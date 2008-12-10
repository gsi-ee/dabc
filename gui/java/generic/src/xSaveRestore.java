package xgui;
import java.util.*;
import java.io.*;
import org.w3c.dom.*;
import javax.xml.parsers.*;
/**
 * Base class for DIM SaveRestore data.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xSaveRestore{
/**
 * DIM SaveRestore object:
 */
public xSaveRestore(){}
/**
 * Save all records attached to parameters (meters and histograms).
 * @param vpar Vector of xiDimParameters.
 * @param file Xml file name.
 */
public static final void saveRecords(Vector<xiDimParameter> vpar, String file){
    StringBuffer str=new StringBuffer();
    int ipar=vpar.size();
    str.append(xSet.XmlHeader());
    str.append(xSet.XmlTag("Record",xSet.OPEN));
    for(int i=0;i<ipar;i++) {
        xRecordMeter recmet=vpar.get(i).getMeter();
        if(recmet!=null)str.append(recmet.XmlLine());
        xRecordHisto rechis=vpar.get(i).getHisto();
        if(rechis!=null)str.append(rechis.XmlLine());
    }
    str.append(xSet.XmlTag("Record",xSet.CLOSE));
    try{
        FileWriter fw = new FileWriter(file);
        fw.write(str.toString());
        fw.close();
    }catch(IOException ioe){System.out.println("Error writing record file "+file);}
}
/**
 * Restore all records attached to parameters (meters and histograms).
 * XML tree is stored in xSet and can be retrieved by getRecordXml called in xPanelParameter.
 * @param file Xml file name.
 * @see xSet
 * @see xPanelParameter
 */
public static final void restoreRecords(String file){
Element root,elem,el;
try{
    DocumentBuilderFactory factory=DocumentBuilderFactory.newInstance();
    DocumentBuilder builder=factory.newDocumentBuilder();
    Document document=builder.parse(new File(file));
    root=document.getDocumentElement();
    xSet.setRecordXml(root.getElementsByTagName("*")); 
    // for(int i=0;i<list.getLength();i++){
        // el=(Element)list.item(i);
        // if(el.getTagName().equals("Meter"))recmet.restoreRecord(el);
        // if(el.getTagName().equals("Histo"))rechis.restoreRecord(el);
    // }
}catch(Exception e){System.out.println("Error restoring "+file);}
}

/**
 * Save layouts to xml file
 * @param file File name (ending with .xml).
 * @see xSet
 */
protected final static void saveLayouts(String file){
    Vector<xLayout> layouts = xSet.getLayouts();
    int[] ParTableWidth = xSet.getParTableWidth();
    StringBuffer str=new StringBuffer();
    str.append(xSet.XmlHeader());
    str.append(xSet.XmlTag("Layout",xSet.OPEN));
    str.append(xSet.XmlTag("WindowLayout",xSet.OPEN));
    // append window specs
    for(int i =0; i < layouts.size(); i++) str.append(layouts.elementAt(i).XmlLine()); 
    str.append(xSet.XmlTag("WindowLayout",xSet.CLOSE));
    // append table specs
    str.append(xSet.XmlTag("TableLayout",xSet.OPEN));
    str.append(String.format("<Parameter width=\"%d",ParTableWidth[0]));
    for(int i=1;i<ParTableWidth.length;i++) str.append(","+Integer.toString(ParTableWidth[i]));
    str.append("\" />\n");
    str.append(xSet.XmlTag("TableLayout",xSet.CLOSE));
    str.append(xSet.XmlTag("Layout",xSet.CLOSE));
    try{
        FileWriter fw = new FileWriter(file);
        fw.write(str.toString());
        fw.close();
    }catch(IOException ioe){System.out.println("Error writing layout file");}
}
/**
 * Restore layouts from xml file
 * @param file File name (ending with .xml).
 * @see xSet
 */
protected final static void restoreLayouts(String file){
String s;
int col;
boolean show;
Element root,elem,el;
try{
    DocumentBuilderFactory factory=DocumentBuilderFactory.newInstance();
    DocumentBuilder builder=factory.newDocumentBuilder();
    Document document=builder.parse(new File(file));
    root=document.getDocumentElement();
    elem=(Element)root.getElementsByTagName("WindowLayout").item(0); // only one WindowLayout!
    NodeList list=elem.getElementsByTagName("*"); // list of layout names
    for(int i=0;i<list.getLength();i++){
        el=(Element)list.item(i);
        xSet.setWindowLayout(el);
        // System.out.println("Layout elements "+el.getTagName());
    }
    elem=(Element)root.getElementsByTagName("TableLayout").item(0); // only one TableLayout!
    xSet.setTableLayout("Parameter",elem);
}catch(Exception e){}
}

} // class xSaveRestore
