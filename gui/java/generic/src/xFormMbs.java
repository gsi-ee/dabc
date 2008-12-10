package xgui;
import java.util.*;
import org.w3c.dom.*;
import org.xml.sax.*;
import javax.xml.parsers.*;
import java.io.*;
import java.awt.event.*;
/**
 * Base class to keep the data of the setup forms for MBS
 * Reads/writes XML setup file.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xFormMbs extends xForm {
private String Command;
private DocumentBuilderFactory factory;
private DocumentBuilder builder;
private Document document;
private Element root, elem;

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
str.append("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
str.append("<MbsLaunch>\n");
str.append("<Labels\n");
// append prompt line specs
str.append("MbsMaster=\"MBS Master\"\n");
str.append("MbsUserPath=\"MBS User path\"\n");
str.append("MbsSystemPath=\"MBS system path\"\n");
str.append("MbsScript=\"MBS Script\"\n");
str.append("MbsCommand=\"Script command\"\n");
str.append("MbsServers=\"%Number of needed DIM servers%\"\n");
str.append("/>\n");
// append value specs
str.append("<Fields\n");
str.append("MbsMaster=\""+Master+"\"\n");
str.append("MbsUserPath=\""+UserPath+"\"\n");
str.append("MbsSystemPath=\""+SystemPath+"\"\n");
str.append("MbsScript=\""+Script+"\"\n");
str.append("MbsCommand=\""+Command+"\"\n");
str.append("MbsServers=\""+Servers+"\"\n");
str.append("/>\n");
str.append("</MbsLaunch>\n");
return str;
}
protected void saveSetup(String file){
try{
    FileWriter fw = new FileWriter(file);
    fw.write(build().toString());
    fw.close();
    System.out.println("Mbs launch setup: "+file);
}catch(IOException ioe){System.out.println("Error writing Mbs launch setup "+file);}
}

protected void restoreSetup(String file){
    LaunchFile=new String(file);
try{
    factory=DocumentBuilderFactory.newInstance();
    builder=factory.newDocumentBuilder();
    document=builder.parse(new File(file));
    root=document.getDocumentElement();
    elem=(Element)root.getElementsByTagName("Fields").item(0);
    Master=new String(elem.getAttribute("MbsMaster").toString());
    Servers=new String(elem.getAttribute("MbsServers").toString());
    UserPath=new String(elem.getAttribute("MbsUserPath").toString());
    SystemPath=new String(elem.getAttribute("MbsSystemPath").toString());
    Script=new String(elem.getAttribute("MbsScript").toString());
    Command=new String(elem.getAttribute("MbsCommand").toString());
    nServers=1+Integer.parseInt(Servers);// add DNS
}catch(Exception e){System.out.println("Error reading "+file);}
}
public String getCommand(){return Command;}
protected void setCommand(String command){Command=command;}

}

