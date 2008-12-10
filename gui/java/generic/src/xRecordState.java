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
 * @param type Record type
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
