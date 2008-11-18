#include "dabc/WorkingProcessor.h"

#include "dabc/Folder.h"
#include "dabc/Command.h"
#include "dabc/Parameter.h"
#include "dabc/Manager.h"
#include "dabc/Iterator.h"

dabc::WorkingProcessor::WorkingProcessor() :
   fProcessorThread(0),
   fProcessorId(0),
   fProcessorPriority(-1), // minimum priority per default
   fProcessorCommands(false, true),
   fParsHolder(0),
   fProcessorPars(),
   fProcessorMutex(),
   fParsDfltVisibility(1),
   fParsDfltFixed(false),
   fProcessorActivateTmout(false),
   fProcessorActivateMark(NullTimeStamp),
   fProcessorActivateInterv(0.),
   fProcessorPrevFire(NullTimeStamp),
   fProcessorNextFire(NullTimeStamp)
{
   if (fProcessorPars.length()>0)
      if (fProcessorPars[fProcessorPars.length()-1]!='/')
         fProcessorPars.append("/");
}

dabc::WorkingProcessor::~WorkingProcessor()
{
   DOUT5(("~WorkingProcessor %p %d thrd:%p", this, fProcessorId, fProcessorThread));

   RemoveProcessorFromThread(true);

   DOUT5(("~WorkingProcessor %p %d thrd:%p done", this, fProcessorId, fProcessorThread));
}

void dabc::WorkingProcessor::SetParsHolder(Folder* holder, const char* subfolder)
{
   fParsHolder = holder;
   if (subfolder!=0) fProcessorPars = subfolder;
                else fProcessorPars.clear();
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
      LockGuard lock(fProcessorMutex);

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
   LockGuard lock(fProcessorMutex);
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


// all about parameters handling

dabc::Folder* dabc::WorkingProcessor::MakeFolderForParam(const char* parname)
{
   dabc::String foldname = fProcessorPars + dabc::Folder::GetPathName(parname);

   if ((foldname.length()==0) || (fParsHolder==0)) return fParsHolder;

   return fParsHolder->GetFolder(foldname.c_str(), true, true);
}

dabc::Folder* dabc::WorkingProcessor::GetTopParsFolder()
{
   if ((fProcessorPars.length()==0) || (fParsHolder==0)) return fParsHolder;

   return fParsHolder->GetFolder(fProcessorPars.c_str(), false, false);
}

void dabc::WorkingProcessor::SetParDflts(int visibility, bool fixed)
{
   fParsDfltVisibility = visibility;
   fParsDfltFixed = fixed;
}

dabc::Parameter* dabc::WorkingProcessor::CreatePar(int kind, const char* name, const char* initvalue)
{
   dabc::Parameter* par = 0;

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

   if ((par!=0) && (initvalue!=0) && (kind!=dabc::parString))
      par->SetStr(initvalue);

   return par;
}

dabc::Parameter* dabc::WorkingProcessor::CreateStrPar(const char* name, const char* initvalue)
{
   return new dabc::StrParameter(this, name, initvalue);
}

dabc::Parameter* dabc::WorkingProcessor::CreateIntPar(const char* name, int initvalue)
{
   return new dabc::IntParameter(this, name, initvalue);
}

dabc::Parameter* dabc::WorkingProcessor::CreateDoublePar(const char* name, double initvalue)
{
   return new dabc::DoubleParameter(this, name, initvalue);
}


void dabc::WorkingProcessor::DestroyParameter(const char* name)
{
   dabc::Parameter* par = FindPar(name);
   if (par!=0) delete par;
}

dabc::Parameter* dabc::WorkingProcessor::FindPar(const char* name) const
{
   dabc::Folder* f = ((WorkingProcessor*) this )->GetTopParsFolder();
   return f ? dynamic_cast<dabc::Parameter*>(f->FindChild(name)) : 0;
}

void dabc::WorkingProcessor::DeletePar(const char* name)
{
   dabc::Parameter* par = FindPar(name);

   if (par!=0) delete par;
}

bool dabc::WorkingProcessor::SetParValue(const char* name, const char* value)
{
   dabc::Parameter* par = FindPar(name);

   return par ? par->SetStr(value) : false;
}

bool dabc::WorkingProcessor::SetParValue(const char* name, int value)
{
   dabc::Parameter* par = FindPar(name);

   DOUT5(("SetParInt par = %p name = %s v = %d", par, name, value));

   return par ? par->SetInt(value) : false;
}

bool dabc::WorkingProcessor::SetParFixed(const char* name, bool on)
{
   dabc::Parameter* par = FindPar(name);
   if (par==0) return false;

   par->SetFixed(on);

   return true;
}


int dabc::WorkingProcessor::GetParInt(const char* name, int defvalue) const
{
   dabc::Parameter* par = FindPar(name);

   return par ? par->GetInt() : defvalue;
}

dabc::String dabc::WorkingProcessor::GetParStr(const char* name, const char* defvalue) const
{
   dabc::Parameter* par = FindPar(name);

   dabc::String str;

   if (par && par->GetStr(str)) return str;

   return defvalue;
}

const char* dabc::WorkingProcessor::GetParCharStar(const char* name, const char* defvalue) const
{
   dabc::StrParameter* par = dynamic_cast<StrParameter*> (FindPar(name));

   return par ? par->GetCharStar() : defvalue;
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
   String fullname;
   par->MakeFullName(fullname, GetTopParsFolder());

   DOUT5(("Invoke par change %s topfold:%s", fullname.c_str(), GetTopParsFolder()->GetName()));

   if (cmd==0)
      cmd = new CommandSetParameter(fullname.c_str(), value);
   else {
      cmd->SetStr("ParName", fullname.c_str());
      if (value!=0) cmd->SetStr("ParValue", value);
   }

   Submit(cmd);

   return true;
}


int dabc::WorkingProcessor::PreviewCommand(Command* cmd)
{
   int cmd_res = cmd_ignore;

   if (cmd->IsName(CommandSetParameter::CmdName())) {
      Parameter* par = FindPar(cmd->GetPar("ParName"));

      if (par==0) {
         EOUT(("Did not found parameter %s", cmd->GetPar("ParName")));
         return cmd_false;
      }

      const char* value = cmd->GetPar("ParValue");

      if (par->IsFixed()) {
         EOUT(("Parameter %s is fixed - cannot change to value %s", par->GetName(), value));
         return cmd_false;
      }

      DOUT4(("Change parameter %s to value %s", par->GetName(), value));

      par->SetStr(value);

      ParameterChanged(par);

      cmd_res = cmd_true;

   } else
      cmd_res = CommandReceiver::PreviewCommand(cmd);

   return cmd_res;
}


