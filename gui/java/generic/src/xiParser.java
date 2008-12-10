package xgui;

/**
 * Interface to name parser.
 * Format of names is:<br>
 * Dns/Node:NodeId/applNS::Application:ApplicationID/Name.component
 * @author Hans G. Essel
 * @version 1.0
 */
public interface xiParser
{
public abstract String getDns();
public abstract String getNode();
public abstract String getNodeName();
public abstract String getNodeID();
public abstract String getApplicationFull();
public abstract String getApplication();
public abstract String getApplicationName();
public abstract String getApplicationID();
public abstract String getName();
public abstract String getNameSpace();
public abstract String[] getItems();
public abstract String getFull();
public abstract String getCommand();
public abstract int getType();
public abstract int getState();
public abstract int getVisibility();
public abstract int getMode();
public abstract int getQuality();
public abstract int getNofTypes();
public abstract int[] getTypeSizes();
public abstract String[] getTypeList();
public abstract String getFormat();
public abstract boolean isNotSpecified();
public abstract boolean isSuccess();
public abstract boolean isInformation();
public abstract boolean isWarning();
public abstract boolean isError();
public abstract boolean isFatal();
public abstract boolean isAtomic();
public abstract boolean isGeneric();
public abstract boolean isState();
public abstract boolean isInfo();
public abstract boolean isRate();
public abstract boolean isHistogram(); 
public abstract boolean isCommandDescriptor(); 
public abstract boolean isHidden(); 
public abstract boolean isVisible(); 
public abstract boolean isMonitor(); 
public abstract boolean isChangable(); 
public abstract boolean isImportant();
public abstract boolean isLogging();
public abstract boolean isArray();
public abstract boolean isFloat();
public abstract boolean isDouble();
public abstract boolean isInt();
public abstract boolean isLong();
public abstract boolean isChar();
public abstract boolean isStruct();
}
