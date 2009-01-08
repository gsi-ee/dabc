package xgui;
import java.util.*;
import java.io.*;
import java.awt.*;
/**
 * Remote shell execution.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xRemoteShell{

private String sCom;

public xRemoteShell(String shell){
Properties p = System.getProperties();
// String sOS      = new String(p.getProperty("os.name"));
// if(sOS.equals("Linux"))      sCom = new String("rsh ");
// if(sOS.equals("Windows NT")) sCom = new String("rsh ");
sCom = new String(shell+" ");
}
//********************************************************************
public boolean rsh(String node, String user, String cmd, long timeout)
{
  int ilen;
  int i;
  int ioff;
  int ipos;
  byte bRet[] = new byte[51200];
  byte b;
  String sT;
    String rCmd = new String(sCom+node+" -l "+user+" "+cmd);
    Runtime rtRsh = Runtime.getRuntime();
    xLogger.print(0,rCmd);
    System.out.println(sCom+node+" -l "+user+": ");
    System.out.println(cmd);
  try{
    Process Task = rtRsh.exec(rCmd);
    if(timeout > 0){
        System.out.print("Wait "+timeout+" sec.. ");
        try{Thread.sleep(timeout*1000);}catch(InterruptedException e){}
        System.out.println(" and bye!");
        Task.destroy();
        return true;
        }
    else try{Task.waitFor();}catch(InterruptedException e){}
    DataInputStream  fromTask  = new DataInputStream(Task.getInputStream());
    ioff=0;
    ilen=51200;
    i = fromTask.read(bRet);   // read line
    if(i > 0){
        while(i > 0){
          ioff=ioff+i;
          ilen=ilen-i;
          i = fromTask.read(bRet,ioff,ilen);   // read line
        }
        String s = new String(bRet,0,ioff);
        if(s.indexOf("TIME")>=0)System.out.println(s);
        try{
          StringTokenizer st = new StringTokenizer(s,"\n\b\r");
          int ii = st.countTokens();
          for(i=0;i<ii;i++) xLogger.print(0,st.nextToken());
        }
        catch(NoSuchElementException ee){System.out.println("EOF "+rCmd);}
    }
    Task.destroy();
	}
  catch(IOException e){System.out.println("Error executing "+rCmd); return false;}
return true;
}
//********************************************************************
public String rshout(String node, String user, String cmd)
{
  int ilen=51200;
  int i;
  int ioff=0;
  byte bRet[] = new byte[51200];
  byte b;
    String rCmd = new String(sCom+node+" -l "+user+" "+cmd);
    Runtime rtRsh = Runtime.getRuntime();
    xLogger.print(0,rCmd);
  try{
    Process Task = rtRsh.exec(rCmd);
    DataInputStream  fromTask  = new DataInputStream(Task.getInputStream());
    i = fromTask.read(bRet);   // read line
    if(i > 0){
        while(i > 0){
          ioff=ioff+i;
          ilen=ilen-i;
          i = fromTask.read(bRet,ioff,ilen);   // read line
        }
    }
    Task.destroy();
	}
  catch(IOException e){System.out.println("Error executing "+rCmd); return "";}
return new String(bRet,0,ioff);
}
//********************************************************************
public void dir(String node, String user, String cmd)
{
  int ilen;
  int i;
  int ioff;
  int ipos;
  byte bRet[] = new byte[512];
  byte b;
  String sT;
  try
  {
    String rCmd = new String(sCom+node+" -l "+user+" "+cmd);
    Runtime rtRsh = Runtime.getRuntime();
    Process Task = rtRsh.exec(rCmd);
    DataInputStream  fromTask = new DataInputStream(Task.getInputStream());
    ioff=0;
    ilen=512;
    i = fromTask.read(bRet);   // read line
    if(i > 0)
    {
    while(i > 0)
    {
      ioff=ioff+i;
      ilen=ilen-i;
      i = fromTask.read(bRet,ioff,ilen);   // read line
    }
    String s = new String(bRet,0,ioff);
    try{
      StringTokenizer st = new StringTokenizer(s,"\n\b\r");
      int ii = st.countTokens();
      for(i=0;i<ii;i++)
      {
        sT = new String(st.nextToken());
        ipos = sT.indexOf(".scom");
        if(ipos > 0)
        System.out.println(sT.substring(0,ipos));
      }
    }
    catch(NoSuchElementException ee){}
    Task.destroy();
    }
    else // retry because time out
    {
    Task.destroy();
	System.out.println("rsh dir timed out, retry!");
    Task = rtRsh.exec(rCmd);
    fromTask = new DataInputStream(Task.getInputStream());
    ioff=0;
    ilen=512;
    i = fromTask.read(bRet);   // read line
    while(i > 0)
    {
      ioff=ioff+i;
      ilen=ilen-i;
      i = fromTask.read(bRet,ioff,ilen);   // read line
    }
    String s = new String(bRet,0,ioff);
    try{
      StringTokenizer st = new StringTokenizer(s,"\n\b\r");
      int ii = st.countTokens();
      for(i=0;i<ii;i++)
      {
        sT = new String(st.nextToken());
        ipos = sT.indexOf(".scom");
        if(ipos > 0)
        System.out.println(sT.substring(0,ipos));
      }
    }
    catch(NoSuchElementException ee){}
    Task.destroy();
    }
  }
  catch(IOException e){System.err.println("Rshell.dir Error");}

}

}
