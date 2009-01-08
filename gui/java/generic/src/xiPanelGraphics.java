package xgui;
import java.awt.Dimension;

/**
 * Interface for JPanels to be put in xPanelGraphics.
 * @author Hans G. Essel
 * @version 1.0
 */
public interface xiPanelGraphics
{
public abstract String getName();
public abstract void setSizeXY();
public abstract void setSizeXY(Dimension d);
public abstract void setID(int id);
}