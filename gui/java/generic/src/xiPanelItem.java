package xgui;
import java.awt.Dimension;
import java.awt.Point;
import java.awt.event.ActionListener;
import javax.swing.JPanel;
/**
 * JPanel items to be placed into xPanelGraphics.
 * @see xPanelGraphics
 */
public interface xiPanelItem
{
/**
 * @return Characteristic string of item.
 */
public abstract String getName();
/**
 * Sets the preferred size of item to internal vale.
 */
public abstract void setSizeXY();
/**
 * @return Current position (relative to frame).
 */
public abstract Point getPosition();
/**
 * @return Current dimension (size).
 */
public abstract Dimension getDimension();
/**
 * Sets the preferred size of item to specified dimension.
 * Some items may resize all elements.
 * @param d Dimension.
 */
public abstract void setSizeXY(Dimension d);
/**
 * Set internal ID.
 * @param id ID unique in PanelGraphics (index).
 */
public abstract void setID(int id);
/**
 * @return ID unique in PanelGraphics (index).
 */
public abstract int getID();
/**
 * @return The panel of the class implementing the Interface.
 * This panel is displayed in the PanelDisplay.
 */
public abstract JPanel getPanel();
/**
 * @param actionlistener Optional actionlistener. If set, the action events from the
 * panel item are passed through to this action listener.
 */
public void setActionListener(ActionListener actionlistener);
}
