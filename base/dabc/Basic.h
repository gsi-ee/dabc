#ifndef DABC_Basic
#define DABC_Basic

#ifndef DABC_string
#include "dabc/string.h"
#endif

#ifndef DABC_ConfigIO
#include "dabc/ConfigIO.h"
#endif

namespace dabc {

   class Manager;
   class CommandReceiver;
   class Folder;
   class Mutex;
   class Configuration;

   extern const char* xmlInputQueueSize;
   extern const char* xmlOutputQueueSize;
   extern const char* xmlUserHeaderSize;
   extern const char* xmlUseAcknowledges;

   extern const char* xmlPoolName;
   extern const char* xmlWorkPool;
   extern const char* xmlBufferSize;
   extern const char* xmlNumBuffers;
   extern const char* xmlNumInputs;
   extern const char* xmlNumOutputs;

   extern const char* xmlTrueValue;
   extern const char* xmlFalseValue;

   class Basic {
      friend class Folder;
      friend class Manager;

      public:
         Basic(Basic* parent, const char* name);
         virtual ~Basic();

         // these three methods describes look of element in xml file
         virtual const char* MasterClassName() const { return "Basic"; }
         virtual const char* ClassName() const { return "Basic"; }
         virtual bool UseMasterClassName() const { return false; }

         // here we listed all thread-safe methods

         inline Mutex* GetMutex() const { return fMutex; }
         inline Basic* GetParent() const { return fParent; }
         virtual CommandReceiver* GetCmdReceiver() { return 0; }

         virtual void AddChild(Basic* obj);
         virtual void RemoveChild(Basic* obj);

         virtual void DependendDestroyed(Basic*) {}

         void MakeFullName(std::string &fullname, Basic* upto = 0) const;
         std::string GetFullName(Basic* upto = 0) const;

         // operations with object name (and info) are not thread-safe
         // therefore, in the case when object name must be changed,
         // locking should be applied by any other means

         const char* GetName() const { return fName.c_str(); }
         bool IsName(const char* str) const { return fName.compare(str)==0; }
         virtual void FillInfo(std::string& info);

         int GetAppId() const { return fAppId; }
         void SetAppId(int id = 0) { fAppId = id; }

         virtual bool Store(ConfigIO &cfg) { return true; }
         virtual bool Find(ConfigIO &cfg);
         virtual bool Read(ConfigIO &cfg) { return true; }

         static long NumInstances() { return gNumInstances; }

      protected:
         void SetName(const char* name);
         void DeleteThis();

         virtual void _SetParent(Mutex* mtx, Basic* parent);

         void _MakeFullName(std::string &fullname, Basic* upto) const;

      private:
         Mutex*   fMutex;    // direct pointer on main mutex
         Basic*   fParent;   // parent object
         std::string   fName;     // object name
         bool     fCleanup;  // indicates if object should inform manager about its destroyment
         int      fAppId;    // indicates id of application, to which object belongs to (0 - default application)

         static long gNumInstances;
   };
};

#endif
