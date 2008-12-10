package xgui;
import java.util.*;
import org.w3c.dom.*;
import org.xml.sax.*;
import javax.xml.parsers.*;
import java.io.*;
import java.awt.event.*;
/**
 * Base class to keep the data of the setup forms for DABC.
 * Reads/writes XML setup file.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xFormDabc extends xForm{
private String Name;
private String Setup;
private DocumentBuilderFactory factory;
private DocumentBuilder builder;
private Document document;
private Element root, elem;

public xFormDabc() {
setDefaults();
}
public xFormDabc(String file) {
setDefaults();
restoreSetup(file);
}
public xFormDabc(String file, ActionListener action) {
super(action);
setDefaults();
restoreSetup(file);
}

protected void setDefaults(){
    Master=new String("<MasterNode>");
    Name=new String("<MasterName>");
    Servers=new String("0");
    UserPath=new String("<UserPath>");
    SystemPath=new String("<DABC system path>");
    Script=new String("<ssh command>");
    Setup=new String("<DABC setup file>");
}

protected void printForm(){
System.out.println(build().toString());
}
private StringBuffer build(){
StringBuffer str=new StringBuffer();
str.append("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
str.append("<DabcLaunch>\n");
str.append("<Labels\n");
// append prompt line specs
str.append("DabcMaster=\"DABC Master\"\n");
str.append("DabcName=\"DABC Name\"\n");
str.append("DabcUserPath=\"DABC user path\"\n");
str.append("DabcSystemPath=\"DABC system path\"\n");
str.append("DabcSetup=\"DABC setup file\"\n");
str.append("DabcScript=\"DABC Script\"\n");
str.append("DabcServers=\"%Number of needed DIM servers%\"\n");
str.append("/>\n");
// append value specs
str.append("<Fields\n");
str.append("DabcMaster=\""+Master+"\"\n");
str.append("DabcName=\""+Name+"\"\n");
str.append("DabcUserPath=\""+UserPath+"\"\n");
str.append("DabcSystemPath=\""+SystemPath+"\"\n");
str.append("DabcSetup=\""+Setup+"\"\n");
str.append("DabcScript=\""+Script+"\"\n");
str.append("DabcServers=\""+Servers+"\"\n");
str.append("/>\n");
str.append("</DabcLaunch>\n");
return str;
}
protected void saveSetup(String file){
try{
    FileWriter fw = new FileWriter(file);
    fw.write(build().toString());
    fw.close();
    System.out.println("Dabc launch setup: "+file);
}catch(IOException ioe){System.out.println("Error writing Dabc launch setup "+file);}
}

protected void restoreSetup(String file){
LaunchFile=new String(file);
try{
    factory=DocumentBuilderFactory.newInstance();
    builder=factory.newDocumentBuilder();
    document=builder.parse(new File(file));
    root=document.getDocumentElement();
    elem=(Element)root.getElementsByTagName("Fields").item(0);
    Master=new String(elem.getAttribute("DabcMaster").toString());
    Name=new String(elem.getAttribute("DabcName").toString());
    Servers=new String(elem.getAttribute("DabcServers").toString());
    UserPath=new String(elem.getAttribute("DabcUserPath").toString());
    SystemPath=new String(elem.getAttribute("DabcSystemPath").toString());
    Script=new String(elem.getAttribute("DabcScript").toString());
    Setup=new String(elem.getAttribute("DabcSetup").toString());
    nServers=1+Integer.parseInt(Servers);// add DNS
}catch(Exception e){System.out.println("Error reading "+file);}
}
/**
 * @param setup DABC setup file name
 */
protected void setSetup(String setup){Setup=setup;}
/**
 * @return DABC setup file name
 */
public String getSetup(){return Setup;}
/**
 * @param name DABC master name
 */
protected void setName(String name){Name=name;}
/**
 * @return DABC master name
 */
public String getName(){return Name;}
}

