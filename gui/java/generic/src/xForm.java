package xgui;
import java.util.*;
import java.awt.event.*;
/**
 * Base class to keep the data of the setup forms for MBS and DABC
 * @author Hans G. Essel
 * @version 1.0
 */
public class xForm {
// because these fields must be accessible in subclasses
// they cannot be private
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
protected void setLaunchFile(String file){LaunchFile=file;}
protected void setMaster(String master){Master=master;}
protected void setServers(String servers){Servers=servers;}
protected void setUserPath(String userpath){UserPath=userpath;}
protected void setSystemPath(String systempath){SystemPath=systempath;}
protected void setScript(String script){Script=script;}
protected void addActionListener(ActionListener ae){action=ae;}
public ActionListener getActionListener(){return action;}

}

