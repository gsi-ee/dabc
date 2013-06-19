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

#include <stdio.h>
#include <stdlib.h>


#include "dabc/Manager.h"
#include "dabc/logging.h"
#include "dabc/Iterator.h"
#include "dabc/Worker.h"
#include "dabc/ConfigBase.h"
#include "dabc/Hierarchy.h"

dabc::ParameterContainer::ParameterContainer(Reference worker, const std::string& name, const std::string& parkind) :
   dabc::RecordContainer(worker, name),
   fKind(parkind),
   fLastChangeTm(),
   fInterval(1.),
   fAsynchron(false),
   fStatistic(kindNone),
   fRateValueSum(0.),
   fRateTimeSum(0.),
   fRateNumSum(0.),
   fMonitored(false),
   fAttrModified(false),
   fDeliverAllEvents(false),
   fRateWidth(5),
   fRatePrec(1)
{
}

dabc::ParameterContainer::~ParameterContainer()
{
   DOUT4("-------- Destroy Parameter %s %p", GetName(), this);
}

const std::string dabc::ParameterContainer::GetActualUnits() const
{
   std::string units = Field("units").AsStdStr();

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


bool dabc::ParameterContainer::SetField(const std::string& name, const char* value, const char* kind)
{

   DOUT4("Par:%s ParameterContainer::SetField %s = %s kind = %s", GetName(), name.c_str(), value ? value : "---", kind ? kind : "---");

   bool res(false), fire(false), doset(false), doworker(false);
   std::string sbuf;

   bool is_dflt_name = name.empty() || (name == DefaultFiledName());

   {
      LockGuard lock(ObjectMutex());

      if (!_CanChangeField(name)) return false;

      doworker = fMonitored;

      // remember parameter kind
      if (fKind.empty() && is_dflt_name && (kind!=0))
         fKind = kind;

      if ((fStatistic!=kindNone) && is_dflt_name && (value!=0)) {
         bool isint = strpbrk(value,".eE")==0;
         double value_double(0);
         int value_int(0);

         if (isint && dabc::str_to_int(value,&value_int)) {
            value_double = value_int;
            res = true;
         }

         if (!res)
            if (dabc::str_to_double(value,&value_double)) res = true;

         if (res) {
            fRateValueSum += value_double;
            fRateNumSum+=1.;
         }

         // if rate calculated asynchron, do rest in timeout processing
         if (fAsynchron) return res;

         if (res) {

            TimeStamp tm = dabc::Now();

            if (fLastChangeTm.null()) fLastChangeTm = tm;

            fRateTimeSum = tm - fLastChangeTm;

            fire = _CalcRate(sbuf);

            if (fire) {
               fLastChangeTm = tm;
               doset = true;
               value = sbuf.c_str();
            }
         }

      } else {

         doset = true;

         if (is_dflt_name) {
            TimeStamp tm = dabc::Now();

            if (!fAsynchron)
               if (fLastChangeTm.null() || (fInterval<=0.) || (tm > fLastChangeTm+fInterval)) fire = true;

            if (fire) fLastChangeTm = tm;
         }
      }
   }

   if (doset) {
      res = dabc::RecordContainer::SetField(name, value, kind);

      if (res && !is_dflt_name) {
         // indicate that not only central value, but also parameter attributes were modified
         LockGuard lock(ObjectMutex());
         fAttrModified = true;
      }
   }

   // inform worker that parameter is changed
   if (doset && res && fire && doworker) {
      Worker* w = GetWorker();
      if (w) w->WorkerParameterChanged(this);
   }

   if (fire && res) FireModified(value);


   DOUT4("ParameterContainer::SetField %s = %s  doset %s res %s fire %s",
         name.c_str(), value ? value : "---", DBOOL(doset), DBOOL(res),DBOOL(fire));

   return res;
}

void dabc::ParameterContainer::FireModified(const char* value)
{
   FireEvent(parModified);

   int doout = GetDebugLevel();

   if (doout>=0)
      dabc::Logger::Debug(doout, __FILE__, __LINE__, __func__, dabc::format("%s = %s %s", GetName(), value ? value : "(null)", GetActualUnits().c_str()).c_str() );
}

const char* dabc::ParameterContainer::GetField(const std::string& name, const char* dflt)
{
   // FIXME: one should not use const char* as return value - it is not thread safe
   return RecordContainer::GetField(name, dflt);
}


void dabc::ParameterContainer::FireEvent(int id)
{
   if (dabc::mgr() && IsParent(dabc::mgr()))
      dabc::mgr()->ProduceParameterEvent(this, id);
}


void dabc::ParameterContainer::ObjectCleanup()
{
   FireEvent(parDestroy);

   dabc::RecordContainer::ObjectCleanup();
}

void dabc::ParameterContainer::ProcessTimeout(double last_dif)
{
   bool fire(false), doset(false);
   std::string value;
   const char* cvalue(0);

//   DOUT0("Par %s Process timeout !!!", GetName());

   {
      LockGuard lock(ObjectMutex());

      if ((fStatistic!=kindNone) && fAsynchron) {
         fRateTimeSum += last_dif;
         fire = _CalcRate(value);
         if (fire) { fLastChangeTm = dabc::Now(); doset = true; }
      } else

      if (fAsynchron) {
         fRateTimeSum += last_dif;

         if (!fLastChangeTm.null() && (fRateTimeSum>fInterval)) {
            fire = true;
            fLastChangeTm.Reset();
            fRateTimeSum = 0.;
         }
      }
   }

   if (doset) {
      dabc::RecordContainer::SetField("", value.c_str(), RecordField::kind_double());
      cvalue = value.c_str();
   } else
      cvalue = dabc::RecordContainer::GetField("");

   if (fire)
      FireModified(cvalue);
}

bool dabc::ParameterContainer::_CalcRate(std::string& value)
{
   if ((fRateTimeSum < fInterval) || (fRateTimeSum<1e-6)) return false;

   double res = 0.;

   if (fStatistic==kindRate)
      res = fRateValueSum / fRateTimeSum;
   else
   if ((fStatistic==kindAverage) && (fRateNumSum>0)) {
      res = fRateValueSum / fRateNumSum;
   }

   fRateValueSum = 0.;
   fRateTimeSum = 0.;
   fRateNumSum = 0.;

   dabc::formats(value, "%*.*f", fRateWidth, fRatePrec, res);

   return true;
}


void dabc::ParameterContainer::SetSynchron(bool on, double interval, bool everyevnt)
{
   LockGuard lock(ObjectMutex());

   fAsynchron = !on;

   fInterval = interval > 0 ? interval : 0.;

   fDeliverAllEvents = everyevnt;
}

dabc::Worker* dabc::ParameterContainer::GetWorker() const
{
   Object* prnt = GetParent();
   while (prnt!=0) {
      Worker* w = dynamic_cast<Worker*> (prnt);
      if (w!=0) return w;
   }
   return 0;
}

const std::string& dabc::ParameterContainer::Kind() const
{
   LockGuard lock(ObjectMutex());
   return fKind;
}

void dabc::ParameterContainer::BuildHierarchy(HierarchyContainer* cont)
{
   dabc::Hierarchy rec(cont);

   const char* val = GetField("");
   if (val) rec.Field("value").SetStr(val);

   LockGuard lock(ObjectMutex());

   if (fStatistic == ParameterContainer::kindRate)
      rec.Field("rate").SetBool(true);
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

   GetObject()->fRateWidth = Field("width").AsInt(GetObject()->fRateWidth);
   GetObject()->fRatePrec = Field("prec").AsInt(GetObject()->fRatePrec);

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
   Field("width").SetInt(width);
   Field("prec").SetInt(prec);

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

   GetObject()->fRateWidth = Field("width").AsInt(GetObject()->fRateWidth);
   GetObject()->fRatePrec = Field("prec").AsInt(GetObject()->fRatePrec);

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


dabc::Parameter& dabc::Parameter::SetSynchron(bool on, double interval, bool everyevnt)
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
   CmdSetParameter setcmd = cmd;

   bool res = SetStr(setcmd.ParValue().AsStr());

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
   if (GetObject()) GetObject()->FireEvent(parConfigured);
}

void dabc::Parameter::FireModified()
{
   if (GetObject())
      GetObject()->FireModified(AsStr());
}

// ========================================================================================

std::string dabc::CommandDefinition::ArgName(int n) const
{
   return Field(dabc::format("#Arg%d",n)).AsStdStr();
}

bool dabc::CommandDefinition::HasArg(const std::string& name) const
{
   int num = NumArgs();

   for (int n=0;n<num;n++)
      if (ArgName(n)==name) return true;
   return false;
}



dabc::CommandDefinition& dabc::CommandDefinition::AddArg(const std::string& name, const std::string& kind, bool required, const std::string& dflt)
{
   if (name.empty() || (GetObject()==0)) return *this;

   if (!HasArg(name)) {
      int num = NumArgs();
      Field(dabc::format("#Arg%d",num)).SetStr(name);
      Field("#NumArgs").SetInt(num+1);
   }

   Field(name).SetStr(dflt);
   Field(name+"_kind").SetStr(kind);
   Field(name+"_req").SetBool(required);

   FireConfigured();

   return *this;
}

int dabc::CommandDefinition::NumArgs() const
{
   return Field("#NumArgs").AsInt(0);
}

bool dabc::CommandDefinition::GetArg(int n, std::string& name, std::string& kind, bool& required, std::string& dflt) const
{
   name = ArgName(n);
   if (name.empty()) return false;

   dflt = Field(name).AsStdStr();
   kind = Field(name+"_kind").AsStdStr();
   required = Field(name+"_req").AsBool();

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
         if (required || !dflt.empty()) cmd.Field(name).SetStr(dflt);
   }

   return cmd;
}

