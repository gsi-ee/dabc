package xgui;
import dim.*;

/**
 * Interface to be implemented by application panels.
 * Application panels are JPanels.
 * In setDimServices (xiUserPanel) this interface must be attached to a DIM parameter
 * by browser function addInfoHandler.
 * @author Hans G. Essel
 * @version 1.0
 * @see xiDimBrowser
 */
public interface xiUserInfoHandler
{
/**
 * Called in callback of DIM parameter.
 * @param parameter Interface to parameter which has been changed.
 * @see xiDimParameter
 */
public abstract void infoHandler(xiDimParameter parameter);
/**
 * Called in callback of DIM parameter.
 * @return Unique name of handler.
 */
public abstract String getName();
}
