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
import javax.swing.ImageIcon;
import java.awt.event.*;
/**
 * Interface to be implemented by application panels.
 * Application panels are JPanels.
 * @author Hans G. Essel
 * @version 1.0
 */
public interface xiUserPanel
{
/**
 * Called by xDesktop.
 * @param desktop Interface to desktop.
 * @param actionlistener The action listener of the desktop. Events may be passed to this.
 * Not sure if this parameter will remain, because it is dangerous to allow the application
 * to pass events to the desktops listener.
 */
public abstract void init(xiDesktop desktop, ActionListener actionlistener);
/**
 * Called by xDesktop.
 * @return String to be used as header.
 */
public abstract String getHeader();
/**
 * Called by xDesktop.
 * @return String for tool tip.
 */
public abstract String getToolTip();
/**
 * Called by xDesktop.
 * @return ImageIcon for start button.
 */
public abstract ImageIcon getIcon();
/**
 * Called by xDesktop. This should ensure that the user panel creates a layout.
 * @return xLayout.
 */
public abstract xLayout checkLayout();
/**
 * Called by xDesktop.
 * @return Object implementing xiUserCommand
 */
public abstract xiUserCommand getUserCommand();
/**
 * Called from desktop after releaseDimServices() after every change in DIM services.
 * Application panel must build all references to DIM services.
 * @param browser Interface to browser.
 */
public abstract void setDimServices(xiDimBrowser browser);
/**
 * Called from desktop after every change in DIM services.
 * Application panel must release local references to DIM parameters and commands.
 */
public abstract void releaseDimServices();
}
