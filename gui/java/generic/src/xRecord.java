package xgui;
/**
*
* @author goofy
*/
import java.util.*;
/**
*/
public class xRecord{
private int Type;
private String Name;
private String Color;
private xParser pars;
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
public xRecord(String name, int type){
    Type=type;
    Name=name;
}
public void setColor(String color){Color=color;}
public String getColor(){return Color;}
public String getName(){return Name;}
public int getType(){return Type;}
} // class xRecord
