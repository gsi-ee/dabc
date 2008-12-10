package xgui;
import dim.*;
/**
 * Interface to parameter objects. Mainly getter methods to access to parameter values.
 * @author Hans G. Essel
 * @version 1.0
 */
public interface xiDimParameter
{
/**
 * Builds and executes a DIM command <b> SetParameter name=vale </b>
 * where name is the name part of the full DIM name string. The command <b> SetParameter </b>
 * of cause must be implemented on the DIM server side. Called in xPanelParameter when a value is
 * changed in the table.
 * @param value
 * @return completion status.
 */
public abstract boolean setParameter(String value);
public abstract xRecordMeter getMeter();
public abstract xRecordState getState();
public abstract xRecordInfo getInfo();
public abstract xRecordHisto getHisto();
/**
 * @return interface to parser provides access to all name fields.
 */
public abstract xiParser getParserInfo();
public abstract String getValue();
public abstract int getIntValue();
public abstract long getLongValue();
public abstract float getFloatValue();
public abstract double getDoubleValue();
}
