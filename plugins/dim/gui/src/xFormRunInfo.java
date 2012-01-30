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
public class xFormRunInfo extends xForm {
	private String lrun;
	private String lexperiment;
	private String lcomment;

public xFormRunInfo() {
setDefaults();
}
public xFormRunInfo(String file) {
setDefaults();
restoreSetup(file);
}
public xFormRunInfo(String file, ActionListener action) {
super(action);
setDefaults();
restoreSetup(file);
}

protected void setDefaults(){
    lrun=new String("%run%");
    lexperiment=new String("%experiment%");
    lcomment=new String("%comment%");
}
protected void printForm(){
System.out.println(build().toString());
}
/**
 * Reads Xml file and restores run info setup values.
 * @param file Xml file name.
 */
public void restoreSetup(String file){
NodeList li;
Element el;
String att;
System.out.println("Restore run information from "+file);
    Element root=xXml.read(file);
    if(root != null){
        li=root.getElementsByTagName("Run");
        lrun=((Element)li.item(0)).getAttribute("val");
        li=root.getElementsByTagName("Experiment");
        lexperiment=((Element)li.item(0)).getAttribute("val");
        li=root.getElementsByTagName("Comment");
        lcomment=((Element)li.item(0)).getAttribute("val");
    }
}
/**
 * Writes Xml string with run information values.
 */
public StringBuffer build(){
    StringBuffer str=new StringBuffer();
    str.append(xXml.header());
    str.append(xXml.tag("RunInfo",xXml.OPEN));
    str.append("<Run "+xXml.attr("val",lrun,"/>\n"));
    str.append("<Experiment "+xXml.attr("val",lexperiment,"/>\n"));
    str.append("<Comment "+xXml.attr("val",lcomment,"/>\n"));
    str.append(xXml.tag("RunInfo",xXml.CLOSE));
return str;
}
/**
 * Writes Xml file with run information values.
 * @param file Xml file name.
 */
public void saveSetup(String file){
System.out.println("Store run information to "+file);
    xXml.write(file,build().toString());
}
public String getRun(){return lrun;}
public void setRun(String run){lrun=run;}
protected void setExperiment(String experiment){lexperiment=experiment;}
protected String getExperiment(){return lexperiment;}
public String getComment(){return lcomment;}
protected void setComment(String comment){lcomment=comment;}

}

