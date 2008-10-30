package xgui;
import javax.swing.ImageIcon;
import java.awt.event.*;
public interface xiUserPanel
{
public abstract void init(xiDesktop desktop, ActionListener actionlistener);
public abstract String getHeader();
public abstract String getToolTip();
public abstract ImageIcon getIcon();
public abstract xiUserCommand getUserCommand();
public abstract void setDimServices(xiDimBrowser browser);
public abstract void releaseDimServices();
}