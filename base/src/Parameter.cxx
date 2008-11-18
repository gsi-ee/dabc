#include "dabc/Parameter.h"

#include <stdio.h>


#include "dabc/Manager.h"
// #include "dabc/Command.h"
#include "dabc/CommandClient.h"
#include "dabc/logging.h"
#include "dabc/Iterator.h"
#include "dabc/WorkingProcessor.h"

dabc::Parameter::Parameter(WorkingProcessor* lst, const char* name) :
   Basic(lst ? lst->MakeFolderForParam(name) : 0, dabc::Folder::GetObjectName(name)),
   fLst(lst),
   fFixed(lst ? lst->fParsDfltFixed : false),
   fVisibility(lst ? lst->fParsDfltVisibility : 1),
   fDebug(false)
{
   DOUT5(("Create parameter %s", GetFullName(dabc::mgr()).c_str()));
}

dabc::Parameter::~Parameter()
{
   RaiseEvent(parDestroyed);
   DOUT5(("Destroy parameter %s", GetName()));
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

void dabc::Parameter::Ready()
{
   RaiseEvent(parCreated);
}

void dabc::Parameter::Changed()
{
   RaiseEvent(parModified);

   if (IsDebugOutput()) DoDebugOutput();
}

void dabc::Parameter::RaiseEvent(int evt)
{
   if (dabc::mgr()) dabc::mgr()->ParameterEvent(this, evt);
}

dabc::Basic* dabc::Parameter::GetHolder()
{
   return fLst ? fLst->fParsHolder : 0;
}

bool dabc::Parameter::InvokeChange(const char* value)
{
   return fLst ? fLst->InvokeParChange(this, value, 0) : false;
}

bool dabc::Parameter::InvokeChange(dabc::Command* cmd)
{
   return cmd && fLst ? fLst->InvokeParChange(this, 0, cmd) : false;
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

bool dabc::Parameter::Find(ConfigIO &cfg)
{
   if (GetParent()==0) return false;

   DOUT3(("Start search of parameter %s", GetFullName().c_str()));

   cfg.SetExact(true);

   if (GetParent()->Find(cfg) && cfg.FindItem(GetName(), ConfigIO::findChild)) return true;

   DOUT3(("Search par %s in defaults", GetFullName().c_str()));

   cfg.SetExact(false);

   while (GetParent()->Find(cfg))
      if (cfg.FindItem(GetName(), ConfigIO::findChild)) return true;

   DOUT3(("Parameter %s not found", GetFullName().c_str()));

   return false;

}

bool dabc::Parameter::Read(ConfigIO &cfg)
{
   if (!Find(cfg)) return false;

   const char* value = cfg.GetItemValue();
   DOUT0(("Set %s par %s = %s", (cfg.IsExact() ? "direct" : "via mask"),
           GetFullName().c_str(), (value ? value : "null")));
   SetStr(value);
   cfg.SetExact(true);

   return true;
}

// __________________________________________________________

dabc::RateParameter::RateParameter(WorkingProcessor* parent, const char* name, bool synchron, double interval) :
   Parameter(parent, name),
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

   if (!fSynchron) fRateMutex = new Mutex;

   Ready();
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

const char* dabc::RateParameter::GetDisplayMode()
{
   switch (fRecord.displaymode) {
      case DISPLAY_ARC: return "ARC";
      case DISPLAY_BAR: return "BAR";
      case DISPLAY_TREND: return "TREND";
      case DISPLAY_STAT: return "STAT";
   }

   return "BAR";
}

void dabc::RateParameter::SetDisplayMode(const char* v)
{
    if (v==0) return;

    if (strcmp(v,"ARC")==0) fRecord.displaymode = DISPLAY_ARC; else
    if (strcmp(v,"BAR")==0) fRecord.displaymode = DISPLAY_BAR; else
    if (strcmp(v,"TREND")==0) fRecord.displaymode = DISPLAY_TREND; else
    if (strcmp(v,"STAT")==0) fRecord.displaymode = DISPLAY_STAT;
}


bool dabc::RateParameter::Store(ConfigIO &cfg)
{
   cfg.CreateItem("Ratemeter");
   cfg.CreateAttr("name", GetName());

   cfg.CreateItem("value", FORMAT(("%f", fRecord.value)));
   cfg.PopItem();

   cfg.CreateItem("units", fRecord.units);
   cfg.PopItem();

   cfg.CreateItem("displaymode", GetDisplayMode());
   cfg.PopItem();

   cfg.CreateItem("lower", FORMAT(("%f", fRecord.lower)));
   cfg.PopItem();

   cfg.CreateItem("upper", FORMAT(("%f", fRecord.upper)));
   cfg.PopItem();

   cfg.CreateItem("color", fRecord.color);
   cfg.PopItem();

   cfg.CreateItem("alarmlower", FORMAT(("%f", fRecord.alarmlower)));
   cfg.PopItem();

   cfg.CreateItem("alarmupper", FORMAT(("%f", fRecord.alarmlower)));
   cfg.PopItem();

   cfg.CreateItem("alarmcolor", fRecord.alarmcolor);
   cfg.PopItem();

   cfg.PopItem();

   return true;
}

const char* dabc::RateParameter::FindRateAttr(ConfigIO &cfg, const char* name)
{
   if (GetParent()==0) return 0;

   cfg.SetExact(true);

   if (GetParent()->Find(cfg) &&
       cfg.FindItem("Ratemeter", ConfigIO::findChild) &&
       cfg.CheckAttr("name", GetName()) &&
       cfg.FindItem(name, ConfigIO::findChild)) return cfg.GetItemValue();

   DOUT3(("Search par %s in defaults", GetFullName().c_str()));

   cfg.SetExact(false);

   while (GetParent()->Find(cfg))
      if (cfg.FindItem("Ratemeter", ConfigIO::findChild) &&
          cfg.CheckAttr("name", GetName()) &&
          cfg.FindItem(name, ConfigIO::findChild)) return cfg.GetItemValue();

   DOUT1(("Attribute %s for ratemeter %s not found", name, GetName()));

   return 0;
}

bool dabc::RateParameter::Read(ConfigIO &cfg)
{
   const char* v = FindRateAttr(cfg, "value");
   if (v) sscanf(v, "%f", &fRecord.value);

   if (v) DOUT0(("Ratemeter %s value = %s", GetName(), v));

   v = FindRateAttr(cfg, "units");
   if (v) strncpy(fRecord.units, v, sizeof(fRecord.units));

   v = FindRateAttr(cfg, "displaymode");
   if (v) SetDisplayMode(v);

   v = FindRateAttr(cfg, "lower");
   if (v) sscanf(v, "%f", &fRecord.lower);

   v = FindRateAttr(cfg, "upper");
   if (v) sscanf(v, "%f", &fRecord.upper);

   v = FindRateAttr(cfg, "color");
   if (v) strncpy(fRecord.color, v, sizeof(fRecord.color));

   v = FindRateAttr(cfg, "alarmlower");
   if (v) sscanf(v, "%f", &fRecord.alarmlower);

   v = FindRateAttr(cfg, "alarmupper");
   if (v) sscanf(v, "%f", &fRecord.alarmupper);

   v = FindRateAttr(cfg, "alarmcolor");
   if (v) strncpy(fRecord.alarmcolor, v, sizeof(fRecord.alarmcolor));

   cfg.SetExact(true);

   return true;
}


// ___________________________________________________

dabc::HistogramParameter::HistogramParameter(WorkingProcessor* parent, const char* name, int nchannles) :
   Parameter(parent, name),
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

   Ready();
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
