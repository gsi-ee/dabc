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
