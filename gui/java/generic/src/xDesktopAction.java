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
 * Constructor of desktop runnable.
 * @param desktop Interface to desktop where the commands are implemented.
 */
public xDesktopAction(xiDesktop desktop){
    desk=desktop;
}
/**
 * Function running on event level.
 * By command, functions of desktop are called.
 */
public void run(){
	System.out.println(" *** action command "+command);	
	if(command.equals("Update"))desk.updateDim(false);  // no cleanup
	else if(command.equals("RebuildCommands"))desk.rebuildCommands(); 
	else if(command.equals("RemoveFrame"))desk.removeFrame(arg1);
	Running=false; 
}
/**
 * Launch a command to be executed at event level.
 * If previous 
 * @param actioncommand Command to be executed. 
 * Dispatch is done in run function. Command functions
 * are implemented in Desktop.
 * @see xDesktop
 */
public void execute(String actioncommand){
	if(Running){
		System.out.println(" *** action command "+command+" overrun, skipped!");	
		return;
	}
	Running=true; 
	command=new String(actioncommand);
    SwingUtilities.invokeLater(this); // call run
}
/**
 * Stores argument and callls execute. 
 * @param actioncommand Command to be executed. 
 * @param arg Command argument.
 */
public void execute(String actioncommand, String arg){
	arg1=new String(arg);
	execute(actioncommand);
}
public boolean isRunning(){return Running;}
}
