package xgui;
import java.util.Date;
import java.text.DateFormat;
import java.awt.Color;

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
 * @param head name of parameter displayed
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
