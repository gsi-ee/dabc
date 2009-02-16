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
import java.util.Date;
import java.text.DateFormat;
import java.awt.Color;
/**
 * Central print function into logger window.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xLogger
{
private static String sSev[]={"U","S","I","W","E","F","L"};
private static Color cColBack  = new Color(0.0f,0.0f,0.0f);
private static Color cColSuccess  = new Color(1.0f,1.0f,1.0f);
private static Color cColWarning = new Color(0.0f,0.0f,0.0f);
private static Color cColError = new Color(1.0f,0.5f,0.5f);
private static xPanelLogger logpan;
private static DateFormat dateform=DateFormat.getDateTimeInstance();

/**
 * Creates a State canvas.
 */
public xLogger(){
}
public final static void print(int severity, String s){
Date date=new Date();
String logdate=new String(dateform.format(date));
if(logpan!=null){
logpan.print(logdate);
logpan.print(" |"+sSev[severity]+"| "+s+"\n");
}
else {
System.out.print(logdate);
System.out.println("|"+sSev[severity]+"| "+s);
}}
public final static void setLoggerPanel(xPanelLogger p){
logpan=p;
}
}
