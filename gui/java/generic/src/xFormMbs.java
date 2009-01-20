package xgui;
import java.util.*;
import org.w3c.dom.*;
import javax.xml.parsers.*;
import java.awt.event.*;
/**
 * Base class to keep the data of the setup forms for MBS
 * Reads/writes XML setup file.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xFormMbs extends xForm {
private String Command;

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
    Master=new String("<MasterNode>");
    Servers=new String("0");
    UserPath=new String("<UserPath>");
    SystemPath=new String("<MBS system path>");
    Script=new String("script/remote_exe.sc");
    Command=new String("<Command>");
}
protected void printForm(){
System.out.println(build().toString());
}
private StringBuffer build(){
    StringBuffer str=new StringBuffer();
    str.append(xSet.XmlHeader());
    str.append(xSet.XmlTag("MbsLaunch",xSet.OPEN));
    str.append("<MbsMaster "+key("prompt","MBS Master")+key("value",Master)+"/>\n");
    str.append("<MbsUserPath "+key("prompt","MBS User path")+key("value",UserPath)+"/>\n");
    str.append("<MbsSystemPath "+key("prompt","MBS system path")+key("value",SystemPath)+"/>\n");
    str.append("<MbsScript "+key("prompt","MBS Script")+key("value",Script)+"/>\n");
    str.append("<MbsCommand "+key("prompt","Script command")+key("value",Command)+"/>\n");
    str.append("<MbsServers "+key("prompt","%Number of needed DIM servers%")+key("value",Servers)+"/>\n");
    str.append(xSet.XmlTag("MbsLaunch",xSet.CLOSE));
return str;
}
/**
 * Writes Xml file with filter setup values.
 * @param file Xml file name.
 */
protected void saveSetup(String file){
System.out.println("Mbs launch setup:  "+file);
    xSet.writeXml(file,build().toString());
}
/**
 * Reads Xml file and restores filter setup values.
 * @param file Xml file name.
 */
protected void restoreSetup(String file){
NodeList li;
System.out.println("Restore Mbs launch setup from  "+file);
LaunchFile=new String(file);
    Element root=xSet.readXml(file);
    if(root != null){
        li=root.getElementsByTagName("MbsMaster");
        Master=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("MbsUserPath");
        UserPath=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("MbsSystemPath");
        SystemPath=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("MbsScript");
        Script=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("MbsCommand");
        Command=((Element)li.item(0)).getAttribute("value");
        li=root.getElementsByTagName("MbsServers");
        Servers=((Element)li.item(0)).getAttribute("value");
        nServers=1+Integer.parseInt(Servers);// add DNS
    }
}
public String getCommand(){return Command;}
protected void setCommand(String command){Command=command;}

}

