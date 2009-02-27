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
/**
 * Dim record data for state.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xRecordState extends xRecord
{
private int Severity;
private String Value;
/**
 * DIM record object: Meter
 * @param name DABC format full parameter name
 * @param type Record type
 */
public xRecordState(String name, int type){
super(name,type);
}
public void setValue(int severity, String color, String value){
setColor(color);
Severity=severity;
Value=value;
}
public String getValue(){return Value;}
public int getSeverity(){return Severity;}
public String XmlLine(){
	String str = String.format("<State name=\"%s\" visible=\"%s\"/>\n",
	getName(),isVisible().toString());
	return str;}
public void restoreRecord(Element el){
	String att;
	    att=el.getAttribute("visible");
	    if(att.length()>0)setVisible(att);
	}
} // class xRecordState
