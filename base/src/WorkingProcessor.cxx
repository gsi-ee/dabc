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
   fProcessorAssignCommands(CommandsQueue::kindAssign),
   fProcessorCommands(CommandsQueue::kindSubmit),
   fProcessorCommandsLevel(0),

   fParsHolder(parsholder),
   fParsDefaults(0),
   fProcessorActivateTmout(false),
   fProcessorActivateMark(NullTimeStamp),
   fProcessorActivateInterv(0.),
   fProcessorPrevFire(NullTimeStamp),
   fProcessorNextFire(NullTimeStamp),
   fProcessorRecursion(0),
   fProcessorDestroyment(false),
   fProcessorHalted(false),
   fProcessorHaltCommands(CommandsQueue::kindSubmit),
   fProcessorStopped(false)
{
   SetParDflts(1, false, true);
}

dabc::WorkingProcessor::~WorkingProcessor()
{
   DOUT5(("~WorkingProcessor %p %d thrd:%p", this, fProcessorId, fProcessorThread));

   CancelCommands();

   DestroyAllPars();

   HaltProcessor();

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

/** Method to exit from recursion of event processing
 * No any event will be processed afterwards */

bool dabc::WorkingProcessor::HaltProcessor()
{
   DOUT3(("Halt processor %p thrd %p stopped %s halted %s", this, fProcessorThread, DBOOL(fProcessorStopped), DBOOL(fProcessorHalted)));

   {
      LockGuard lock(fProcessorMainMutex);

      if (fProcessorStopped) {
         if (!fProcessorHalted)
            EOUT(("Second Halt processor while previous is not finished"));
         return fProcessorHalted;
      }

      if (fProcessorThread==0) return true;

      fProcessorStopped = true;

      if (fProcessorThread->IsItself()) {
         fProcessorHalted = true;
         if (fProcessorRecursion==0) return true;
         EOUT(("Call HaltProcessor from the thread itself when recursion is non-zero, should be solved by other way"));
         return false;
      }
   }

   DOUT3(("Halt processor %p thrd %p with command", this, fProcessorThread));

   return Execute("HaltProcessor", -1., priorityMagic) == cmd_true;
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
   if (fProcessorDestroyment) return;

   fProcessorDestroyment = true;

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

         dabc::Command* cmd = 0;
         {
            LockGuard lock(fProcessorMainMutex);
            cmd = fProcessorSubmCommands.PopWithId(GetEventArg(evnt));
         }


         ProcessCommand(cmd);

         while ((fProcessorCommandsLevel==0) && (fProcessorCommands.Size()>0))
            ProcessCommand(fProcessorCommands.Pop());

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

      case evntPostCommand: {
         if (fProcessorCommandsLevel!=0) {
            EOUT(("One should finish here with commands"));
            exit(1);
         }

         while (fProcessorCommands.Size()>0)
            ProcessCommand(fProcessorCommands.Pop());
      }

      default:
         EOUT(("Core event %u arg:%u not processed", GetEventCode(evnt), GetEventArg(evnt)));
   }
}

int dabc::WorkingProcessor::ProcessCommand(dabc::Command* cmd)
{
   if (cmd==0) return cmd_false;

   // when other command is in processing state,
   // and this cmd not need to be synchronous,
   // one can process such command later
   // synchronous command must be processed immediately
   if ((fProcessorCommandsLevel>0) && !cmd->IsLastCallerSync()) {
      fProcessorCommands.Push(cmd);
      return cmd_postponed;
   }

   DOUT3(("ProcessCommand cmd %p %s lvl %d isync %s", cmd, cmd->GetName(), fProcessorCommandsLevel, DBOOL(cmd->IsLastCallerSync())));

   WorkingThread::IntGuard guard(fProcessorCommandsLevel);

   int cmd_res = PreviewCommand(cmd);

   if (cmd_res == cmd_ignore)
      cmd_res = ExecuteCommand(cmd);

   if (cmd_res == cmd_ignore) {
      EOUT(("Command ignored %s", cmd->GetName()));
      cmd_res = cmd_false;
   }

   bool completed = (cmd_res>=0);

   if (completed)
      dabc::Command::Reply(cmd, cmd_res);

   DOUT3(("ProcessCommand cmd %p lvl %d done still queued %u", cmd, fProcessorCommandsLevel, fProcessorCommands.Size()));

   return cmd_res;
}

void dabc::WorkingProcessor::ProcessEvent(EventId evnt)
{
   EOUT(("Event %u arg:%u not processed req:%s", GetEventCode(evnt), GetEventArg(evnt),
         (RequiredThrdClass() ? RequiredThrdClass() : "dflt")));
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

dabc::Parameter* dabc::WorkingProcessor::CreateParInfo(const char* name, int verbose, const char* color, unsigned flags)
{
   unsigned oldflags = 0;
   if (flags & parsValidMask) {
      oldflags = fParsDefaults;
      fParsDefaults = flags;
   }

   dabc::Parameter* par = new InfoParameter(this, name, verbose, color);

   if (flags & parsValidMask)
      fParsDefaults = oldflags;

   return par;
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

   DOUT5(("WorkingProcessor::PreviewCommand %s", cmd->GetName()));

   if (cmd->IsName("HaltProcessor")) {

      DOUT3(("WorkingProcessor::HaltProcessor thrd %p", fProcessorThread));

      if (fProcessorHalted)
         cmd_res = cmd_true;
      else
      if ((fProcessorRecursion!=1) || (fProcessorCommandsLevel!=1)) {
         EOUT(("Event %d or command %d recursion not 1 when halt processor %p", fProcessorRecursion, fProcessorCommandsLevel, this));
         fProcessorHaltCommands.Push(cmd);
         cmd_res = cmd_postponed;
      } else {
         LockGuard lock(fProcessorMainMutex);
         fProcessorHalted = true;
         cmd_res = cmd_true;
      }
   } else

   if (cmd->IsName(CmdSetParameter::CmdName())) {

      Parameter* par = FindPar(cmd->GetPar("ParName"));

      DOUT3(("Found par %s = %p", cmd->GetPar("ParName"), par));

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
         DOUT3(("Change parameter %s to value %s", par->GetName(), value));
         cmd_res = cmd_true;
      } else {
         EOUT(("Fail to set %s to parameter %s", value, par->GetName()));
         cmd_res = cmd_false;
      }

      // ParameterChanged(par);
   } else

   if (cmd->IsName("CreateInfoParameter")) {
      const char* parname = cmd->GetPar("ParName");

      Parameter* par = FindPar(parname);
      if (par!=0)
         cmd_res = cmd_false;
      else {
         unsigned flags = MakeParsFlags(1, false, false);

         cmd_res = cmd_bool(CreateParInfo(parname, 1 , "Green", flags)!=0);
      }
   } else

   if (cmd->IsName("DestroyParameter")) {
      cmd_res = cmd_bool(DestroyPar(cmd->GetPar("ParName")));
   } else

   if (cmd->IsName("SyncProcessor")) {
      // this is just dummy command, which is submitted with minimum priority
      cmd_res = cmd_true;
   }

   return cmd_res;
}

int dabc::WorkingProcessor::ExecuteCommand(Command* cmd)
{
   return cmd_false;
}

bool dabc::WorkingProcessor::ReplyCommand(Command* cmd)
{
   /** For backward compatibility keep call to _ProcessReply,
    *  will be removed with next major release */
   return ! _ProcessReply(cmd);
}


/** This method should be used to execute command synchronously from processor itself.
 *  Method let thread event loop running.
 */

int dabc::WorkingProcessor::ExecuteIn(dabc::WorkingProcessor* dest, dabc::Command* cmd, double tmout, int priority)
{
   WorkingThread::IntGuard iguard(fProcessorRecursion);

   int res = cmd_true;

   if (cmd==0) {
      EOUT(("Command not specified"));
      res = cmd_false;
   } else {
      LockGuard lock(fProcessorMainMutex);

      if (fProcessorThread==0) {
         EOUT(("Cannot execute command %s without working thread", cmd->GetName()));
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

   if (dest->Submit(cmd, priority)) {

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
         }

         // FIXME when break processor, one should cancel command execution correctly
         if (IsProcessorDestroyment() || IsProcessorHalted()) { res = cmd_false; break; }

         SingleLoop((tmout<=0) ? 0.1 : tmout);
      }
   }

   DOUT5(("Proc %p Cmd %p ready = %s", this, cmd, DBOOL(exe_ready)));

   if (exe_ready) res = cmd->GetResult();

   dabc::Command::Finalise(cmd);

   return res;
}

int dabc::WorkingProcessor::ExecuteIn(WorkingProcessor* dest, const char* cmdname, double tmout, int priority)
{
   return ExecuteIn(dest, new dabc::Command(cmdname), tmout, priority);
}

int dabc::WorkingProcessor::Execute(Command* cmd, double tmout, int priority)
{
   if (cmd==0) return cmd_false;

   WorkingThread* thrd = 0;
   bool exe_direct = false;

   {
      LockGuard lock(fProcessorMainMutex);

      if ((fProcessorThread==0) || (fProcessorId==0)) {
         EOUT(("Cannot execute command %s without working thread, do directly", cmd->GetName()));
         exe_direct = true;
      } else
      if (fProcessorThread->IsItself()) thrd = fProcessorThread;
   }

   if (exe_direct) return ProcessCommand(cmd);

   // if command was called not from the same thread,
   // find handle of caller thread to let run its event loop
   if ((thrd==0) && (dabc::mgr()!=0))
      thrd = dabc::mgr()->CurrentThread();

   if (thrd) return ((WorkingProcessor*) thrd->fExec)->ExecuteIn(this, cmd, tmout, priority);

   // if there is no Thread with such id (most probably, some user-managed thrd)
   // than we create fake object only to handle commands and events,
   // but not really take over thread mainloop
   // This object is not seen from the manager, therefore many such instances may exist in parallel

   DOUT5(("We really creating dummy thread for cmd %s", cmd->GetName()));

   WorkingThread curr(0, "Current");
   curr.Start(0, true);
   return ((WorkingProcessor*) curr.fExec)->ExecuteIn(this, cmd, tmout, priority);
}

int dabc::WorkingProcessor::Execute(const char* cmdname, double tmout, int priority)
{
   return Execute(new dabc::Command(cmdname), tmout, priority);
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
   if (ProcessorThread()!=0)
      ProcessorThread()->SysCommand("SyncProcessor", this, priorityMinimum);
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

dabc::Command* dabc::WorkingProcessor::Assign(Command* cmd)
{
   if (cmd==0) return 0;

   LockGuard lock(fProcessorMainMutex);

   if (fProcessorThread==0) {
      EOUT(("Command %s cannot be assigned - thread is not assigned", cmd->GetName()));
      return 0;
   }

   fProcessorAssignCommands.Push(cmd);

   cmd->AddCaller(this, 0);

   return cmd;
}

bool dabc::WorkingProcessor::Submit(dabc::Command* cmd, int priority)
{
   if (cmd==0) return false;

   bool res = true;

   {
      LockGuard lock(fProcessorMainMutex);

      if ((fProcessorThread==0) || (fProcessorId==0)) {
         EOUT(("Command %s cannot be submitted - thread is not assigned", cmd->GetName()));
         res = false;
      } else
      if (fProcessorStopped && (priority != priorityMagic)) {
         res = false;
      } else {
         if (priority == priorityMagic) priority = fProcessorPriority; else
         if (priority == priorityDefault) priority = 0; else
         if (priority == priorityMinimum) priority = -1;


         uint32_t arg = fProcessorSubmCommands.Push(cmd);

         DOUT5(("Submit command %s with id %u to processor %p %u thrd %s priority %d", cmd->GetName(), arg, this, fProcessorId, DNAME(fProcessorThread), priority));

         fProcessorThread->_Fire(CodeEvent(evntSubmitCommand, fProcessorId, arg), priority);
      }
   }

   if (!res) dabc::Command::Reply(cmd, cmd_false);

   return res;
}

bool dabc::WorkingProcessor::GetCommandReply(dabc::Command* cmd, bool* exe_ready)
{
   if (cmd==0) return false;

   if (fProcessorMainMutex==0) {
      EOUT(("!!!!!!!!!!! Proc %p GetCommandReply %s without mutex", this, cmd->GetName()));
   }

   LockGuard lock(fProcessorMainMutex);

   if (exe_ready) {
      *exe_ready = true;
      _FireDoNothingEvent();
      return true;
   }

   fProcessorAssignCommands.RemoveCommand(cmd);

   if (!_IsFireEvent()) {
      EOUT(("Command %s cannot be get for reply - thread is not assigned", cmd->GetName()));
      return false;
   }

   uint32_t id = fProcessorReplyCommands.Push(cmd);
   _FireEvent(evntReplyCommand, id, 0);

   return true;
}

void dabc::WorkingProcessor::CancelCommands()
{
   fProcessorSubmCommands.Cleanup(fProcessorMainMutex);
   fProcessorReplyCommands.Cleanup(fProcessorMainMutex);
   fProcessorAssignCommands.Cleanup(fProcessorMainMutex, this);

//   fProcessorSubmCommands.Cleanup(0);
//   fProcessorReplyCommands.Cleanup(0);
//   fProcessorAssignCommands.Cleanup(0, this);

}

void dabc::WorkingProcessor::ProcessorSleep(double tmout)
{
   if (ProcessorThread())
      ProcessorThread()->RunEventLoop(this, tmout);
   else {
      while (tmout>1) { dabc::LongSleep(1); tmout-=1.; }
      dabc::MicroSleep(lrint(tmout*1e6));
   }
}
