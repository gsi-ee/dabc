package xgui;
import java.util.*;
import org.w3c.dom.*;
import javax.xml.parsers.*;
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
    str.append(xSet.XmlHeader());
    str.append(xSet.XmlTag("DabcLaunch",xSet.OPEN));
    str.append("<DabcMaster "+key("prompt","DABC Master")+key("value",Master)+"/>\n");
    str.append("<DabcName "+key("prompt","DABC Name")+key("value",Name)+"/>\n");
    str.append("<DabcUserPath "+key("prompt","DABC user path")+key("value",UserPath)+"/>\n");
    str.append("<DabcSystemPath "+key("prompt","DABC system path")+key("value",SystemPath)+"/>\n");
    str.append("<DabcSetup "+key("prompt","DABC setup file")+key("value",Setup)+"/>\n");
    str.append("<DabcScript "+key("prompt","DABC Script")+key("value",Script)+"/>\n");
    str.append("<DabcServers "+key("prompt","%Number of needed DIM servers%")+key("value",Servers)+"/>\n");
    str.append(xSet.XmlTag("DabcLaunch",xSet.CLOSE));
return str;
}
/**
 * Writes Xml file with filter setup values.
 * @param file Xml file name.
 */
protected void saveSetup(String file){
System.out.println("Dabc launch setup: "+file);
    xSet.writeXml(file,build().toString());
}
/**
 * Reads Xml file and restores filter setup values.
 * @param file Xml file name.
 */
protected void restoreSetup(String file){
NodeList li;
System.out.println("Restore Dabc launch setup from "+file);
LaunchFile=new String(file);
    Element root=xSet.readXml(file);
    if(root != null){
        li=root.getElementsByTagName("DabcMaster");
        Master=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("DabcName");
        Name=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("DabcUserPath");
        UserPath=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("DabcSystemPath");
        SystemPath=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("DabcScript");
        Script=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("DabcSetup");
        Setup=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("DabcServers");
        Servers=((Element)li.item(0)).getAttribute("value");
        nServers=1+Integer.parseInt(Servers);// add DNS
    }
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

