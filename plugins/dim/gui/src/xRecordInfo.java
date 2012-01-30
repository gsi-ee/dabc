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
/**
 * Dim record data for info.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xRecordInfo extends xRecord
{
private int Severity;
private String Value;
/**
 * DIM record object: Info
 * @param name DABC format full parameter name
 * @param type Record type
 */
public xRecordInfo(String name, int type){
super(name,type);
}
/**
 * DIM record object: Info
 * @param severity
 * @param color color of text.
 * @param value Text
 */
public void setValue(int severity, String color, String value){
setColor(color);
Severity=severity;
Value=value;
}
public String getValue(){return Value;}
public int getSeverity(){return Severity;}
} // class xRecordState
