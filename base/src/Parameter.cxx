#include "dabc/Parameter.h"

#include "dabc/Manager.h"
#include "dabc/Command.h"
#include "dabc/CommandClient.h"
#include "dabc/logging.h"
#include "dabc/Iterator.h"
#include "dabc/WorkingProcessor.h"

dabc::Parameter::Parameter(WorkingProcessor* lst, const char* name, bool visible) :
   Basic(lst ? lst->GetParsFolder() : 0, name),
   fLst(lst),
   fFixed(false),
   fVisible(visible),
   fDebug(false)
{
}

dabc::Parameter::~Parameter()
{
   RaiseEvent(2);
}

void dabc::Parameter::FillInfo(String& info)
{
   String mystr;
   if (GetStr(mystr)) {
      info.assign("Value: ");
      info.append(mystr.c_str());
   } else
      dabc::Basic::FillInfo(info);
}

void dabc::Parameter::Changed()
{
   RaiseEvent(1);

   if (IsDebugOutput()) DoDebugOutput();

}

void dabc::Parameter::RaiseEvent(int evt)
{
   if (dabc::mgr()) dabc::mgr()->ParameterEvent(this, evt);
}

dabc::Basic* dabc::Parameter::GetHolder()
{
   return GetParsLst() ? GetParsLst()->GetParsHolder() : 0;
}

bool dabc::Parameter::InvokeChange(const char* value)
{
   Command* cmd = new CommandSetParameter(GetName(), value);

   int cmd_res = InvokeChange(cmd);

   if (cmd_res!=CommandReceiver::cmd_postponed)
      dabc::Command::Reply(cmd, cmd_res == CommandReceiver::cmd_true);

   return cmd_res != CommandReceiver::cmd_false;
}

int dabc::Parameter::InvokeChange(dabc::Command* cmd)
{
   if (cmd==0) return CommandReceiver::cmd_false;

   if (fLst==0) return CommandReceiver::cmd_bool(SetStr(cmd->GetStr("ParValue")));

   cmd->SetStr("ParName", GetName());

   // after submit of the command one should not touch cmd pointer
   fLst->Submit(cmd);

   return CommandReceiver::cmd_postponed;
}

void dabc::Parameter::DoDebugOutput()
{
   String str;
   if (GetStr(str))
      DOUT0(("%s = %s", GetName(), str.c_str()));
   else
      DOUT0(("%s changed", GetName()));
}

bool dabc::Parameter::Store(ConfigIO &cfg)
{
   String s;

   if (GetStr(s)) {
      cfg.CreateItem(GetName(), s.c_str());
      cfg.PopItem();
   }

   return true;
}

// __________________________________________________________

dabc::RateParameter::RateParameter(WorkingProcessor* parent, const char* name, bool synchron, double interval, bool visible) :
   Parameter(parent, name, visible),
   fSynchron(synchron),
   fInterval(interval),
   fTotalSum(0.),
   fLastUpdateTm(NullTimeStamp),
   fDiffSum(0.),
   fRateMutex(0)
{
   SetFixed(true); // one cannot change rate from outside application
   if (fInterval<1e-3) fInterval = 1e-3;

   fRecord.value = 0.;
   fRecord.lower = 0.;
   fRecord.upper = 0.;
   fRecord.displaymode = dabc::DISPLAY_BAR;
   fRecord.alarmlower = 0.;
   fRecord.alarmupper = 0.;
   strcpy(fRecord.color, col_Blue);
   strcpy(fRecord.alarmcolor, col_Yellow);
   strcpy(fRecord.units, "1/s");
   RaiseEvent(0);

   if (!fSynchron) fRateMutex = new Mutex;
}

dabc::RateParameter::~RateParameter()
{
   delete fRateMutex; fRateMutex = 0;
}


void dabc::RateParameter::DoDebugOutput()
{
   DOUT0(("%s = %5.1f %s", GetName(), GetDouble(), fRecord.units));
}


void dabc::RateParameter::ChangeRate(double rate)
{
//   DOUT3(("Change rate %s - %5.1f %s", GetName(), rate, fRecord.units));

   fRecord.value = rate;
   Changed();
}

void dabc::RateParameter::AccountValue(double v)
{
   if (fSynchron) {
      fTotalSum += v;
      TimeStamp_t tm = TimeStamp();
      if (IsNullTime(fLastUpdateTm)) fLastUpdateTm = tm;
      double dist = TimeDistance(fLastUpdateTm, tm);
      if (dist>GetInterval()) {
         ChangeRate(fTotalSum / dist);
         fTotalSum = 0.;
      }
   } else {
      LockGuard lock(fRateMutex);
      fTotalSum += v;
   }

}

void dabc::RateParameter::ProcessTimeout(double last_diff)
{
   if (last_diff<=0.) return;

   bool isnewrate = false;
   double newrate = 0.;

   {
      LockGuard lock(fRateMutex);
      fDiffSum += last_diff;

      if (fDiffSum >= GetInterval()) {
         isnewrate = true;
         newrate = fTotalSum / fDiffSum;
         fTotalSum = 0.;
         fDiffSum = 0.;
      }
   }

   if (isnewrate) ChangeRate(newrate);
}

void dabc::RateParameter::SetUnits(const char* name)
{
   strncpy(fRecord.units, name, sizeof(fRecord.units)-1);
}

void dabc::RateParameter::SetLimits(double lower, double upper)
{
   fRecord.lower = lower;
   fRecord.upper = upper;
}

// ___________________________________________________

dabc::HistogramParameter::HistogramParameter(WorkingProcessor* parent, const char* name, int nchannles, bool visible) :
   Parameter(parent, name, visible),
   fRecord(0),
   fLastTm(NullTimeStamp),
   fInterval(1.)
{
   int sz = sizeof(HistogramRec) + (nchannles-1) * sizeof(int);

   fRecord = (HistogramRec*) malloc(sz);

   memset(fRecord, 0, sz);
   fRecord->channels = nchannles;

   SetColor(col_Blue);
   SetLimits(0, nchannles);

   RaiseEvent(0);
}


void dabc::HistogramParameter::SetLimits(float xmin, float xmax)
{
   fRecord->xlow = xmin;
   fRecord->xhigh = xmax;
}

void dabc::HistogramParameter::SetColor(const char* color)
{
   strncpy(fRecord->color, color, sizeof(fRecord->color));
}

void dabc::HistogramParameter::SetLabels(const char* xlabel, const char* ylabel)
{
   strncpy(fRecord->xlett, xlabel, sizeof(fRecord->xlett));
   strncpy(fRecord->cont, ylabel, sizeof(fRecord->cont));
}


void dabc::HistogramParameter::SetInterval(double sec)
{
   fInterval = sec;

   if (fInterval<1e-3) fInterval = 1e-3;
}

void dabc::HistogramParameter::Clear()
{
   int* arr = &(fRecord->data);
   for (int n=0;n<fRecord->channels;n++)
      arr[n] = 0;
   CheckChanged(true);
}

bool dabc::HistogramParameter::Fill(float x)
{
   if ((x<fRecord->xlow) || (x>=fRecord->xhigh)) return false;

   if (fRecord->xlow >= fRecord->xhigh) return false;

   x = (x - fRecord->xlow) / (fRecord->xhigh - fRecord->xlow);

   int np = (int) x * fRecord->channels;

   if ((np<0) || (np>=fRecord->channels)) return false;

   int* arr = &(fRecord->data);

   arr[np]++;

   CheckChanged();

   return true;
}

void dabc::HistogramParameter::CheckChanged(bool force)
{
   TimeStamp_t tm = TimeStamp();

   if (force || IsNullTime(fLastTm) || (TimeDistance(fLastTm, tm) > fInterval)) {
      fLastTm = tm;
      Changed();
   }
}
