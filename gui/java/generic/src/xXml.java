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
import java.io.*;
import org.w3c.dom.*;
import org.xml.sax.*;
import javax.xml.parsers.*;
/**
 * Singleton for basic XML functions.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xXml
{
public static boolean OPEN=true;
public static boolean CLOSE=false;
/**
 * Singleton
 */
public xXml(){
}
/**
 * XML opening string.
 * @return Standard XML opening string.
 */
public final static String header(){
return new String("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
}
/**
 * XML tag string.
 * @param name Tag name.
 * @param open True: open tag, false: close tag.
 * @return XML string.
 */
public final static String tag(String name, boolean open){
if(open)return new String("<"+name+">\n");
else return new String("</"+name+">\n");
}
/**
 * @param key key name.
 * @param value value.
 * @return formatted string key="value". Space appended.
 */
public final static String attr(String key, String value){
return attr(key,value," ");
}
/**
 * @param key key name.
 * @param value value. Written as true or false, respectively.
 * @return formatted string key="true"/"false". Space appended.
 */
public final static String attr(String key, boolean value){
return attr(key,value," ");
}
/**
 * @param key key name.
 * @param value value.
 * @param term terminator (should include \n).
 * @return formatted string key="value". Space appended.
 */
public final static String attr(String key, String value, String term){
return new String(key+"=\""+value+"\""+term);
}
/**
 * @param key key name.
 * @param value value. Written as true or false, respectively.
 * @return formatted string key="true"/"false". Space appended.
 */
public final static String attr(String key, boolean value, String term){
return new String(key+"=\""+new Boolean(value).toString()+"\""+term);
}
/**
 * Write xml file
 * @param file File name (ending with .xml).
 * @param xml Xml string to write.
 */
public final static void write(String file, String xml){
try{
    FileWriter fw = new FileWriter(file);
    fw.write(xml);
    fw.close();
}catch(IOException ioe){System.out.println("Error writing XML file "+file);}
}
/**
 * Read XML file
 * @param file File name (ending with .xml).
 * @return Top element.
 */
public final static Element read(String file){
Element root=null;
try{
    DocumentBuilderFactory factory=DocumentBuilderFactory.newInstance();
    DocumentBuilder builder=factory.newDocumentBuilder();
    Document document=builder.parse(new File(file));
    root=document.getDocumentElement();
}catch(Exception e){System.out.println("Error reading XML file "+file);}
return root;
}
/**
 * Read XML string. 
 * @param xmlstring Encoded XML string.
 * @return Top element.
 */
public final static Element string(String xmlstring){
Element root=null;
try{
    DocumentBuilderFactory factory=DocumentBuilderFactory.newInstance();
    DocumentBuilder builder=factory.newDocumentBuilder();
    Document document=builder.parse(new InputSource(new StringReader(xmlstring)));
    root=document.getDocumentElement();
}catch(Exception e){e.printStackTrace();}
return root;
}
/**
 * Get named subelement of an element. 
 * @param root Top element.
 * @param tagname Name of element to be returned.
 * @return Element.
 */
public final static Element get(Element root, String tagname){
if(root == null) return null;
return (Element)root.getElementsByTagName(tagname).item(0);
}
}
