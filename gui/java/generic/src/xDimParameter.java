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
import java.awt.Color;
import java.awt.Dimension;
import java.util.*;
import dim.*;
/**
 * A list of these objects is the central management of DIM parameters.
 * It has a DIM handler, creates the graphical elements, the records keeping
 * parameter values, and interface functions to access these values.
 * It also has reference to table model. On parameter update, the table and all graphical objects are updated.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xDimParameter implements xiDimParameter {
private xParser pars, compars;
private xXmlParser xmlpars;
private DimHandler dimhandler;
private boolean isactive=false;
private int i,ii;
private int quality=-1;
private boolean doprint=false, dolog=false;
private int[] intarr;
private long[] longarr;
private float[] floatarr;
private double[] doublearr;
private String[] stringarr;
private String value;
private long lvalue;
private int ivalue;
private float fvalue;
private double dvalue;
private String setcommand;
private int tabrow=-1, myparindex=-1;
private boolean tabinit=false;
private xParTable tabmod;
private xMeter meter=null;
private boolean paint=false;
private boolean paraShown=false;
private boolean meterInit=false;
private int meterMode;
private float meterLow;
private float meterUp;
private float meterAlarmLow;
private float meterAlarmUp;
private String meterColor;
private String meterAlarmColor;
private String meterUnits;
private xHisto histo=null;
private int histoChannels;
private float histoLow;
private float histoHigh;
private String histoContent;
private String histoLetter;
private String histoColor;
private xState stat=null;
private xInfo info=null;
private int stateSeverity;
private String stateColor;
private String stateName;
private xPanelMeter metpan;
private xPanelHisto hispan;
private xPanelState stapan;
private xPanelInfo infpan;
private String color;
private String sNolink;
private int    iNolink;
private float  fNolink;
private int Version;
private Vector<xiUserInfoHandler> userHandler=null;
private xRecordHisto rechis=null;
private xRecordMeter recmet=null;
private xRecordState recsta=null;
private xRecordInfo recinf=null;
private boolean LF=true;
private boolean skip=false;

/**
 * DIM integer parameter. Calls initParser
 * @param name DABC format full parameter name
 * @param format DIM format list
 * @param noLink default value if no connection to DIM server
 * @param version instance number (for internal debugging only)
 */
public xDimParameter(String name, String format, int noLink, int version){
    Version=version;
    sNolink=new String("%BROKEN%");
    iNolink=noLink;
    initParser(name,format);
    dimhandler = new DimHandler(name,noLink,this);
}
/**
 * DIM float parameter. Calls initParser
 * @param name DABC format parameter name
 * @param format DIM format list
 * @param noLink default value if no connection to DIM server
 * @param version instance number (for internal debugging only)
 */
public xDimParameter(String name, String format, float noLink, int version){
    Version=version;
    sNolink=new String("%BROKEN%");
    fNolink=noLink;
    initParser(name,format);
    dimhandler = new DimHandler(name,noLink,this);
}
/**
 * DIM string parameter. Calls initParser
 * @param name DABC format parameter name
 * @param format DIM format list
 * @param noLink default value if no connection to DIM server
 * @param version instance number (for internal debugging only)
 */
public xDimParameter(String name, String format, String noLink, int version){
    Version=version;
    sNolink=new String(noLink);
    initParser(name,format);
    dimhandler = new DimHandler(name,noLink,this);
}
/**
 * Initializes name parser. Creates XML parser. Creates command to set parameter value by preceding underscore
 * to the parameter name. Value is set to string of NOLINK. 
 * @param name DABC format parameter name
 * @param format DIM format list
 */
protected void initParser(String name, String format){
    pars = new xParser();
    xmlpars = new xXmlParser();
    i=pars.parse(name,xParser.PARSE_STORE_FULL); // check, parse and store
    i=pars.format(format,xParser.PARSE_STORE_FULL); // check, parse and store
    value=new String(sNolink);
}

public String getName(){
	return pars.getFull();
}
protected void releaseService(){
	dimhandler.releaseService();
	dimhandler=null;
}
public void printParameter(int index){
System.out.println(index+": "+dimhandler.getName());
if(quality != 2304){
System.out.println("  I:"+myparindex+" TI:"+tabrow+" Q:"+quality+" Active:"+isactive+" Shown: "+paraShown);
System.out.println("  Value: "+value);
}
}
public void printParameter(boolean comdef){
if(comdef || (quality != 2304))
System.out.println(dimhandler.getName()+
		" F:"+pars.getFormat() +
		" Q:"+String.format("%08x",quality)+
		" Active:"+isactive+" Shown: "+paraShown+" Value: "+value);

//System.out.println(dimhandler.getName()+
//" I:"+myparindex+" TI:"+tabrow+
//" Q:"+String.format("%08x",quality)+
//" Active:"+isactive+" Shown: "+paraShown+" Value: "+value);

// exclude command descriptors
// if(quality != 2304) {
// if(pars.getName().equals("RunStatus"))
// System.out.println(dimhandler.getName()+"  Value: "+value);
//}
}

// protected void finalize(){
// if(pars.getName().equals("Transport"))
// System.out.println("    "+Version+" finpar "+pars.getFull());
// pars=null;
// }
/**
 * Imports the references to the graphic panels. Called from browser.
 * @param histogramPanel
 * @param meterPanel
 * @param statePanel
 * @param infoPanel
 */
protected void setPanels(xPanelHisto histogramPanel, xPanelMeter meterPanel, xPanelState statePanel, xPanelInfo infoPanel){
    hispan=histogramPanel;
    metpan=meterPanel;
    stapan=statePanel;
    infpan=infoPanel;
}
/**
 * Add user handler. Called from user panels through xiDimBrowser interface.
 * @param pu Interface of user handler
 * @see xiDimBrowser
 */
protected void addInfoHandler(xiUserInfoHandler pu){
if(userHandler== null)userHandler=new Vector<xiUserInfoHandler>();
else for(int i=0;i<userHandler.size();i++)
    if(userHandler.get(i).getName().equals(pu.getName()))return; // already attached
userHandler.add(pu);
}
/**
 * Remove all user handlers. 
 * @see xiDimBrowser
 */
protected void removeInfoHandler(){
removeInfoHandler(null);
}
/**
 * Remove user handler. Called from user panels through xiDimBrowser interface.
 * @param pu Interface of user handler. If null, all handlers are removed.
 * @see xiDimBrowser
 */
protected void removeInfoHandler(xiUserInfoHandler pu){
if(userHandler==null) return;
if(pu == null){
	for(int i=0;i<userHandler.size();i++)
	userHandler.clear();
}
else{
for(int i=0;i<userHandler.size();i++)
if(userHandler.get(i).getName().equals(pu.getName())){
    userHandler.remove(i);
    break;
    }
}}
/**
 * (De)activate parameter.
 * A deactivated parameter does no drawing in infoHandler function neither calls user handler.
 * Before any changes in the parameter list is done, all parameters are deactivated (browser).
 * @param activ true: Activate and redraw all graphic objects, otherwise deactivate.
 */
protected void setParameterActiv(boolean activ) {
if(activ){
if(info!=null)info.redraw();
if(meter!=null)meter.redraw();
if(stat!=null)stat.redraw();
if(histo!=null)histo.redraw();
isactive=true;
} else isactive=false;
}
public boolean parameterActive(){
	return isactive;
}
protected void setLogging(boolean yes){
	dolog=yes;
}
/**
 * Adds a new row to the table. Called in xPanelParameter.initPanel.
 * Only visible parameters are handled. Graphical elements are created like meters,
 * if they are monitored.
 * @param table Table model assigned to this parameter.
 * @param rowindex index of row in table.
 * @return true parameter is visible and has been added,
 * else parameter is not visible and has not been added.
 * @see xPanelParameter
 */
protected boolean addRow(xParTable table, int rowindex){
boolean ret=false;
boolean show=false;
if(pars.isVisible()){
    tabmod=table;
    tabinit=true; // otherwise following elements would not be put to panels
    if(pars.isRate() && paint) {createMeter(true);}
    if(pars.isInfo() && paint) {createInfo(true);}
    if(pars.isState() && paint) {createState(true);}
    if(pars.isHistogram() && paint) {createHisto(true);}
    Vector<Object> row = new Vector<Object>();
    row.add(this);
    row.add(pars.getNode().replace(".gsi.de",""));
    row.add(pars.getApplication());
    row.add(pars.getName());
    if(pars.isRate()) row.add(new String("rate"));
    else if(pars.isInfo()) row.add(new String("info"));
    else if(pars.isState()) row.add(new String("state"));
    else if(pars.isHistogram()) row.add(new String("histo"));
    else if(pars.isFloat()) row.add(new String("float"));
    else if(pars.isDouble()) row.add(new String("double"));
    else if(pars.isInt()) row.add(new String("int"));
    else if(pars.isLong()) row.add(new String("long"));
    else if(pars.isChar()) row.add(new String("char"));
    else row.add(new String("unknown"));
    row.add(value);
    if(pars.isChangable())row.add("");
    else row.add("-");
    row.add(new Boolean(paraShown)); // set by the create.. functions
    tabmod.addRow(row);
    setTableIndex(rowindex); // only now we can use the new row
//    print(dimhandler.getName()+" i="+rowindex,LF);
    ret = true;
}
return ret;
}
/**
 * Called by xPanelParameter after sorting
 * @param index parameter index shown as text in first column.
 */
protected void setIndex(int index) {
myparindex=index;
paraShown=false;
}
/**
 * Called by PanelParameter after sorting
 * @param index table index needed to update correct row.
 */
protected void setTableIndex(int index) {tabrow=index;}
/**
 * Execute DIM command from internal parameter string .
 * @param arg string for command argument
 */
public boolean setParameter(String arg){
//    System.out.println(setcommand+" "+arg);
if(pars.isChangable()){
    if((!pars.isArray()) && (!pars.isStruct())){
        compars=new xParser();
        compars.parse(pars.getFull(),xParser.PARSE_STORE_FULL); // check, parse and store
        String name=pars.getName(); // store original name
        compars.setName("SetParameter"); // generic command name, hopefully defined
        System.out.print(compars.getFull(true));
        System.out.println(" "+new String(name+"="+arg));
        DimClient.sendCommand(compars.getFull(true),new String(xSet.getAccess()+" "+name+"="+arg));
        // pars.setName(name); // restore name
        // pars.buildFull(); // restore full name
        compars=null;
        xLogger.print(1,new String(pars.getFull()+"="+arg));
        return true;
    } else System.out.println(pars.getFull()+" no scalar!");
} else {
    try {tabmod.setValueAt("-", tabrow, 6);}
    catch (NullPointerException e){}
    System.out.println(pars.getFull()+" not changable!");
    }
return false;
}
/**
 * @param create Create or remove meter (to/from panel).
 */
protected void createMeter(Boolean create){
int len1, len2;
String node;
if(pars.isRate()){
    if(create){
        if(meter == null){
            len1=pars.getNodeName().indexOf(".");
            if(len1 == -1)len1=pars.getNodeName().length();
            len2=pars.getName().indexOf(".");
            if(len2 == -1)len2=pars.getName().length();
            meter = new xMeter(xMeter.ARC,
                    new String(pars.getNodeName().substring(0,len1)+":"+pars.getName().substring(0,len2)),
                    0.0,10.0,xMeter.XSIZE,xMeter.YSIZE,new Color(1.0f,0.0f,0.0f));
            meter.setColorBack(xSet.getColorBack());
        }
        if(meterInit){
            // meter.setColor(recmet.getColor());
            // int i = pars.getNode().indexOf(".");
            // if(i >=0) node=pars.getNode().substring(0,i);
            // else node=pars.getNode();
            // meter.setLettering(node,pars.getApplication(),pars.getName(),recmet.getUnits());
            // meter.setDefaultLimits(recmet.getLower(),recmet.getUpper());
            // meter.setDefaultAutoScale(false);
            // meter.setDefaultMode(recmet.getMode());
        }
        if(tabinit){
            if(!paraShown)metpan.addMeter(meter);
            paraShown=true;
        }
    } else if(paraShown && (meter != null)) {metpan.removeMeter(meter);paraShown=false;}
}
}
/**
 * @param create Create or remove histogram (to/from panel).
 */
protected void createHisto(Boolean create){
int len;
if(pars.isHistogram()){
    if(create){
        if(histo == null){
            len=pars.getNodeName().indexOf(".");
            if(len == -1)len=pars.getNodeName().length();
            histo = new xHisto(pars.getFull(), new String(pars.getNodeName().substring(0,len)+":"+pars.getName()),
                    histoContent,histoLetter,xHisto.XSIZE,xHisto.YSIZE);
            histo.setColorBack(xSet.getColorBack());
            histo.setColor(histoColor);
        }
        if(tabinit){
            if(!paraShown){
            hispan.addHistogram(histo);
            paraShown=true;
        }}
    } else if(paraShown && (histo != null)) {hispan.removeHistogram(histo); paraShown=false;}
}
}
/**
 * @param create Create or remove state (to/from panel).
 */
protected void createState(Boolean create){
int len;
if(pars.isState()){
    if(create){
        if(stat == null){
            len=pars.getNodeName().indexOf(".");
            if(len == -1)len=pars.getNodeName().length();
            if(pars.getName().equals("RunStatus"))
                stat = new xState(new String(pars.getNodeName().substring(0,len)+":"+pars.getApplicationName()),
                        xState.XSIZE,xState.YSIZE);
            else stat = new xState(new String(pars.getNodeName().substring(0,len)+":"+pars.getName()),
                    xState.XSIZE,xState.YSIZE);
            stat.setColorBack(xSet.getColorBack());
        }
        if(tabinit){
            if(!paraShown){
            stapan.addState(stat);
            paraShown=true;
        }}
    } else if(paraShown && (stat != null)) {stapan.removeState(stat);paraShown=false;}
}
}
/**
 * @param create Create or remove info (to/from panel).
 */
protected void createInfo(Boolean create){
int len;
if(pars.isInfo()){
    if(create){
        if(info == null){
            len=pars.getNodeName().indexOf(".");
            if(len == -1)len=pars.getNodeName().length();
            info = new xInfo(new String(pars.getNodeName().substring(0,len)+": "),xInfo.XSIZE,xInfo.YSIZE);
            info.setColorBack(xSet.getColorBack());
        }
        if(tabinit){
            if(!paraShown)infpan.addInfo(info);
            paraShown=true;
        }
    } else if(paraShown && (info != null)) {infpan.removeInfo(info);paraShown=false;}
}
}

/**
 * @return true if command descriptor
 */
protected boolean isCommandDescriptor(){return pars.isCommandDescriptor();}
/**
 * @return node:ID form parser.
 */
protected String getNode(){return pars.getNode();}
/**
 * @return value.
 */
public String getValue(){return value;}
/**
 * @return value.
 */
public int getIntValue(){return ivalue;}
/**
 * @return value.
 */
public long getLongValue(){return lvalue;}
/**
 * @return value.
 */
public float getFloatValue(){return fvalue;}
/**
 * @return value.
 */
public double getDoubleValue(){return dvalue;}
/**
 * @return quality.
 */
public int getDimQuality(){return quality;}
/**
 * The first row of the table is the DimParameter object. The string seen in the table
 * is the string returned by this function.
 * @return index as string used for table.
 */
public String toString(){return String.format("%05d",myparindex);}
/**
 * @return parser interface
 */
public xiParser getParserInfo(){return pars;}
/**
 * @return parser object.
 */
protected xParser getParser(){return pars;}
/**
 * @return command parser object.
 */
protected xXmlParser getXmlParser(){return xmlpars;}

protected void setPrint(boolean dop){doprint=dop;}

private void print(String s){print(s,false);}

private void print(String s, boolean lf){
    if(doprint){
        if(lf)System.out.println(s);
        else System.out.print(s);
    }}
/**
 * Meter record is updated from meter settings.
 * @return Meter record.
 */
public xRecordMeter getMeter(){
if(recmet!=null){
if(meter != null){
    recmet.setColor(meter.getColor());
    recmet.setSize(meter.getDimension());
    recmet.setLower(meter.getMin());
    recmet.setUpper(meter.getMax());
    recmet.setMode(meter.getMode());
    recmet.setVisible(paraShown);
    recmet.setAutoScale(new Boolean(meter.getAutoScale()));
    recmet.setLogScale(new Boolean(meter.getLogScale()));
} else System.out.println("No meter for "+pars.getFull());
}
return recmet;
}
public void setAttributeMeter(){
if(recmet!=null){
if(meter != null){
    meter.setColor(recmet.getColor());
    meter.setDefaultAutoScale(recmet.getAutoScale().booleanValue());
    meter.setDefaultLimits(recmet.getLower(),recmet.getUpper());
    meter.setDefaultMode(recmet.getMode());
    meter.setDefaultLogScale(recmet.getLogScale().booleanValue());
    paint=recmet.isVisible().booleanValue();
} else System.out.println("No meter to set for "+pars.getFull());
}}

public xRecordState getState(){
	if(recsta != null) recsta.setVisible(paraShown);
	return recsta;
	}
public void setAttributeState(){
if(recsta!=null){
	if(stat != null){
	    paint=recsta.isVisible().booleanValue();
	} else System.out.println("No State to set for "+pars.getFull());
	}	
}
public xRecordInfo getInfo(){return recinf;}

public xRecordHisto getHisto(){
if(rechis!=null){
if(histo != null){
    rechis.setColor(histo.getColor());
    rechis.setSize(histo.getDimension());
    rechis.setMode(histo.getMode());
    rechis.setVisible(paraShown);
    rechis.setLogScale(new Boolean(histo.getLogScale()));
} else System.out.println("No histo for "+pars.getFull());
}
return rechis;
}
public void setAttributeHisto(){
if(rechis!=null){
if(histo != null){
    histo.setColor(rechis.getColor());
    histo.setSizeXY(rechis.getSize());
    histo.setMode(rechis.getMode());
    histo.setLogScale(rechis.getLogScale().booleanValue());
    paint=rechis.isVisible().booleanValue();
} else System.out.println("No histo to set for "+pars.getFull());
}}

// This class holds mainly the infoHandler.
// If it is created, infoHandler might be called immediately.
// Therefore in constructor of xDimParameter this class is instantiated
// after init. Otherwise e.g. parser could be not yet available. 
private class DimHandler extends DimInfo{
private xDimParameter mydimparam;

public DimHandler(String name, int noLink, xDimParameter dimpar){
    super(name,noLink);
    mydimparam=dimpar;
}
public DimHandler(String name, float noLink, xDimParameter dimpar){
    super(name,noLink);
    mydimparam=dimpar;
}
public DimHandler(String name, String noLink, xDimParameter dimpar){
    super(name,noLink);
    mydimparam=dimpar;
}
/**
 * Info handler.
 * Checks the incoming name and format against the stored ones.
 * Table field and rate meter are updated, if known.
 */
public void infoHandler(){
int[] intArr; 
if(skip)return;
String format=new String("NOFORM");
String node;
String lname=new String("LNAME");
String pname=new String("PNAME");
// check name and format
// quality is sent correctly for broken links.
// whith first reconnect LSBit seems to be zero!
// Fortunately we do not use this bit
try{
	quality=super.getQuality();
    lname=super.getName();
    format=getFormat();
	if(format == null)
	    System.out.println("ERROR: no format for "+lname);
	else if(pars == null)
	    System.out.println("ERROR: no parser for "+lname);
	else if(!pars.getFull().equals(lname))
	    System.out.println("ERROR name: "+pars.getFull()+" != "+lname);
	else if(!pars.getFormat().equals(format))
	    System.out.println("ERROR format: "+pars.getFormat()+" != "+format);
} catch (NullPointerException e){
    System.out.println("ERROR: NULL "+lname+" qual "+quality);
	if(format == null)
	    System.out.println("ERROR: no format for "+lname);
	else System.out.println("ERROR: format "+format);
    return;
}
pars.parseQuality(quality);  

if(dolog)System.out.print(pars.getFull()); // diagnostics

	if(pars.isCommandDescriptor()){
        int len=pars.getFull().length();
        String s= new String(getString());
        if(!s.equals(sNolink)){
            // we assume that the parameter name is the command name followed by an _
            xmlpars.parseXmlString(pars.getFull().substring(0,len-1),s);
            // System.out.println(xmlpars.getCommandName()+" "+xmlpars.getCommandScope()+" args "+xmlpars.getNargs());
            // for(int i=0;i<xmlpars.getNargs();i++)
            // System.out.println(xmlpars.getArgumentName(i)+"."+
            // xmlpars.getArgumentType(i)+"."+xmlpars.getArgumentValue(i)+"."+xmlpars.getArgumentRequired(i));
            value=new String(xmlpars.getCommandName());
        } //else quality=-1;
    }

    else if(pars.isState()){
        int severity=getInt();
        if(recsta==null){
            recsta=new xRecordState(pars.getFull(), pars.getType());
            paint=pars.isMonitor();
        }
        if(severity == iNolink){
            color=new String("Gray");
            value=sNolink;
            severity=0;
            //quality=-1;
        } else {
            color=new String(getString());
            value=new String(getString());
        }
        if(paint && (stat==null)) createState(true);
        if(stat!=null)stat.redraw(severity,color,value,isactive);
        recsta.setValue(severity,color,value);
    }

    else if(pars.isInfo()){
        int severity=getInt();
        if(recinf==null){
            recinf=new xRecordInfo(pars.getFull(), pars.getType());
            paint=pars.isMonitor();
        }
        if(severity == iNolink){
            color=new String("Gray");
            value=new String(sNolink+" "+pars.getFull());
            severity=0;
            //quality=-1;
        } else {
            color=new String(getString());
            value=new String(getString());
        }
        if(paint && (info==null)) createInfo(true);
        if(info!=null)info.redraw(severity,color,value,isactive);
        recinf.setValue(severity,color,value);
    }

    else if(pars.isRate()){
        float val=getFloat();
        if(val==fNolink){
            val=0;
            //quality=-1;
        } else {
        if(!meterInit){
            meterMode=getInt();
            meterLow=getFloat();
            meterUp=getFloat();
            meterAlarmLow=getFloat();
            meterAlarmUp=getFloat();
            meterColor=new String(getString());
            meterAlarmColor=new String(getString());
            meterUnits=new String(getString());
            meterInit=true;
        }}
        if(recmet == null){
            recmet = new xRecordMeter(pars.getFull(), pars.getType(), meterMode,
                meterLow, meterUp, meterAlarmLow, meterAlarmUp, 
                meterColor, meterAlarmColor, meterUnits);
            paint=pars.isMonitor();
           }
        recmet.setValue(val);
        value=String.valueOf(val);
        if(paint && (meter==null)) {
            createMeter(true);
            meter.setColor(recmet.getColor());
            int i = pars.getNode().indexOf(".");
            if(i >=0) node=pars.getNode().substring(0,i);
            else node=pars.getNode();
            meter.setLettering(node,pars.getApplication(),pars.getName(),recmet.getUnits());
            meter.setDefaultLimits(recmet.getLower(),recmet.getUpper());
            meter.setDefaultAutoScale(false);
            meter.setDefaultMode(recmet.getMode());
        }
        if(meter!=null){
            meter.redraw((double)val,(quality!=-1), isactive);
            recmet.setSize(meter.getDimension());
            recmet.setPosition(meter.getPosition());
        }
    }

    else if(pars.isHistogram()){
        value=new String("Histogram");
        histoChannels=getInt();
        if(histoChannels == iNolink){
            histoChannels=10;
            intArr=new int[10];
            //quality=-1;
        } else {
            histoLow=getFloat();
            histoHigh=getFloat();
            histoLetter=new String(getString());
            histoContent=new String(getString());
            histoColor=new String(getString());
            intArr=getIntArray();
            if(rechis==null) {
                rechis = new xRecordHisto(pars.getFull(), pars.getType(),
                        histoLow,histoHigh,histoLetter,histoContent,histoColor);
                paint=pars.isMonitor();
            }
        }
        if(paint && (histo==null)) createHisto(true);
        if(histo!=null){
            histo.redraw(histoChannels,intArr, isactive);
            rechis.setSize(histo.getDimension());
            rechis.setPosition(histo.getPosition());
        }
        if(rechis!=null)rechis.setValue(histoChannels,intArr);
    }
    else if(pars.isStruct()){
        value=new String("structure");
        String[] types=pars.getTypeList();
        int[] sizes=pars.getTypeSizes();
        print(super.getName()+" "+types.length+" "+format,LF);
        for(i=0;i<types.length;i++){
            print(" "+i+": ",false);
            if(types[i].equals("I")){
                if(sizes[i]==1)print(" "+getInt(),false);
                else {intarr=getIntArray();for(ii=0;ii<sizes[i];ii++)print(" "+intarr[ii],false);}
            }
            if(types[i].equals("X")){
                if(sizes[i]==1)print(" "+getLong(),false);
                else {longarr=getLongArray();for(ii=0;ii<sizes[i];ii++)print(" "+longarr[ii],false);}
            }
            if(types[i].equals("F")){
                if(sizes[i]==1)print(" "+getFloat(),false);
                else {floatarr=getFloatArray();for(ii=0;ii<sizes[i];ii++)print(" "+floatarr[ii],false);}
            }
            if(types[i].equals("D")){
                if(sizes[i]==1)print(" "+getDouble(),false);
                else {doublearr=getDoubleArray();for(ii=0;ii<sizes[i];ii++)print(" "+doublearr[ii],false);}
            }
            if(types[i].equals("C")){
                if(sizes[i]>0)print(getString(),false);
                else {stringarr=getStringArray();for(ii=0;ii<stringarr.length;ii++)print(" "+stringarr[ii],false);}
            }
            print(" ",LF);
        }
    } // struct
    else {
        if(pars.isArray()){
            value=new String("array");
            if(pars.isInt()){
                intarr=getIntArray();
                print(super.getName()+" I size "+intarr.length+" " + format,LF);
                print("					"+intarr[0]+" "+intarr[1]+" "+intarr[2] ,LF);
            }
            else if(pars.isLong()){
                longarr=getLongArray();
                print(super.getName()+" X size "+longarr.length+" " + format,LF);
                print("					"+longarr[0]+" "+longarr[1]+" "+longarr[2] ,LF);
            }
            else if(pars.isFloat()){
                floatarr=getFloatArray();
                print(super.getName()+" F size "+floatarr.length+" " + format,LF);
                print("					"+floatarr[0]+" "+floatarr[1]+" "+floatarr[2] ,LF);
            }
            else if(pars.isDouble()){
                doublearr=getDoubleArray();
                print(super.getName()+" D size "+doublearr.length+" " + format,LF);
                print("					"+doublearr[0]+" "+doublearr[1]+" "+doublearr[2],LF );
            }
        } // array
        else{
            if(pars.isInt())   {
                ivalue=getInt();
                value=String.valueOf(ivalue);    print(super.getName()+" I = "+value+" " + format,LF);
                if(meter!=null)meter.redraw((double)ivalue);
            }
            else if(pars.isLong())  {
                lvalue=getLong();
                value=String.valueOf(lvalue);   print(super.getName()+" L = "+value+" " + format,LF);
                if(meter!=null)meter.redraw((double)lvalue);
            }
            else if(pars.isChar())  {
                value=new String(getString());     print(super.getName()+" C = "+value+" " + format,LF);
            }
            else if(pars.isFloat()) {
                fvalue=getFloat();
                value=String.valueOf(fvalue);  print(super.getName()+" F = "+value+" " + format,LF);
                if(meter!=null)meter.redraw((double)fvalue);
            }
            else if(pars.isDouble()) {
                dvalue=getDouble();
                value=String.valueOf(dvalue); print(super.getName()+" D = "+value+" " + format,LF);
                if(meter!=null)meter.redraw(dvalue);
            }
        }	// scalar
    } // no struct, no array
    if(isactive){
    if(pars.isLogging()) xLogger.print(pars.getState(),new String(pars.getFull()+": "+value));
    if((tabmod!=null)&&(tabrow != -1)){
    if(tabrow >= tabmod.getRowCount())
        System.out.println("Table rows:"+tabmod.getRowCount()+" index:"+tabrow);
        try{ 
//        if(pars.getName().equals("CfgNodeId.BnetPlugin"))printParameter(false);
            tabmod.setValueAt(value, tabrow, 5);
        } catch (ArrayIndexOutOfBoundsException e){
            System.out.println("Exception Table index: "+tabrow+" "+super.getName());}
    }
    // call user info handlers if attached:
    if(userHandler != null)
        for(int i=0;i<userHandler.size();i++)userHandler.get(i).infoHandler(mydimparam);
    }
if(dolog)System.out.println(" "+value);
} // info handler
} // class DimHandler
} // class xDimParameter
