/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
package xgui;
import java.util.*;
import org.w3c.dom.*;
import java.awt.Dimension;
/**
 * Dim record data for histogram.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xRecordHisto extends xRecord
{
private int Mode=0;
private int[] intArr;
private int ichannels;
private float Low;
private float Up;
private String Lettering;
private String Content;
private Boolean Log;
/**
 * DIM record object: Histo
 * @param name DABC format full parameter name
 * @param type Record type
 * @param lower lower limit
 * @param upper upper limit
 * @param lettering Lettering of X axis.
 * @param content Lettering of Y axis
 * @param color color of pointer
 */
public xRecordHisto(String name, int type, 
        float lower, float upper, String lettering, String content,
        String color){
        super(name,type);
    setColor(color);
    setLower(lower);
    setUpper(upper);
    setLettering(lettering);
    setContent(content);
}
public void setValue(int ich, int[] iar){intArr=iar; ichannels=ich;}
public void setLower(float low){Low=low;}
public void setUpper(float up){Up=up;}
public void setContent(String content){Content=new String(content);}
public void setLettering(String lettering){Lettering=new String(lettering);}
public void setMode(int mode){Mode=mode;}
public int getMode(){return Mode;}
public Boolean getLogScale(){return Log;}
public void setLogScale(Boolean log){Log=new Boolean(log);}

public int[] getValue(){return intArr;}
public float getLower(){return Low;}
public float getUpper(){return Up;}
public String XmlLine(){
String str = String.format("<Histo name=\"%s\" visible=\"%s\" mode=\"%d\" log=\"%s\" width=\"%d\" height=\"%d\" color=\"%s\"/>\n",
getName(),isVisible().toString(),getMode(),getLogScale().toString(),(int)(getSize().getWidth()),(int)(getSize().getHeight()),getColor());
return str;}
public void restoreRecord(Element el){
String att;
int w,h;
    att=el.getAttribute("mode");
    if(att.length()>0)Mode=Integer.parseInt(att);
    att=el.getAttribute("log");
    if(att.length()>0)Log=new Boolean(att);
    att=el.getAttribute("width");
    if(att.length()>0){
        w=Integer.parseInt(att);
        att=el.getAttribute("height");
        if(att.length()>0){
            h=Integer.parseInt(att);
            setSize(new Dimension(w,h));
        }}
    att=el.getAttribute("visible");
    if(att.length()>0)setVisible(att);
    att=el.getAttribute("color");
    if(att.length()>0)setColor(att);
}
public void printRecord(){
System.out.println(getType()+" "+getName()+" "+getColor()+" "+getMode()+" "+
(int)(getSize().getWidth())+" "+(int)(getSize().getHeight()));
}
} // class xRecordHisto
