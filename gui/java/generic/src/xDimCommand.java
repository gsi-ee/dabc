package xgui;
/**
*
* @author goofy
*/
import java.util.*;
import dim.*;
/**
* DIM command class
*/
public class xDimCommand implements xiDimCommand {
private xParser pars=null;
private xXmlParser xpars=null;
private int i,ii;
private int[] intarr;
private long[] longarr;
private float[] floatarr;
private double[] doublearr;
private String[] stringarr;
private int indent=0;
private int version;
/**
 * Create DIM command object
 * @param name DABC format full command string. String and format are parsed and stored in parser.
 * @param format DIM format list
 */
// protected void finalize(){
// if(pars.getApplication().equals("OpenFile"))
// System.out.println("  "+version+" fincom "+pars.getFull());
// pars=null;
// }
xDimCommand(String name, String format, int v){
    version=v;
    pars = new xParser();
    i=pars.parse(name,xParser.PARSE_STORE_FULL,xParser.IS_COMMAND);
    i=pars.format(format,xParser.PARSE_STORE_FULL);
// if(pars.getApplication().equals("OpenFile"))
// System.out.println(version+" create dimcom "+pars.getFull());
}
/**
 * @param parser XML parser
 */
public void setXmlParser(xXmlParser parser){if(xpars==null)xpars=parser;}
/**
 * @return XML parser
 */
public xXmlParser getXmlParser(){return xpars;}
/**
 * @return parser
 */
public xiParser getParserInfo(){return pars;}
/**
 * @return parser
 */
public xParser getParser(){return pars;}
/**
 * @return data type
 */
public String getType(){return pars.getTypeList()[0];}
/**
 * Execute DIM command from internal parameter string (not from commandstring which is used only for sorting).
 * If the application name is $:0 this was no DABC formatted command like DIM server EXIT and is handled differently.
 * @param arg string for command argument
 */
public void exec(String arg){
int iarg=0;
float farg=0.0F;
if(arg.length()>0){
try{
    if(pars.isInt())iarg=Integer.parseInt(arg);
    else if(pars.isFloat())farg=Float.parseFloat(arg);
} catch(Exception e) {}
}
    if(pars.getApplication().equals("$:0")){ // EXIT commnand
        String cmd=new String(pars.getDns()+"/"+pars.getNode()+"/"+pars.getName());
        System.out.println("command: "+cmd+" ()");
        if(pars.isInt()) DimClient.sendCommand(cmd,iarg);
        else DimClient.sendCommand(cmd,arg);
    }else {
        if(arg.length()>14) System.out.println("command: "+pars.getFull()+" ("+arg.substring(14)+")");
        else  System.out.println("command: "+pars.getFull()+" ()");
        if(pars.isInt()) DimClient.sendCommand(pars.getFull(),iarg);
        else if(pars.isFloat()) DimClient.sendCommand(pars.getFull(),farg);
        else         DimClient.sendCommand(pars.getFull(),arg);
    }
}
/**
 * Set indentation level. This controls which field is returned by toString (used by tree browser).
 * In the browser the order goes from command to application to node.
 */
public void setIndent(int ind){indent=ind;}
/**
 * Return field according indentation and store indentation used in toString()
 */
public String toString(int ind){
    indent=ind;
    return pars.toString(ind,xParser.IS_COMMAND);
}
/**
 * Return field according indentation
 */
public String toString(){return pars.toString(indent,xParser.IS_COMMAND);}
} // class xDimCommand
