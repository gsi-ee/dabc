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
import javax.swing.JTextArea;
import java.lang.*;
import java.util.*;
import dim.*;

/**
 * InfoHandler to manage list of DIM servers. List is composed in a text area.
 * DIM parameter is DIS_DNS/SERVER_LIST.
 * @author Hans G. Essel
 * @version 1.0
 * @see xDesktop
 */
public class xDimNameInfo extends DimInfo implements Runnable {
private JTextArea lab;
private StringBuffer serverlist;
private String[] namelist;
private int nDataServers=0;
private xiDesktop desk;
private Vector<xDimServiceList> servicelist;

/**
 * Constructor of DIM parameter handler.
 * @param service DIM name of service: DIS_DNS/SERVER_LIST
 * @param label Text area to store the DIM server list. 
 */
public xDimNameInfo(String service, JTextArea label, xiDesktop desktop){
    super(service,"none");
    lab=label;
    desk=desktop;
    // at startup text area of label is filled with server name list
    // from browser.getServers();
    // in infoHandler only de/increments as coming from name server
    // are processed.
    serverlist=new StringBuffer(label.getText());
    servicelist = new Vector<xDimServiceList>(0);
    //System.out.println("____ "+serverlist.toString());
    }
public void run(){
}
/**
 * The DIM parameter DIS_DNS/SERVER_LIST is either a list, or incremental.
 * That means it may start with + to add a server, or with - to remove a server,
 * or a list of servers separated by |:<br>
 * +name@node -name@node name@node|name@node|name@node<br>
 * This handler only handles the increments. On startup the text area is
 * filled with the server list by xBrowser.getServers function.
 * @see xDimBrowser 
 */
public void infoHandler(){
    String str=getString();
    namelist=str.split("\\|"); // split around "|"
    //System.out.println("----- "+namelist.length+" "+str);
    if(namelist.length > 1) nDataServers=namelist.length-1; // exclude DNS
    boolean append=str.startsWith("+");
    boolean remove=str.startsWith("-");
    if(append||remove) {
	    String[] items=str.split("@");
	    str=items[0].substring(1); // skip leading sign
	    String sname = new String(str+"/SERVICE_LIST");
	    items=str.split("/");
	    if(items.length > 1)str=items[1]; // skip name part before /
	    int i = str.indexOf(".");
	    if(i >= 0)str=new String(" "+str.substring(0,i)); // skip trailing ...
	    else str=new String(" "+str);
	    if(append) {
	        serverlist.append(str);
	        boolean found=false;
	        for(i=0;i<servicelist.size();i++)
	        	if(servicelist.get(i).getName().equals(sname)){
	        		found=true;
	        		break;
	        	}
	        if(!found){
    		    xDimServiceList xl = new xDimServiceList(sname,desk);
    		    servicelist.add(xl);	        	
	        }
	        nDataServers++;
	        }
	    if(remove) {
	        nDataServers--;
	        int off=serverlist.indexOf(str);
	        if(off > 0) serverlist.replace(off,off+str.length(),"");
	    }
	    if(lab != null)lab.setText(serverlist.toString());
    } // append or remove
    else { // list of server names
    	for(int i=0;i<namelist.length;i++){
    	    if(!namelist[i].startsWith("DIS_DNS")){
    	    	//System.out.println(" ****** "+namelist[i]);
    		    String[] parts=namelist[i].split("@");
    		    String sname = new String(parts[0]+"/SERVICE_LIST");
    		    xDimServiceList xl = new xDimServiceList(sname,desk);
    		    servicelist.add(xl);
    	    }
    }}
xSet.setNofServers(nDataServers);
}
}
