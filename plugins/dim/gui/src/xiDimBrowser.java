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
/**
 * Interface of DIM browser.
 * @author Hans G. Essel
 * @version 1.0
 */
 public interface xiDimBrowser
{
/**
 * Called in setDimServices of application panel to get available parameters.
 * @return Vector of parameter objects.
 */
public abstract Vector<xiDimParameter> getParameters();
/**
 * Called in setDimServices of application panel to get available commands.
 * @return Vector of command objects.
 */
public abstract Vector<xiDimCommand> getCommands();
/**
 * Called in setDimServices of application panel to attach an info handler to a parameter.
 * From the list returned by getParameters each to be handled by the panels handler must be added.
 * @param parameter Interface to parameter
 * @param infohandler Interface of user info handler (application panel implementing xiUserInfohandler).
 */
public abstract void addInfoHandler(xiDimParameter parameter, xiUserInfoHandler infohandler);
/**
 * An info handler previously added to a parameter is removed (reference only).
 * @param parameter Interface to parameter
 * @param infohandler Interface of user info handler (application panel implementing xiUserInfohandler).
 */
public abstract void removeInfoHandler(xiDimParameter parameter, xiUserInfoHandler infohandler);
/**
 * Sleep some seconds.
 * @param seconds
 */
public abstract void sleep(int seconds);
}
