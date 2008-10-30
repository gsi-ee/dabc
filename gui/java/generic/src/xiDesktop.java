package xgui;
import javax.swing.JDesktopPane;
import javax.swing.JInternalFrame;

public interface xiDesktop
{
public abstract boolean frameExists(String title); 
public abstract void addDesktop(JInternalFrame frame);
public abstract void addDesktop(JInternalFrame frame, boolean manage);
public abstract void removeDesktop(String title);
public abstract void frameSelect(String title, boolean select);
public abstract void frameToFront(String title);
}