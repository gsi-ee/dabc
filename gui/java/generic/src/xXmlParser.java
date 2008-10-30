package xgui;

import org.w3c.dom.*;
import org.xml.sax.*;
import javax.xml.parsers.*;
import java.io.*;

public class xXmlParser{
private DocumentBuilderFactory factory;
private DocumentBuilder builder;
private Document document;
private Element root;
private NodeList args;
private String cname,xml;
private StringBuffer str;
private boolean finalized=false;
private boolean domcreated=false;
private boolean ischanged=false;

public xXmlParser(){
}
/**
 * Starts to build internal XML string buffer.
 * @param command name of the command
 */
public void newCommand(String command, boolean header){
finalized=false;
str=new StringBuffer();
if(header)str.append("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
str.append(String.format("<command name=\"%s\" scope=\"*\" content=\"default\">\n",command));
}
/**
 * Starts to build internal XML string buffer.
 * @param command name of the command
 * @param scope scope of command, could be like public, hidden, ...
 */
public void newCommand(String command, boolean header, String scope){
finalized=false;
str=new StringBuffer();
if(header)str.append("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
str.append(String.format("<command name=\"%s\" scope=\"%s\" content=\"default\">\n",command,scope));
}
/**
 * Starts to build internal XML string buffer.
 * @param command name of the command
 * @param scope scope of command, could be like public, hidden, ...
 * @param changed false for default values, true for changed values
 */
public void newCommand(String command, boolean header, String scope, boolean changed){
finalized=false;
ischanged=changed;
str=new StringBuffer();
if(header)str.append("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
if(changed)str.append(String.format("<command name=\"%s\" scope=\"%s\" content=\"values\">\n",command,scope));
else       str.append(String.format("<command name=\"%s\" scope=\"%s\" content=\"default\">\n",command,scope));
}
/**
 * Adds argument description to internal XML string buffer.
 * @param argument name of argument
 * @param type type of argument (I,F,D,C)
 * @param value value of argument. In command definition would be the default.
 * @param required specifies if argument is required. Stored as string "true" or "false".
 */
public void addArgument(String argument, String type, String value, String required){
if(!finalized) 
str.append(String.format("<argument name=\"%s\" type=\"%s\" value=\"%s\" required=\"%s\"/>\n",argument,type,value,required));
}
/**
 * @return finalized internal XML string buffer.
 */
public String getXmlString(){
if(str==null)return xml;
if(!finalized) str.append("</command>\n");
finalized=true;
return str.toString();
}
public void saveXmlString(String file, String xml){
    try{
        FileWriter fw = new FileWriter(file);
        fw.write(xml);
        fw.close();
    }catch(IOException ioe){System.out.println("Error writing xml file "+file);}
}
/**
 * @param xmlstring encoded XML string. Must be called before the getter methods.
 */
public void parseXmlString(String name, String xmlstring){
try{
factory=DocumentBuilderFactory.newInstance();
builder=factory.newDocumentBuilder();
document=builder.parse(new InputSource(new StringReader(xmlstring)));
root=document.getDocumentElement();
args=root.getElementsByTagName("argument");
}catch(Exception e){System.out.println(name);e.printStackTrace();}
domcreated=true;
cname=name;
xml=xmlstring;
}
/**
 * @return component name
 */
public boolean isChanged(){return ischanged                                                                                                                        ;}
/**
 * @return component name
 */
public String getName(){return cname;}
/**
 * @return command name
 */
public String getCommandName(){return root.getAttribute("name");}
/**
 * @return command scope
 */
public String getCommandScope(){return root.getAttribute("scope");}
/**
 * @return command scope
 */
public String getCommandContent(){return root.getAttribute("content");}
/**
 * @return number of arguments
 */
public int getNargs(){return args.getLength();}
/**
 * @return argument name
 */
public String getArgumentName(int index){
return ((Element)args.item(index)).getAttribute("name").toString();}
/**
 * @return argument type (I,F,D,C)
 */
public String getArgumentType(int index){
return ((Element)args.item(index)).getAttribute("type").toString().toUpperCase();}
/**
 * @return argument value
 */
public String getArgumentValue(int index){
return ((Element)args.item(index)).getAttribute("value").toString();}
/**
 * @return argument requirement (false or true)
 */
public String getArgumentRequired(int index){
return ((Element)args.item(index)).getAttribute("required").toString().toUpperCase();}
}
