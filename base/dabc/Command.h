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

   class WorkingThread;
   class WorkingProcessor;
   class Mutex;
   class Condition;
   class CommandsQueue;
   class CallersQueue;


   enum CommandRes {
      cmd_false = 0,
      cmd_true = 1,

      cmd_timedout = -1,
      cmd_ignore = -2,
      cmd_postponed = -3
   };

   class Command : public Basic {

      friend class WorkingThread;
      friend class CommandsQueue;
      friend class WorkingProcessor;

      protected:

         class CommandParametersList;

         CommandParametersList* fParams;          /** list of command parameters */
         CallersQueue*          fCallers;         /** list of callers */
         unsigned               fCmdId;           /** Current command id */
         Mutex*                 fCmdMutex;        /** mutex used to protect callers list */

         virtual ~Command();

      public:

         Command(const char* name = "Command");

         void SetCommandName(const char* name) { SetName(name); }

         /** Method allows to access command handler after proc->Execute(cmd) is done
          * Parameter /param on is obsolete, only when true function perform any kind of action
          * Parameter on will be removes with next major release, kept for backward compatibility */
         void SetKeepAlive(bool on = true) { if (on) AddCaller(0, 0); }

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

         /** Copies all parameters from URL into the command */
         bool AssignUrl(const char* url, bool canoverwrite = true);

         void SetResult(int res) { SetInt("_result_", res); }
         int GetResult() const { return GetInt("_result_", 0); }
         void ClearResult() { RemovePar("_result_"); }
         bool HasResult() const { return HasPar("_result_"); }

         /** Method to inform caller that command is executed.
          * After this call command pointer must not be used in caller */
         static void Reply(Command* cmd, int res = 1);

         /** Method to cleanup command. If required, object will be destroyed */
         static void Finalise(Command* cmd);

      private:

         void AddCaller(WorkingProcessor* proc, bool* exe_ready = 0);

         void RemoveCaller(WorkingProcessor* proc);

         bool IsLastCallerSync();
   };

}

#endif
