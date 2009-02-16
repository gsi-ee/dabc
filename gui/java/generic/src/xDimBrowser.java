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
import java.util.*;
import dim.*;
/**
 * DIM browser.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xDimBrowser implements xiDimBrowser {
private int i,ncom,npar,ist;
private int version=0;
private xDimCommand com;
private xDimParameter par;
private Vector<xDimParameter> vpar;
private Vector<xDimCommand> vcom;
private Vector<xiDimParameter> vipar;
private Vector<xiDimCommand> vicom;
private xPanelMeter metpan;
private xPanelHisto hispan;
private xPanelState stapan;
private xPanelInfo infpan;

/**
 * Constructor adds DIM error handler.
 * @param histogram Panel
 * @param meter Panel
 * @param state Panel
 * @param info Panel
 */
public xDimBrowser(xPanelHisto histogram, xPanelMeter meter, xPanelState state, xPanelInfo info){
    hispan=histogram;
    stapan=state;
    infpan=info;
    metpan=meter;
    DimErrorHandler eid = new DimErrorHandler(){
        public void errorHandler(int severity, int code, String msg){
            System.out.println("Error: "+msg+" sev: "+severity);
        }	};
        DimClient.addErrorHandler(eid);
}
/**
 * Timer
 */
public static void startTimer(int secs){
    DimTimer atimer = new DimTimer(secs){
        public void timerHandler(){
            System.out.println("tick");
            startTimer(10);
        }	};
}
// xiDimBrowser
public void addInfoHandler(xiDimParameter parameter, xiUserInfoHandler infohandler){
((xDimParameter)parameter).addInfoHandler(infohandler);}

// xiDimBrowser
public void removeInfoHandler(xiDimParameter parameter, xiUserInfoHandler infohandler){
((xDimParameter)parameter).removeInfoHandler(infohandler);}

protected xPanelHisto getPanelHistogram(){return hispan;}
protected xPanelMeter getPanelMeter(){return metpan;}
protected xPanelState getPanelState(){return stapan;}
protected xPanelInfo  getPanelInfo(){return infpan;}

/**
 * Get list of services filtered by wildcard. 
 * @param wildcard Wildcard string
 * @return String array of service names. 
 */
protected String[] getServices(String wildcard){
return DimBrowser.getServices(wildcard);
}
/**
 * Search for server node names. Searches for * / EXIT services. 
 * @return Space separated list of servres names. 
 */
protected String getServers(){
    StringBuffer list=new StringBuffer();
    String item;
    int ii;
    String[] node;
    String[] srv=DimBrowser.getServices("*/EXIT");
    for(int i=0;i<srv.length;i++){
    	if(srv[i].equals("DIS_DNS/EXIT")) {
            ii=xSet.getDimDns().indexOf(".");
            if(ii==-1)list.append("DNS="+xSet.getDimDns());
            else list.append("DNS="+xSet.getDimDns().substring(0,ii));
    	} else {
    	node=srv[i].split("/");
        if(node.length > 2 )item=node[1]; // DABC has DABC/node/EXIT
        else                item=node[0]; // MBS has node:type/EXIT
        ii=item.indexOf(".");
        if(ii==-1)list.append(" "+item);
        else list.append(" "+item.substring(0,ii));
    }}
    return list.toString();
}
/**
 * @return Number of servers (number of EXIT commands).
 */
protected int getNofServers(){
    String[] srv=DimBrowser.getServices("*/EXIT");
    return srv.length;
}
protected void listServices(boolean all){
    if(vpar != null){
        Iterator<xDimParameter> vi = vpar.listIterator();
        for(i=0;i<vpar.size();i++){
            par=vi.next();
            par.printParameter(all);
        }
    }
}
/** 
 * Steps through parameter list and activates all by setParameterActiv.
 * @see xDimParameter
 */
protected void enableServices(){
    if(vpar != null){
    System.out.println("Browser enableServices");
        Iterator<xDimParameter> vi = vpar.listIterator();
        for(i=0;i<vpar.size();i++){
            par=vi.next();
            par.setParameterActiv(true);
        }
    }
}
/**
 * @param cleanup True: Remove all services, otherwise only deactivate.
 * @see xDimParameter
 */
protected void releaseServices(boolean cleanup){
// look for commands and parameters available
// release registered parameters
if(cleanup){
System.out.println("Browser releaseServices, rebuild all");
    if(vpar != null){
        Iterator<xDimParameter> vi = vpar.listIterator();
        for(i=0;i<vpar.size();i++){
            par=vi.next();
            par.releaseService();
            par.setParameterActiv(false);
        }
    }
    if(vpar != null)vpar.clear();
    if(vipar != null)vipar.clear();
    vpar=null;
    vipar=null;
    if(vcom != null)vcom.clear();
    if(vicom != null)vicom.clear();
    vcom=null;
    vicom=null;
} else {
System.out.println("Browser releaseServices, update");
    if(vpar != null){
        Iterator<xDimParameter> vi = vpar.listIterator();
        for(i=0;i<vpar.size();i++){
            par=vi.next();
            par.removeInfoHandler();// clear all registrations of user handlers
            par.setParameterActiv(false);
        }
    }
}

}
/**
 * Get list of services from DIM name server defined by DIM_DNS_NODE, filtered by wildcard.
 * Commands and parameters are kept in separate lists and ordered alphabetically.
 * Names not matching the DABC format (four elements separated by slashes) are handled separately. 
 * Only DIM EXIT commands are formatted according DABC,
 * but in xDimCommand they are restored for execution. This might need better solution.
 * @param wildcard services name filter
 * @see xParser
 */
protected void initServices(String wildcard){
    xParser pars=new xParser();

    String[] srvcs = DimBrowser.getServices(wildcard);
    ncom=0;
    npar=0;
    int numberserv=srvcs.length;
    int[] servlist=new int[numberserv];
// count number of commands and parameters
    for(i = 0; i <numberserv; i++){
        //		if(!(srvcs[i].startsWith("$")||srvcs[i].startsWith("DIS_DNS")))
        servlist[i]=0; // exclude
        // check if dabc format
        if(pars.parse(srvcs[i],xParser.PARSE_ONLY)==0) // dabc format
            if(DimBrowser.isCommand(srvcs[i])){
            ncom++;
            servlist[i]=1; // command
            }else{
            npar++;
            servlist[i]=2; // parameter
            }
        else // no dabc format, take only EXIT commands (not for DIS_DNS)
            if(srvcs[i].contains("/EXIT") && srvcs[i].contains(":")){
            if(srvcs[i].split("/").length==3){
                ncom++;
                servlist[i]=3; // free format command
                //System.out.println(srvcs[i]);
            }}}
    String[] parlist = new String[npar];
    String[] comlist = new String[ncom];
    ncom=0;
    npar=0;
// in the following we append the format string to the names for sorting later
// make list of commands and parameters
    for(i = 0; i <numberserv; i++)
        if(servlist[i]==1){ // command
            ist=pars.parse(srvcs[i],xParser.PARSE_STORE_FULL); // parse, compose and store
            comlist[ncom++]=pars.toString(xParser.IS_COMMAND) +" "+DimBrowser.getFormat(srvcs[i]);
            //System.out.println(comlist[ncom-1]);
        }else if(servlist[i]==2){ // parameter
            parlist[npar++]=srvcs[i]+" "+DimBrowser.getFormat(srvcs[i]);
        }else if(servlist[i]==3){ // server EXIT
            String[] it = srvcs[i].split("/");
            String full = new String(it[0]+"/EXIT/$:0/"+it[1]);
            ist=pars.parse(full,xParser.PARSE_STORE_FULL,xParser.IS_COMMAND); // parse, compose and store
            comlist[ncom++]=pars.toString(xParser.IS_COMMAND) +" "+DimBrowser.getFormat(srvcs[i]);
    }
// sort both lists
    List<String> plist = Arrays.asList(parlist);
    Collections.sort(plist);
    String[] parsort = plist.toArray(new String[0]);
    List<String> clist = Arrays.asList(comlist);
    Collections.sort(clist);
    String[] comsort = clist.toArray(new String[0]);
// System.out.println("List sorted commands");
// for(i=0;i<ncom;i++) System.out.println("CMD "+comsort[i]);
// System.out.println("List sorted parameters");
// for(i=0;i<npar;i++) System.out.println("PAR "+i+" "+parsort[i]);

    version++;
// now we would have to separate the format string from the names
    String[] pname;
int iold=0, inew=0, iqual, ioldsize;
    if(vpar == null) vpar = new Vector<xDimParameter>(npar);
    if(vipar == null) vipar = new Vector<xiDimParameter>(npar);
    ioldsize=vpar.size();
    System.out.println("Browser setup services list. Current "+ioldsize+", New "+npar);
// create parameter list and setup handlers
    if(npar == 0){ // no parameter at all, cleanup existing
        releaseServices(true);
        vpar = new Vector<xDimParameter>(npar);
        vipar = new Vector<xiDimParameter>(npar);
    } else {
    while(true) {
        pname=parsort[inew].split(" "); // separate format string
        if(iold == ioldsize) iqual=1; // create new
        else iqual=vpar.get(iold).getParser().getFull().compareTo(pname[0]);
        if(iqual == 0){
         //System.out.println("Equals: "+iold+" "+vpar.get(iold).getParser().getFull()+ " "+pname[0]);
            iold++;
            inew++;
        } else if(iqual > 0){ // create new and insert before old
        // System.out.println("Add:  "+iold+" "+pname[0]);
            par=newParameter(pname[0],pname[1]);
            vpar.add(iold,par);
            vipar.add(iold,par);
            ioldsize++;
            iold++;
            inew++;
        } else if (iqual < 0){ // remove old, no increment
        // System.out.println("Remove: "+vpar.get(iold).getParser().getFull()+ " "+pname[0]);
            // par=vpar.get(iold);
            // par.releaseService();
            // par.deactivateParameter();
            // vpar.remove(iold);
            // vipar.remove(iold);
            // ioldsize--;
            iold++;
        }
        if((iold > ioldsize) || (inew >= npar)) break;
    }// parlist
    // now ioldsize must be npar
    // if(npar != ioldsize) {
    // System.out.println("Cleanup: parameters="+npar+","+inew+" table="+ioldsize+","+iold);
        // for(i=ioldsize-1;i>=iold;i--){
         // // System.out.println("Remove: "+vpar.get(i).getParser().getFull());
            // par=vpar.get(i);
            // par.releaseService();
            // par.deactivateParameter();
            // vpar.remove(i);
            // vipar.remove(i);
        // }
    // }
    }
    // //Checking
    // if(npar != vpar.size())System.out.println("Mismatch: params="+npar+" table="+vpar.size());
    // for(i=0;i<npar;i++){
        // pname=parsort[i].split(" "); // separate format string
        // if(vpar.get(i).getParser().getFull().compareTo(pname[0])!=0)
            // System.out.println("Parameter mismatch: "+vpar.get(i).getParser().getFull()+ " "+pname[0]);
    // }
    
    iold=0;
    inew=0;
// Now the same for the commands. Update existing list
    if(vcom == null) vcom = new Vector<xDimCommand>(ncom);
    if(vicom == null) vicom = new Vector<xiDimCommand>(ncom);
    ioldsize=vcom.size();
    System.out.println("Browser setup command list. Current "+ioldsize+", New "+ncom);
// create Command list and setup handlers
    if(ncom > 0){ // no Command at all, cleanup existing
    while(true) {
        pname=comsort[inew].split(" "); // separate format string
        if(iold == ioldsize) iqual=1; // create new
        else iqual=vcom.get(iold).getParser().getCommand().compareTo(pname[0]);
        if(iqual == 0){
         //System.out.println("Equals: "+iold+" "+vcom.get(iold).getParser().getCommand()+ " "+pname[0]);
            iold++;
            inew++;
        } else if(iqual > 0){ // create new and insert before old
        // System.out.println("Add:  "+iold+" "+pname[0]);
            com=new xDimCommand(pname[0],pname[1],version);
            vcom.add(iold,com);
            vicom.add(iold,com);
            ioldsize++;
            iold++;
            inew++;
        } else if (iqual < 0){ // remove old, no increment
        // System.out.println("Remove: "+vcom.get(iold).getParser().getCommand()+ " "+pname[0]);
            // com=vcom.get(iold);
            // vcom.remove(iold);
            // vicom.remove(iold);
            // ioldsize--;
            iold++;
        }
        if((iold > ioldsize) || (inew >= ncom)) break;
    }// parlist
    // now ioldsize must be ncom
    // if(ncom != ioldsize) {
        // System.out.println("Cleanup: commands="+ncom+","+inew+" table="+ioldsize+","+iold);
        // for(i=ioldsize-1;i>=iold;i--){
         // // System.out.println("Remove: "+vcom.get(i).getParser().getCommand());
            // com=vcom.get(i);
            // vcom.remove(i);
            // vicom.remove(i);
        // }
    // }
    }
    // // Checking
    // if(ncom != vcom.size())System.out.println("Mismatch: commands="+ncom+" table="+vcom.size());
    // for(i=0;i<ncom;i++){
        // pname=comsort[i].split(" "); // separate format string
        // if(vcom.get(i).getParser().getCommand().compareTo(pname[0])!=0)
            // System.out.println("Command mismatch: "+vcom.get(i).getParser().getCommand()+ " "+pname[0]);
    // }
    // Cleanup panels in PanelParameter
}

private xDimParameter newParameter(String name, String format){
xDimParameter p;
         if(format.startsWith("C")) p= new xDimParameter(name,format,"%BrokenLink%",version);
    else if(format.startsWith("F")) p= new xDimParameter(name,format,(float)-1.,version);
    else                            p= new xDimParameter(name,format,-1,version);
    p.setPanels(hispan,metpan,stapan,infpan);
    return p;
}
protected int getNumberOfCommands(){return ncom;}
protected int getNumberOfParameters(){return npar;}

protected Vector<xDimParameter> getParameterList(){return vpar;}
protected Vector<xDimCommand> getCommandList(){return vcom;}
public Vector<xiDimParameter> getParameters(){return vipar;}
public Vector<xiDimCommand> getCommands(){return vicom;}
public void sleep(int s){DimTimer.sleep(s);}
}
