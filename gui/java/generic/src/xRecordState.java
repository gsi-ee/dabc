package xgui;
import java.util.*;
/**
 * Dim record data for state.
 * @author Hans G. Essel
 * @version 1.0
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
