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
   fProcessorCommands(false, true),
   fProcessorNewCommands(false, true),
   fProcessorReplyCommands(true, true),
   fProcessorExecutingCmd(0),
   fProcessorExecutingFlag(false),
   fParsHolder(parsholder),
   fProcessorPrivateMutex(),
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

   if (forget_thread) fProcessorThread = 0;
}

void dabc::WorkingProcessor::DestroyProcessor()
{
   if (fProcessorThread)
      fProcessorThread->DestroyProcessor(this);
   else
      delete this;
}


bool dabc::WorkingProcessor::Submit(Command* cmd)
{
   DOUT5(("Submit command %s to thread %p %d", cmd->GetName(), fProcessorThread,
         fProcessorThread ? fProcessorThread->Id() : 0));

   if (fProcessorThread)
      return fProcessorThread->SubmitProcessorCmd(this, cmd);

   return dabc::CommandReceiver::Submit(cmd);
}

bool dabc::WorkingProcessor::IsExecutionThread()
{
   return (fProcessorThread==0) || fProcessorThread->IsItself();
}

void dabc::WorkingProcessor::ActivateTimeout(double tmout_sec)
{
   if (fProcessorThread==0) return;

   TimeStamp_t now = fProcessorThread->ThrdTimeStamp();

   bool dofire = false;

   {
      LockGuard lock(fProcessorPrivateMutex);

      dofire = !fProcessorActivateTmout;

      fProcessorActivateMark = now;
      fProcessorActivateInterv = tmout_sec;
      fProcessorActivateTmout = true;
   }

   if (dofire)
      fProcessorThread->Fire(CodeEvent(WorkingThread::evntCheckTmout, 0, fProcessorId), ProcessorPriority());
}

bool dabc::WorkingProcessor::TakeActivateData(TimeStamp_t& mark, double& interval)
{
   LockGuard lock(fProcessorPrivateMutex);
   if (!fProcessorActivateTmout) return false;

   mark = fProcessorActivateMark;
   interval = fProcessorActivateInterv;

   fProcessorActivateTmout = false;
   return true;
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

   if (cmd==0) exit(1);

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
      cmd_res = CommandReceiver::PreviewCommand(cmd);

   return cmd_res;
}

/** This method should be used to execute command synchronously from processor itself.
 *  Method let thread event loop running.
 */

int dabc::WorkingProcessor::NewCmd_ExecuteIn(dabc::WorkingProcessor* dest, dabc::Command* cmd, double tmout)
{

   if ((fProcessorThread==0) && (dest!=0))
      return dest->NewCmd_Execute(cmd, tmout);


   int res = cmd_true;

   if ((cmd==0) || (dest==0))
      res = cmd_false;
   else {
      LockGuard lock(fProcessorPrivateMutex);

      if (fProcessorExecutingCmd!=0) {
         EOUT(("Processor already executing command"));
         res = cmd_false;
      } else {
         fProcessorExecutingCmd = cmd;
         cmd->fProcessor = this;
         cmd->fProcessorMutex = &fProcessorPrivateMutex;

         fProcessorExecutingFlag = true;
      }
   }

   if (res==cmd_false) {
      dabc::Command::Finalise(cmd);
      return cmd_false;
   }

   if (dest->NewCmd_Submit(cmd)) {
      // here one should wait for reply event during specified time

      TimeStamp_t last_tm = NullTimeStamp;

      while (fProcessorExecutingFlag) {

         // account timeout
         if (tmout>0.) {
            TimeStamp_t tm = fProcessorThread ? fProcessorThread->ThrdTimeStamp() : TimeStamp();

            if (!IsNullTime(last_tm)) {
               tmout -= TimeDistance(last_tm, tm);
               if (tmout<=0.) { res = cmd_false; break; }
            }
            last_tm = tm;
         };

         SingleLoop(tmout);
      }

   } else
      res = cmd_false;


   {
      LockGuard lock(fProcessorPrivateMutex);
      if (fProcessorExecutingCmd) {
         fProcessorExecutingCmd->fProcessor = 0;
         fProcessorExecutingCmd->fProcessorMutex = 0;
      }

      cmd = fProcessorExecutingCmd;

      fProcessorExecutingCmd = 0;
      fProcessorExecutingFlag = false;
   }

   if (res == cmd_true)
      res = cmd->GetResult();

   dabc::Command::Finalise(cmd);

   return res;
}

int dabc::WorkingProcessor::NewCmd_ExecuteIn(WorkingProcessor* dest, const char* cmdname, double tmout)
{
   return NewCmd_ExecuteIn(dest, new dabc::Command(cmdname), tmout);
}


int dabc::WorkingProcessor::NewCmd_Execute(Command* cmd, double tmout)
{
   if (fProcessorThread==0)
      return NewCmd_ProcessCommand(cmd);

   if (fProcessorThread && fProcessorThread->IsItself()) {
      DOUT5(("Direct execute from thread itself %s", cmd->GetName()));

      return NewCmd_ExecuteIn(this, cmd, tmout);
   }

   dabc::Condition cond;

   cmd->fProcessorMutex = cond.CondMutex();
   cmd->fProcessorCond = &cond;

   if (!NewCmd_Submit(cmd)) {
      dabc::Command::Finalise(cmd);
      return cmd_false;
   }

   int res = cmd_false;

   if (cond.DoWait(tmout))
      res = cmd->GetResult();

   dabc::Command::Finalise(cmd);

   return res;
}

int dabc::WorkingProcessor::NewCmd_Execute(const char* cmdname, double tmout)
{
   return NewCmd_Execute(new dabc::Command(cmdname), tmout);
}


bool dabc::WorkingProcessor::NewCmd_Submit(dabc::Command* cmd)
{
   if (fProcessorThread)
      return fProcessorThread->NewCmd_SubmitProcessorCmd(this, cmd);

   return NewCmd_ProcessCommand(cmd) != cmd_false;;
}

int dabc::WorkingProcessor::NewCmd_ProcessCommand(dabc::Command* cmd)
{
   /// this method perform command processing
   // return true if command processed and result is true

   if (cmd==0) return cmd_false;

   DOUT5(("ProcessCommand command %p cli:%p", cmd, this));

   int cmd_res = cmd_ignore;

   if (cmd->IsCanceled())
      cmd_res = cmd_false;
   else
      cmd_res = PreviewCommand(cmd);

   if (cmd_res == cmd_ignore)
      cmd_res = ExecuteCommand(cmd);

   if (cmd_res == cmd_ignore) {
      EOUT(("Command ignored %s", cmd->GetName()));
      cmd_res = cmd_false;
   }

   bool completed = (cmd_res>=0);

   DOUT5(("Execute command %p %s res = %d", cmd, (completed ? cmd->GetName() : "-----"), cmd_res));

   if (completed)
      dabc::Command::Reply(cmd, cmd_res);

   DOUT5(("ProcessCommand command %p done res = %d", cmd, cmd_res));

   return cmd_res;
}

bool dabc::WorkingProcessor::NewCmd_ProcessReply(dabc::Command* cmd)
{
   // returns true, if object can be deleted, otherwise user is responsible for the command

   return true;
}


bool dabc::WorkingProcessor::NewCmd_GetReply(dabc::Command* cmd)
{
   // return true indicates that responsibility is completely back to this processor,
   // return false will cause immediate deletion of the command

   if (fProcessorThread)
      if (fProcessorThread->NewCmd_SubmitProcessorReplyCmd(this, cmd)) return true;

   if (cmd == fProcessorExecutingCmd) { fProcessorExecutingFlag = false; return true; }

   return !NewCmd_ProcessReply(cmd);
}

void dabc::WorkingProcessor::NewCmd__Forget(dabc::Command* cmd)
{
   // exclude command from any lists, while command will be destroyed immediately
   // Actually, command should never be in the list when it is destroyed.
   // If something goes wrong, one should call dabc::Command::Reply(cmd, false).
   // In this place just exit

   if (cmd==0) return;

   EOUT(("_Forget should never be called, just exit"));

   exit(1);
}



