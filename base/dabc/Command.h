#ifndef DABC_Command
#define DABC_Command

#ifndef DABC_Basic
#include "dabc/Basic.h"
#endif

namespace dabc {
    
   class CommandClientBase;
   class Mutex;
    
   class Command : public Basic {
      
      friend class CommandClientBase;
      
      protected:
      
         class CommandParametersList;
         
         CommandParametersList* fParams;
         CommandClientBase*     fClient;
         Mutex*                 fClientMutex; // pointer on client mutex, must be locked when we access cleint itself
         bool                   fKeepAlive;   // if true, object should not be deleted by client, but later by user
         bool                   fCanceled;    // true if command was canceled by client, will not be executed

         virtual ~Command();
         
         void _CleanClient();
         
         void SetCanceled() { fCanceled = true; }
         
      public:

         Command(const char* name = "Command");
         
         void SetCommandName(const char* name) { SetName(name); }
         
         bool IsKeepAlive() const { return fKeepAlive; }
         void SetKeepAlive(bool on = true) { fKeepAlive = on; }
         
         bool IsCanceled() const { return fCanceled; }
         
         bool HasPar(const char* name) const;
         void SetPar(const char* name, const char* value);
         const char* GetPar(const char* name) const;
         void RemovePar(const char* name);

         void SetStr(const char* name, const char* value);
         const char* GetStr(const char* name, const char* deflt = 0) const;
         
         void SetBool(const char* name, bool v);
         bool GetBool(const char* name, bool deflt = false) const;
         
         void SetInt(const char* name, int v);
         int GetInt(const char* name, int deflt = 0) const;

         void SetUInt(const char* name, unsigned v);
         unsigned GetUInt(const char* name, unsigned deflt = 0) const;
         
         void SetPtr(const char* name, void* p);
         void* GetPtr(const char* name, void* deflt = 0) const;

         void SaveToString(String& v);
         bool ReadFromString(const char* s, bool onlyparameters = false);
         
         void AddValuesFrom(const Command* cmd, bool canoverwrite = true);

         void SetResult(bool res) { SetBool("_result_", res); }
         bool GetResult() const { return GetBool("_result_", false); }
         void ClearResult() { RemovePar("_result_"); }
         
         bool IsClient();
         void CleanClient();
         
         static void Reply(Command* cmd, bool res = true);
         static void Finalise(Command* cmd);
   };
   
}

#endif
