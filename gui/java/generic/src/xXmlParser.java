package xgui;

import org.w3c.dom.*;
import org.xml.sax.*;
import javax.xml.parsers.*;
import java.io.*;

/**
 * @author H.G.Essel
 * @version 1.0
*/
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

/**
 * The XML parser can be used to build a command description string.
 * Or it can parse a given XML string and return the fields.
 */
public xXmlParser(){
}
/**
 * Starts to build internal XML string buffer.
 * <br> command name="" scope="" content="default"
 * @param command Name of the command
 * @param header True: write XML header line (version, encoding).
 */
public void newCommand(String command, boolean header){
xml=null;
finalized=false;
str=new StringBuffer();
if(header)str.append("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
str.append(String.format("<command name=\"%s\" scope=\"*\" content=\"default\">\n",command));
}
/**
 * Starts to build internal XML string buffer.
 * <br> command name="" scope="" content="default"
 * @param command Name of the command
 * @param header True: write XML header line (version, encoding).
 * @param scope Scope of command, could be like public, hidden, MBS ...
 */
public void newCommand(String command, boolean header, String scope){
xml=null;
finalized=false;
str=new StringBuffer();
if(header)str.append("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
str.append(String.format("<command name=\"%s\" scope=\"%s\" content=\"default\">\n",command,scope));
}
/**
 * Starts to build internal XML string buffer.
 * <br> command name="" scope="" content="default"
 * @param command Name of the command
 * @param header True: write XML header line (version, encoding).
 * @param scope Scope of command, could be like public, hidden, ...
 * @param changed False: content="default", true: content="values"
 */
public void newCommand(String command, boolean header, String scope, boolean changed){
xml=null;
finalized=false;
ischanged=changed;
str=new StringBuffer();
if(header)str.append("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
if(changed)str.append(String.format("<command name=\"%s\" scope=\"%s\" content=\"values\">\n",command,scope));
else       str.append(String.format("<command name=\"%s\" scope=\"%s\" content=\"default\">\n",command,scope));
}
/**
 * Adds argument description to internal XML string buffer.
 * <br> argument name="" type="" value="" required=""
 * @param argument Name of argument
 * @param type Type of argument (I,F,D,C)
 * @param value Value of argument. In command definition would be the default.
 * @param required Specifies if argument is required. Stored as string "true" or "false".
 */
public void addArgument(String argument, String type, String value, String required){
if(!finalized) 
str.append(String.format("<argument name=\"%s\" type=\"%s\" value=\"%s\" required=\"%s\"/>\n",argument,type,value,required));
}
/**
 * @return Finalized internal XML string buffer, or XML string read by parseXmlString.
 */
public String getXmlString(){
if(str==null)return xml;
if(!finalized) str.append("</command>\n");
finalized=true;
return str.toString();
}
/**
 * Save string to file.
 * @param file File name.
 * @param xml XML string.
 */
public void saveXmlString(String file, String xml){
    try{
        FileWriter fw = new FileWriter(file);
        fw.write(xml);
        fw.close();
    }catch(IOException ioe){System.out.println("Error writing xml file "+file);}
}
/**
 * Read and parse XML string. String is stored. Must be called before the getter methods.
 * @param name Just a name returned by getName.
 * @param xmlstring Encoded XML string.
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
str=null;
}
/**
 * @return True: XML string contains values of arguments.
 */
public boolean isChanged(){return ischanged;}
/**
 * @return Component name
 */
public String getName(){return cname;}
/**
 * @return Command name field
 */
public String getCommandName(){return root.getAttribute("name");}
/**
 * @return Command scope field
 */
public String getCommandScope(){return root.getAttribute("scope");}
/**
 * @return Command content field
 */
public String getCommandContent(){return root.getAttribute("content");}
/**
 * @return Number of arguments
 */
public int getNargs(){return args.getLength();}
/**
 * @param index Argument index.
 * @return Argument name field
 */
public String getArgumentName(int index){
return ((Element)args.item(index)).getAttribute("name").toString();}
/**
 * @param index Argument index.
 * @return Argument type  field(I,F,D,C)
 */
public String getArgumentType(int index){
return ((Element)args.item(index)).getAttribute("type").toString().toUpperCase();}
/**
 * @param index Argument index.
 * @return Argument value field
 */
public String getArgumentValue(int index){
return ((Element)args.item(index)).getAttribute("value").toString();}
/**
 * @param index Argument index.
 * @return Argument required field (FALSE or TRUE)
 */
public String getArgumentRequired(int index){
return ((Element)args.item(index)).getAttribute("required").toString().toUpperCase();}
}
