package xgui;
import dim.*;

public interface xiDimParameter
{
public abstract boolean setParameter(String value);
public abstract xRecordMeter getMeter();
public abstract xRecordState getState();
public abstract xRecordInfo getInfo();
public abstract xiParser getParserInfo();
public abstract String getValue();
public abstract int getIntValue();
public abstract long getLongValue();
public abstract float getFloatValue();
public abstract double getDoubleValue();
}