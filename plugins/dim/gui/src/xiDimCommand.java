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
 * Interface to command objects.
 * @author Hans G. Essel
 * @version 1.0
 */
public interface xiDimCommand
{
/**
 * Execute DIM command from internal parameter string (not from commandstring which is used only for sorting).
 * If the application name is $:0 this was no DABC formatted command like DIM server EXIT and is handled differently.
 * @param argument String for command argument<br>
 * <b>Note:</b> If the command expects an integer or float argument, the string must be formatted properly.
 */
public abstract void exec(String argument);
/**
 * Get parser interface (keeps definitions and values).<br>
 * This interface provides only getter functions. It is called from external classes.
 * @return interface to parser provides access to all name fields.
 */
public abstract xiParser getParserInfo();
}
