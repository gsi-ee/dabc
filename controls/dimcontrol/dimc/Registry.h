#ifndef DIMC_REGISTRY_H
#define DIMC_REGISTRY_H

#include <vector>
#include <string>
#include "dimc/nameParser.h"
#include "dabc/threads.h"

namespace dabc{

   class CommandDefinition;
   class Parameter;
   class Configuration;
}


class DimCommand;

namespace dimc{

class Server;
class ParameterInfo;
class ServiceEntry;
class Manager;

class RegistryEntry {
public:

   RegistryEntry() : fMgrName(), fDimServer(), fDimPrefix(), fActive(true) {}

   std::string  fMgrName;
   std::string  fDimServer;
   std::string  fDimPrefix;
   bool         fActive;
};


/** This is the registry of all parameters and commands exported over the
 *  DIM server
 *  */
class Registry
{

public:


	Registry(dimc::Manager* owner, dabc::Configuration* cfg);

   virtual  ~Registry();

    /** parse fullname of parameter and evaluate single module and variable names.
      * uses nameparser with threadlock*/
    void ParseFullParameterName(const std::string& fullname, std::string& modname, std::string& varname);

  /** derive command descriptor name from command name.
    * uses nameparser object with threadlock*/
    std::string CreateCommandDescriptorName(const std::string& commandname);

    /*
     *  export command descriptor (xml description descr) as string service of name.
     *  Note that this service is never updated after creation and is kept internally here
     */
   void AddCommandDescriptor(const std::string& name, const std::string& descr);

    /**
     * Export Parameter par as DIM service. Keep in registry list.
     */
  void RegisterParameter(dabc::Parameter* par, const std::string& registername);

/*
 * update DIM service if parameter is changed
 */
  void ParameterUpdated(dabc::Parameter* par);


  /**
     * Remove Parameter from DIM services.
     */
  void UnregisterParameter(dabc::Parameter* par);

 /*
 * Remove all parameters from dim server
 */
  void UnregisterParameterAll();




   /**
	*  Submit command for local execution in manager
	*/
    void SubmitLocalDIMCommand(const std::string& com, const std::string& par);



    /** Register new module command of name and with command definition def.
    * Will create dim command and command descriptor record*/
   void RegisterUserCommand(std::string name, dabc::CommandDefinition* def);

   /** Unregister module command of name*/
   void UnRegisterUserCommand(std::string name);


   /** find out if module command of name exists. Returns false if none*/
    bool  FindModuleCommand(std::string name);





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
	*  Starts the dim server.
	*/
    void StartDIMServer(const std::string& servername, const std::string& dnsnode, unsigned int dnsport);

    /**
	*  Stops the dim server.
	*/
    void StopDIMServer();

    /**
	*  Update the service corresponding to the xdaq serializable variable.,
    * i.e. send new value to registered clients.
    * optionally, service may tell client to output to log window
	*/
    void UpdateDIMService(const std::string& name, bool logoutput=false, dimc::nameParser::recordstat priority=dimc::nameParser::SUCCESS);

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
     * Remove dim service associated with a parameter reference
     */
    void RemoveDIMService(dabc::Parameter* par);


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
    *  Remove and unregister all existing command descriptors
    */
     void RemoveCommandDescriptorsAll();


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
	 *  Send dim command of comname with string parameter. Destination node is
    * given as full name in target string
    */
    void SendDIMCommand(const std::string& target, const std::string& comname, const std::string& par);



     /** Expand local variable name to full dim service name.
      * dim service contains prefix of node and context */
    std::string BuildDIMName(const std::string& localname);

       /** Reduce dim service name to local parameter name.
       *  Prefix of node and context will be removed*/
    std::string ReduceDIMName(const std::string& dimname);

     /** Returns prefix for dim service name of this node. */
    std::string& GetDIMPrefix() { return fDimPrefix; }


    unsigned NumNodes() const { return fClusterInfo.size(); }

    const char* GetManagerName(unsigned n) { return n<fClusterInfo.size() ? fClusterInfo[n].fMgrName.c_str() : 0; }

    bool IsNodeActive(unsigned n) { return n<fClusterInfo.size() ? fClusterInfo[n].fActive : false; }

    /** Evaluate name of dim server from cluster node id*/
    std::string GetDIMServerName(unsigned n) { return n<fClusterInfo.size() ? fClusterInfo[n].fDimServer : std::string(); }

    /** Evaluate DIM prefix from given cluster node id.*/
    std::string GetDIMPrefix(unsigned n) { return n<fClusterInfo.size() ? fClusterInfo[n].fDimPrefix : std::string(); }

    std::string GetDIMPrefixByName(const std::string& mgrname);

    bool IsManagerActive(const std::string& mgrname);


    /** hold prefix for dim server name , e.g. DABC  */
    static const std::string gServerPrefix;

    static const std::string& GetServerPrefix() { return gServerPrefix; }


protected:

   void AddService(dimc::ServiceEntry* nentry, bool allowdimchange=true, bool iscommanddescriptor=false);

   /** Search for DIM service  by name*/
   ServiceEntry* FindDIMService(const std::string& name);

   /** Search for DIM service  by parameter reference*/
   ServiceEntry* FindDIMService(dabc::Parameter* par);

 private:

    /** protect services used by many threads*/
     dabc::Mutex fMainMutex;

     /** protect global string buffers (nameparser etc.)*/
     dabc::Mutex fParseMutex;

    /** backpointer to our owner*/
    dimc::Manager* fManager;

      /** handle to dim server*/
    dimc::Server* fDimServer;

    /** hold prefix for dim service names  */
    std::string fDimPrefix;

    /** keep track of assigned dim services.*/
    std::vector<dimc::ServiceEntry*> fDimServices;

     /** keep track of assigned dim commands.*/
    std::vector<DimCommand*> fDimCommands;


    /** list of registerd module command names to check on callback*/
   std::vector<std::string> fModuleCommandNames;


   /** keep track of subscribed parameter info services.*/
    std::vector<dimc::ParameterInfo*> fParamInfos;

    std::vector<dimc::RegistryEntry>  fClusterInfo;

};










} // end namespace dimc

#endif

