#ifndef DABC_XD_REGISTRY_H
#define DABC_XD_REGISTRY_H

#include "xdata/xdata.h"
#include "xdaq/ApplicationGroup.h"
#include "xdaq/ApplicationStub.h"
#include "xdaq/ApplicationContext.h"
#include "xdaq/ApplicationRegistry.h"
#include "toolbox/exception/Handler.h"
#include "toolbox/net/URN.h"

#include <list>
#include <algorithm>
#include <map>
#include "toolbox/task/WorkLoopFactory.h"
#include <toolbox/squeue.h>



#include "DABCLockGuard.h"
#include "DABCParameter.h"
#include "nameParser.h"

//#include "DABCDimServer.h"
#include "DABCApplication.h"
#include "dabc/CommandDefinition.h"

#define DIMSTRINGBUFSIZE 512

#define _DABC_COMMAND_GETINFOSPACE_ "GetInfospace"
#define _DABC_COMMAND_GETSTATUS_ "GetStatus"


class DimCommand;

namespace dabc{
namespace xd{
    
class DimServer;
class DimServiceEntry;
class DimParameterInfo;

class Application;



/**
*  This class registers all parameters to infospace and dim.
*  Interface instance between application and dim server.
*  Hosts the DIM server and the http SOAP/web interface.
*  Provides internal workloop threads for sending and local executing commands
*  @author J. Adamczewski, GSI
*/
class Registry: public toolbox::lang::Class
{
	
public:

    class DimCommandDescriptor
        {
           public:
           DimCommandDescriptor(xdaq::ApplicationDescriptor* ad, 
                                    const std::string& comname,
                                    const std::string& par) : 
            ad_(ad),comname_(comname),par_(par){;}                      
            DimCommandDescriptor() : ad_(0),comname_(""),par_(""){;}   

           xdaq::ApplicationDescriptor* ad_; 
           std::string comname_;
           std::string par_;
        };    


     class LocalCommandDescriptor
        {
           public:
           LocalCommandDescriptor(  const std::string& comname,
                                    const std::string& par) : 
            comname_(comname),par_(par){;}                      
            LocalCommandDescriptor() : comname_(""),par_(""){;}   
           std::string comname_;
           std::string par_;
        };    




	Registry(dabc::xd::Application* owner) throw (xdaq::exception::Exception);
	
    virtual  ~Registry();
    
    
    
    
    void SetInfoSpace(xdata::InfoSpace* infospace){infoSpace_=infospace;}
    
    xdata::InfoSpace* GetInfoSpace(){return infoSpace_;}
  
    Logger & GetApplicationLogger(){return theApplication_->getApplicationLogger();}
    
    /** returns application id string, consisting of class name and local id.
      * If application descriptor not specified, use this application*/
    std::string GetUniqueApplicationName(xdaq::ApplicationDescriptor* ad=0);


   /** returns full parameter name from modulename and simple parameter name.
     * uses internally name parser with threadsafety*/
    std::string CreateFullParameterName(const std::string& modulename, const std::string& varname);
    
    /** parse fullname of parameter and evaluate single module and variable names.
      * uses nameparser with threadlock*/  
    void ParseFullParameterName(const std::string& fullname, std::string& modname, std::string& varname);

  /** derive command descriptor name from command name.
    * uses nameparser object with threadlock*/
    std::string CreateCommandDescriptorName(const std::string& commandname);
 


  
  	/** Send command string with parameter to context node, address application of targetid */
	xoap::MessageReference SendSOAP(const std::string& command, const std::string& parameter, const std::string& node, unsigned long targetid);
	
    
    /** Convert command string into SOAP message to send.
     * May include one string parameter*/
	xoap::MessageReference CreateSOAPCommand(const std::string& command, const std::string& parameter);

     /**
	*  Load all registered parameters (infospace) from a xml/SOAP file.
	*/
    void LoadParameters(const std::string filename) throw (xdaq::exception::ParameterSetFailed);
    
    /**
	*  Save all registered parameters (whole infospace) to a xml/SOAP file.
	*/
    void SaveParameters(const std::string filename);  
  
    

    /**
	*  Put command to the local command queue
	*/
    void PushLocalCommand(const std::string& com, const std::string& par);

    
    /** Register external xdaq serializable variable as infospace and DIM service.
      * if dimchangeable is set true, the variable is changeable from dim controls system
      * if setupchangeable is true, variable can be changed from configuration system*/
    void RegisterSerializable(const std::string &name, xdata::Serializable *serializable, bool dimchangeable=true, bool setupchangeable=true);
    
    /**
	*  Unregister xdata variable from application infospace and  
    *  delete the corresponding DIM service.
	*/
    void UnRegisterSerializable(const std::string &name); 




    /** This will link external variable of reference to a corresponding infospace and dim
      * variable. If infospace variable is changed, also the original
      * value will be changed. Optionally, a user callback will be invoked on change*/
    void AddControlParameter(std::string name, int* ref, bool allowchange, void (*callback)()=0);
    void AddControlParameter(std::string name, unsigned int* ref, bool allowchange, void (*callback)()=0);
    void AddControlParameter(std::string name, float* ref, bool allowchange, void (*callback)()=0);
    void AddControlParameter(std::string name, double* ref, bool allowchange, void (*callback)()=0);
    void AddControlParameter(std::string name, bool* ref, bool allowchange, void (*callback)()=0);
    void AddControlParameter(std::string name, std::string* ref, bool allowchange, void (*callback)()=0);

 
    /** Removes infospace and dim reference to external control parameter of name*/
    void RemoveControlParameter(std::string name);
    
    /** Clean up all control parameter objects*/
    void RemoveControlParameterAll();
  
  
    /** Inform that variable at address has changed.
      * To update listeners and dim*/
    void ChangedControlParameter(void* address);
    
 
    /** Register new module command of name and with command definition def.
    * Will create dim command and command descriptor record*/ 
   void RegisterModuleCommand(std::string name, dabc::CommandDefinition* def);
   
   /** Unregister module command of name*/
   void UnRegisterModuleCommand(std::string name);


   /** find out if module command of name exists. Returns false if none*/
    bool  FindModuleCommand(std::string name);

    
       /** Create new dabc record of type rate and initialize with values.
       float value
       int displaymode (arc, bar, statistics, trend)
       float lower limit
       float upper limit
        float lower alarm
        float upper alarm
        char color
        char alarm color
        char units
   */
    void AddRateRecord(std::string name, float val, int displaymode=0, 
                        float ll=0, float ul=1.0E8, float la=1.0, float ua=100.0,
                        std::string color="Green", std::string alcol="Red",
                        std::string units="Mbyte/s", 
                        dabc::Parameter*  par=0);
   
     /** Create new dabc record of type rate from structure*/
     void AddRateRecord(std::string name, dabc::RateRec* rate, dabc::Parameter*  par=0);
   
   
   /** This method allows to fully set existing Rate Record of name.
     * Will also update monitoring services. 
     * Default parameters will just update value val.*/
   void UpdateRateRecord(std::string name, float val, int displaymode=-1, 
                        float ll=-1, float ul=-1, float la=-1, float ua=-1,
                        std::string color="", std::string alcol="",
                        std::string units="");
    
 
   /** Set existing rate record to values from structure and update dim service*/
   void UpdateRateRecord(std::string name, dabc::RateRec* rate);
    
  
     /** Create new dabc status record with specified values
        int severity; // (0=success, 1=warning, 2=error, 3=fatal)
        char color[16]; // (color name: Red, Green, Blue, Cyan, Yellow, Magenta)
         char status[16]; */
     void AddStatusRecord(std::string name, std::string status, int severity=0,
                          std::string color="Green", dabc::Parameter*  par=0);
   
      /** Create new dabc record of type status from structure
        * optionally register backpointer to module that owns status struct*/
     void AddStatusRecord(std::string name, dabc::StatusRec* status, dabc::Parameter*  par=0);
   
   
       /** This method allows to fully update existing Status Record of name.
         * Will also update monitoring services. */
     void UpdateStatusRecord(std::string name, std::string status, int severity=-1,
                          std::string color="");
     
    
       /** Set existing rate record to values from structure and update dim service*/
      void UpdateStatusRecord(std::string name, dabc::StatusRec* rate);


//////////////
     /** Create new dabc info record with specified values
        int verbosity; //  //(0=Plain text, 1=Node:text)
        char color[16]; // (color name: Red, Green, Blue, Cyan, Yellow, Magenta)
         char info[128]; */
     void AddInfoRecord(std::string name, std::string info, int verbosity=0,
                          std::string color="Green", dabc::Parameter*  par=0);
   
      /** Create new dabc record of type status from structure
        * optionally register backpointer to module that owns status struct*/
     void AddInfoRecord(std::string name, dabc::InfoRec* info, dabc::Parameter*  par=0);
   
   
       /** This method allows to fully update existing Info Record of name.
         * Will also update monitoring services. */
     void UpdateInfoRecord(std::string name, std::string info, int severity=-1,
                          std::string color="");
     
    
       /** Set existing rate record to values from structure and update dim service*/
      void UpdateInfoRecord(std::string name, dabc::InfoRec* rate);




   
    
      /** Create new dabc histogram record with specified values
        int channels;  // channels of data
      float xlow;
      float xhigh;
      char xlett[32];
      char cont[32];
      char color[16]; // (color name: White, Red, Green, Blue, Cyan, Yellow, Magenta)
      int data; */
     void AddHistogramRecord(std::string name, 
                                int channels, 
                                float xlow,
                                float xhigh,
                                int* data,
                                std::string xlett="x",
                                std::string cont="N",
                                std::string color="Green", 
                                dabc::Parameter*  par=0
                                    );
   
       /** Create new dabc record of type histogram from structure*/
     void AddHistogramRecord(std::string name, dabc::HistogramRec* his, dabc::Parameter*  par=0);
   
   
       /** This method allows to fully update existing Histogram Record of name.
         * Will also update monitoring services. */
     void UpdateHistogramRecord(std::string name,
                                int channels, 
                                float xlow,
                                float xhigh,
                                int* data,
                                std::string xlett="x",
                                std::string cont="N",
                                std::string color="Green");
                                
     /** This method allows to fully update existing Histogram Record of name.
         * Will also update monitoring services. */
     void UpdateHistogramRecord(std::string name, dabc::HistogramRec* status);
   
                                
      
      /** Fill (increment) histogram record name at bin with value */
      void FillHistogramRecord(std::string name, int bin, int value=1);
      
      /** Set bins of histogram record name to zero*/  
      void ClearHistogramRecord(std::string name);
  
  
     /** Create new dabc simple record of integer type. */
     void AddSimpleRecord(std::string name, int val, bool changable=false, dabc::Parameter*  par=0);
     
     /** Change record to new value val and update dim service. */
     void UpdateSimpleRecord(std::string name, int val);
          
      /** Create new dabc simple record of  double type. */
     void AddSimpleRecord(std::string name, double val, bool changable=false,  dabc::Parameter*  par=0);
     
      /** Change record to new value val and update dim service. */
     void UpdateSimpleRecord(std::string name, double val);
     
      /** Create new dabc simple record of  string type. */
     void AddSimpleRecord(std::string name, std::string val, bool changable=false,  dabc::Parameter*  par=0);
     
      /** Change record to new value val and update dim service. */
     void UpdateSimpleRecord(std::string name, std::string val);
     
       /** Create new dabc command descriptor record. */
     void AddCommandDescriptorRecord(std::string name, std::string descr);
     
          /** Remove record from infospace and dim. */    
     void RemoveRecord(std::string name);
  
     /** Subscribe parameter par to be updated whenever the corresponding value of
       * remote parameter of fulldimname is changed in cluster context.
       * Will use DIM info mechanism*/
     bool SubscribeParameter(dabc::Parameter* par, const std::string& fulldimname);

     /** Remove parameter from remote subscription list*/
     bool UnsubscribeParameter(dabc::Parameter* par);
  
  
    

   
   
    
     /**
	*  Define a dim string command and register dim server as handler.
     * Command action is to be implemented in OnDIMCommand. 
	*/
    void DefineDIMCommand(const std::string &name);
    
    
      /**
       *  Define a SOAP string command and register it to the SOAP message callback.
       * Command action depending on this name must be implemented in method OnSOAPCommand
       */
    void DefineSOAPCommand(const std::string &name);
    
    
	/**
	*  Starts the dim server.
	*/
    void StartDIMServer(const std::string& dnsnode, unsigned int dnsport);
    
    /**
	*  Stops the dim server.
	*/
    void StopDIMServer();
    
    /**
	*  Update the service corresponding to the xdaq serializable variable.,
    * i.e. send new value to registered clients.
    * optionally, service may tell client to output to log window
	*/
    void UpdateDIMService(const std::string& name, bool logoutput=false, dabc::xd::nameParser::recordstat priority=dabc::xd::nameParser::SUCCESS);
    
    /**
	*  Update all existing dim services, i.e. send new values to registered
    *  clients.
	*/
    void UpdateDIMServiceAll();
    
    
    /** set dim service variable by DIM command parameter.
      * variable name and new value is derived from parameter string
      * with = as separator (name=value)*/
    void SetDIMVariable(std::string parameter);
    
  
     /**
	*  Remove dim service of name (reduced form), i.e.
    *  unregister from server and delete the instance
	*/
    void RemoveDIMService(const std::string& name);
    
        
    /**
	*  Remove all existing dim services, i.e.
    *  unregister from server and delete the instances
	*/
    void RemoveDIMServiceAll();
    
     /**
	*  Remove dim command of name (reduced form), i.e.
    *  unregister from server and delete the instance
	*/
    void RemoveDIMCommand(const std::string& name);
 
      /**
	*  Remove all existing dim commands, i.e.
    *  unregister from server and delete the instances
	*/
    void RemoveDIMCommandsAll();
    
     /**
	*  Callback when client of name has exited.  
	*/
    virtual void OnExitDIMClient(const char* name);
    
     /**
	*  Callback when client has send EXIT to dim server.  
	*/
    virtual void OnExitDIMServer(int code);
    
    
    /**
	*  Callback when dim server sees error.  
	*/
    virtual void OnErrorDIMServer(int severity, int code, char *msg); 
    
   /**
	*  Callback for commands send to dim server.
    *  This checks if com is registered and then executes OnDimCommand  
	*/
    void HandleDIMCommand(DimCommand* com);
   
   /**
	*  Callback for commands send to dim server. May be extended in subclass  
	*/
    virtual void OnDIMCommand(DimCommand* com);
   

    /**
	*  Send dim command of comname with string parameter. Full command name is
    * evaluated from target context name (node:port) and application id  
	*/
    //void dabc::Application::SendDIMCommand(const std::string& contextname, unsigned int appid, const std::string& comname, const std::string& par);

    /** send dim command in command thread independent of callback*/
    void SendDIMCommandAsync(xdaq::ApplicationDescriptor* ad, const std::string& comname, const std::string& par);

      /**
	*  Send dim command of comname with string parameter. Destination node is
    * evaluated from target application  descriptor  
	*/
    void SendDIMCommand(xdaq::ApplicationDescriptor* ad, const std::string& comname, const std::string& par);

    
	
    	/**
	*  This will handle all incoming SOAP messages.
	*/
	xoap::MessageReference HandleSOAPMessage (xoap::MessageReference msg) throw (xoap::exception::Exception);


      /**
	*  Callback for SOAP commands. May be extended in subclass.
    * command name is given as string, optional parameter string may evaluated. 
    * Result of command reply, if necessary, may be put into reply envelope 
    * as provided from  calling method HandleSOAPMessage.
    * Optionally, user subclass implementation may access full SOAP command
    * message for extraction of parameters.  
	*/
    virtual void OnSOAPCommand(std::string& com, std::string& par, xoap::SOAPEnvelope &replyenvelope, xoap::MessageReference msg);

//    
      /** This method will wait on the DIM command queue and
      * eventually processes async com descriptors*/
    bool ProcessDIMQueue(toolbox::task::WorkLoop* wl);


    /** This method will execute next command from local queue*/
    bool ProcessLocalcommandQueue(toolbox::task::WorkLoop* wl);

     /** Expand local variable name to full dim service name.
      * dim service contains prefix of context and application class*/
    std::string BuildDIMName(const std::string& localname);
    
       /** Reduce dim service name to local infospace variable name.
       *  Prefix of context and application class will be removed*/
    std::string ReduceDIMName(const std::string& dimname);
    
     /** Returns prefix for dim service name, from contextname, classname, and lid of this application. */
    std::string& GetDIMPrefix();
 
    /** Evaluate DIM prefix for given context and application. If
     * descriptors are not specified, use this application*/
    std::string CreateDIMPrefix(xdaq::ContextDescriptor* cd=0, xdaq::ApplicationDescriptor* ad=0);

 /** Evaluate full name for the parameter file of this application.
   * will add prefix (like dim prefix) to name, but avoid
   * problematic characters for file system*/
    std::string CreateFullParameterfileName(std::string name);


    
    /** Evaluate name of dim server from xdaq context name (drop http:// prefix)
      * If context descriptor is not specified, use self context of this application*/
    std::string GetDIMServerName(xdaq::ContextDescriptor* cd=0);
   

    /** Add control parameter par to list. Checks internally if control parameter
      * of same name already exists, then false is returned and control
      * parameter object is deleted. */    
    bool RegisterControlParameter( dabc::xd::ControlParameter* par,  
            bool retrievelistener=false, bool changelistener=false,  
            bool allowdimchange=true, bool iscommanddescriptor=false);
 

   
        /** Search for control parameter by name*/
    dabc::xd::ControlParameter* FindControlParameter(std::string name);
 
 



  
 
    
       /** register serializable variable that shall be linked to an external reference in
      * the dynamic control parameter list. Will keep (own) serializable variable and register it to infospace and dim service*/
    void RegisterControlParameter(std::string& name, xdata::Serializable *serializable, void* reference, bool changeable=true, void (*callback)()=0);



           /** register serializable variable as control parameter  in the
      * the dynamic control parameter list. Will keep (own) serializable variable and register it to infospace and dim service
      * Reference to module may be kept for change command, if control record stems from module parameter*
      * Record may not be changed by external command if changable is false.
      * If commandescr is true, the record is command descriptor string for controls GUI.
      */
    void RegisterModuleParameter(std::string& name, xdata::Serializable *serializable,  bool changable=true, bool commanddescriptor=false, dabc::Parameter*  par=0);



    /**
	*  Register xdata variable to application infospace and export to the DIM server.
    *  recordtype may be specified for dim gui to display appropriate widget.
    *  add this application as item retrieve listener and item change listener
    *  if flags are set. May be changed by dim command only if allowdimchange flag is true.
    *  If no action listener is specified, this application itself is listener
    *  and gets the infospace events into actionPerformed
	*/
    void RegisterInfoSpaceAndDIM(const std::string &name, xdata::Serializable *serializable, 
            bool retrievelistener=false, bool changelistener=false,  
            bool allowdimchange=true, bool iscommanddescriptor=false, 
            xdata::ActionListener* listener=0)
        throw (xdata::exception::Exception);

 
  
     /** Search for DIM service  by name*/
    DimServiceEntry* FindDIMService(std::string name);
 
 
    /** hold prefix for dim server name , e.g. XDAQ  */
    static const std::string serverPrefix_;
 
    static const std::string& GetServerPrefix(){return serverPrefix_;}
 
 private:  


#if __XDAQVERSION__  > 310        
    /** protect services used by many threads*/
    toolbox::BSem* mainMutex_;
    /** protect global string buffers (nameparser etc.)*/ 
    toolbox::BSem* parseMutex_;   
#else 
    /** protect services used by many threads*/
    BSem* mainMutex_;
    /** protect global string buffers (nameparser etc.)*/
    BSem* parseMutex_;   
#endif

    /** hold prefix for dim service names  */
    std::string dimPrefix_;
    /** init switch for dim prefix evaluation*/
    bool dimPrefixReady_;
    
//    /** decide if finite state machine was switched
//      * from web state machine (true), or from DIM/SOAP interface*/
//    bool switchedByWSM_;
//    
//    

    
    /** keep track of assigned dim services.*/
    std::vector<dabc::xd::DimServiceEntry*> dimServices_;   
    
     /** keep track of assigned dim commands.*/
    std::vector<DimCommand*> dimCommands_;   
    
    
    /** handle to dim server*/
    dabc::xd::DimServer* dimServer_;
   
     /** keep track of infospace control parameters for modules.*/
    std::vector<dabc::xd::ControlParameter*> ctrlPars_;   
   
   
    /** list of registerd module command names to check on callback*/ 
   std::vector<std::string> fxModuleCommandNames; 
 

   /** keep track of subscribed parameter info services.*/
    std::vector<dabc::xd::DimParameterInfo*> fxParamInfos;   
   
   
   
   
    /** link to infospace that contains the registered serrializables*/
    xdata::InfoSpace* infoSpace_;
   


       
    /** this workloop processes async dim commands from the output queue:*/
    toolbox::task::ActionSignature* dimCommandAction_;
    toolbox::task::WorkLoop* dimCommandLoop_; 
    
    /** queue for outgoing dim commands*/
    toolbox::squeue<DimCommandDescriptor*> fxDimQueue;

//
 //** this workloop processes incoming commands from the input queue:*/
    toolbox::task::ActionSignature* localCommandAction_;
    toolbox::task::WorkLoop* localCommandLoop_; 
     /** queue for incoming  commands (dim or soap)*/
    toolbox::squeue<LocalCommandDescriptor*> fxLocalComQueue;
    
    dabc::xd::Application* theApplication_;
    
};










} // end namespace xd
} // end namespace dabc

#endif

