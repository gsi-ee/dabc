package xgui;
/**
*
* @author goofy
*/
import java.util.*;
/**
*/
public class xRecordInfo extends xRecord
{
private int Severity;
private String Value;
/**
 * DIM record object: Info
 * @param name DABC format full parameter name
 * @param type Record type
 */
public xRecordInfo(String name, int type){
super(name,type);
}
/**
 * DIM record object: Info
 * @param severity
 * @param color color of text.
 * @param value Text
 */
public void setValue(int severity, String color, String value){
setColor(color);
Severity=severity;
Value=value;
}
public String getValue(){return Value;}
public int getSeverity(){return Severity;}
} // class xRecordState
