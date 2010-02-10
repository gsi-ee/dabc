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
 * Base class to keep the data of the setup forms for MBS
 * Reads/writes XML setup file.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xFormMbs extends xForm {
	private String Command;
	private String Shut;
	private String Start;

public xFormMbs() {
setDefaults();
}
public xFormMbs(String file) {
setDefaults();
restoreSetup(file);
}
public xFormMbs(String file, ActionListener action) {
super(action);
setDefaults();
restoreSetup(file);
}

protected void setDefaults(){
    Master=new String("%MasterNode%");
    Servers=new String("0");
    nServers=0;
    UserPath=new String("%UserPath%");
    SystemPath=new String("%MBS system path%");
    Command=new String("%Command%");
    Start=new String("startup.scom");
    Shut=new String("shutdown.scom");
}
protected void printForm(){
System.out.println(build().toString());
}
private StringBuffer build(){
    StringBuffer str=new StringBuffer();
    str.append(xXml.header());
    str.append(xXml.tag("MbsLaunch",xXml.OPEN));
    str.append("<MbsMaster "+xXml.attr("prompt","MBS Master")+xXml.attr("value",Master,"/>\n"));
    str.append("<MbsUserPath "+xXml.attr("prompt","MBS User path")+xXml.attr("value",UserPath,"/>\n"));
    str.append("<MbsSystemPath "+xXml.attr("prompt","MBS system path")+xXml.attr("value",SystemPath,"/>\n"));
    str.append("<MbsStartup "+xXml.attr("prompt","Startup script")+xXml.attr("value",Start,"/>\n"));
    str.append("<MbsShutdown "+xXml.attr("prompt","Shutdown script")+xXml.attr("value",Shut,"/>\n"));
    str.append("<MbsCommand "+xXml.attr("prompt","Script command")+xXml.attr("value",Command,"/>\n"));
    str.append("<MbsServers "+xXml.attr("prompt","%Number of required DIM servers%")+xXml.attr("value",Servers,"/>\n"));
    str.append("<Minimized "+xXml.attr("value",Shrink,"/>\n"));
    str.append(xXml.tag("MbsLaunch",xXml.CLOSE));
return str;
}
/**
 * Writes Xml file with filter setup values.
 * @param file Xml file name.
 */
protected void saveSetup(String file){
System.out.println("Store Mbs  launch setup to "+file);
    xXml.write(file,build().toString());
}
/**
 * Reads Xml file and restores filter setup values.
 * @param file Xml file name.
 */
protected void restoreSetup(String file){
NodeList li;
System.out.println("Restore Mbs  launch setup from "+file);
LaunchFile=new String(file);
    Element root=xXml.read(file);
    if(root != null){
        li=root.getElementsByTagName("MbsMaster");
        Master=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("MbsUserPath");
        UserPath=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("MbsSystemPath");
        SystemPath=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("MbsCommand");
        Command=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("MbsStartup");
        Start=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("MbsShutdown");
        Shut=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("MbsServers");
        Servers=((Element)li.item(0)).getAttribute("value");
        nServers=Integer.parseInt(Servers);// add DNS
        li=root.getElementsByTagName("Minimized");
        if(li.getLength() > 0)
        Shrink=new Boolean(((Element)li.item(0)).getAttribute("value")).booleanValue();
    }
}
public String getCommand(){return Command;}
protected void setCommand(String command){Command=command;}
public String getShut(){return Shut;}
protected void setShut(String shutdown){Shut=shutdown;}
public String getStart(){return Start;}
protected void setStart(String startup){Start=startup;}

}

