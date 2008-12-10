package xgui;

import java.net.URL;
import java.io.IOException;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Point;
import java.util.*;
/**
 * Layout objects keep information about the appearance of panels:
 * Position, size, columns, and visibility. Layouts are managed by xSet.
 * Layout can be stored/retrieved to/from XML file.
 * @author Hans G. Essel
 * @version 1.0
 * @see xSet
 */
public class xLayout {
private String name;
private Dimension size;
private Point pos;
private int cols;
private boolean show;
/**
 * Create layout object with a name.
 */
public xLayout(String Name) {name=Name;}
/**
 * Set layout.
 * @param lpos Position or null.
 * @param lsize Size or null.
 * @param columns Columns or 0.
 * @param lshow visibility.
 */
public void set(Point lpos, Dimension lsize, int columns, boolean lshow) {
if(lpos != null)  pos=lpos;
if(lsize != null) size=lsize;
if(columns != 0)  cols=columns;
show=lshow;
}
/**
 * @return XML formatted line to be inserted in XML file by caller.<br>
 * name shape="x,y,w,h" columns="" show=""
 */
public String XmlLine(){
return String.format("<%s shape=\"%d,%d,%d,%d\" columns=\"%d\" show=\"%b\"/>\n",
name,(int)pos.getX(),(int)pos.getY(),(int)size.getWidth(),(int)size.getHeight(),cols,show);}

public String getName(){return name;}
public boolean show(){return show;}
public int getColumns(){return cols;}
public Point getPosition(){return pos;}
public Dimension getSize(){return size;}
/**
 * @return Formatted string <br>
 * name:x=,y=,w=,h=,columns=,show=.
 */
public String toString(){
return new String(
name+
":x="+(int)pos.getX()+
",y="+(int)pos.getY()+
",w="+(int)size.getWidth()+
",h="+(int)size.getHeight()+
",columns="+cols+
",show="+show);
}
}

