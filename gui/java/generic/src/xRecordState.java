package xgui;
/**
*
* @author goofy
*/
import java.util.*;
/**
*/
public class xRecordState extends xRecord
{
private int Severity;
private String Value;
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
public xRecordState(String name, int type){
super(name,type);
}
public void setValue(int severity, String color, String value){
setColor(color);
Severity=severity;
Value=value;
}
public String getValue(){return Value;}
public int getSeverity(){return Severity;}
} // class xRecordState
