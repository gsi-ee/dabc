package xgui;
/**
*
* @author goofy
*/
import java.util.*;
/**
*/
public class xRecordHisto extends xRecord
{
private int Mode;
private int[] intArr;
private int ichannels;
private float Low;
private float Up;
private String Lettering;
private String Content;
/**
 * DIM record object: Histo
 * @param name DABC format full parameter name
 * @param index Index in parameter table
 * @param low lower limit
 * @param up upper limit
 * @param color color of pointer
 * @param units units
 */
public xRecordHisto(String name, int type, 
        float lower, float upper, String letter, String cont,
        String color){
        super(name,type);
    setColor(color);
    setLower(lower);
    setUpper(upper);
}
public void setValue(int ich, int[] iar){intArr=iar; ichannels=ich;}
public void setLower(float low){Low=low;}
public void setUpper(float up){Up=up;}

public int[] getValue(){return intArr;}
public float getLower(){return Low;}
public float getUpper(){return Up;}
} // class xRecordHisto
