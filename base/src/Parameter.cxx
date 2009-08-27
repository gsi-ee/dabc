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
   fFixed(lst ? lst->GetParDfltsFixed() : false),
   fVisible(lst ? lst->GetParDfltsVisible() : true),
   fChangable(lst ? lst->GetParDfltsChangable() : true),
   fDebug(false),
   fRegistered(false)
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
}

bool dabc::Parameter::Read(ConfigIO &cfg)
{
   std::string value;
   if (!cfg.Find(this, value)) return false;

   bool wasfixed = fFixed;
   fFixed = false;

   DOUT1(("Set par %s = %s", GetFullName().c_str(), value.c_str()));
   SetValue(value.c_str());

   fFixed = wasfixed;

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
      if (sscanf(value.c_str(), "%lf", &fValue) != 1) return false;
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
      if (sscanf(value.c_str(), "%d", &fValue) != 1) return false;
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

dabc::RateParameter::RateParameter(WorkingProcessor* parent,
                                   const char* name, bool synchron, double interval,
                                   const char* units, double lower, double upper) :
   Parameter(parent, name),
   fSynchron(synchron),
   fInterval(interval),
   fTotalSum(0.),
   fLastUpdateTm(NullTimeStamp),
   fDiffSum(0.)
{
   if (fInterval<1e-3) fInterval = 1e-3;

   fRecord.value = 0.;
   fRecord.lower = lower;
   fRecord.upper = upper;
   fRecord.displaymode = dabc::DISPLAY_BAR;
   fRecord.alarmlower = 0.;
   fRecord.alarmupper = 0.;
   strncpy(fRecord.color, col_Blue, sizeof(fRecord.color));
   strncpy(fRecord.alarmcolor, col_Yellow, sizeof(fRecord.alarmcolor));
   strncpy(fRecord.units, units ? units : "1/s", sizeof(fRecord.units));

   DOUT2(("Create ratemeter %s %s %f %f", GetName(), fRecord.units, fRecord.lower, fRecord.upper));

   Ready();
}

void dabc::RateParameter::DoDebugOutput()
{
   LockGuard lock(fValueMutex);

   DOUT0(("%s = %5.1f %s", GetName(), fRecord.value, fRecord.units));
}


void dabc::RateParameter::ChangeRate(double rate)
{
   DOUT5(("Change rate %s - %5.1f %s", GetName(), rate, fRecord.units));

   {
      LockGuard lock(fValueMutex);
      fRecord.value = rate;
   }
   Changed();
}

bool dabc::RateParameter::GetValue(std::string& value) const
{
   LockGuard lock(fValueMutex);
   dabc::formats(value, "%f", fRecord.value);
   return true;
}

bool dabc::RateParameter::SetValue(const std::string &value)
{
   double v = 0.;
   {
      LockGuard lock(fValueMutex);
      if (fFixed || (value.length()==0)) return false;
      if (sscanf(value.c_str(), "%lf", &v)!=1) return false;
   }
   ChangeRate(v);
   return true;
}

void dabc::RateParameter::AccountValue(double v)
{
   if (fSynchron) {
      fTotalSum += v;
      TimeStamp_t tm = TimeStamp();
      if (IsNullTime(fLastUpdateTm)) fLastUpdateTm = tm;
      double dist = TimeDistance(fLastUpdateTm, tm);
      if (dist > GetInterval()) {
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

   cfg.CreateAttr(xmlNameAttr, GetName());

   cfg.CreateAttr(xmlValueAttr, FORMAT(("%f", fRecord.value)));

   cfg.CreateAttr("units", fRecord.units);

   cfg.CreateAttr("displaymode", _GetDisplayMode());

   cfg.CreateAttr("lower", FORMAT(("%f", fRecord.lower)));

   cfg.CreateAttr("upper", FORMAT(("%f", fRecord.upper)));

   cfg.CreateAttr("color", fRecord.color);

   cfg.CreateAttr("alarmlower", FORMAT(("%f", fRecord.alarmlower)));

   cfg.CreateAttr("alarmupper", FORMAT(("%f", fRecord.alarmlower)));

   cfg.CreateAttr("alarmcolor", fRecord.alarmcolor);

   cfg.CreateAttr("debug", IsDebugOutput() ? "true" : "false");

   cfg.CreateAttr("interval", FORMAT(("%f", fInterval)));

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

   std::string v;

   if (!cfg.Find(this, v)) return false;

   LockGuard lock(fValueMutex);

   if (cfg.Find(this, v, xmlValueAttr))
      if (!v.empty()) sscanf(v.c_str(), "%f", &fRecord.value);

   if (!v.empty()) DOUT0(("Ratemeter %s value = %s", GetName(), v.c_str()));

   if (cfg.Find(this, v, "units"))
      if (!v.empty()) strncpy(fRecord.units, v.c_str(), sizeof(fRecord.units));

   if (cfg.Find(this, v, "displaymode"))
      _SetDisplayMode(v.c_str());

   if (cfg.Find(this, v, "lower"))
      if (!v.empty()) sscanf(v.c_str(), "%f", &fRecord.lower);

   if (cfg.Find(this, v, "upper"))
      if (!v.empty()) sscanf(v.c_str(), "%f", &fRecord.upper);

   if (cfg.Find(this, v, "color"))
      if (!v.empty()) strncpy(fRecord.color, v.c_str(), sizeof(fRecord.color));

   if (cfg.Find(this, v, "alarmlower"))
      if (!v.empty()) sscanf(v.c_str(), "%f", &fRecord.alarmlower);

   if (cfg.Find(this, v, "alarmupper"))
      if (!v.empty()) sscanf(v.c_str(), "%f", &fRecord.alarmupper);

   if (cfg.Find(this, v, "alarmcolor"))
      if (!v.empty()) strncpy(fRecord.alarmcolor, v.c_str(), sizeof(fRecord.alarmcolor));

   if (cfg.Find(this, v, "debug"))
      if (!v.empty()) SetDebugOutput((v == "true") || (v=="1") || (v=="TRUE"));

   if (cfg.Find(this, v, "interval"))
      if (!v.empty()) sscanf(v.c_str(), "%lf", &fInterval);

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

dabc::InfoParameter::InfoParameter(WorkingProcessor* parent, const char* name, int verbose, const char* color) :
   Parameter(parent, name)
{
   fRecord.verbose = verbose;
   strncpy(fRecord.color, (color ? color : "Cyan"), sizeof(fRecord.color));
   strcpy(fRecord.info, "None"); // info message
   Ready();
}

bool dabc::InfoParameter::GetValue(std::string &value) const
{
   LockGuard lock(fValueMutex);
   value = fRecord.info;
   return true;
}

bool dabc::InfoParameter::SetValue(const std::string &value)
{
   {
      LockGuard lock(fValueMutex);
      if (fFixed) return false;
      strncpy(fRecord.info, value.c_str(), sizeof(fRecord.info));
   }
   Changed();
   return true;
}

bool dabc::InfoParameter::SetColor(const std::string &color)
{
   LockGuard lock(fValueMutex);
   if (fFixed) return false;
   strncpy(fRecord.color, color.c_str(), sizeof(fRecord.color));
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

      x = (x - fRecord->xlow) / (fRecord->xhigh - fRecord->xlow) * fRecord->channels;

      int np = (int) x;

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
