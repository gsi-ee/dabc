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
import java.util.*;
import java.lang.*;
import javax.swing.SwingUtilities;

/**
 * Execute action in event thread
 * @author Hans G. Essel
 * @version 1.0
 * @see xDesktop
 */
public class xDesktopAction implements Runnable{
private xiDesktop desk;
private String command;
private String arg1;
private boolean Running=false;

/**
 * Constructor of DIM parameter handler.
 * @param service DIM name of service: DIS_DNS/SERVER_LIST
 * @param label Text area to store the DIM server list. 
 */
public xDesktopAction(xiDesktop desktop){
    desk=desktop;
}
public void run(){
	System.out.println(" *** action command "+command);	
	if(command.equals("Update"))desk.updateDim(false);  // no cleanup
	else if(command.equals("RebuildCommands"))desk.rebuildCommands(); 
	else if(command.equals("RemoveFrame"))desk.removeFrame(arg1);
	Running=false; 
}
public void execute(String actioncommand){
	Running=true; 
	command=new String(actioncommand);
    SwingUtilities.invokeLater(this); // call run
}
public void execute(String actioncommand, String arg){
	if(Running){
		System.out.println(" *** action command "+command+" overrun, skipped!");	
		return;
	}
	Running=true; 
	arg1=new String(arg);
	command=new String(actioncommand);
    SwingUtilities.invokeLater(this); // call run
}
public boolean isRunning(){return Running;}
}
