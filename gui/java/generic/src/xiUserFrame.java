package xgui;
import javax.swing.ImageIcon;
import java.awt.event.*;
public interface xiUserFrame
{
public abstract void init(xDimBrowser b, ActionListener a);
public abstract String getHeader();
public abstract String getToolTip();
public abstract ImageIcon getIcon();
public abstract void setDimServices();
public abstract void releaseDimServices();
public abstract void infoHandler(xDimParameter p);
}