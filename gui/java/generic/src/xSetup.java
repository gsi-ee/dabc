package xgui;

import java.util.*;
import java.io.*;
import org.w3c.dom.*;
import org.xml.sax.*;
import javax.xml.transform.*;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import javax.xml.parsers.*;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Point;
import javax.swing.ImageIcon;

public class xSetup
{
private Vector<String> names;
private Vector<String> values;
private Vector<String> types;
private NodeList Contexts;
private int NoContexts;
private Document document;
private Element partition;
private String typeInt=new String("int");
private String typeChar=new String("char");
private String typeFloat=new String("float");
private String typeDouble=new String("double");
private String typeUInt=new String("Uint");


public xSetup(){}

// Read file
public boolean parseSetup(String file){
int i,ii,iii,iiii;
boolean show;
String ty;
Element Context,Application,Property,elem;
NodeList Applications,Properties,Parameters,Modules;

try{
    DocumentBuilderFactory factory=DocumentBuilderFactory.newInstance();
    DocumentBuilder builder=factory.newDocumentBuilder();
    document=builder.parse(new File(file));
    partition=document.getDocumentElement();
    partition.normalize();
}catch(Exception e){partition=null;}

if(partition == null) return false;

names=new Vector<String>(0);
values=new Vector<String>(0);
types=new Vector<String>(0);

// loop over contexts (nodes)
Contexts = partition.getElementsByTagName("xc:Context");
NoContexts=Contexts.getLength();
for(i = 0;i<NoContexts;i++){
    Context=(Element)Contexts.item(i); 
    //Context.getAttributeNode("url").setValue("fake url"); // test, if this works
    names.add("Context");
    values.add(Context.getAttribute("url"));
    types.add("char");
    // Assume we have only one application in context
    Applications=Context.getElementsByTagName("xc:Application"); 
    Application=(Element)Applications.item(0);
    names.add("Application");
    values.add(Application.getAttribute("class"));
    types.add("char");
    names.add("Application ID");
    values.add(Application.getAttribute("id"));
    types.add("int");
    names.add("Application instance");
    values.add(Application.getAttribute("instance"));
    types.add("int");
    // An application has one list of properties
    Properties=Application.getElementsByTagName("properties"); 
    // Add first one to the list before the module list
    Property=(Element)Properties.item(0);
    names.add("Property");
    values.add(Property.getAttribute("xmlns"));
    types.add("char");
    // get list of modules
    Modules=Context.getElementsByTagName("xc:Module"); 
    for(ii=0;ii<Modules.getLength();ii++){
        elem=(Element)Modules.item(ii);
        //elem.getFirstChild().setNodeValue("mod "+ii);
        names.add("Module");
        values.add(elem.getFirstChild().getNodeValue());
        types.add("char");
        }
    // get list of properties
    Parameters=Property.getElementsByTagName("*"); 
    for(iiii=0;iiii<Parameters.getLength();iiii++){
        elem=(Element)Parameters.item(iiii);
        ty=elem.getAttribute("xsi:type");
        names.add(elem.getTagName());
        if(elem.getFirstChild()==null) values.add("");
        else if(elem.getFirstChild().getNodeValue()==null) values.add("");
        else values.add(elem.getFirstChild().getNodeValue());
        if(ty.equals("xsd:integer"))types.add(typeInt);
        else if(ty.equals("xsd:unsignedInt"))types.add(typeUInt);
        else if(ty.equals("xsd:float"))types.add(typeFloat);
        else if(ty.equals("xsd:double"))types.add(typeDouble);
        else if(ty.equals("xsd:string"))types.add(typeChar);
    }
}
return true;
}

// Replace new values
public boolean updateSetup(){
int i,ii,iii,iiii,iv;
boolean show;
String ty;
Element Context,Application,Property,elem;
NodeList Applications,Properties,Parameters,Modules;

if(Contexts == null) return false;

iv=0;
for(i = 0;i<NoContexts;i++){
    Context=(Element)Contexts.item(i); 
    if(!Context.getAttribute("url").equals(values.get(iv)))
        Context.getAttributeNode("url").setValue(values.get(iv));
    iv++;
    // Assume we have only one application in context
    Applications=Context.getElementsByTagName("xc:Application"); 
    Application=(Element)Applications.item(0);
    if(!Application.getAttribute("class").equals(values.get(iv)))
        Application.getAttributeNode("class").setValue(values.get(iv));
    iv++;
    if(!Application.getAttribute("id").equals(values.get(iv)))
        Application.getAttributeNode("id").setValue(values.get(iv));
    iv++;
    if(!Application.getAttribute("instance").equals(values.get(iv)))
        Application.getAttributeNode("instance").setValue(values.get(iv));
    iv++;
    // An application has one list of properties
    Properties=Application.getElementsByTagName("properties"); 
    // Add first one to the list before the module list
    Property=(Element)Properties.item(0);
    if(!Property.getAttribute("xmlns").equals(values.get(iv)))
        Property.getAttributeNode("xmlns").setValue(values.get(iv));
    iv++;
    // get list of modules
    Modules=Context.getElementsByTagName("xc:Module"); 
    for(ii=0;ii<Modules.getLength();ii++){
        elem=(Element)Modules.item(ii);
        //elem.getFirstChild().setNodeValue("mod "+ii);
        if(!elem.getFirstChild().getNodeValue().equals(values.get(iv)))
            elem.getFirstChild().setNodeValue(values.get(iv));
        iv++;
        }
    // get list of properties
    Parameters=Property.getElementsByTagName("*"); 
    for(iiii=0;iiii<Parameters.getLength();iiii++){
        elem=(Element)Parameters.item(iiii);
        if(elem.getFirstChild()!=null){ if(elem.getFirstChild().getNodeValue()!=null){
        if(!elem.getFirstChild().getNodeValue().equals(values.get(iv)))
            elem.getFirstChild().setNodeValue(values.get(iv));}}
        iv++;
    }
}
return true;
}

// Write file
public boolean writeSetup(String filename){
// This method writes a DOM document to a file
boolean retOK=true;
try {
    // Prepare the DOM document for writing
    Source source = new DOMSource(document);
    // Prepare the output file
    File file = new File(filename);
    Result result = new StreamResult(file);
    // Write the DOM document to the file
    Transformer xformer = TransformerFactory.newInstance().newTransformer();
    xformer.transform(source, result);
} catch (TransformerConfigurationException e) { retOK=false;
} catch (TransformerException e) { retOK=false;
}
return retOK;
}


public void printSetup(){
for(int i=0;i<names.size();i++)
System.out.println(names.get(i)+" ["+types.get(i)+"] = "+values.get(i));
}

public int getContextNumber(){return NoContexts;}
public NodeList getContexts(){return Contexts;}
public Vector<String> getNames(){return names;}
public Vector<String> getValues(){return values;}
public Vector<String> getTypes(){return types;}


}
