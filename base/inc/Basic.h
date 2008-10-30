#ifndef DABC_Basic
#define DABC_Basic

#ifndef DABC_string
#include "dabc/string.h"
#endif

namespace dabc {
    
   class Manager;
   class CommandReceiver;
   class Folder;
   class Mutex;
   
   class Basic {
      friend class Folder;
      friend class Manager; 
       
      public:
         Basic(Basic* parent, const char* name);
         virtual ~Basic();
         
         // here we listed all thread-safe methods
         
         inline Manager* GetManager() const { return fManager; }
         inline Mutex* GetMutex() const { return fMutex; }
         inline Basic* GetParent() const { return fParent; }
         virtual CommandReceiver* GetCmdReceiver() { return 0; }
         
         virtual void AddChild(Basic* obj);
         virtual void RemoveChild(Basic* obj);
         
         virtual void DependendDestroyed(Basic*) {}
         
         void MakeFullName(String &fullname, Basic* upto = 0) const;
         String GetFullName(Basic* upto = 0) const;
         
         // operations with object name (and info) are not thread-safe
         // therefore, in the case when object name must be changed,
         // locking should be applied by any other means
         
         const char* GetName() const { return fName.c_str(); }
         bool IsName(const char* str) const { return fName.compare(str)==0; }
         virtual void FillInfo(String& info);
         
         int GetAppId() const { return fAppId; }
         void SetAppId(int id = 0) { fAppId = id; }
         
         static long NumInstances() { return gNumInstances; }

      protected:   
         void SetName(const char* name);
         void DeleteThis();
         
         virtual void _SetParent(Manager* mgr, Mutex* mtx, Basic* parent);
         
         void _MakeFullName(String &fullname, Basic* upto) const;

      private:  
         Manager* fManager;  // direct pointer on the manager
         Mutex*   fMutex;    // direct pointer on main mutex
         Basic*   fParent;   // parent object
         String   fName;     // object name
         bool     fCleanup;  // indicates if object should inform manager about its destroyement
         int      fAppId;    // indicates id of application, to which object belongs to (0 - default application)
         
         static long gNumInstances;
   };
};

#endif
