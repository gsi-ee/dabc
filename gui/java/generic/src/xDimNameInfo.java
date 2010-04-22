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
import java.util.*;
import dim.*;

/**
 * InfoHandler to manage list of DIM servers. List is composed in a text area.
 * DIM parameter is DIS_DNS/SERVER_LIST.
 * @author Hans G. Essel
 * @version 1.0
 * @see xDesktop
 */
public class xDimNameInfo extends DimInfo {
private JTextArea lab;
private StringBuffer serverlist;
private int nDataServers=0;

/**
 * Constructor of DIM parameter handler.
 * @param service DIM name of service: DIS_DNS/SERVER_LIST
 * @param label Text area to store the DIM server list. 
 */
public xDimNameInfo(String service, JTextArea label){
    super(service,"none");
    lab=label;
    // at startup text area of label is filled with server name list
    // from browser.getServers();
    // in infoHandler only de/increments as coming from name server
    // are processed.
    serverlist=new StringBuffer(label.getText());
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
    String[] name;
    String str=getString();
    //System.out.println("-----"+str);
    name=str.split("\\|"); // split around "|"
    if(name.length > 1)nDataServers=name.length-1; // exclude DNS
    boolean append=str.startsWith("+");
    boolean remove=str.startsWith("-");
    String[] items=str.split("@");
    str=items[0].substring(1); // skip leading sign
    items=str.split("/");
    if(items.length > 1)str=items[1]; // no leading sign
    int i = str.indexOf(".");
    if(i >= 0)str=new String(" "+str.substring(0,i));
    else str=new String(" "+str);
    if(append) {
        serverlist.append(str);
        nDataServers++;
        }
    if(remove) {
        nDataServers--;
        int off=serverlist.indexOf(str);
        if(off > 0) serverlist.replace(off,off+str.length(),"");
    }
    if((append||remove)&&(lab != null))lab.setText(serverlist.toString());
xSet.setNofServers(nDataServers);
}
}
