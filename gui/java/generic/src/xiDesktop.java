package xgui;
import javax.swing.JDesktopPane;
import javax.swing.JInternalFrame;

/**
 * Interface to desktop. External components can let
 * the xDesktop open/close frames (JInternalFrame).
 * Interface is passed as argument in init function of xiUserPanel.
 * @see xiUserPanel
 * @author Hans G. Essel
 * @version 1.0
 */
public interface xiDesktop
{
/**
 * Checks if a frame exists on the desktop.
 * @param title Title of the frame to searched for.
 * @return true if frame with specified title exists, or false.
 */
public abstract boolean findFrame(String title); 
/**
 * Adds a frame to desktop if a frame with same title does not exist.
 * @param frame Frame to put on desktop. Frame will be managed.
 */
public abstract void addFrame(JInternalFrame frame);
/**
 * Adds a frame to desktop if a frame with same title does not exist.
 * Managed frames store/retreive their layout like GUI frames.
 * @param frame Frame to put on desktop. 
 * @param manage If true, frame will be managed by GUI: layout is saved and restored.
 */
public abstract void addFrame(JInternalFrame frame, boolean manage);
/**
 * Remove (dispose) a frame from the desktop and list of managed frames.
 * @param title Title of the frame to be removed.
 */
public abstract void removeFrame(String title);
/**
 * Switch a frames selection state (setSelected).
 * @param title Title of the frame to be selected.
 * @param select passed to setSelected method of frame.
 */
public abstract void setFrameSelected(String title, boolean select);
/**
 * Set frames to front.
 * @param title Title of the frame.
 */
public abstract void toFront(String title);
}
