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
/**
 * Parser for (de)composing DIM service names.
 * The DIM service names of DABC follow a defined structure,
 * mainly Dns/Node[:NodeId]/[applNS::]Application[:ApplicationId]/Name.
 * @author H.G.Essel
 * @version 1.0
*/
public class xParser implements xiParser{

private String fullname;
private String formlist;
private String comname;
private String dns;
private String node;
private String nodename;
private String nodeID;
private String appl;
private String applfull;
private String applname;
private String applID;
private String applNS;
private String itemname;
private int nargs;
private String[] items;
private String[] dtype;
private int[] dsize;
private int quality;
private int type;
private int state;
private int visibility;
private int mode;
private boolean isarray=false;
private boolean isfloat=false;
private boolean isdouble=false;
private boolean isint=false;
private boolean islong=false;
private boolean ischar=false;
private boolean isstruct=false;
private boolean iscommand=false;
/** Parser steering switch to store full name inside. */
public static final boolean PARSE_STORE_FULL=true;
/** Parser steering switch to parse only. */
public static final boolean PARSE_ONLY=false;
/** Used for command style names. */
public static final boolean IS_COMMAND=true;
/** Used for parameter style names. */
public static final boolean IS_PARAMETER=false;
/** Build name from components. */
public static final boolean MAKE_FULL=true;
/** Do not build name from components */
public static final boolean COPY_FULL=false;
/** Values for DIM quality mask 0xff (state field): state undefined. */
public static final int NOTSPEC=0;
/** Values for DIM quality mask 0xff (state field): success. */
public static final int SUCCESS=1;
/** Values for DIM quality mask 0xff (state field): info. */
public static final int INFORMATION=2; 
/** Values for DIM quality mask 0xff (state field): warning. */
public static final int WARNING=3;  
/** Values for DIM quality mask 0xff (state field): error. */
public static final int ERROR=4;    
/** Values for DIM quality mask 0xff (state field): fatal. */
public static final int FATAL=5; 
/** Values for DIM quality mask 0xff00 (type field): atomic data type. */
public static final int ATOMIC=0; 
/** Values for DIM quality mask 0xff00 (type field): generic data encoded in string (XML). */
public static final int GENERIC=1; 
/** Values for DIM quality mask 0xff00 (type field): state structure. */
public static final int STATE=2;  
/** Values for DIM quality mask 0xff00 (type field): rate structure. */
public static final int RATE=3;  
/** Values for DIM quality mask 0xff00 (type field): histogram structure. */
public static final int HISTOGRAM=4;  
/** Values for DIM quality mask 0xff00 (type field): module structure. */
public static final int MODULE=5;  
/** Values for DIM quality mask 0xff00 (type field): port structure. */
public static final int PORT=6;
/** Values for DIM quality mask 0xff00 (type field): device structure. */
public static final int DEVICE=7;
/** Values for DIM quality mask 0xff00 (type field): queue structure. */
public static final int QUEUE=8;
/** Values for DIM quality mask 0xff00 (type field): command descriptor. */
public static final int COMMANDDESC=9;
/** Values for DIM quality mask 0xff00 (type field): info structure. */
public static final int INFO=10;
/** Values for DIM quality mask 0xff0000 (visibility field): to show. */
public static final int VISIBLE=1; 
/** Values for DIM quality mask 0xff0000 (visibility field): to monitor. */
public static final int MONITOR=2; 
/** Values for DIM quality mask 0xff0000 (visibility field): can be modified. */
public static final int CHANGABLE=4;  
/** Values for DIM quality mask 0xff0000 (visibility field): is important. */
public static final int IMPORTANT=8;  
/** Values for DIM quality mask 0xff0000 (visibility field): changes in logfile. */
public static final int LOGGING=16;  
/** Values for DIM quality mask 0xff000000 (mode field): not used. */
public static final int NOMODE=0; 

public boolean isNotSpecified(){return (state == NOTSPEC);}
public boolean isSuccess()     {return (state == SUCCESS);}
public boolean isInformation() {return (state == INFORMATION);}
public boolean isWarning()     {return (state == WARNING);}
public boolean isError()       {return (state == ERROR);}
public boolean isFatal()       {return (state == FATAL);}
public boolean isAtomic()      {return (type   == ATOMIC);}
public boolean isGeneric()     {return (type   == GENERIC);}
public boolean isState()       {return (type   == STATE);}
public boolean isInfo()        {return (type   == INFO);}
public boolean isRate()        {return (type   == RATE);}
public boolean isHistogram()   {return (type   == HISTOGRAM);}
public boolean isCommandDescriptor()  {return (type   == COMMANDDESC);}
public boolean isHidden()      {return (visibility == 0);}
public boolean isVisible()     {return (visibility > 0);}
public boolean isMonitor()     {return ((visibility & MONITOR) > 0);}
public boolean isChangable()   {return ((visibility & CHANGABLE) > 0);}
public boolean isImportant()   {return ((visibility & IMPORTANT) > 0);}
public boolean isLogging()     {return ((visibility & LOGGING) > 0);}

// protected void finalize(){
// if(itemname != null) System.out.println("finalize parser "+itemname);
// else System.out.println("finalize parser ");
// }
/**
 * Create an empty parser object.
 */
public xParser(){
}
/**
 * Create a parser object and store string.
 * String must have the format <p> <
 * Dns/Node[:NodeId]/[applNS::]Application[:ApplicationId]/Name > <p>
 * for standard parameters and <p> <
 * Dns/Name/[applNS::]Application[:ApplicationId]/Node[:NodeId] > <p>
 * for commands (only used for sorting purposes)
 * @param full full string to be parsed later.<p>
 */
public xParser(String full){
    setFull(full);
}
/**
 * Parse internal full string which must have been set by constructor or setFull(string).
 * The items of the string are copied into internal string items and can be retrieved by getter methods.
 * return code:
 * @return 0 OK, -1 syntax error
 */
public int parse(){
    String full=fullname;
    return parse(full,PARSE_ONLY);
}
/**
 * Parse given full string.
 * The items of the string are copied into internal strings.
 * return code:
 * @param full string to parse.
 * String must have the format <p> <
 * Dns/Node[:NodeId]/[applNS::]Application[:ApplicationId]/Name >
 * @param store PARSE_STORE_FULL: store composed strings (parameter and command format), PARSE_ONLY: parse only
 * @return 0 OK, -1 syntax error
 */
public int parse(String full, boolean store){
    return parse(full,store,IS_PARAMETER);
}
/**
 * Parse given full string.
 * The items of the string are copied into internal strings.
 * @param full string
 * String must have the format <p> <
 * Dns/Node[:NodeId]/[applNS::]Application[:ApplicationId]/Name >
 * @param store PARSE_STORE_FULL: store composed strings (parameter and command format), PARSE_ONLY: parse only
 * @param command IS_COMMAND: given string is command format <p> <
 * Dns/Name/[applNS::]Application[:ApplicationId]/Node[:NodeId] > <p>
 * IS_PARAMETER: standard
 * @return 0 OK,  -1 syntax error
 */
public int parse(String full, boolean store, boolean command){
// in case of parsing error, getFull and getCommand return empty string
   fullname=new String("");
   comname=new String("");
    String[] nodec;
    String[] applc;
    nodeID=null;
    applID=null;
    applNS=null;
    iscommand=command;
    items=full.split("/");
    if(items.length < 4){
        //System.out.println("Wrong syntax, field missing: "+full);
        return -1;}
    if(command){
        node=new String(items[3]);
        itemname = new String(items[1].replace("%","/"));
    }else{
        node=new String(items[1]);
        itemname = new String(items[3]);
    }
    dns = new String(items[0]);
    applfull=new String(items[2]);
    nodec=node.split(":");
    applc=applfull.split(":");
    nodename = new String(nodec[0]);
    if(nodec.length < 1) {
        System.out.println("Wrong syntax, node ID missing: "+items[1]);
        return -1;
        // nodeID = new String("");
        }
    if(nodec.length == 2) nodeID = new String(nodec[1]);
    applname = new String(applc[0]);
    if(applc.length < 1) {
        System.out.println("Wrong syntax, application ID missing: "+items[2]);
        return -1;
        // applname = new String(applc[0]);
        // applID = new String("");
        // appl = new String(applname);
        }
    if(applc.length == 2) {
        applID = new String(applc[1]);
        appl = new String(applname+":"+applID);
    }
    if(applc.length == 4) {
        applNS = new String(applc[0]); // namespace separated by ::
        applname = new String(applc[2]);
        applID = new String(applc[3]);
        appl = new String(applname+":"+applID);
    }
    if(items.length > 4)
        for(int i = 4; i< items.length; i++)itemname=itemname+"/"+items[i];
    if(applname.length() == 0) {
        System.out.println("Wrong syntax, application field missing: "+items[2]);
        return -1;}
    if(nodename.length() == 0) {
        System.out.println("Wrong syntax, node field missing: "+items[1]);
        return -1;}
    if(store) {
        buildFull();
        buildCommand();
    }
    return 0;
}
/**
 * parse internal DIM format string set by setFormat(string).
 * The items of the string are copied into internal string arrays and can be retreived by getter methods.  <p>
 * DIM format strings have format <p> <
 * T:S;T:S;...> <p>
 * For scalar data set default size to 1 (eventually this should be 0?);
 * for structures, the last item size defaults to 0 indicating that it has arbitrary size
 * @return 0 OK,  -1 syntax error
 */
public int format(){
    String form=formlist;
    return format(form, PARSE_ONLY);
}
/**
 * parse given DIM format string.
 * The items of the string are copied into internal string arrays and can be retreived by getter methods. <p>
 * DIM format strings have format <p> <
 * T:S;T:S;...> <p>
 * For scalar data set default size to 1 (eventually this should be 0?);
 * for structures, the last item size defaults to 0 indicating that it has arbitrary size
 * @param formstring DIM format string
 * @param store PARSE_STORE_FULL: store formstring, PARSE_ONLY: parse only
 * @return 0 OK,  -1 syntax error
 */
public int format(String formstring, boolean store){
    String[] items;
    String[] elem;
// DIM browser returns L for int, but DimInfo handler returns I
    String formstr=formstring.replace("L","I");
    items=formstr.split("[;,]");
    nargs=items.length;
    if(nargs > 1) isstruct=true;
    dtype=new String[nargs];
    dsize=new int[nargs];
    for(int i=0;i<nargs; i++){
        elem=items[i].split(":");
        dtype[i]=elem[0];
        if(elem.length ==2)dsize[i]=Integer.parseInt(elem[1]);
        else{
            if(isstruct) dsize[i]=0; // arbitrary length
            else dsize[i]=1;
        }
        if(!isstruct){
            if(dsize[0]!=1)isarray=true;
            if(dtype[0].startsWith("I"))isint=true;
            if(dtype[0].startsWith("X"))islong=true;
            if(dtype[0].startsWith("F"))isfloat=true;
            if(dtype[0].startsWith("D"))isdouble=true;
            if(dtype[0].startsWith("C"))ischar=true;
        }
    }
    if(store)formlist=formstr;
    return 0;}
/**
 * @return DNS
 */
public String getDns(){return dns;}
/**
 * @return Node spec, which is  name:port
 */
public String getNode(){
    if(nodeID == null) return new String(nodename);
    else               return new String(nodename+":"+nodeID);
}
/**
 * @return Node name
 */
public String getNodeName(){return nodename;}
/**
 * @return Node ID
 */
public String getNodeID(){return nodeID;}
/**
 * returns Application full, which is namespace::name:port
 * @return Application full, which is namespace::name:port
 */
public String getApplicationFull(){
    if(applNS == null) {
        if(applID == null) return new String(applname);
        else               return new String(applname+":"+applID);
    }else{
        if(applID == null) return new String(applNS+"::"+applname);
        else               return new String(applNS+"::"+applname+":"+applID);
    }
}
/**
 * returns Application spec, which is name:port
 * @return Application spec, which is name:port
 */
public String getApplication(){
    if(applID == null) return new String(applname);
    else               return new String(applname+":"+applID);
}
/**
 * @return Application name
 */
public String getApplicationName(){return applname;}
/**
 * @return Application ID
 */
public String getApplicationID(){return applID;}
/**
 * @return parameter name
 */
public String getName(){return itemname;}
/**
 * @return application namespace name
 */
public String getNameSpace(){return applNS;}
/**
 * @return item list (as separated by / in full name)
 */
public String[] getItems(){return items;}
/**
 * @return internal full string
 */
public String getFull(){return fullname;}
/**
 * @param build MAKE_FULL: build string form fields and return, COPY_FULL: return current internal string
 * @return full string
 */
public String getFull(boolean build){
    if(build) buildFull();
    return fullname;
}
/**
 * @return internal full command string
 */
public String getCommand(){return comname;}
/**
 * @param build MAKE_FULL: build string form fields and return, COPY_FULL: return current internal string
 * @return full string
 */
public String getCommand(boolean build){
    if(build) buildCommand();
    return comname;
}
/**
 * set DNS name
 * @param DNS
 */
public void setDns(String DNS){dns=DNS;}
/**
 * set node name
 * @param node
 */
public void setNodeName(String node){nodename=node;}
/**
 * set node ID
 * @param nodeID
 */
public void setNodeID(String nodeID){nodeID=nodeID;}
/**
 * set application name
 * @param application
 */
public void setApplicationName(String application){applname=application;}
/**
 * set application ID
 * @param applicationID
 */
public void setApplicationID(String applicationID){applID=applicationID;}
/**
 * set parameter name
 * @param name
 */
public void setName(String name){itemname=name;}
/**
 * set application namespace name
 * @param name
 */
public void setNameSpace(String name){applNS=name;}
/**
 * store quality built from state, type, visibility, mode
 * @param s state: UpToDate, Unsolicited, Obsolete, Invalid, Undefined
 * @param t type: Plain, State, Rate, Histogram, Module
 * @param v visibility: Monitored, Changable
 * @param m mode: not yet used
 */
public void buildQuality(int s, int t, int v, int m){
    state=s;
    type=t;
    visibility=v;
    mode=m;
    quality= s | (t << 8) | (v << 16) | (m << 24);
}
/**
 * store quality and parse into state, type, visibility, mode
 * @param qual DIM quality
 */
public void parseQuality(int qual){
if(qual != -1){
    quality   = qual;
    state    =  quality       & 0xff;
    type      = (quality>>>8)  & 0xff;
    visibility= (quality>>>16) & 0xff;
    mode      = (quality>>>24) & 0xff;
}}
/**
 * get structure type (from quality))
 * @return type <p>
 * ATOMIC=0; <p>
 * GENERIC=1; <p>
 * STATE=2;  <p>
 * RATE=3;  <p>
 * HISTOGRAM=4;  <p>
 * MODULE=5;  <p>
 * PORT=6; <p>
 * DEVICE=7; <p>
 * QUEUE=8;
 */
public int getType(){return type;}
public String getTypeName(){
if(isRate())return "Rate";
else if(isState())return "State";
else if(isInfo())return "Info";
else if(isHistogram())return "Histo";
return "Unknown";
}
/**
 * get state (from quality))
 * @return state <p>
 * NOTSPEC=0;  <p>
 * UPTODATE=1;  <p>
 * UNSOLICITED=2;    <p>
 * OBSOLETE=3;    <p>
 * INVALID=4;    <p>
 * UNDEFINED=5;
 */
public int getState(){return state;}
/**
 * get visibility (from quality))
 * @return visibility <p>
 * HIDDEN=0;  <p>
 * MONITOR=1;  <p>
 * CHANGABLE=2;   <p>
 * IMPORTANT=4;
 */
public int getVisibility(){return visibility;}
/**
 * get mode (from quality))
 * @return mode
 */
public int getMode(){return mode;}
/**
 * get quality
 * @return quality
 */
public int getQuality(){return quality;}
/**
 * set full string
 * @param full
 */
public void setFull(String full){fullname=full;}
/**
 * build full string from items
 */
public void buildFull(){
    fullname=new String(dns+"/"+getNode()+"/"+getApplicationFull()+"/"+itemname);
}
/**
 * build full string from items in command order
 */
public void buildCommand(){
    String item=itemname.replace("/","%"); // to be able to extract itemname between /
    comname=new String(dns+"/"+item+"/"+getApplicationFull()+"/"+getNode());
}
/**
 * set DIM format string
 * @param formatstring
 */
public void setFormat(String formatstring){formlist=formatstring;}
/**
 * get number of types
 * @return Ntypes
 */
public int getNofTypes(){return nargs;}
/**
 * get vector of sizes
 * @return sizes
 */
public int[] getTypeSizes(){return dsize;}
/**
 * get vector of types
 * @return types
 */
public String[] getTypeList(){return dtype;}
/**
 * @return true if array
 */
public boolean isArray(){return isarray;}
/**
 * @return true if float
 */
public boolean isFloat(){return isfloat;}
/**
 * @return true if double
 */
public boolean isDouble(){return isdouble;}
/**
 * @return true if 32 bit int
 */
public boolean isInt(){return isint;}
/**
 * @return true if 64 bit long
 */
public boolean isLong(){return islong;}
/**
 * @return true if char
 */
public boolean isChar(){return ischar;}
/**
 * @return true if struct (all others false)
 */
public boolean isStruct(){return isstruct;}
/**
 * @return formatstring
 */
public String getFormat(){return formlist;}
/**
 * @return full name
 */
public String toString(){return fullname;}
/**
 * @param command IS_COMMAND: return command format (name/appl/node), IS_PARAMETER: standard
 * @return full name in standard or command order
 */
public String toString(boolean command){
    if(command) return comname;
    else return fullname;
}
/**
 * @param indent Indentation level for browser: 0 DNS, 1 node (name for command), 2 application, 3 node (name for command).
 * @param command IS_COMMAND: return command format (name/appl/node), IS_PARAMETER: standard
 * @return lettering depending on indent level
 */
public String toString(int indent, boolean command){
    if(command){
        if(indent == 3)return getNode();
        if(indent == 2)return getApplication();
        if(indent == 0)return dns;
        if(indent == 1)return itemname;
    }else{
        if(indent == 3)return itemname;
        if(indent == 2)return getApplication();
        if(indent == 0)return dns;
        if(indent == 1)return getNode();
    }
    return formlist;
}
/**
 * Print all internal items
 */
public void printItems(){
    System.out.println("fullanme  "+fullname);
    System.out.println("formlist  "+formlist);
    System.out.println("comname   "+comname);
    System.out.println("dns       "+dns);
    System.out.println("node      "+node);
    System.out.println("nodename  "+nodename);
    System.out.println("nodeID    "+nodeID);
    System.out.println("appl      "+appl);
    System.out.println("applfull  "+applfull);
    System.out.println("applname  "+applname);
    System.out.println("applID    "+applID);
    System.out.println("namespace "+applNS);
    System.out.println("itemname  "+itemname);
    System.out.println("isarray   "+isarray);
    System.out.println("isfloat   "+isfloat);
    System.out.println("isdouble  "+isdouble);
    System.out.println("isint     "+isint);
    System.out.println("islong    "+islong);
    System.out.println("ischar    "+ischar);
    System.out.println("isstruct  "+isstruct);
    System.out.println("iscommand "+iscommand);
    System.out.println("nargs     "+nargs);
    for(int i=0;i<nargs;i++){
        System.out.print(" dtype "+dtype[i]);
        System.out.println(" dsize "+dsize[i]);
    }
    System.out.println(" ");
}
}
