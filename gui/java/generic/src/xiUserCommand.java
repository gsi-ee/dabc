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
/**
 * Interface to be implemented by application panels.
 * Application panels are JPanels.
 * @author Hans G. Essel
 * @version 1.0
 */
public interface xiUserCommand
{
/**
 * Called by xPanelCommand.
 * @see xPanelCommand
 * @param scope of command
 * @param command string
 * @return true, if command should be XML coded, false otherwise (MBS).
 */
public abstract boolean getArgumentStyleXml(String scope, String command);
}
