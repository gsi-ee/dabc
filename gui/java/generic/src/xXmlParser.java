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

import org.w3c.dom.*;
import java.io.*;

/**
 * Parser for XML formatted command descriptions.
 * @author H.G.Essel
 * @version 1.0
*/
public class xXmlParser{
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
newCommand(command, header, "*",false);
}
/**
 * Starts to build internal XML string buffer.
 * <br> command name="" scope="" content="default"
 * @param command Name of the command
 * @param header True: write XML header line (version, encoding).
 * @param scope Scope of command, could be like public, hidden, MBS ...
 */
public void newCommand(String command, boolean header, String scope){
newCommand(command, header, scope, false);
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
String content;
if(changed) content = new String("values");
else        content = new String("default");
xml=null;
finalized=false;
ischanged=changed;
str=new StringBuffer();
if(header)str.append(xXml.header());
str.append("<command "+
		xXml.attr("name",command)+
		xXml.attr("scope",scope)+
		xXml.attr("content",content,">\n"));
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
str.append("<argument "+
		xXml.attr("name",argument)+
		xXml.attr("type",type)+
		xXml.attr("value",value)+
		xXml.attr("required",required,"/>\n"));
}
/**
 * @return Finalized internal XML string buffer, or XML string read by parseXmlString.
 */
public String getXmlString(){
if(str==null)return xml;
if(!finalized) str.append(xXml.tag("command",xXml.CLOSE));
finalized=true;
return str.toString();
}
/**
 * Save string to file.
 * @param file File name.
 * @param xml XML string.
 */
public void saveXmlString(String file, String xml){
xXml.write(file,xml);
}
/**
 * Read and parse XML string. String is stored. Must be called before the getter methods.
 * @param name Just a name returned by getName.
 * @param xmlstring Encoded XML string.
 */
public void parseXmlString(String name, String xmlstring){
root=xXml.string(xmlstring);
args=root.getElementsByTagName("argument");
domcreated=true;
cname=new String(name);
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
