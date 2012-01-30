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
import org.w3c.dom.*;
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
    Master=new String("%MasterNode%");
    Name=new String("%MasterName%");
    Servers=new String("0");
    nServers=0;
    UserPath=new String("%UserPath%");
    SystemPath=new String("%DABC system path%");
    Script=new String("%ssh command%");
    Setup=new String("%DABC setup file%");
}

protected void printForm(){
System.out.println(build().toString());
}
private StringBuffer build(){
    StringBuffer str=new StringBuffer();
    str.append(xXml.header());
    str.append(xXml.tag("DabcLaunch",xXml.OPEN));
    str.append("<DabcMaster "+xXml.attr("prompt","DABC Master")+xXml.attr("value",Master,"/>\n"));
    str.append("<DabcName "+xXml.attr("prompt","DABC Name")+xXml.attr("value",Name,"/>\n"));
    str.append("<DabcUserPath "+xXml.attr("prompt","DABC user path")+xXml.attr("value",UserPath,"/>\n"));
    str.append("<DabcSystemPath "+xXml.attr("prompt","DABC system path")+xXml.attr("value",SystemPath,"/>\n"));
    str.append("<DabcSetup "+xXml.attr("prompt","DABC setup file")+xXml.attr("value",Setup,"/>\n"));
    str.append("<DabcScript "+xXml.attr("prompt","DABC Script")+xXml.attr("value",Script,"/>\n"));
    str.append("<DabcServers "+xXml.attr("prompt","%Number of required DIM servers%")+xXml.attr("value",Servers,"/>\n"));
    str.append("<Minimized "+xXml.attr("value",Shrink,"/>\n"));
    str.append(xXml.tag("DabcLaunch",xXml.CLOSE));
return str;
}
/**
 * Writes Xml file with filter setup values.
 * @param file Xml file name.
 */
protected void saveSetup(String file){
System.out.println("Store Dabc launch setup to "+file);
    xXml.write(file,build().toString());
}
/**
 * Reads Xml file and restores filter setup values.
 * @param file Xml file name.
 */
protected void restoreSetup(String file){
NodeList li;
System.out.println("Restore Dabc launch setup from "+file);
LaunchFile=new String(file);
    Element root=xXml.read(file);
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
        nServers=Integer.parseInt(Servers);// add DNS
        li=root.getElementsByTagName("Minimized");
        if(li.getLength() > 0)
        Shrink=new Boolean(((Element)li.item(0)).getAttribute("value")).booleanValue();
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

