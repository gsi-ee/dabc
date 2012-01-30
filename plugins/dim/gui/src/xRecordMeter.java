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
 * Dim record data for meter.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xRecordMeter extends xRecord
{
private int Mode;
private double Value;
private double Low;
private double Up;
private double AlarmLow;
private double AlarmUp;
private String AlarmColor;
private String Units;
private Boolean Auto;
private Boolean Log;
/**
 * DIM record object: Meter
 * @param name DABC format full parameter name
 * @param type Record type
 * @param mode Display mode
 * @param lower lower limit
 * @param upper upper limit
 * @param alarmLower alarm lower limit
 * @param alarmUpper alarm upper limit
 * @param color color of pointer
 * @param alarmColor color of frame in alarm
 * @param units units
 */
public xRecordMeter(String name, int type, int mode,
        double lower, double upper, double alarmLower, double alarmUpper, 
        String color, String alarmColor, String units){
        super(name,type);
    AlarmLow=alarmLower;
    AlarmUp=alarmUpper;
    Mode=mode;
    setColor(color);
    setLower(lower);
    setUpper(upper);
    if(lower>=upper)Auto=new Boolean(true);
    else Auto=new Boolean(false);
    Units = new String(units);
}
public void setValue(double value){Value=value;}
public void setLower(double low){Low=low;}
public void setUpper(double up){Up=up;}
public void setMode(int mode){Mode=mode;}
public void setAutoScale(Boolean auto){Auto=new Boolean(auto);}
public Boolean getAutoScale(){return Auto;}
public Boolean getLogScale(){return Log;}
public void setLogScale(Boolean log){Log=new Boolean(log);}
public int getMode(){return Mode;}
public double getValue(){return Value;}
public double getLower(){return Low;}
public double getUpper(){return Up;}
public String getUnits(){return Units;}

public String XmlLine(){
String mode="";
     if(Mode == xMeter.ARC)  mode="ARC";
else if(Mode == xMeter.BAR)  mode="BAR";
else if(Mode == xMeter.TREND)mode="TREND";
else if(Mode == xMeter.STAT) mode="STAT";
String str = String.format("<Meter name=\"%s\" visible=\"%s\" mode=\"%s\" auto=\"%s\" log=\"%s\" low=\"%010.1f\" up=\"%010.1f\" color=\"%s\"/>\n",
getName(),isVisible().toString(),mode,getAutoScale().toString(),getLogScale().toString(),getLower(),getUpper(),getColor());
return str;
}
public void restoreRecord(Element el){
String att;
    // setName(el.getAttribute("name").toString());
    att=el.getAttribute("mode");
    if(att.length()>0){
    	if(att.equals("ARC"))       Mode=xMeter.ARC;
    	else if(att.equals("BAR"))  Mode=xMeter.BAR;
    	else if(att.equals("TREND"))Mode=xMeter.TREND;
    	else if(att.equals("STAT")) Mode=xMeter.STAT;
    }
    att=el.getAttribute("low");
    if(att.length()>0)Low=Double.parseDouble(att);
    att=el.getAttribute("up");
    if(att.length()>0)Up=Double.parseDouble(att);
    att=el.getAttribute("auto");
    if(att.length()>0)Auto=new Boolean(att);
    att=el.getAttribute("log");
    if(att.length()>0)Log=new Boolean(att);
    att=el.getAttribute("visible");
    if(att.length()>0)setVisible(att);
    att=el.getAttribute("color");
    if(att.length()>0)setColor(att);
//    printRecord();
}
public void printRecord(){
System.out.println(getType()+" "+getName()+" "+getColor()+" "+getMode()+" "+getLower()+" "+getUpper()+" "+getUnits());
}

} // class xRecordMeter
