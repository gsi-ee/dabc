package xgui;
import java.util.*;
import java.io.*;
import java.awt.Point;
import java.awt.Dimension;
/**
 * Base class for DIM record data.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xRecord{
private int Type;
private String Name;
private String Color;
private xParser pars;
private Dimension dim;
private Point pos;
private Boolean visible;
/**
 * DIM record object:
 * @param name DABC format full parameter name
 * @param type Type of record (like Meter, State, Info, Histo) as defined in parser
 * @see xParser
 */
public xRecord(String name, int type){
    Type=type;
    Name=name;
    visible=new Boolean(false);
}
/**
 * Set size.
 * @param dimension Size of graphics.
 */
public void setSize(Dimension dimension){dim=dimension;}
public Dimension getSize(){return dim;}
/**
 * Set position. Normally the position is determined by the container panel like xPanelMeter.
 * The position is relative to the container panel.
 * @param position Position of graphics.
 */
public void setPosition(Point position){pos=position;}
public Point getPosition(){return pos;}
/**
 * Set record visible
 */
 public void setVisible(String Visible){visible=new Boolean(Visible);}
 public void setVisible(boolean Visible){visible=new Boolean(Visible);}
 public Boolean isVisible(){return visible;}
/**
 * DIM record object:
 * @param color Color depending on record.
 */
public void setColor(String color){Color=color;}
public String getColor(){return Color;}
public String getName(){return Name;}
public void setName(String name){Name=new String(name);}
/**
 * @return record type as defined in parser.
 * @see xParser
 */
public int getType(){return Type;}

} // class xRecord
