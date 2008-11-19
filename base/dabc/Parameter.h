#ifndef DABC_Parameter
#define DABC_Parameter

#ifndef DABC_Basic
#include "dabc/Basic.h"
#endif

#ifndef DABC_records
#include "dabc/records.h"
#endif

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#ifndef DABC_threads
#include "dabc/threads.h"
#endif

namespace dabc {

   class WorkingProcessor;
   class Command;
   class Manager;

   enum EParamKind { parNone, parString, parDouble, parInt, parSyncRate, parAsyncRate, parPoolStatus, parStatus, parHisto, parInfo };

   enum EParamEvent { parCreated = 0, parModified = 1, parDestroyed = 2 };

   class Parameter : public Basic {
      friend class WorkingProcessor;
      friend class Manager;

      public:
         Parameter(WorkingProcessor* lst, const char* name);
         virtual ~Parameter();

         virtual const char* MasterClassName() const { return "Parameter"; }
         virtual const char* ClassName() const { return "Parameter"; }

         virtual EParamKind Kind() const { return parNone; }

         bool IsVisible() const { return fVisibility > 0; }
         int Visibility() const { return fVisibility; }

         bool IsFixed() const;
         void SetFixed(bool on = true);

         bool IsDebugOutput() const { return fDebug; }
         void SetDebugOutput(bool on = true) { fDebug = on; }

         inline std::string GetStr() const { std::string res; return GetValue(res) ? res : ""; }
         inline bool SetStr(const std::string &value) { return SetValue(value); }

         // this is generic virtual methods to get/set parameter valus in string form
         virtual bool GetValue(std::string& value) const { return false; }
         virtual bool SetValue(const std::string &value) { return false; }
         virtual void FillInfo(std::string& info);

         Basic* GetHolder();

         // these methods change parameter value not directly,
         // but via parameters list object. This is required in case
         // if parameters belong to object with its own tread otherwise
         // one can get concurrent usage/changing of non-locked value of parameter
         bool InvokeChange(const char* value);
         bool InvokeChange(dabc::Command* cmd);

         virtual bool Store(ConfigIO &cfg);
         virtual bool Find(ConfigIO &cfg);
         virtual bool Read(ConfigIO &cfg);

      protected:

         // reimplement these two methods if one wants some
         // timeouts processing in the parameter.
         // First method just indicate if parameters wants to be timedout.
         // Second method called with time difference to last call that
         // user should not call timestamp function itself
         // ProcessTimeout() is called from manager thread, therefore one should protect values with mutex
         virtual bool NeedTimeout() const { return false; }
         virtual void ProcessTimeout(double last_diff) { }

         void Ready();

         void Changed();

         void RaiseEvent(int evt);

         virtual void DoDebugOutput();

         WorkingProcessor* fLst;
         Mutex             fValueMutex;
         bool              fFixed;
         int               fVisibility;
         bool              fDebug;
   };


   class StrParameter : public Parameter {
      protected:
         friend class WorkingProcessor;

         StrParameter(WorkingProcessor* parent, const char* name, const char* istr = 0);

      public:

         virtual EParamKind Kind() const { return parString; }

         virtual bool GetValue(std::string &value) const;
         virtual bool SetValue(const std::string &value);

      protected:
         std::string fValue;
   };


   class DoubleParameter : public Parameter {
      protected:
         friend class WorkingProcessor;

         DoubleParameter(WorkingProcessor* parent, const char* name, double iv = 0.);

      public:

         virtual EParamKind Kind() const { return parDouble; }

         virtual bool GetValue(std::string &value) const;
         virtual bool SetValue(const std::string &value);

         double GetDouble() const;
         bool SetDouble(double v);

      protected:
         double fValue;
   };


   class IntParameter : public Parameter {
      protected:
         friend class WorkingProcessor;

         IntParameter(WorkingProcessor* parent, const char* name, int ii = 0);

      public:

         virtual EParamKind Kind() const { return parInt; }

         virtual bool GetValue(std::string &value) const;
         virtual bool SetValue(const std::string &value);

         int GetInt() const;
         bool SetInt(int v);

      protected:
         int fValue;
   };

   class RateParameter : public Parameter {
      public:
         RateParameter(WorkingProcessor* parent, const char* name, bool synchron, double interval = 1.);

         virtual EParamKind Kind() const { return fSynchron ? parSyncRate : parAsyncRate; }

         RateRec* GetRateRec() { return &fRecord; }

         void ChangeRate(double rate);
         double GetInterval() const { return fInterval; }
         void SetInterval(double v) { fInterval = v; }

         void SetUnits(const char* name);
         void SetLimits(double lower = 0., double upper = 0.);

         virtual void AccountValue(double v);

         virtual bool UseMasterClassName() const { return true; }

         virtual bool Store(ConfigIO &cfg);
         virtual bool Find(ConfigIO &cfg);
         virtual bool Read(ConfigIO &cfg);

      protected:

         const char* _GetDisplayMode();
         void _SetDisplayMode(const char* v);

         virtual bool NeedTimeout() const { return !fSynchron; }
         virtual void ProcessTimeout(double last_diff);

         virtual void DoDebugOutput();

         RateRec     fRecord;
         bool        fSynchron;
         double      fInterval;
         double      fTotalSum;
         TimeStamp_t fLastUpdateTm; // used in synchron mode
         double      fDiffSum;      // used in asynchron mode
   };


   class StatusParameter : public Parameter {
      public:
         StatusParameter(WorkingProcessor* parent, const char* name, int severity = 0);

         virtual EParamKind Kind() const { return parStatus; }

         StatusRec* GetStatusRec() { return &fRecord; }

      protected:

         StatusRec   fRecord;
   };


   class InfoParameter : public Parameter {
      public:
         InfoParameter(WorkingProcessor* parent, const char* name, int verbose = 0);

         virtual EParamKind Kind() const { return parInfo; }

         InfoRec* GetInfoRec() { return &fRecord; }

      protected:

         InfoRec   fRecord;
   };


   class HistogramParameter : public Parameter {
      public:
         HistogramParameter(WorkingProcessor* parent, const char* name, int nchannles = 100);

         virtual ~HistogramParameter();

         virtual EParamKind Kind() const { return parHisto; }

         void SetLimits(float xmin, float xmax);
         void SetColor(const char* color);
         void SetInterval(double sec);
         void SetLabels(const char* xlabel, const char* ylabel);

         void Clear();
         bool Fill(float x);

         HistogramRec* GetHistogramRec() { return fRecord; }

      protected:

         bool _CheckChanged(bool force = false);
         HistogramRec  *fRecord;

         TimeStamp_t fLastTm; // last time when histogram was updated
         double fInterval; // interval for histogram update
   };
}

#endif
