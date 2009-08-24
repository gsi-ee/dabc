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
private int Version;
/**
 * Create DIM command object
 * @param name DABC format full command string. String and format are parsed and stored in parser.
 * @param format DIM format list
 * @param version number of instance (debug purpose only)
 */
public xDimCommand(String name, String format, int version){
    Version=version;
    pars = new xParser();
    i=pars.parse(name,xParser.PARSE_STORE_FULL,xParser.IS_COMMAND);
    i=pars.format(format,xParser.PARSE_STORE_FULL);
}
// protected void finalize(){
// if(pars.getApplication().equals("OpenFile"))
// System.out.println("  "+Version+" fincom "+pars.getFull());
// pars=null;
// }
/**
 * Specify XML parser to be used (keeps definitions and values)
 * @param parser XML parser
 */
protected void setXmlParser(xXmlParser parser){if(xpars==null)xpars=parser;}
/**
 * Get XML parser (keeps definitions and values)
 * @return XML parser
 */
protected xXmlParser getXmlParser(){return xpars;}
// xiDimCommand
public xiParser getParserInfo(){return pars;}
/**
 * Get parser keeping fields and formats ov DIM command.
 * @return parser
 */
protected xParser getParser(){return pars;}
/**
 * Used by xPanelCommand.
 * @return data type
 */
protected String getType(){return pars.getTypeList()[0];}

// xiDimCommand
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
 * @param ind indent level for tree browser.
 */
protected void setIndent(int ind){indent=ind;}
/**
 * @param ind indent level for tree browser.
 * @return field according indentation and store indentation used in toString()
 */
public String toString(int ind){
    indent=ind;
    return pars.toString(ind,xParser.IS_COMMAND);
}
/**
 * @return field with last indentation
 */
public String toString(){
    return pars.toString(indent,xParser.IS_COMMAND);
    }
} // class xDimCommand
