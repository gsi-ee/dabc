package xgui;
/*
This client expects server tDABCserver from eclipse DIM project.
*/


/**
* @author goofy
*/

import java.net.URL;
import java.io.IOException;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Point;
import java.util.*;

/**
* DIM GUI class
*/
public class xLayout {
private String name;
private Dimension size;
private Point pos;
private int cols;
private boolean show;

public xLayout(String n) {name=n;}

public void set(Point lpos, Dimension lsize, int columns, boolean lshow) {
if(lpos != null)  pos=lpos;
if(lsize != null) size=lsize;
if(columns != 0)  cols=columns;
show=lshow;
}
public String getXmlString(){
return String.format("<%s shape=\"%d,%d,%d,%d\" columns=\"%d\" show=\"%b\"/>\n",
name,(int)pos.getX(),(int)pos.getY(),(int)size.getWidth(),(int)size.getHeight(),cols,show);}

public String getName(){return name;}
public boolean show(){return show;}
public int getColumns(){return cols;}
public Point getPosition(){return pos;}
public Dimension getSize(){return size;}
public String toString(){
return new String(
name+
":posx="+(int)pos.getX()+
",posy="+(int)pos.getY()+
",sizx="+(int)size.getWidth()+
",sizy="+(int)size.getHeight()+
",colums="+cols+
",show="+show);
}
}

