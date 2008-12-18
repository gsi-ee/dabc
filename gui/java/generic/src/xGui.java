package xgui;
import java.lang.*;
import javax.swing.*;
import java.awt.event.*;
import java.awt.*;
import java.util.*;
import java.io.*;
import dim.*;
import java.text.NumberFormat;
import java.beans.PropertyVetoException;
 
/**
 * Main class. Creates desktop.<br>
 * Optionally a switch -m may be passed to indicate that no control panels will be shown (monitoring mode).<br>
 * A class name (class must implement interface xiUserPanel) may optionally be specified which is instantiated
 * and the object is passed as xiUserPanel to the created desktop.
 * @see xDesktop
 * @author Hans G. Essel
 * @version 1.0
 */
public class xGui {

private static xiUserPanel     usrpan;
private static boolean enableControl=true;

// private PsActionSupport actionHandler;
public xGui() {
/**
 * Create the desktop xDesktop and show it. For thread safety,
 * this method is invoked from the
 * event-dispatching thread.
 */
 }
private static void createAndShowGUI() {
xDesktop frame;
//Make sure we have nice window decorations.
    JFrame.setDefaultLookAndFeelDecorated(true);

//Create and set up the window.
    frame = new xDesktop(usrpan,enableControl);
    frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

//Display the window.
    frame.setVisible(true);
}

/**
 * Main entry. Checks for DIM_DNS_NODE and application class argument,
 * then starts event-dispatching thread.
 * Sets default Locale to "en" and "US". 
 * @param args optional -m for monitoring only or optional class name of user panel.
 */
public static void main(String[] args) {
int i=0;
Locale.setDefault(new Locale("en","US"));
System.out.println("Using Java: "+System.getProperties().getProperty("java.version"));
System.out.println("From      : "+System.getProperties().getProperty("java.home"));

// Without DIM name server nothing can go
if(System.getenv("DIM_DNS_NODE")==null){
System.out.println("No DIM name server defined, set DIM_DNS_NODE!");
return;
}

if(args.length > i){
    if(args[i].startsWith("-m")){
        enableControl=false;
        i++;
        }
}
if(args.length > i){
    try{
     usrpan = (xiUserPanel) Class.forName(args[i]).newInstance();
    }   catch(ClassNotFoundException ee){System.out.println("NotFound: Error creating "+args[i]);}
        catch(InstantiationException x){System.out.println("Instant: Error creating "+args[i]);}
        catch(IllegalAccessException xx){System.out.println("IllAccess: Error creating "+args[i]);}
    }
javax.swing.SwingUtilities.invokeLater(
new Runnable() {
public void run() {createAndShowGUI();}
});
}
}
