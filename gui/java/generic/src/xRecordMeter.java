package xgui;
/**
*
* @author goofy
*/
import java.util.*;
/**
*/
public class xRecordMeter extends xRecord
{
private int Mode;
private float Value;
private float Low;
private float Up;
private float AlarmLow;
private float AlarmUp;
private String AlarmColor;
private String Units;
/**
 * DIM record object: Meter
 * @param name DABC format full parameter name
 * @param index Index in parameter table
 * @param low lower limit
 * @param up upper limit
 * @param alarmLow alarm lower limit
 * @param alarmUp alarm upper limit
 * @param color color of pointer
 * @param alarmcol color of frame in alarm
 * @param units units
 */
public xRecordMeter(String name, int type, int mode,
        float lower, float upper, float alarmLower, float alarmUpper, 
        String color, String alarmColor, String units){
        super(name,type);
    AlarmLow=alarmLower;
    AlarmUp=alarmUpper;
    Mode=mode;
    setColor(color);
    setLower(lower);
    setUpper(upper);
    Units = new String(units);
}
public void setValue(float value){Value=value;}
public void setLower(float low){Low=low;}
public void setUpper(float up){Up=up;}

public int getMode(){return Mode;}
public float getValue(){return Value;}
public float getLower(){return Low;}
public float getUpper(){return Up;}
public String getUnits(){return Units;}
} // class xRecordMeter
