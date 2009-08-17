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
import java.io.*;
import org.w3c.dom.*;
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
    str.append(xXml.header());
    str.append(xXml.tag("Record",xXml.OPEN));
    for(int i=0;i<ipar;i++) {
        xRecordMeter recmet=vpar.get(i).getMeter();
        if(recmet!=null)str.append(recmet.XmlLine());
        xRecordHisto rechis=vpar.get(i).getHisto();
        if(rechis!=null)str.append(rechis.XmlLine());
        xRecordState recsta=vpar.get(i).getState();
        if(recsta!=null)str.append(recsta.XmlLine());
    }
    str.append(xXml.tag("Record",xXml.CLOSE));
    xXml.write(file, str.toString());
	System.out.println("Store record attributes to "+file);
}
/**
 * Restore all records attached to parameters (meters and histograms).
 * XML tree is stored in xSet and can be retrieved by getRecordXml 
 * called in xPanelParameter.
 * @param file Xml file name.
 * @see xSet
 * @see xPanelParameter
 */
public static final void restoreRecords(String file){
Element root,elem,el;
    root=xXml.read(file);
    if(root != null){
    	System.out.println("Restore record attributes from "+file);
    	xSet.setRecordXml(root.getElementsByTagName("*")); 
    }
    else xSet.setRecordXml(null); 
}
/**
 * Save all records attached to parameters (meters and histograms).
 * @param vpar Vector of xiDimParameters.
 * @param file Xml file name.
 */
public static final void saveCommands(Vector<xDimCommand> vcom, String file){
    StringBuffer str=new StringBuffer();
    int icom=vcom.size();
	System.out.println("Store command attributes to "+file);
    str.append(xXml.header());
    str.append(xXml.tag("Commands",xXml.OPEN));
    for(int i=0;i<icom;i++) {
    	if(vcom.get(i).getXmlParser()!=null){
    	if(vcom.get(i).getXmlParser().isChanged()){
        		str.append(vcom.get(i).getXmlParser().getXmlString());
        		System.out.println("Attributes "+vcom.get(i).getParser().getApplication());
        }}
    }
    str.append(xXml.tag("Commands",xXml.CLOSE));
    xXml.write(file, str.toString());
}
/**
 * Restore all records attached to parameters (meters and histograms).
 * XML tree is stored in xSet and can be retrieved by getRecordXml 
 * called in xPanelParameter.
 * @param file Xml file name.
 * @see xSet
 * @see xPanelParameter
 */
public static final void restoreCommands(String file){
Element root;
NodeList comlist;
comlist=null;
root=xXml.read(file);
if(root != null){
	comlist = root.getElementsByTagName("command");
	System.out.println("Restore command attributes from "+file);
}
xSet.setCommandXml(comlist); 
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
    str.append(xXml.header());
    str.append(xXml.tag("Layout",xXml.OPEN));
    str.append(xXml.tag("WindowLayout",xXml.OPEN));
    // append window specs
    for(int i =0; i < layouts.size(); i++) str.append(layouts.elementAt(i).XmlLine()); 
    str.append(xXml.tag("WindowLayout",xXml.CLOSE));
    // append table specs
    str.append(xXml.tag("TableLayout",xXml.OPEN));
    str.append(String.format("<Parameter width=\"%d",ParTableWidth[0]));
    for(int i=1;i<ParTableWidth.length;i++) str.append(","+Integer.toString(ParTableWidth[i]));
    str.append("\" />\n");
    str.append(xXml.tag("TableLayout",xXml.CLOSE));
    str.append(xXml.tag("Layout",xXml.CLOSE));
    xXml.write(file, str.toString());
	System.out.println("Store layouts to "+file);
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
root=xXml.read(file);
if(root != null){
	System.out.println("Restore layouts from "+file);
	elem=xXml.get(root,"WindowLayout"); // only one WindowLayout!
    NodeList list=elem.getElementsByTagName("*"); // list of layout names
    for(int i=0;i<list.getLength();i++){
        el=(Element)list.item(i);
        xSet.setWindowLayout(el);
    }
    elem=xXml.get(root,"TableLayout"); // only one TableLayout!
    xSet.setTableLayout("Parameter",elem);
}}
} // class xSaveRestore
