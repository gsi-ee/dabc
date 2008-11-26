#include "dabc/Parameter.h"

#include <stdio.h>


#include "dabc/Manager.h"
// #include "dabc/Command.h"
#include "dabc/CommandClient.h"
#include "dabc/logging.h"
#include "dabc/Iterator.h"
#include "dabc/WorkingProcessor.h"
#include "dabc/ConfigBase.h"

dabc::Parameter::Parameter(WorkingProcessor* lst, const char* name) :
   Basic(lst ? lst->MakeFolderForParam(name) : 0, dabc::Folder::GetObjectName(name)),
   fLst(lst),
   fValueMutex(),
   fFixed(lst ? lst->fParsDfltFixed : false),
   fVisibility(lst ? lst->fParsDfltVisibility : 1),
   fDebug(false)
{
   DOUT5(("Create parameter %s", GetFullName(dabc::mgr()).c_str()));
}

dabc::Parameter::~Parameter()
{
   DOUT5(("Destroy parameter %s", GetName()));
   if (GetParent()!=0)
      EOUT(("Parameter %s not regularly destroyed", GetFullName().c_str()));
}

bool dabc::Parameter::FireEvent(Parameter* par, int evid)
{
   if ((dabc::mgr()==0) || (par==0)) return false;
   dabc::mgr()->FireParamEvent(par, evid);
   return true;
}

bool dabc::Parameter::IsFixed() const
{
   LockGuard lock(fValueMutex);
   return fFixed;
}

void dabc::Parameter::SetFixed(bool on)
{
   LockGuard lock(fValueMutex);
   fFixed = on;
}

void dabc::Parameter::FillInfo(std::string& info)
{
   std::string mystr;
   if (GetValue(mystr)) {
      info.assign("Value: ");
      info.append(mystr.c_str());
   } else
      dabc::Basic::FillInfo(info);
}

void dabc::Parameter::Ready()
{
   FireEvent(this, parCreated);
}

void dabc::Parameter::Changed()
{
   if (fLst) fLst->ParameterChanged(this);

   FireEvent(this, parModified);

   if (IsDebugOutput()) DoDebugOutput();
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
   std::string str;
   if (GetValue(str))
      DOUT0(("%s = %s", GetName(), str.c_str()));
   else
      DOUT0(("%s changed", GetName()));
}

bool dabc::Parameter::Store(ConfigIO &cfg)
{
   std::string s;

   if (GetValue(s)) {
      cfg.CreateItem(GetName());
      cfg.CreateAttr(xmlValueAttr, s.c_str());
      cfg.PopItem();
   }

   return true;
}

bool dabc::Parameter::Find(ConfigIO &cfg)
{
   return cfg.FindItem(GetName());

//   if (!cfg.FindItem(GetName())) return false;
//   return cfg.CheckAttr(xmlValueAttr, 0);
}

bool dabc::Parameter::Read(ConfigIO &cfg)
{
   const char* item = cfg.Find(this);
   if (item==0) return false;

   const char* value = cfg.GetAttrValue(xmlValueAttr);
   if (value==0) value = item;

   DOUT0(("Set par %s = %s", GetFullName().c_str(), value));
   SetValue(value);

   return true;
}

// __________________________________________________________

dabc::StrParameter::StrParameter(WorkingProcessor* parent, const char* name, const char* istr) :
   Parameter(parent, name),
   fValue(istr ? istr : "")
{
   Ready();
}


bool dabc::StrParameter::GetValue(std::string& value) const
{
   LockGuard lock(fValueMutex);
   value = fValue;
   return true;
}

bool dabc::StrParameter::SetValue(const std::string &value)
{
   {
      LockGuard lock(fValueMutex);
      if (fFixed) return false;
      fValue = value;
   }
   Changed();
   return true;
}

// __________________________________________________________


dabc::DoubleParameter::DoubleParameter(WorkingProcessor* parent, const char* name, double iv) :
   Parameter(parent, name),
   fValue(iv)
{
   Ready();
}

bool dabc::DoubleParameter::GetValue(std::string &value) const
{
   LockGuard lock(fValueMutex);
   dabc::formats(value, "%lf", fValue);
   return true;
}

bool dabc::DoubleParameter::SetValue(const std::string &value)
{
   {
      LockGuard lock(fValueMutex);
      if (fFixed || (value.length()==0)) return false;
      sscanf(value.c_str(), "%lf", &fValue);
   }
   Changed();
   return true;
}

double dabc::DoubleParameter::GetDouble() const
{
   LockGuard lock(fValueMutex);
   return fValue;
}

bool dabc::DoubleParameter::SetDouble(double v)
{
   {
      LockGuard lock(fValueMutex);
      if (fFixed) return false;
      fValue = v;
   }
   Changed();
   return true;
}

// __________________________________________________________

dabc::IntParameter::IntParameter(WorkingProcessor* parent, const char* name, int ii) :
   Parameter(parent, name),
   fValue(ii)
{
   Ready();
}

bool dabc::IntParameter::GetValue(std::string &value) const
{
   LockGuard lock(fValueMutex);
   dabc::formats(value, "%d", fValue);
   return true;
}

bool dabc::IntParameter::SetValue(const std::string &value)
{
   {
      LockGuard lock(fValueMutex);
      if (fFixed || (value.length()==0)) return false;
      sscanf(value.c_str(), "%d", &fValue);
   }
   Changed();
   return true;
}

int dabc::IntParameter::GetInt() const
{
   LockGuard lock(fValueMutex);
   return fValue;
}

bool dabc::IntParameter::SetInt(int v)
{
   {
      LockGuard lock(fValueMutex);
      if (fFixed) return false;
      fValue = v;
   }
   Changed();
   return true;
}

// __________________________________________________________

dabc::RateParameter::RateParameter(WorkingProcessor* parent, const char* name, bool synchron, double interval) :
   Parameter(parent, name),
   fSynchron(synchron),
   fInterval(interval),
   fTotalSum(0.),
   fLastUpdateTm(NullTimeStamp),
   fDiffSum(0.)
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

   Ready();
}

void dabc::RateParameter::DoDebugOutput()
{
   LockGuard lock(fValueMutex);

   DOUT0(("%s = %5.1f %s", GetName(), fRecord.value, fRecord.units));
}


void dabc::RateParameter::ChangeRate(double rate)
{
//   DOUT3(("Change rate %s - %5.1f %s", GetName(), rate, fRecord.units));

   {
      LockGuard lock(fValueMutex);
      fRecord.value = rate;
   }
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
      LockGuard lock(fValueMutex);
      fTotalSum += v;
   }

}

void dabc::RateParameter::ProcessTimeout(double last_diff)
{
   if (last_diff<=0.) return;

   bool isnewrate = false;
   double newrate = 0.;

   {
      LockGuard lock(fValueMutex);
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
   LockGuard lock(fValueMutex);
   strncpy(fRecord.units, name, sizeof(fRecord.units)-1);
}

void dabc::RateParameter::SetLimits(double lower, double upper)
{
   LockGuard lock(fValueMutex);
   fRecord.lower = lower;
   fRecord.upper = upper;
}

const char* dabc::RateParameter::_GetDisplayMode()
{
   switch (fRecord.displaymode) {
      case DISPLAY_ARC: return "ARC";
      case DISPLAY_BAR: return "BAR";
      case DISPLAY_TREND: return "TREND";
      case DISPLAY_STAT: return "STAT";
   }

   return "BAR";
}

void dabc::RateParameter::_SetDisplayMode(const char* v)
{
    if (v==0) return;

    if (strcmp(v,"ARC")==0) fRecord.displaymode = DISPLAY_ARC; else
    if (strcmp(v,"BAR")==0) fRecord.displaymode = DISPLAY_BAR; else
    if (strcmp(v,"TREND")==0) fRecord.displaymode = DISPLAY_TREND; else
    if (strcmp(v,"STAT")==0) fRecord.displaymode = DISPLAY_STAT;
}


bool dabc::RateParameter::Store(ConfigIO &cfg)
{
   LockGuard lock(fValueMutex);

   cfg.CreateItem("Ratemeter");

   cfg.CreateAttr("name", GetName());

   cfg.CreateAttr("value", FORMAT(("%f", fRecord.value)));

   cfg.CreateAttr("units", fRecord.units);

   cfg.CreateAttr("displaymode", _GetDisplayMode());

   cfg.CreateAttr("lower", FORMAT(("%f", fRecord.lower)));

   cfg.CreateAttr("upper", FORMAT(("%f", fRecord.upper)));

   cfg.CreateAttr("color", fRecord.color);

   cfg.CreateAttr("alarmlower", FORMAT(("%f", fRecord.alarmlower)));

   cfg.CreateAttr("alarmupper", FORMAT(("%f", fRecord.alarmlower)));

   cfg.CreateAttr("alarmcolor", fRecord.alarmcolor);

   cfg.PopItem();

   return true;
}

bool dabc::RateParameter::Find(ConfigIO &cfg)
{
   if (!cfg.FindItem("Ratemeter")) return false;

   return cfg.CheckAttr("name", GetName());
}

bool dabc::RateParameter::Read(ConfigIO &cfg)
{
   // no any suitable matches found for top node
   if (cfg.Find(this)==0) return false;

   LockGuard lock(fValueMutex);

   const char* v = cfg.Find(this, "value");
   if (v) sscanf(v, "%f", &fRecord.value);

   if (v) DOUT0(("Ratemeter %s value = %s", GetName(), v));

   v = cfg.Find(this, "units");
   if (v) strncpy(fRecord.units, v, sizeof(fRecord.units));

   v = cfg.Find(this, "displaymode");
   if (v) _SetDisplayMode(v);

   v = cfg.Find(this, "lower");
   if (v) sscanf(v, "%f", &fRecord.lower);

   v = cfg.Find(this, "upper");
   if (v) sscanf(v, "%f", &fRecord.upper);

   v = cfg.Find(this, "color");
   if (v) strncpy(fRecord.color, v, sizeof(fRecord.color));

   v = cfg.Find(this, "alarmlower");
   if (v) sscanf(v, "%f", &fRecord.alarmlower);

   v = cfg.Find(this, "alarmupper");
   if (v) sscanf(v, "%f", &fRecord.alarmupper);

   v = cfg.Find(this, "alarmcolor");
   if (v) strncpy(fRecord.alarmcolor, v, sizeof(fRecord.alarmcolor));

   return true;
}

// ___________________________________________________

dabc::StatusParameter::StatusParameter(WorkingProcessor* parent, const char* name, int severity) :
   Parameter(parent, name)
{
   fRecord.severity = severity;
   strcpy(fRecord.color,"Cyan");
   strcpy(fRecord.status, "None"); // status name
   Ready();
}


bool dabc::StatusParameter::SetStatus(const char* status, const char* color)
{
   {
      LockGuard lock(fValueMutex);
      if (fFixed) return false;
      if(status)
         strncpy(fRecord.status, status ,16);
      if (color)
         strncpy(fRecord.color, color,16);
   }
   Changed();
   return true;

}

// ___________________________________________________

dabc::InfoParameter::InfoParameter(WorkingProcessor* parent, const char* name, int verbose) :
   Parameter(parent, name)
{
   fRecord.verbose = verbose;
   strcpy(fRecord.color,"Cyan");
   strcpy(fRecord.info, "None"); // info message
   Ready();
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

dabc::HistogramParameter::~HistogramParameter()
{
   free(fRecord);
   fRecord = 0;
}

void dabc::HistogramParameter::SetLimits(float xmin, float xmax)
{
   LockGuard lock(fValueMutex);

   fRecord->xlow = xmin;
   fRecord->xhigh = xmax;
}

void dabc::HistogramParameter::SetColor(const char* color)
{
   LockGuard lock(fValueMutex);

   strncpy(fRecord->color, color, sizeof(fRecord->color));
}

void dabc::HistogramParameter::SetLabels(const char* xlabel, const char* ylabel)
{
   LockGuard lock(fValueMutex);

   strncpy(fRecord->xlett, xlabel, sizeof(fRecord->xlett));
   strncpy(fRecord->cont, ylabel, sizeof(fRecord->cont));
}


void dabc::HistogramParameter::SetInterval(double sec)
{
   LockGuard lock(fValueMutex);

   fInterval = sec;

   if (fInterval<1e-3) fInterval = 1e-3;
}

void dabc::HistogramParameter::Clear()
{
   {
      LockGuard lock(fValueMutex);

      int* arr = &(fRecord->data);
      for (int n=0;n<fRecord->channels;n++)
         arr[n] = 0;

      _CheckChanged(true);
   }

   Changed();
}

bool dabc::HistogramParameter::Fill(float x)
{
   bool ch = false;

   {
      LockGuard lock(fValueMutex);

      if ((x<fRecord->xlow) || (x>=fRecord->xhigh)) return false;

      if (fRecord->xlow >= fRecord->xhigh) return false;

      x = (x - fRecord->xlow) / (fRecord->xhigh - fRecord->xlow);

      int np = (int) x * fRecord->channels;

      if ((np<0) || (np>=fRecord->channels)) return false;

      int* arr = &(fRecord->data);

      arr[np]++;

      ch = _CheckChanged();
   }

   if (ch) Changed();

   return true;
}

bool dabc::HistogramParameter::_CheckChanged(bool force)
{
   TimeStamp_t tm = TimeStamp();

   if (force || IsNullTime(fLastTm) || (TimeDistance(fLastTm, tm) > fInterval)) {
      fLastTm = tm;
      return true;
   }

   return false;
}
