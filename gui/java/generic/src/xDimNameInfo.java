package xgui;
/**
* @author goofy
*/
import javax.swing.JTextArea;
import java.util.*;
import dim.*;

public class xDimNameInfo extends DimInfo {
private JTextArea lab;
private StringBuffer serverlist;

xDimNameInfo(String service, JTextArea label){
    super(service,"none");
    lab=label;
    // at startup text area of label is filled with server name list
    // from browser.getServers();
    // in infoHandler only de/increments as coming from name server
    // are processed.
    serverlist=new StringBuffer(label.getText());
}
public void infoHandler(){
    String[] name;
    String str=getString();
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
    }
    if(remove) {
        int off=serverlist.indexOf(str);
        if(off > 0) serverlist.replace(off,off+str.length(),"");
    }
    if((append||remove)&&(lab != null))lab.setText(serverlist.toString());
}
}
