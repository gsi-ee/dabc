// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "dabc/Parameter.h"

#include <cstdio>
#include <cstdlib>

#include "dabc/Manager.h"
#include "dabc/logging.h"
#include "dabc/Worker.h"
#include "dabc/ConfigBase.h"
#include "dabc/Hierarchy.h"
#include "dabc/defines.h"

dabc::ParameterContainer::ParameterContainer(Reference worker, const std::string &name, const std::string &parkind, bool hidden) :
   dabc::RecordContainer(worker, name, flIsOwner | (hidden ? flHidden : 0)),
   fKind(parkind),
   fLastChangeTm(),
   fInterval(1.),
   fAsynchron(false),
   fStatistic(kindNone),
   fRateValueSum(0.),
   fRateTimeSum(0.),
   fRateNumSum(0.),
   fMonitored(false),
   fRecorded(false),
   fWaitWorker(false),
   fAttrModified(false),
   fDeliverAllEvents(false),
   fRateWidth(5),
   fRatePrec(1)
{
   #ifdef DABC_EXTRA_CHECKS
   DebugObject("Parameter", this, 10);
   #endif
}

dabc::ParameterContainer::~ParameterContainer()
{
   #ifdef DABC_EXTRA_CHECKS
   DebugObject("Parameter", this, -10);
   #endif
}


const std::string dabc::ParameterContainer::GetActualUnits() const
{
   std::string units = GetField("units").AsStr();

   LockGuard lock(ObjectMutex());
   if (fStatistic==kindRate) {
      if (units.empty()) units = "1/s";
                    else units += "/s";
   }
   return units;
}


bool dabc::ParameterContainer::IsDeliverAllEvents() const
{
   LockGuard lock(ObjectMutex());
   return fDeliverAllEvents;
}


std::string dabc::ParameterContainer::DefaultFiledName() const
{
   return xmlValueAttr;
}


dabc::RecordField dabc::ParameterContainer::GetField(const std::string &name) const
{
   LockGuard lock(ObjectMutex());

   std::string n = name.empty() ? DefaultFiledName() : name;

   if (Fields().HasField(n)) return Fields().Field(n);

   return dabc::RecordField();
}


bool dabc::ParameterContainer::SetField(const std::string &_name, const RecordField &value)
{
   bool res(false), fire(false), doworker(false);
   double ratevalue(0.);
   std::string svalue;

   std::string name = _name.empty() ? DefaultFiledName() : _name;
   bool is_dflt_name = (name == DefaultFiledName());

   DOUT4("ParameterContainer::SetField %s dflt %s", name.c_str(), DBOOL(is_dflt_name));

   {
      LockGuard lock(ObjectMutex());

      if (!_CanChangeField(name)) return false;

      doworker = fMonitored || fRecorded;

      // remember parameter kind

      if ((fStatistic!=kindNone) && is_dflt_name && !value.null()) {

         fRateValueSum += value.AsDouble();
         fRateNumSum += 1.;

         // if rate calculated asynchron, do rest in timeout processing
         if (fAsynchron) return true;

         TimeStamp tm = dabc::Now();

         if (fLastChangeTm.null()) fLastChangeTm = tm;

         fRateTimeSum = tm - fLastChangeTm;

         fire = _CalcRate(ratevalue, svalue);

         if (fire) {
            fLastChangeTm = tm;
            res = Fields().Field(name).SetDouble(ratevalue);
         }

      } else {

         res = Fields().Field(name).SetValue(value);

         if (is_dflt_name) {
            TimeStamp tm = dabc::Now();

            if (!fAsynchron)
               if (fLastChangeTm.null() || (fInterval<=0.) || (tm > fLastChangeTm+fInterval)) {
                  fire = true;
                  svalue = Fields().Field(name).AsStr();
               }

            if (fire) fLastChangeTm = tm;
         }
      }

      if (res && !is_dflt_name) {
         // indicate that not only central value, but also parameter attributes were modified
         fAttrModified = true;
      }

      // we submit any signal to worker only when previous is processed
      if ((fMonitored || fRecorded) && res && fire && !fWaitWorker) {
         fWaitWorker = true;
         doworker = true;
      }
   }

   // inform worker that parameter is changed
   if (doworker) {
      Worker* w = GetWorker();
      if (w)
         w->WorkerParameterChanged(false, this, svalue);
      else
         ConfirmFromWorker();
   }

   if (fire && res) FireModified(svalue);

   DOUT4("ParameterContainer::SetField %s = %s  res %s fire %s",
            name.c_str(), value.AsStr().c_str(), DBOOL(res), DBOOL(fire));

   return res;
}

unsigned dabc::ParameterContainer::ConfirmFromWorker()
{
   LockGuard lock(ObjectMutex());

   fWaitWorker = false;

   unsigned res = 0;
   if (fRecorded) res = res | 1;
   if (fMonitored) res = res | 2;
   return res;
}

void dabc::ParameterContainer::FireModified(const std::string &svalue)
{
   FireParEvent(parModified);

   int doout = GetDebugLevel();

   if (doout>=0)
      dabc::Logger::Debug(doout, __FILE__, __LINE__, __func__, dabc::format("%s = %s %s", GetName(), svalue.c_str(), GetActualUnits().c_str()).c_str() );
}


void dabc::ParameterContainer::FireParEvent(int id)
{
   if (!dabc::mgr.null())
      dabc::mgr()->ProduceParameterEvent(this, id);
}


void dabc::ParameterContainer::ObjectCleanup()
{
   FireParEvent(parDestroy);

   dabc::RecordContainer::ObjectCleanup();
}

void dabc::ParameterContainer::ProcessTimeout(double last_dif)
{
   bool fire(false), res(false), doworker(false);
   double value(0);
   std::string svalue;

//   DOUT0("Par %s Process timeout !!!", GetName());

   {
      LockGuard lock(ObjectMutex());

      if ((fStatistic!=kindNone) && fAsynchron) {
         fRateTimeSum += last_dif;
         fire = _CalcRate(value, svalue);
         if (fire) {
            fLastChangeTm = dabc::Now();
            res = Fields().Field(DefaultFiledName()).SetDouble(value);
         }
      } else

      if (fAsynchron) {
         fRateTimeSum += last_dif;

         if (!fLastChangeTm.null() && (fRateTimeSum>fInterval)) {
            fire = true;
            fLastChangeTm.Reset();
            fRateTimeSum = 0.;
            svalue = Fields().Field(DefaultFiledName()).AsStr();
         }
      }

      if ((fMonitored || fRecorded) && res && fire && !fWaitWorker) {
         fWaitWorker = true;
         doworker = true;
      }
   }

   if (doworker) {
      Worker* w = GetWorker();
      if (w) w->WorkerParameterChanged(false, this, svalue);
        else ConfirmFromWorker();
   }

   if (fire)
      FireModified(svalue);
}

bool dabc::ParameterContainer::_CalcRate(double& value, std::string& svalue)
{
   if ((fRateTimeSum < fInterval) || (fRateTimeSum<1e-6)) return false;

   if (fStatistic==kindRate)
      value = fRateValueSum / fRateTimeSum;
   else
   if ((fStatistic==kindAverage) && (fRateNumSum>0))
      value = fRateValueSum / fRateNumSum;

   fRateValueSum = 0.;
   fRateTimeSum = 0.;
   fRateNumSum = 0.;

   dabc::formats(svalue, "%*.*f", fRateWidth, fRatePrec, value);

   return true;
}


void dabc::ParameterContainer::SetSynchron(bool on, double interval, bool everyevnt)
{
   LockGuard lock(ObjectMutex());

   fAsynchron = !on;

   fInterval = interval > 0 ? interval : 0.;

   fDeliverAllEvents = everyevnt;
}

dabc::Worker *dabc::ParameterContainer::GetWorker() const
{
   Object *prnt = GetParent();
   while (prnt) {
      Worker* w = dynamic_cast<Worker*> (prnt);
      if (w) return w;
      prnt = prnt->GetParent();
   }
   return nullptr;
}

const std::string &dabc::ParameterContainer::Kind() const
{
   LockGuard lock(ObjectMutex());
   return fKind;
}

void dabc::ParameterContainer::BuildFieldsMap(RecordFieldsMap *cont)
{
   LockGuard lock(ObjectMutex());

   bool force_value_modified = false;

   // mark that we are producing rate
   if (fStatistic == ParameterContainer::kindRate) {
      cont->Field(dabc::prop_kind).SetStr("rate");
      force_value_modified = true;
   } else if (fKind == "cmddef") {
      cont->Field(dabc::prop_kind).SetStr("DABC.Command");
      cont->Field("_parcmddef").SetBool(true);
   } else if (fKind == "info") {
      cont->Field(dabc::prop_kind).SetStr("log");
   } else if (Fields().HasField("#record")) {
      force_value_modified = true;
   }

   // just copy all fields, including value
   cont->CopyFrom(Fields());

   if (force_value_modified && cont->HasField("value"))
      cont->Field("value").SetModified(true);
}

// --------------------------------------------------------------------------------

bool dabc::Parameter::NeedTimeout()
{
   LockGuard lock(ObjectMutex());

   return GetObject() ? GetObject()->fAsynchron : false;
}

bool dabc::Parameter::TakeAttrModified()
{
   LockGuard lock(ObjectMutex());

   bool res = false;

   if (GetObject()) {
      res = GetObject()->fAttrModified;
      GetObject()->fAttrModified = false;
   }

   return res;
}

int dabc::Parameter::GetDebugLevel() const
{
   return GetObject() ? GetObject()->GetDebugLevel() : -1;
}

dabc::Parameter& dabc::Parameter::SetRatemeter(bool synchron, double interval)
{

   if (GetObject()==0) return *this;

   GetObject()->fRateWidth = GetField("width").AsInt(GetObject()->fRateWidth);
   GetObject()->fRatePrec = GetField("prec").AsInt(GetObject()->fRatePrec);

   {
      LockGuard lock(ObjectMutex());

      GetObject()->fStatistic = ParameterContainer::kindRate;

      GetObject()->fInterval = interval;

      GetObject()->fAsynchron = !synchron;

      DOUT3("Asynchron = %s", DBOOL(GetObject()->fAsynchron));

      GetObject()->fRateValueSum = 0.;
      GetObject()->fRateTimeSum = 0.;
      GetObject()->fRateNumSum = 0.;
   }

   FireConfigured();

   return *this;
}

dabc::Parameter& dabc::Parameter::DisableRatemeter()
{
   if (GetObject()==0) return *this;

   {
      LockGuard lock(ObjectMutex());

      GetObject()->fStatistic = ParameterContainer::kindNone;

      GetObject()->fAsynchron = false;
   }

   FireConfigured();

   return *this;
}

bool dabc::Parameter::IsRatemeter() const
{
   if (GetObject()==0) return false;

   LockGuard lock(ObjectMutex());

   return GetObject()->fStatistic == ParameterContainer::kindRate;
}

dabc::Parameter& dabc::Parameter::SetWidthPrecision(unsigned width, unsigned prec)
{
   SetField("width", width);
   SetField("prec", prec);

   if (GetObject()) {
      LockGuard lock(ObjectMutex());
      GetObject()->fRateWidth = width;
      GetObject()->fRatePrec = prec;
   }
   return *this;
}


dabc::Parameter& dabc::Parameter::SetAverage(bool synchron, double interval)
{
   if (GetObject()==0) return *this;

   GetObject()->fRateWidth = GetField("width").AsInt(GetObject()->fRateWidth);
   GetObject()->fRatePrec = GetField("prec").AsInt(GetObject()->fRatePrec);

   {
      LockGuard lock(ObjectMutex());

      GetObject()->fStatistic = ParameterContainer::kindAverage;

      GetObject()->fInterval = interval;

      GetObject()->fAsynchron = !synchron;

      DOUT3("Asynchron = %s", DBOOL(GetObject()->fAsynchron));

      GetObject()->fRateValueSum = 0.;
      GetObject()->fRateTimeSum = 0.;
      GetObject()->fRateNumSum = 0.;
   }

   FireConfigured();

   return *this;
}

dabc::Parameter& dabc::Parameter::DisableAverage()
{
   if (GetObject()==0) return *this;

   {
      LockGuard lock(ObjectMutex());

      GetObject()->fStatistic = ParameterContainer::kindNone;

      GetObject()->fAsynchron = false;
   }

   FireConfigured();

   return *this;

}


bool dabc::Parameter::IsAverage() const
{
   if (GetObject()==0) return false;

   LockGuard lock(ObjectMutex());

   return GetObject()->fStatistic == ParameterContainer::kindAverage;
}


dabc::Parameter &dabc::Parameter::SetSynchron(bool on, double interval, bool everyevnt)
{
   if (GetObject()!=0)
      GetObject()->SetSynchron(on, interval, everyevnt);

   FireConfigured();

   return *this;
}


dabc::Reference dabc::Parameter::GetWorker() const
{
   return GetObject() ? GetObject()->GetWorker() : 0;
}


int dabc::Parameter::ExecuteChange(Command cmd)
{
   bool res = SetField("", cmd.GetField(CmdSetParameter::ParValue()));

   return res ? cmd_true : cmd_false;
}

bool dabc::Parameter::IsMonitored()
{
   if (GetObject()==0) return false;

   LockGuard lock(ObjectMutex());

   return GetObject()->fMonitored;
}

dabc::Parameter& dabc::Parameter::SetMonitored(bool on)
{
   if (GetObject()!=0) {

      LockGuard lock(ObjectMutex());

      GetObject()->fMonitored = on;
   }

   return *this;
}

const std::string dabc::Parameter::Kind() const
{
   if (GetObject()==0) return std::string();

   return GetObject()->Kind();
}

const std::string dabc::Parameter::GetActualUnits() const
{
   if (GetObject()==0) return std::string();

   return GetObject()->GetActualUnits();
}

void dabc::Parameter::FireConfigured()
{
   if (GetObject()) GetObject()->FireParEvent(parConfigured);
}

void dabc::Parameter::FireModified()
{
   if (GetObject())
      GetObject()->FireModified(Value().AsStr());
}

bool dabc::Parameter::SubmitSetValue(const RecordField& v)
{
   WorkerRef w = GetWorker();
   if (w.null()) return SetValue(v);

   CmdSetParameter cmd(GetName(), v);

   return w.Submit(cmd);
}


// ========================================================================================

int dabc::CommandDefinition::NumArgs() const
{
   return GetField("numargs").AsInt(0);
}

std::string dabc::CommandDefinition::ArgName(int n) const
{
   return GetField(dabc::format("arg%d",n)).AsStr();
}

int dabc::CommandDefinition::FindArg(const std::string &name) const
{
   int num = NumArgs();

   for (int n=0;n<num;n++)
      if (ArgName(n)==name) return n;
   return -1;
}

dabc::CommandDefinition& dabc::CommandDefinition::AddArg(const std::string &name, const std::string &kind, bool required, const RecordField& dflt)
{
   if (name.empty() || null()) return *this;

   int id = FindArg(name);

   if (id<0) {
      id = NumArgs();
      SetField("numargs", id+1);
   }

   std::string prefix = dabc::format("arg%d",id);
   SetField(prefix, name);

   if (!dflt.null())
      SetField(prefix+"_dflt", dflt);

   SetField(prefix+"_kind", kind);

   if (required)
      SetField(prefix+"_req", required);

   return *this;
}

dabc::CommandDefinition& dabc::CommandDefinition::SetArgMinMax(const std::string &name, const RecordField& min, const RecordField& max)
{
   int id = FindArg(name);
   if (id<0) return *this;

   std::string prefix = dabc::format("arg%d",id);

   SetField(prefix+"_min", min);
   SetField(prefix+"_max", max);

   return *this;
}


bool dabc::CommandDefinition::GetArg(int n, std::string& name, std::string& kind, bool& required, std::string& dflt) const
{

   if ((n<0) || (n>=NumArgs())) return false;

   std::string prefix = dabc::format("arg%d",n);

   name = GetField(prefix).AsStr();

   kind = GetField(prefix+"_kind").AsStr();

   if (HasField(prefix+"_dflt"))
      dflt = GetField(prefix+"_dflt").AsStr();
   else
      dflt = "";

   if (HasField(prefix+"_req"))
      required = GetField(prefix+"_req").AsBool();
   else
      required = false;

   return true;
}

dabc::Command dabc::CommandDefinition::MakeCommand() const
{
   dabc::Command cmd(GetName());

   int num = NumArgs();

   for (int n=0;n<num;n++) {
      std::string name, kind, dflt;
      bool required(false);
      if (GetArg(n,name,kind,required, dflt))
         if (required || !dflt.empty()) cmd.SetStr(name, dflt);
   }

   return cmd;
}

