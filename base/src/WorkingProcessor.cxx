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
#include "dabc/WorkingProcessor.h"

#include <stdlib.h>
#include <math.h>

#include "dabc/Folder.h"
#include "dabc/Command.h"
#include "dabc/Parameter.h"
#include "dabc/Manager.h"
#include "dabc/Iterator.h"
#include "dabc/CommandDefinition.h"

unsigned  dabc::WorkingProcessor::gParsVisibility = 1;
unsigned  dabc::WorkingProcessor::gParsCfgDefaults = dabc::WorkingProcessor::MakeParsFlags(5, true, false);

dabc::WorkingProcessor::WorkingProcessor(Folder* parsholder) :
   fProcessorThread(0),
   fProcessorId(0),
   fProcessorPriority(-1), // minimum priority per default

   fProcessorMainMutex(0),
   fProcessorSubmCommands(CommandsQueue::kindSubmit),
   fProcessorReplyCommands(CommandsQueue::kindReply),

   fParsHolder(parsholder),
   fParsDefaults(0),
   fProcessorActivateTmout(false),
   fProcessorActivateMark(NullTimeStamp),
   fProcessorActivateInterv(0.),
   fProcessorPrevFire(NullTimeStamp),
   fProcessorNextFire(NullTimeStamp)
{
   SetParDflts(1, false, true);
}

dabc::WorkingProcessor::~WorkingProcessor()
{
   DOUT5(("~WorkingProcessor %p %d thrd:%p", this, fProcessorId, fProcessorThread));

   DestroyAllPars();

   RemoveProcessorFromThread(true);

   DOUT5(("~WorkingProcessor %p %d thrd:%p done", this, fProcessorId, fProcessorThread));
}

bool dabc::WorkingProcessor::AssignProcessorToThread(WorkingThread* thrd, bool sync)
{
   if (fProcessorThread!=0) {
      EOUT(("Thread is already assigned"));
      return false;
   }

   if (thrd==0) {
      EOUT(("Thread is not specified"));
      return false;
   }

   if (RequiredThrdClass()!=0)
     if (strcmp(RequiredThrdClass(), thrd->ClassName())!=0) {
        EOUT(("Processor requires class %s than thread %s of class %s" , RequiredThrdClass(), thrd->GetName(), thrd->ClassName()));
        return false;
     }

   return thrd->AddProcessor(this, sync);
}

void dabc::WorkingProcessor::RemoveProcessorFromThread(bool forget_thread)
{
   if (fProcessorThread && (fProcessorId>0))
      fProcessorThread->RemoveProcessor(this);

   if (forget_thread) {
      LockGuard lock(fProcessorMainMutex);
      fProcessorThread = 0;
      fProcessorMainMutex = 0;
   }
}

void dabc::WorkingProcessor::DestroyProcessor()
{
   if (fProcessorThread)
      fProcessorThread->DestroyProcessor(this);
   else
      delete this;
}

void dabc::WorkingProcessor::ActivateTimeout(double tmout_sec)
{
   LockGuard lock(fProcessorMainMutex);

   if (fProcessorThread==0) return;

   bool dofire = !fProcessorActivateTmout;

   fProcessorActivateMark = fProcessorThread->ThrdTimeStamp();
   fProcessorActivateInterv = tmout_sec;
   fProcessorActivateTmout = true;

   if (dofire)
      fProcessorThread->_Fire(CodeEvent(WorkingThread::evntCheckTmout, 0, fProcessorId), ProcessorPriority());
}

bool dabc::WorkingProcessor::TakeActivateData(TimeStamp_t& mark, double& interval)
{
   LockGuard lock(fProcessorMainMutex);
   if (!fProcessorActivateTmout) return false;

   mark = fProcessorActivateMark;
   interval = fProcessorActivateInterv;

   fProcessorActivateTmout = false;
   return true;
}

void dabc::WorkingProcessor::ProcessCoreEvent(EventId evnt)
{
   DOUT5(("Processor %p %u thrd %p CoreEvent %u", this, fProcessorId, fProcessorThread, GetEventCode(evnt)));

   switch (GetEventCode(evnt)) {
      case evntSubmitCommand: {

         DOUT5(("Process evntSubmitCommand proc %p %u thrd %p arg %u", this, fProcessorId, fProcessorThread, GetEventArg(evnt)));

         unsigned size = 0;

         dabc::Command* cmd = 0;
         {
            LockGuard lock(fProcessorMainMutex);
            size = fProcessorSubmCommands.Size();
            cmd = fProcessorSubmCommands.PopWithId(GetEventArg(evnt));
         }

         if (cmd==0) {
            EOUT(("evntSubmitCommand: No command with specified id %u size %u was %u", GetEventArg(evnt), fProcessorSubmCommands.Size(), size));
            exit(1);
         } else {
//            DOUT0(("ProcessSubmit Cmd %p start", cmd));

//            DOUT5(("ProcessSubmit command %p id %u processor:%p", cmd, cmd->fCmdId, this));

            int cmd_res = PreviewCommand(cmd);

//            DOUT0(("ProcessSubmit Cmd %p preview = %d", cmd, cmd_res));

            if (cmd_res == cmd_ignore) {
               cmd_res = ExecuteCommand(cmd);
//               DOUT0(("ProcessSubmit Cmd %p exe = %d", cmd, cmd_res));
            }

            if (cmd_res == cmd_ignore) {
               EOUT(("Command ignored %s", cmd->GetName()));
               cmd_res = cmd_false;
            }

            bool completed = (cmd_res>=0);

//            DOUT5(("Execute command %p : %s res = %d", cmd, (completed ? cmd->GetName() : "-- not allowed --"), cmd_res));

//            DOUT0(("ProcessSubmit Cmd %p completed %s", cmd, DBOOL(completed)));

            if (completed)
               dabc::Command::Reply(cmd, cmd_res);

//            DOUT0(("ProcessSubmit Cmd %p end", cmd));
         }

         break;
      }
      case evntReplyCommand: {

//         DOUT0(("Process evntReplyCommand arg %u", GetEventArg(evnt)));

         dabc::Command* cmd = 0;
         {
            LockGuard lock(fProcessorMainMutex);
            cmd = fProcessorReplyCommands.PopWithId(GetEventArg(evnt));
         }

         if (cmd==0)
            EOUT(("evntReplyCommand: no command with specified id %u", GetEventArg(evnt)));
         else {
            if (ReplyCommand(cmd))
               dabc::Command::Finalise(cmd);
         }

         break;
      }
      default:
         EOUT(("Core event %u arg:%u not processed", GetEventCode(evnt), GetEventArg(evnt)));
   }
}

void dabc::WorkingProcessor::ProcessEvent(EventId evnt)
{
   EOUT(("Event %u arg:%u not processed", GetEventCode(evnt), GetEventArg(evnt)));
}

bool dabc::WorkingProcessor::ActivateMainLoop()
{
   if (ProcessorThread())
      return ProcessorThread()->SetExplicitLoop(this);
   else
      return false;
}

void dabc::WorkingProcessor::ExitMainLoop()
{
   // exits main loop execution
   // MUST be called in destructor of processor, where virtual methods are defined

   if (ProcessorThread())
      ProcessorThread()->ExitMainLoop(this);
}

dabc::CommandDefinition* dabc::WorkingProcessor::NewCmdDef(const char* cmdname)
{
   if ((cmdname==0) || (strlen(cmdname)==0) || (fParsHolder==0)) return 0;

   return new dabc::CommandDefinition(fParsHolder, cmdname);
}

bool dabc::WorkingProcessor::DeleteCmdDef(const char* cmdname)
{
   if ((cmdname==0) || (strlen(cmdname)==0) || (fParsHolder==0)) return false;

   CommandDefinition* cmd = dynamic_cast<CommandDefinition*> (fParsHolder->FindChild(cmdname));

   if (cmd!=0) delete cmd;

   return cmd!=0;
}


// all about parameters handling

dabc::Folder* dabc::WorkingProcessor::MakeFolderForParam(const char* parname)
{
   std::string foldname = dabc::Folder::GetPathName(parname);

   if ((foldname.length()==0) || (fParsHolder==0)) return fParsHolder;

   return fParsHolder->GetFolder(foldname.c_str(), true, true);
}

unsigned dabc::WorkingProcessor::MakeParsFlags(unsigned visibility, bool fixed, bool changable)
{
   if (visibility > parsVisibilityMask) visibility = parsVisibilityMask;

   return visibility | (fixed ? parsFixedMask : 0) | (changable ? parsChangableMask : 0) | parsValidMask;
}


unsigned dabc::WorkingProcessor::SetParDflts(unsigned visibility, bool fixed, bool changable)
{
   unsigned old = fParsDefaults;

   fParsDefaults = MakeParsFlags(visibility, fixed, changable);

   return old;
}

bool dabc::WorkingProcessor::GetParDfltsVisible()
{
   unsigned level = fParsDefaults & parsVisibilityMask;

   return (level>0) && (level<=gParsVisibility);
}

bool dabc::WorkingProcessor::GetParDfltsFixed()
{
   return fParsDefaults & parsFixedMask;
}

bool dabc::WorkingProcessor::GetParDfltsChangable()
{
   return fParsDefaults & parsChangableMask;
}

dabc::Parameter* dabc::WorkingProcessor::CreatePar(int kind, const char* name, const char* initvalue, unsigned flags)
{
   dabc::Parameter* par = 0;

   unsigned oldflags = 0;
   if (flags & parsValidMask) {
      oldflags = fParsDefaults;
      fParsDefaults = flags;
   }

   switch (kind) {
      case dabc::parNone:
         break;
      case dabc::parString:
         par = new dabc::StrParameter(this, name, initvalue);
         break;
      case dabc::parDouble:
         par = new dabc::DoubleParameter(this, name, 0.0);
         break;
      case dabc::parInt:
         par = new dabc::IntParameter(this, name, 0);
         break;
      case dabc::parSyncRate:
         par = new dabc::RateParameter(this, name, true, 0.0);
         break;
      case dabc::parAsyncRate:
         par = new dabc::RateParameter(this, name, false, 0.0);
         break;
      case dabc::parStatus:
         par = new dabc::StatusParameter(this, name, 0);
         break;
      case dabc::parHisto:
         par = new dabc::HistogramParameter(this, name, 10);
         break;
      default:
         EOUT(("Unsupported parameter type"));
   }

   if (flags & parsValidMask)
      fParsDefaults = oldflags;

   if ((par!=0) && (initvalue!=0) && (kind!=dabc::parString)) {
      bool wasfixed = par->IsFixed();
      if (wasfixed) par->SetFixed(false);
      par->SetValue(initvalue);
      if (wasfixed) par->SetFixed(true);
   }

   return par;
}

dabc::Parameter* dabc::WorkingProcessor::CreateParStr(const char* name, const char* initvalue, unsigned flags)
{
   unsigned oldflags = 0;
   if (flags & parsValidMask) {
      oldflags = fParsDefaults;
      fParsDefaults = flags;
   }

   dabc::Parameter* par = new dabc::StrParameter(this, name, initvalue);

   if (flags & parsValidMask)
      fParsDefaults = oldflags;

   return par;
}

dabc::Parameter* dabc::WorkingProcessor::CreateParInt(const char* name, int initvalue, unsigned flags)
{
   unsigned oldflags = 0;
   if (flags & parsValidMask) {
      oldflags = fParsDefaults;
      fParsDefaults = flags;
   }

   dabc::Parameter* par = new dabc::IntParameter(this, name, initvalue);

   if (flags & parsValidMask)
      fParsDefaults = oldflags;

   return par;
}

dabc::Parameter* dabc::WorkingProcessor::CreateParDouble(const char* name, double initvalue, unsigned flags)
{
   unsigned oldflags = 0;
   if (flags & parsValidMask) {
      oldflags = fParsDefaults;
      fParsDefaults = flags;
   }

   dabc::Parameter* par = new dabc::DoubleParameter(this, name, initvalue);

   if (flags & parsValidMask)
      fParsDefaults = oldflags;

   return par;
}

dabc::Parameter* dabc::WorkingProcessor::CreateParBool(const char* name, bool initvalue, unsigned flags)
{
   return CreateParStr(name, initvalue ? xmlTrueValue : xmlFalseValue, flags);
}

dabc::Parameter* dabc::WorkingProcessor::CreateParInfo(const char* name, int verbose, const char* color)
{
   return new InfoParameter(this, name, verbose, color);
}


void dabc::WorkingProcessor::DestroyAllPars()
{
   Folder* f = GetTopParsFolder();
   if (f==0) return;

   Queue<Parameter*> pars(64, true);

   Iterator iter(f);
   while (iter.next()) {
      Parameter* par = dynamic_cast<Parameter*> (iter.current());
      if (par!=0) pars.Push(par);
   }

   while (pars.Size()>0)
      DestroyPar(pars.Pop());
}

bool dabc::WorkingProcessor::DestroyPar(Parameter* par)
{
   if (par==0) return false;

   Basic* prnt = par->GetParent();
   if (prnt) prnt->RemoveChild(par);

   if (!Parameter::FireEvent(par, parDestroy)) delete par;

   return true;
}


dabc::Parameter* dabc::WorkingProcessor::FindPar(const char* name) const
{
   dabc::Folder* f = ((WorkingProcessor*) this )->GetTopParsFolder();
   return f ? dynamic_cast<dabc::Parameter*>(f->FindChild(name)) : 0;
}

std::string dabc::WorkingProcessor::GetParStr(const char* name, const std::string& defvalue) const
{
   dabc::Parameter* par = FindPar(name);

   std::string str;

   if (par && par->GetValue(str)) return str;

   return defvalue;
}

int dabc::WorkingProcessor::GetParInt(const char* name, int defvalue) const
{
   dabc::Parameter* par = FindPar(name);

   return par && (par->Kind()==parInt) ? ((IntParameter*) par)->GetInt() : defvalue;
}

double dabc::WorkingProcessor::GetParDouble(const char* name, double defvalue) const
{
   dabc::Parameter* par = FindPar(name);

   if (par==0) return defvalue;

   if (par->Kind()==parDouble) return ((DoubleParameter*) par)->GetDouble();

   std::string sbuf;
   if (!par->GetValue(sbuf)) return defvalue;

   double value(defvalue);

   return sscanf(sbuf.c_str(), "%lf", &value) == 1 ? value : defvalue;
}

bool dabc::WorkingProcessor::GetParBool(const char* name, bool defvalue) const
{
   std::string res = GetParStr(name);
   if (res == xmlTrueValue) return true;
   if (res == xmlFalseValue) return false;
   return defvalue;
}

bool dabc::WorkingProcessor::HasCfgPar(const char* name, Command* cmd)
{
   if (cmd && cmd->HasPar(name)) return true;

   if (dabc::mgr()==0) return false;

   return dabc::mgr()->FindInConfiguration(GetTopParsFolder(), name);
}

std::string dabc::WorkingProcessor::GetCfgStr(const char* name, const std::string& dfltvalue, Command* cmd)
{
   if (cmd && cmd->HasPar(name))
      return cmd->GetStr(name, dfltvalue.c_str());

   WorkingProcessor* cfgsrc = this;
   while (cfgsrc)
      if (cfgsrc->FindPar(name))
         return cfgsrc->GetParStr(name, dfltvalue);
      else
      if (cfgsrc->HasCfgPar(name))
         break;
      else
         cfgsrc = cfgsrc->GetCfgMaster();

   if (cfgsrc==0) cfgsrc = this;

   cfgsrc->CreateParStr(name, dfltvalue.c_str(), gParsCfgDefaults);
   // if (par) par->SetFixed(true);
   return cfgsrc->GetParStr(name, dfltvalue);
}

double dabc::WorkingProcessor::GetCfgDouble(const char* name, double dfltvalue, Command* cmd)
{
   if (cmd && cmd->HasPar(name))
      return cmd->GetDouble(name, dfltvalue);

   WorkingProcessor* cfgsrc = this;
   while (cfgsrc)
      if (cfgsrc->FindPar(name))
         return cfgsrc->GetParDouble(name, dfltvalue);
      else
      if (cfgsrc->HasCfgPar(name))
         break;
      else
         cfgsrc = cfgsrc->GetCfgMaster();

   if (cfgsrc==0) cfgsrc = this;

   cfgsrc->CreateParDouble(name, dfltvalue, gParsCfgDefaults);
//   if (par) par->SetFixed(true);
   return cfgsrc->GetParDouble(name, dfltvalue);
}

int dabc::WorkingProcessor::GetCfgInt(const char* name, int dfltvalue, Command* cmd)
{
   if (cmd && cmd->HasPar(name))
      return cmd->GetInt(name, dfltvalue);

   WorkingProcessor* cfgsrc = this;
   while (cfgsrc)
      if (cfgsrc->FindPar(name))
         return cfgsrc->GetParInt(name, dfltvalue);
      else
      if (cfgsrc->HasCfgPar(name))
         break;
      else
         cfgsrc = cfgsrc->GetCfgMaster();

   if (cfgsrc==0) cfgsrc = this;

   cfgsrc->CreateParInt(name, dfltvalue, gParsCfgDefaults);
//   if (par) par->SetFixed(true);
   return cfgsrc->GetParInt(name, dfltvalue);
}

bool dabc::WorkingProcessor::GetCfgBool(const char* name, bool dfltvalue, Command* cmd)
{
   if (cmd && cmd->HasPar(name))
      return cmd->GetBool(name, dfltvalue);

   WorkingProcessor* cfgsrc = this;
   while (cfgsrc)
      if (cfgsrc->FindPar(name))
         return cfgsrc->GetParBool(name, dfltvalue);
      else
      if (cfgsrc->HasCfgPar(name))
         break;
      else
         cfgsrc = cfgsrc->GetCfgMaster();

   if (cfgsrc==0) cfgsrc = this;

   cfgsrc->CreateParBool(name, dfltvalue, gParsCfgDefaults);
//   if (par) par->SetFixed(true);
   return cfgsrc->GetParBool(name, dfltvalue);
}

bool dabc::WorkingProcessor::SetParStr(const char* name, const char* value)
{
   dabc::Parameter* par = FindPar(name);

   return par ? par->SetValue(value) : false;
}

bool dabc::WorkingProcessor::SetParBool(const char* name, bool value)
{
   dabc::Parameter* par = FindPar(name);

   return par ? par->SetValue(value ? xmlTrueValue : xmlFalseValue) : false;
}

bool dabc::WorkingProcessor::SetParInt(const char* name, int value)
{
   dabc::Parameter* par = FindPar(name);

   DOUT5(("SetParInt par = %p name = %s v = %d", par, name, value));

   if (par==0) return false;

   if (par->Kind()==parInt) return ((IntParameter*) par)->SetInt(value);

   return par->SetValue(dabc::format("%d", value));
}

bool dabc::WorkingProcessor::SetParDouble(const char* name, double value)
{
   dabc::Parameter* par = FindPar(name);

   DOUT5(("SetParDouble par = %p name = %s v = %f", par, name, value));

   if (par==0) return false;

   if (par->Kind()==parDouble) return ((DoubleParameter*) par)->SetDouble(value);

   return par->SetValue(dabc::format("%f", value));
}

bool dabc::WorkingProcessor::SetParFixed(const char* name, bool on)
{
   dabc::Parameter* par = FindPar(name);
   if (par==0) return false;

   par->SetFixed(on);

   return true;
}


void dabc::WorkingProcessor::LockUnlockPars(bool on)
{
   dabc::Iterator iter(GetTopParsFolder());

   while (iter.next()) {
      dabc::Parameter* par =
         dynamic_cast<dabc::Parameter*>(iter.current());
      if (par!=0)
         par->SetFixed(on);
   }
}

bool dabc::WorkingProcessor::InvokeParChange(Parameter* par, const char* value, Command* cmd)
{
   std::string fullname;
   par->MakeFullName(fullname, GetTopParsFolder());

   DOUT5(("Invoke par change %s topfold:%s", fullname.c_str(), GetTopParsFolder()->GetName()));

   if (cmd==0)
      cmd = new CmdSetParameter(fullname.c_str(), value);
   else {
      cmd->SetStr("ParName", fullname);
      if (value!=0) cmd->SetStr("ParValue", value);
   }

   Submit(cmd);

   return true;
}


int dabc::WorkingProcessor::PreviewCommand(Command* cmd)
{
   int cmd_res = cmd_ignore;

   DOUT3(("WorkingProcessor::PreviewCommand %s", cmd->GetName()));

   if (cmd->IsName(CmdSetParameter::CmdName())) {

      Parameter* par = FindPar(cmd->GetPar("ParName"));

      DOUT5(("Found par %s = %p", cmd->GetPar("ParName"), par));

      if (par==0) {
         EOUT(("Did not found parameter %s", cmd->GetPar("ParName")));
         return cmd_false;
      }

      const char* value = cmd->GetPar("ParValue");

      if (par->IsFixed()) {
         EOUT(("Parameter %s is fixed - cannot change to value %s", par->GetName(), value));
         return cmd_false;
      }

      if (par->SetValue(value)) {
         DOUT4(("Change parameter %s to value %s", par->GetName(), value));
         cmd_res = cmd_true;
      } else {
         EOUT(("Fail to set %s to parameter %s", value, par->GetName()));
         cmd_res = cmd_false;
      }

      // ParameterChanged(par);
   } else
   if (cmd->IsName("SyncProcessor")) {
      // this is just dummy command, which is submitted with minimum priority
      cmd_res = cmd_true;
   }

   return cmd_res;
}

/** This method should be used to execute command synchronously from processor itself.
 *  Method let thread event loop running.
 */

int dabc::WorkingProcessor::ExecuteIn(dabc::WorkingProcessor* dest, dabc::Command* cmd, double tmout)
{
   int res = cmd_true;

   if (cmd==0) {
      EOUT(("Command not specified"));
      res = cmd_false;
   } else {
      LockGuard lock(fProcessorMainMutex);

      if (fProcessorThread==0) {
         EOUT(("Cannot execute command without working thread"));
         res = cmd_false;
      } else
      if (!fProcessorThread->IsItself()) {
         EOUT(("Cannot call ExecuteIn from other thread!!!"));
         res = cmd_false;
      }
   }

   if ((res == cmd_false) || (dest==0)) {
      dabc::Command::Finalise(cmd);
      return cmd_false;
   }

   bool exe_ready = false;

   cmd->AddCaller(this, &exe_ready);

   DOUT5(("Calling ExecteIn in thread %s", fProcessorThread->GetName()));

   TimeStamp_t last_tm = NullTimeStamp;

   if (dest->Submit(cmd, tmout<-100 ? priorityMinimum : priorityDefault)) {

//      DOUT0(("Command is submitted, we start waiting for result"));

      // we can access exe_ready directly, while this flag only access from caller thread
      while (!exe_ready) {

         // account timeout
         if (tmout>0.) {
            TimeStamp_t tm = fProcessorThread->ThrdTimeStamp();

            if (!IsNullTime(last_tm)) {
               tmout -= TimeDistance(last_tm, tm);
               if (tmout<=0.) { res = cmd_false; break; }
            }
            last_tm = tm;
         };

//         DOUT0(("Calling single loop %5.1f", (tmout<=0) ? 0.1 : tmout));

         SingleLoop((tmout<=0) ? 0.1 : tmout);
      }
   }

   DOUT5(("Proc %p Cmd %p ready = %s", this, cmd, DBOOL(exe_ready)));

   if (exe_ready) res = cmd->GetResult();

   dabc::Command::Finalise(cmd);
   return res;
}


int dabc::WorkingProcessor::ExecuteIn(WorkingProcessor* dest, const char* cmdname, double tmout)
{
   return ExecuteIn(dest, new dabc::Command(cmdname), tmout);
}

int dabc::WorkingProcessor::Execute(Command* cmd, double tmout)
{
   if (cmd==0) return cmd_false;

   int res = cmd_true;

   WorkingThread* thrd = 0;

   if (cmd==0) {
      EOUT(("Command not specified"));
      res = cmd_false;
   } else {
      LockGuard lock(fProcessorMainMutex);

      if (fProcessorThread==0) {
         EOUT(("Cannot execute command without working thread"));
         res = cmd_false;
      } else
      if (fProcessorThread->IsItself()) thrd = fProcessorThread;
   }

   if (res == cmd_false) {
      dabc::Command::Finalise(cmd);
      return cmd_false;
   }

   // if command was called not from the same thread,
   // find handle of caller thread to let run its event loop
   if ((thrd==0) && (dabc::mgr()!=0))
      thrd = dabc::mgr()->CurrentThread();

   if (thrd) return ((WorkingProcessor*) thrd->fExec)->ExecuteIn(this, cmd, tmout);

   // if there is no Thread with such id (most probably, some user-managed thrd)
   // than we create fake object only to handle commands and events,
   // but not really take over thread mainloop
   // This object is not seen from the manager, therefore many such instances may exist in parallel

   DOUT0(("We really creating dummy thread for cmd %s", cmd->GetName()));

   WorkingThread curr(0, "Current");
   curr.Start(0, true);
   return ((WorkingProcessor*) curr.fExec)->ExecuteIn(this, cmd, tmout);
}

int dabc::WorkingProcessor::Execute(const char* cmdname, double tmout)
{
   return Execute(new dabc::Command(cmdname), tmout);
}

int dabc::WorkingProcessor::ExecuteInt(const char* cmdname, const char* intresname, double timeout_sec)
{
   dabc::Command* cmd = new dabc::Command(cmdname);
   cmd->SetKeepAlive();

   int res = -1;

   if (Execute(cmd, timeout_sec))
      res = cmd->GetInt(intresname, -1);

   dabc::Command::Finalise(cmd);

   return res;
}

void dabc::WorkingProcessor::SyncProcessor()
{
   Execute("SyncProcessor", -1000.);
}

std::string dabc::WorkingProcessor::ExecuteStr(const char* cmdname, const char* strresname, double timeout_sec)
{
   dabc::Command* cmd = new dabc::Command(cmdname);
   cmd->SetKeepAlive();

   std::string res;

   if (Execute(cmd, timeout_sec))
      res = cmd->GetStr(strresname, "");

   dabc::Command::Finalise(cmd);

   return res;
}



bool dabc::WorkingProcessor::Assign(Command* cmd)
{
   if (cmd==0) return false;

   LockGuard lock(fProcessorMainMutex);

   if (fProcessorThread==0) {
      EOUT(("Command %s cannot be assigned - thread is not assigned", cmd->GetName()));
      return false;
   }

   cmd->AddCaller(this, 0);

   return true;
}

bool dabc::WorkingProcessor::Submit(dabc::Command* cmd, int priority)
{
   if (cmd==0) return false;

   bool res = true;

   if (priority == priorityDefault) priority = 0;

   {
      LockGuard lock(fProcessorMainMutex);

      if (fProcessorThread==0) {
         EOUT(("Command %s cannot be executed - thread is not assigned", cmd->GetName()));
         res = false;
      } else {
         uint32_t id = fProcessorSubmCommands.Push(cmd);

         DOUT5(("Submit command %s with id %u to processor %p %u thrd %p", cmd->GetName(), id, this, fProcessorId, fProcessorThread));

         _FireEvent(evntSubmitCommand, id, priority);
      }
   }

   if (!res) dabc::Command::Reply(cmd, false);

   return res;
}

bool dabc::WorkingProcessor::GetReply(dabc::Command* cmd)
{
   if (cmd==0) return false;

   LockGuard lock(fProcessorMainMutex);

   if (fProcessorThread==0) {
      EOUT(("Command %s cannot be get for reply - thread is not assigned", cmd->GetName()));
      return false;
   }

   uint32_t id = fProcessorReplyCommands.Push(cmd);
   _FireEvent(evntReplyCommand, id, 0);

   return true;
}

void dabc::WorkingProcessor::CancelCommands()
{
   fProcessorSubmCommands.Cleanup();
   fProcessorReplyCommands.Cleanup();
}

void dabc::WorkingProcessor::ProcessorSleep(double tmout)
{
   if (ProcessorThread())
      ProcessorThread()->RunEventLoop(tmout);
   else {
      while (tmout>1) { dabc::LongSleep(1); tmout-=1.; }
      dabc::MicroSleep(lrint(tmout*1e6));
   }
}
