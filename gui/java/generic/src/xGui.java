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
private static xMeter xprogmet;
private static JFrame proframe;
private static int iprogress;
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
	    frame = new xDesktop(xprogmet, proframe);
	    frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
	    javax.swing.SwingUtilities.invokeLater(frame);
	//Display the window.
//	    frame.setVisible(true);
	}
private static void createAndShowProgress() {
	//Make sure we have nice window decorations.
	    JFrame.setDefaultLookAndFeelDecorated(true);

	//Create and set up the window.
	    ImageIcon workIcon = xSet.getIcon("icons/control.png");
	    proframe = new JFrame("Progress...");
	    proframe.setIconImage(workIcon.getImage());
	    proframe.setLocation(100,500);
	    proframe.setMinimumSize(new Dimension(210,120));
	    proframe.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
	    xprogmet=new xMeter(1,"Progress",0.,140.,200,90,new Color(0.5f,1.f,0.5f));
	    xprogmet.setLettering("DABC xGUI "," Startup","Create","");
	    proframe.getContentPane().add(xprogmet,BorderLayout.CENTER);
	    proframe.pack();
	//Display the window.
	    proframe.setVisible(true);
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
if(System.getenv("HOST")==null){
	System.out.println("No HOST name defined, set HOST to GUI node name!");
	return;
	}
if(System.getenv("USER")==null){
	System.out.println("No USER name defined, set USER to user name!");
	return;
	}

Thread progress=new Thread(){
	public void run(){
		javax.swing.SwingUtilities.invokeLater(
		new Runnable() {public void run() {createAndShowProgress();}});
//		while(iprogress<100){
//			iprogress+=10;
//			try{
//			Thread.sleep(500);
//			}catch(InterruptedException x){}
//			if(xprogmet != null){
//			    xprogmet.setLettering("Progress","Startup","Create","");
//				xprogmet.redraw((double)iprogress);
//			}
//		}
	}
};
//progress.start();
javax.swing.SwingUtilities.invokeLater(
		new Runnable() {public void run() {createAndShowProgress();}});

// parse arguments 
xSet.checkMainArgs(args);

javax.swing.SwingUtilities.invokeLater(
new Runnable() {
public void run() {createAndShowGUI();}
});
}
}
