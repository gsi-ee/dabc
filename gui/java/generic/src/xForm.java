package xgui;
/*
This client 
*/

/**
* @author goofy
*/

import java.util.*;
import java.awt.event.*;

/**
* DIM GUI class
*/
public class xForm {
protected String Master;
protected String Servers;
protected int nServers;
protected String UserPath;
protected String SystemPath;
protected String Script;
protected String LaunchFile;
protected ActionListener action;

public xForm() {}
public xForm(ActionListener a) {action=a;}
public String getLaunchFile(){return LaunchFile;}
public String getMaster(){return Master;}
public String getServers(){return Servers;}
public String getUserPath(){return UserPath;}
public String getSystemPath(){return SystemPath;}
public String getScript(){return Script;}
public void setLaunchFile(String file){LaunchFile=file;}
public void setMaster(String master){Master=master;}
public void setServers(String servers){Servers=servers;}
public void setUserPath(String userpath){UserPath=userpath;}
public void setSystemPath(String systempath){SystemPath=systempath;}
public void setScript(String script){Script=script;}
public void addActionListener(ActionListener ae){action=ae;}
public ActionListener getActionListener(){return action;}

}

