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
    frame = new xDesktop();
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

// parse arguments 
xSet.checkMainArgs(args);

javax.swing.SwingUtilities.invokeLater(
new Runnable() {
public void run() {createAndShowGUI();}
});
}
}
