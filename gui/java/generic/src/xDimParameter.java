package xgui;
/**
*
* @author goofy
*/
import java.awt.Color;
import java.awt.Dimension;
import java.util.*;
import dim.*;
/**
* DIM parameter class implemeting infoHandler. It has reference to table model and a Meter.
* On parameter update, table and meter are updated.
*/
public class xDimParameter extends DimInfo implements xiDimParameter {
private xParser pars, compars;
private xXmlParser xmlpars;
private boolean isactive=false;
private int i,ii;
private int quality=-1;
private boolean doprint=false;
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
private String editvalue;
private String setcommand;
private int tabrow=-1, myparindex=-1;
private boolean tabinit=false;
private xParTable tabmod;
private xMeter meter=null;
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
private xDispHisto histo=null;
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
private xPanelHistogram hispan;
private xPanelState stapan;
private xPanelInfo infpan;
private String color;
private String sNolink;
private int    iNolink;
private float  fNolink;
private int version;
private xiUserInfoHandler userHandler=null;
private xRecordHisto rechis=null;
private xRecordMeter recmet=null;
private xRecordState recsta=null;
private xRecordInfo recinf=null;
public static final boolean LF=true;

public void printParameter(int index){
System.out.println(index+": "+super.getName());
if(quality != 2304){
System.out.println("  I:"+myparindex+" TI:"+tabrow+" Q:"+quality+" Active:"+isactive+" Shown: "+paraShown);
System.out.println("  Value: "+value);
}
}
public void printParameter(boolean comdef){
if(comdef || (quality != 2304))
System.out.println(super.getName()+" I:"+myparindex+" TI:"+tabrow+" Q:"+String.format("%08x",quality)+
" Active:"+isactive+" Shown: "+paraShown+" Value: "+value);
// exclude command descriptors
// if(quality != 2304) {
// if(pars.getName().equals("RunStatus"))
// System.out.println(super.getName()+"  Value: "+value);
//}
}

// protected void finalize(){
// if(pars.getName().equals("Transport"))
// System.out.println("    "+version+" finpar "+pars.getFull());
// pars=null;
// }
/**
 * DIM integer parameter
 * @param name DABC format full parameter name
 * @param format DIM format list
 * @param index index of parameter object in DimGui table. Stored.
 * @param noLink default value if no connection to DIM server
 */
xDimParameter(String name, String format, int noLink, int v){
    super(name,noLink);
    version=v;
    sNolink=new String("%BROKEN%");
    iNolink=noLink;
    initParser(name,format);
// if(pars.getName().equals("NodeList"))
// System.out.println(version+" crepar "+pars.getFull());
}
/**
 * DIM integer parameter
 * @param name DABC format parameter name
 * @param format DIM format list
 * @param index index of parameter object in DimGui table. Stored.
 * @param noLink default value if no connection to DIM server
 */
xDimParameter(String name, String format, float noLink, int v){
    super(name,noLink);
    version=v;
    sNolink=new String("%BROKEN%");
    fNolink=noLink;
    initParser(name,format);
}
/**
 * DIM integer parameter
 * @param name DABC format parameter name
 * @param format DIM format list
 * @param index index of parameter object in DimGui table. Stored.
 * @param noLink default value if no connection to DIM server
 */
xDimParameter(String name, String format, String noLink, int v){
    super(name,noLink);
    version=v;
    sNolink=new String(noLink);
    initParser(name,format);
}
/**
 * Imports the references to the graphic panels. Called from browser.
 * @param histogram
 * @param meter
 * @param state
 */
public void setPanels(xPanelHistogram histogramPanel, xPanelMeter meterPanel, xPanelState statePanel, xPanelInfo infoPanel){
    hispan=histogramPanel;
    metpan=meterPanel;
    stapan=statePanel;
    infpan=infoPanel;
}
public void addInfoHandler(xiUserInfoHandler pu){
userHandler=pu;
}
public void removeInfoHandler(xiUserInfoHandler pu){
userHandler=null;
}
public void deactivateParameter() {isactive=false;}
public void activateParameter() {
if(info!=null)info.redraw();
if(meter!=null)meter.redraw();
if(stat!=null)stat.redraw();
if(histo!=null)histo.redraw();
isactive=true;
}
/**
 * DIM integer parameter
 * @param name DABC format parameter name
 * @param format DIM format list
 * @param index index of parameter object in DimGui table. Stored.
 */
public void initParser(String name, String format){
    pars = new xParser();
    xmlpars = new xXmlParser();
    i=pars.parse(name,xParser.PARSE_STORE_FULL); // check, parse and store
// if(pars.getName().equals("Transport"))System.out.println("Transport "+version);
    // create command to set parameter
    pars.setName(new String("_"+pars.getName()));
    setcommand=pars.getFull(true);
    i=pars.parse(name,xParser.PARSE_STORE_FULL); // check, parse and store
    i=pars.format(format,xParser.PARSE_STORE_FULL); // check, parse and store
    editvalue=new String("-");
    value=new String(sNolink);
}
/**
 * Adds a new row to the table
 * @param table Table model assigned to this parameter
 * @param rowindex index of row in table
 * @return true parameter is visible and has been added
 * @return false parameter is not visible and has not been added
 */
public boolean addRow(xParTable table, int rowindex){
boolean ret=false;
boolean show=false;
    if(pars.isVisible()){
        tabmod=table;
        tabinit=true; // otherwise following elements would not be put to panels
        if(pars.isRate() && pars.isMonitor()) {createMeter(true);}
        if(pars.isInfo() && pars.isMonitor()) {createInfo(true);}
        if(pars.isState() && pars.isMonitor()) {createState(true);}
        if(pars.isHistogram() && pars.isMonitor()) {createDispHisto(true);}
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
        print(super.getName()+" i="+rowindex,LF);
        ret = true;
    }
return ret;
}
/**
 * Called by PanelParameter after sorting
 * @param index parameter index shown as text in first column.
 */
public void setIndex(int index) {
myparindex=index;
paraShown=false;
}
/**
 * Called by PanelParameter after sorting
 * @param index table index needed to update correct row.
 */
public void setTableIndex(int index) {tabrow=index;}
/**
 * Execute DIM command from internal parameter string .
 * @param arg string for command argument
 */
public boolean setParameter(String arg){
//    System.out.println(setcommand+" "+arg);
if(pars.isChangable()){
    editvalue=arg;
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
 * @param create Create or remove meter (from panel).
 */
public void createMeter(Boolean create){
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
            meter.setColor(meterColor);
            int i = pars.getNode().indexOf(".");
            if(i >=0) node=pars.getNode().substring(0,i);
            else node=pars.getNode();
            meter.setLettering(node,pars.getApplication(),pars.getName(),meterUnits);
            if(meterLow<meterUp){meter.setLimits(meterLow,meterUp);meter.setAutoScale(false);}
            meter.setMode(meterMode);
        }
        if(tabinit){
            if(!paraShown)metpan.addMeter(meter);
            paraShown=true;
        }
    } else if(paraShown && (meter != null)) {metpan.removeMeter(meter);paraShown=false;}
}
}
/**
 * @param create Create or remove histogram (from panel).
 */
public void createDispHisto(Boolean create){
int len;
if(pars.isHistogram()){
    if(create){
        if(histo == null){
            len=pars.getNodeName().indexOf(".");
            if(len == -1)len=pars.getNodeName().length();
            histo = new xDispHisto(pars.getFull(), new String(pars.getNodeName().substring(0,len)+":"+pars.getName()),
                    histoContent,histoLetter,200,150);
            histo.setColorBack(xSet.getColorBack());
            histo.setColor(histoColor);
        }
        if(tabinit){
            if(!paraShown)hispan.addHistogram(histo);
            paraShown=true;
        }
    } else if(paraShown && (histo != null)) {hispan.removeHistogram(histo); paraShown=false;}
}
}
/**
 * @param create Create or remove state (from panel).
 */
public void createState(Boolean create){
int len;
if(pars.isState()){
    if(create){
        if(stat == null){
            len=pars.getNodeName().indexOf(".");
            if(len == -1)len=pars.getNodeName().length();
            stat = new xState(new String(pars.getNodeName().substring(0,len)+":"+pars.getApplicationName()),
                xState.XSIZE,xState.YSIZE);
            stat.setColorBack(xSet.getColorBack());
        }
        if(tabinit){
            if(!paraShown)stapan.addState(stat);
            paraShown=true;
        }
    } else if(paraShown && (stat != null)) {stapan.removeState(stat);paraShown=false;}
}
}
public void createInfo(Boolean create){
int len;
if(pars.isInfo()){
    if(create){
    //System.out.println("Create "+super.getName());
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
public boolean isCommandDescriptor(){return pars.isCommandDescriptor();}
/**
 * @return parameter name from parser.
 */
public String getName(){return pars.getName();}
/**
 * @return node:ID form parser.
 */
public String getNode(){return pars.getNode();}
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
public int getQuality(){return quality;}
/**
 * The first row of the table is the DimParameter object. The string seen in the table
 * is the string returned by this function.
 * @return index as string used for table.
 */
public String toString(){return String.format("%05d",myparindex);}
/**
 * @return parser
 */
public xiParser getParserInfo(){return pars;}
/**
 * @return parser object.
 */
public xParser getParser(){return pars;}
/**
 * @return command parser object.
 */
public xXmlParser getXmlParser(){return xmlpars;}

public void setPrint(boolean dop){doprint=dop;}

private void print(String s){print(s,false);}

private void print(String s, boolean lf){
    if(doprint){
        if(lf)System.out.println(s);
        else System.out.print(s);
    }}
//
public xRecordMeter getMeter(){return recmet;}
public xRecordState getState(){return recsta;}
public xRecordInfo getInfo(){return recinf;}
/**
 * Info handler.
 * Checks the incoming name and format against the stored ones.
 * Optionally (setPrint(true)) printout parameter (all formats, structures).
 * Table field and rate meter are updated, if known.
 */
public void infoHandler(){
int[] intArr; 
String format;
// if(pars.getName().equals("RunStatus"))
// System.out.println(isactive+" q "+super.getQuality()+" "+super.getName()+"  Value: "+value);
// check name and format
// quality is sent correctly for broken links.
// whith first reconnect LSBit seems to be zero!
// Fortunately we do not use this bit
// if((quality!=-1)&&(quality!=super.getQuality()))
// System.out.println("Mismatch quality: "+pars.getFull()+" was "+quality+" new "+super.getQuality());
// if(pars.getName().equals("CfgNodeId.BnetPlugin"))System.out.println(pars.getFull()+" t="+tabrow+" "+value);
//if(pars.getName().equals("RunStatus"))System.out.println(pars.getFull()+" t="+tabrow+" "+value);
    quality=super.getQuality();
    //System.out.println(quality+" "+super.getName());
    pars.parseQuality(quality);
    
    // if(pars.getFull().indexOf("X86G-4/Acquis")>0) System.out.print(pars.getFull());
    // if(pars.getFull().indexOf("/RunStatus")>0) System.out.println(pars.getFull());
    format=getFormat();
    if(!pars.getFull().equals(super.getName()))
        System.out.println("ERROR: "+pars.getFull()+" != "+super.getName());
    else if(!pars.getFormat().equals(format))
        System.out.println("ERROR: "+pars.getFormat()+" != "+format);

    else if(pars.isCommandDescriptor()){
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
        if(recsta==null)recsta=new xRecordState(pars.getFull(), pars.getType());
        if(severity == iNolink){
            color=new String("Gray");
            value=sNolink;
            severity=0;
            //quality=-1;
        } else {
            color=new String(getString());
            value=new String(getString());
        }
    // if(pars.getFull().indexOf("X86G-4/Acquis")>0) System.out.println(":"+value);
    // if(pars.getFull().indexOf("/RunStatus")>0) System.out.println(pars.getFull()+":"+value);
        if(pars.isMonitor() && (stat==null)) createState(true);
        if(stat!=null)stat.redraw(severity,color,value,isactive);
        recsta.setValue(severity,color,value);
    }

    else if(pars.isInfo()){
        int severity=getInt();
        if(recinf==null)recinf=new xRecordInfo(pars.getFull(), pars.getType());
        if(severity == iNolink){
            color=new String("Gray");
            value=sNolink;
            severity=0;
            //quality=-1;
        } else {
            color=new String(getString());
            value=new String(getString());
        }
        if(pars.isMonitor() && (info==null)) createInfo(true);
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
        if(recmet == null)
            recmet = new xRecordMeter(pars.getFull(), pars.getType(), meterMode,
                meterLow, meterUp, meterAlarmLow, meterAlarmUp, 
                meterColor, meterAlarmColor, meterUnits);
        recmet.setValue(val);
        value=String.valueOf(val);
        if(pars.isMonitor() && (meter==null)) createMeter(true);
        if(meter!=null)meter.redraw((double)val,(quality!=-1), isactive);
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
            if(rechis==null) rechis = new xRecordHisto(pars.getFull(), pars.getType(),
                        histoLow,histoHigh,histoLetter,histoContent,histoColor);
        }
        if(pars.isMonitor() && (histo==null)) createDispHisto(true);
        if(histo!=null)histo.redraw(histoChannels,intArr, isactive);
        rechis.setValue(histoChannels,intArr);
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
    if(userHandler != null)userHandler.infoHandler(this);
    }
//print(value,LF);
//} else System.out.println("Deprecated parameter: "+super.getName());
} // info handler
} // class xDimParameter
