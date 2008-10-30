#ifndef DABC_XD_MANAGER_H
#define DABC_XD_MANAGER_H

#include "dabc/Manager.h"

 
namespace dabc {
namespace xd{    
    
   class Node;
   class Registry;

   class Manager : public dabc::Manager  {
      public:
        Manager(Node* app, const char* managername);
        virtual ~Manager();
    
  
        void ProcessRemoteCommand(const char* cmddata);
        
        
        virtual bool IsNodeActive(int num); 

        /** Number of active nodes in cluster*/
        virtual int NumNodes();
  
 
         // indicate if manager play central role in the system
        virtual bool IsMainManager() {return fbMainManager;}
        void SetMainManager(bool on=true){fbMainManager=on;}
  
        virtual bool HasClusterInfo(); 
         
  
          
        virtual bool Subscribe(Parameter* par, int remnode, const char* remname);
        virtual bool Unsubscribe(Parameter* par);


        /** Invoke state transition of manager. 
           * Must be overwritten in derivered class.
           * This MUST be asynchron functions means calling thread should not be called.
           * Actual state transition will be performed in state-machine thread.
           * If command object is specified, it will be replyed when state transition is
           * completed or when transition is failed */
        virtual bool InvokeStateTransition(const char* state_transition_name, Command* cmd = 0);
    
       /** this node id number*/
        virtual int NodeId() const;

          /** node name depending on id number*/
        virtual const char* GetNodeName(int num);
  
    
      protected:   
       
        virtual void ParameterEvent(dabc::Parameter* par, int event);
 
        virtual void CommandRegistration(dabc::Module* m, dabc::CommandDefinition* def, bool reg);
  
        virtual bool CanSendCmdToManager(const char*);
         
        virtual bool SendOverCommandChannel(const char* managername, const char* cmddata);
        
       /** Overwritten to handle exceptions occuring in module
           * threads*/
        virtual void ModuleExecption(dabc::Module* m, const char* msg);

                
         
      private:
      
        Node* fxApplication;
        
        Registry* fxRegistry;
        
        bool fbMainManager;  
  
   };
}; // end namespace xd  
}; // end namespace dabc

#endif
