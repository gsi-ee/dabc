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
import java.lang.*;
import javax.swing.*;
import java.awt.event.*;
import java.awt.*;
import java.util.*;
import java.io.*;
import dim.*;
import java.text.NumberFormat;
import java.beans.PropertyVetoException;
 
//------------------------------------------------------
public class testclient {
	private String last;
	private int count, lines;
//------------------------------------------------------
private class dimpar extends DimInfo{

private String color;
private String value;
private int quality, severity, iNoLink;
private String lname;
private String format;
private String sNolink;

public dimpar(String name, int noLink){
	    super(name,noLink);
	    iNoLink=noLink;
}
public void infoHandler(){
	try{
		quality=super.getQuality();
	    lname=super.getName();
	    format=super.getFormat();
		if(format == null) System.out.println("ERROR: no format for "+lname);
	} catch (NullPointerException e){
	    System.out.println("ERROR: NULL "+lname+" qual "+quality);
	    return;
	}
	int severity=getInt();
	if(severity == iNoLink){
	    value=sNolink;
	} else {
	    color=new String(getString());
	    value=new String(getString());
	}
	if(value.equals(last)) count++;
	else {
		if(count < 4)System.out.println("ERROR missing states! "+(count+1)+" of 5");
		count=0;
	}
	System.out.print(count+".");
	//System.out.println("j> "+lname+" "+value);
	last=new String(value);
} // handler
} // dimpar
//------------------------------------------------------
public testclient() { 
	String serv[]=DimBrowser.getServices("*Status");
	for(int i=0;i<serv.length;i++){
		System.out.println("Register "+serv[i]);
		dimpar st1=new dimpar(serv[i],-1);
	}
}
//------------------------------------------------------
public static void main(String[] args) {
int i=0, num=0;
Locale.setDefault(new Locale("en","US"));
System.out.println("Using Java: "+System.getProperties().getProperty("java.version"));
System.out.println("From      : "+System.getProperties().getProperty("java.home"));

// Without DIM name server nothing can go
if(System.getenv("DIM_DNS_NODE")==null){
	System.out.println("No DIM name server defined, set DIM_DNS_NODE!");
	return;
	}
testclient xx = new testclient();
if(args.length > 0){ // send commands
	String pref=new String("DABC/"+args[0]+":0/Controller/Do");
	while(true){
		DimTimer.sleep(5);
		num++;
		System.out.println(num);
		System.out.println("j> "+pref+"Configure ...");
		DimClient.sendCommand(pref+"Configure","x1gSFfpv0JvDA");
		DimTimer.sleep(5);
		System.out.println("j> "+pref+"Enable ...");
		DimClient.sendCommand(pref+"Enable","x1gSFfpv0JvDA");
		DimTimer.sleep(5);
		System.out.println("j> "+pref+"Start ...");
		DimClient.sendCommand(pref+"Start","x1gSFfpv0JvDA");
		DimTimer.sleep(5);
		System.out.println("j> "+pref+"Stop ...");
		DimClient.sendCommand(pref+"Stop","x1gSFfpv0JvDA");
		DimTimer.sleep(5);
		System.out.println("j> "+pref+"Halt ...");
		DimClient.sendCommand(pref+"Halt","x1gSFfpv0JvDA");
	}
} else while(true) DimTimer.sleep(5);
	
}//main
}
