/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
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
/**
 * @return Meter record.
 */
public abstract xRecordMeter getMeter();
/**
 * @return State record.
 */
public abstract xRecordState getState();
/**
 * @return Info record.
 */
public abstract xRecordInfo getInfo();
/**
 * @return Histogram record.
 */
public abstract xRecordHisto getHisto();
/**
 * @return Active state of parameter.
 */
public abstract boolean parameterActive();
/**
 * @return Interface to parser provides access to all name fields.
 */
public abstract xiParser getParserInfo();
/**
 * @return Parameter string value.
 */
public abstract String getValue();
/**
 * @return Parameter integer (32) value.
 */
public abstract int getIntValue();
/**
 * @return Parameter long (64) value.
 */
public abstract long getLongValue();
/**
 * @return Parameter float value.
 */
public abstract float getFloatValue();
/**
 * @return Parameter double value.
 */
public abstract double getDoubleValue();
}
