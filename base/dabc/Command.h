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
#ifndef DABC_Command
#define DABC_Command

#ifndef DABC_Basic
#include "dabc/Basic.h"
#endif

namespace dabc {

   class CommandClientBase;
   class WorkingProcessor;
   class Mutex;
   class Condition;

   enum CommandRes {
      cmd_false = 0,
      cmd_true = 1,
      cmd_ignore = -1,
      cmd_postponed = -2
   };

   class Command : public Basic {

      friend class CommandClientBase;
      friend class WorkingProcessor;

      protected:

         class CommandParametersList;

         CommandParametersList* fParams;
         CommandClientBase*     fClient;
         Mutex*                 fClientMutex; // pointer on client mutex, must be locked when we access client itself
         bool                   fKeepAlive;   // if true, object should not be deleted by client, but later by user
         bool                   fCanceled;    // true if command was canceled by client, will not be executed

         WorkingProcessor*      fProcessor;       /** pointer on processor, waiting for command execution */
         Mutex*                 fProcessorMutex;  /** processor mutex, used for access fProcessor pointer */
         Condition*             fProcessorCond;   /** pointer on the condition, used for waiting of result */

         virtual ~Command();

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
         void SetStr(const char* name, const std::string& value) { SetStr(name, value.c_str()); }
         const char* GetStr(const char* name, const char* deflt = 0) const;

         void SetBool(const char* name, bool v);
         bool GetBool(const char* name, bool deflt = false) const;

         void SetInt(const char* name, int v);
         int GetInt(const char* name, int deflt = 0) const;

         void SetDouble(const char* name, double v);
         double GetDouble(const char* name, double deflt = 0.) const;

         void SetUInt(const char* name, unsigned v);
         unsigned GetUInt(const char* name, unsigned deflt = 0) const;

         void SetPtr(const char* name, void* p);
         void* GetPtr(const char* name, void* deflt = 0) const;

         void SaveToString(std::string& v);
         bool ReadFromString(const char* s, bool onlyparameters = false);
         bool ReadParsFromDimString(const char* s);

         void AddValuesFrom(const Command* cmd, bool canoverwrite = true);

         void SetResult(int res) { SetInt("_result_", res); }
         int GetResult() const { return GetInt("_result_", 0); }
         void ClearResult() { RemovePar("_result_"); }

         bool IsClient();
         void CleanClient();

         bool IsProcessor();
         void CleanProcessor();

         static void Reply(Command* cmd, int res = 1);
         static void Finalise(Command* cmd);
   };

}

#endif
