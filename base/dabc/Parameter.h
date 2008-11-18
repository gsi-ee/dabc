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
         virtual void* GetPtr() { return 0; }

         bool IsVisible() const { return fVisibility > 0; }
         int Visibility() const { return fVisibility; }

         bool IsFixed() const { return fFixed; }
         void SetFixed(bool on = true) { fFixed = on; }

         bool IsDebugOutput() const { return fDebug; }
         void SetDebugOutput(bool on = true) { fDebug = on; }

         virtual bool GetStr(String& s) const { return false; }
         virtual bool SetStr(const char* s) { return false; }

         virtual double GetDouble() const { return 0.; }
         virtual bool SetDouble(double v) { return false; }

         virtual int GetInt() const { return 0; }
         virtual bool SetInt(int) { return false; }

         virtual void FillInfo(String& info);

         WorkingProcessor* GetParsLst() const { return fLst; }

         Basic* GetHolder();

         // these methods change parameter value not directly,
         // but via parameters list object. This is required in case
         // if parameters belong to object with its own tread otherwise
         // one can get concurrent usage/changing of non-locked value of parameter
         bool InvokeChange(const char* value);
         bool InvokeChange(dabc::Command* cmd);

         virtual bool Store(ConfigIO &cfg);
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
         bool              fFixed;
         bool              fVisibility;
         bool              fDebug;
   };

   class StrParameter : public Parameter {
      protected:
         friend class WorkingProcessor;

         StrParameter(WorkingProcessor* parent, const char* name, const char* istr = 0) :
            Parameter(parent, name),
            fValue(istr)
            {
               Ready();
            }

      public:

         virtual EParamKind Kind() const { return parString; }

         virtual void* GetPtr() { return &fValue; }

         virtual bool GetStr(String& s) const
         {
            s.assign(fValue);
            return true;
         }

         virtual bool SetStr(const char* s)
         {
            if (IsFixed()) return false;
            fValue.assign(s ? s : "");
            Changed();
            return true;
         }

         virtual double GetDouble() const
         {
            double v = 0.;
            if (fValue.length()>0)
               sscanf(fValue.c_str(), "%lf", &v);
            return v;
         }

         virtual bool SetDouble(double v)
         {
            if (IsFixed()) return false;
            dabc::formats(fValue, "%lf", v);
            Changed();
            return true;
         }

         virtual int GetInt() const
         {
            int v = 0;
            if (fValue.length()>0)
               sscanf(fValue.c_str(), "%d", &v);
            return v;
         }

         virtual bool SetInt(int v)
         {
            if (IsFixed()) return false;
            dabc::formats(fValue, "%d", v);
            Changed();
            return true;
         }

         const char* GetCharStar() const
         {
            return fValue.c_str();
         }

      protected:
         String fValue;
   };

   class DoubleParameter : public Parameter {
      protected:
         friend class WorkingProcessor;

         DoubleParameter(WorkingProcessor* parent, const char* name, double iv = 0.) :
            Parameter(parent, name),
            fValue(iv)
            {
               Ready();
            }

      public:

         virtual EParamKind Kind() const { return parDouble; }

         virtual void* GetPtr() { return &fValue; }

         virtual bool GetStr(String& s) const
         {
            dabc::formats(s, "%lf", fValue);
            return true;
         }

         virtual bool SetStr(const char* s)
         {
            if (IsFixed()) return false;
            if (s!=0)
               sscanf(s, "%lf", &fValue);
            else
               fValue = 0.;
            Changed();
            return true;
         }

         virtual bool SetDouble(double v)
         {
             if (IsFixed()) return false;
             fValue = v;
             Changed();
             return true;
          }

         virtual double GetDouble() const { return fValue; }

         virtual int GetInt() const{ return (int) fValue; }

         virtual bool SetInt(int v)
         {
            if (IsFixed()) return false;
            fValue = v;
            Changed();
            return true;
         }

      protected:
         double fValue;
   };

   class IntParameter : public Parameter {
      protected:
         friend class WorkingProcessor;

         IntParameter(WorkingProcessor* parent, const char* name, int ii = 0) :
            Parameter(parent, name),
            fValue(ii)
         {
            Ready();
         }

      public:

         virtual EParamKind Kind() const { return parInt; }

         virtual void* GetPtr() { return &fValue; }

         virtual bool GetStr(String& s) const
         {
            dabc::formats(s, "%d", fValue);
            return true;
         }

         virtual bool SetStr(const char* s)
         {
            if (IsFixed()) return false;
            if (s!=0)
               sscanf(s, "%d", &fValue);
            else
               fValue = 0;
            Changed();
            return true;
         }

         virtual bool SetInt(int v)
         {
            if (IsFixed()) return false;
            fValue = v;
            Changed();
            return true;
         }

         virtual int GetInt() const { return fValue; }

         virtual double GetDouble() const { return fValue; }

         virtual bool SetDouble(double v)
         {
            if (IsFixed()) return false;
            fValue = (int) v;
            Changed();
            return true;
         }

      protected:
         int fValue;
   };

   class RateParameter : public Parameter {
      public:
         RateParameter(WorkingProcessor* parent, const char* name, bool synchron, double interval = 1.);
         virtual ~RateParameter();

         virtual EParamKind Kind() const { return fSynchron ? parSyncRate : parAsyncRate; }

         virtual void* GetPtr() { return &fRecord; }

         virtual bool GetStr(String& s) const
         {
            dabc::formats(s, "%f", fRecord.value);
            return true;
         }

         virtual int GetInt() const { return (int) fRecord.value; }

         virtual double GetDouble() const { return fRecord.value; }

         RateRec* GetRateRec() { return &fRecord; }

         void ChangeRate(double rate);
         double GetInterval() const { return fInterval; }
         void SetInterval(double v) { fInterval = v; }

         void SetUnits(const char* name);
         void SetLimits(double lower = 0., double upper = 0.);

         virtual void AccountValue(double v);

      protected:

         virtual bool NeedTimeout() const { return !fSynchron; }
         virtual void ProcessTimeout(double last_diff);

         virtual void DoDebugOutput();

         RateRec     fRecord;
         bool        fSynchron;
         double      fInterval;
         double      fTotalSum;
         TimeStamp_t fLastUpdateTm; // used in synchron mode
         double      fDiffSum;      // used in asynchron mode
         Mutex*      fRateMutex;

   };

   class StatusParameter : public Parameter {
      public:
         StatusParameter(WorkingProcessor* parent, const char* name, int severity = 0) :
            Parameter(parent, name)
         {
            fRecord.severity = severity;
            strcpy(fRecord.color,"Cyan");
            strcpy(fRecord.status, "None"); // status name
            Ready();
         }

         virtual EParamKind Kind() const { return parStatus; }

         virtual void* GetPtr() { return &fRecord; }

         virtual bool GetStr(String& s) const
         {
            dabc::formats(s, "%d", fRecord.severity);
            return true;
         }

         virtual bool SetStr(const char* s)
         {
            if (IsFixed()) return false;
            if (s!=0)
               sscanf(s, "%d", &fRecord.severity);
            else
               fRecord.severity = 0;
            return true;
         }

         virtual bool SetInt(int v)
         {
            if (IsFixed()) return false;
            fRecord.severity = v;
            Changed();
            return true;
         }

         virtual int GetInt() const { return fRecord.severity; }

         virtual double GetDouble() const { return fRecord.severity; }

         virtual bool SetDouble(double v)
         {
            if (IsFixed()) return false;
            fRecord.severity = (int) v;
            Changed();
            return true;
         }

         StatusRec* GetStatusRec() { return &fRecord; }

      protected:

         StatusRec   fRecord;
   };
//////////////
   class InfoParameter : public Parameter {
      public:
         InfoParameter(WorkingProcessor* parent, const char* name, int verbose = 0) :
            Parameter(parent, name)
         {
            fRecord.verbose = verbose;
            strcpy(fRecord.color,"Cyan");
            strcpy(fRecord.info, "None"); // info message
            Ready();
         }

         virtual EParamKind Kind() const { return parInfo; }

         virtual void* GetPtr() { return &fRecord; }

         virtual bool GetStr(String& s) const
         {
            dabc::formats(s, "%d", fRecord.info);
            return true;
         }

         virtual bool SetStr(const char* s)
         {
            if (IsFixed()) return false;
            if (s!=0)
               strncpy(fRecord.info, s,128);
            else
               strcpy(fRecord.info, "");
            Changed();
            return true;
         }

         virtual bool SetInt(int v)
         {
            if (IsFixed()) return false;
            fRecord.verbose = v;
            Changed();
            return true;
         }

         virtual int GetInt() const { return fRecord.verbose; }

         virtual double GetDouble() const { return fRecord.verbose; }

         virtual bool SetDouble(double v)
         {
            if (IsFixed()) return false;
            fRecord.verbose = (int) v;
            Changed();
            return true;
         }

         InfoRec* GetInfoRec() { return &fRecord; }

      protected:

         InfoRec   fRecord;
   };


   class HistogramParameter : public Parameter {
      public:
         HistogramParameter(WorkingProcessor* parent, const char* name, int nchannles = 100);

         virtual ~HistogramParameter()
         {
            free(fRecord);
            fRecord = 0;
         }

         virtual EParamKind Kind() const { return parHisto; }

         virtual void* GetPtr() { return fRecord; }

         virtual bool GetStr(String& s) const
         {
            dabc::formats(s, "%d", fRecord->channels);
            return true;
         }

         virtual bool SetStr(const char* s)
         {
            if (IsFixed()) return false;
            return false;
         }

         virtual bool SetInt(int v)
         {
            if (IsFixed()) return false;
            return false;
         }

         virtual int GetInt() const { return fRecord->channels; }

         virtual double GetDouble() const { return fRecord->channels; }

         virtual bool SetDouble(double v)
         {
            if (IsFixed()) return false;
            return false;
         }

         void SetLimits(float xmin, float xmax);
         void SetColor(const char* color);
         void SetInterval(double sec);
         void SetLabels(const char* xlabel, const char* ylabel);

         void Clear();
         bool Fill(float x);


         HistogramRec* GetHistogramRec() { return fRecord; }

      protected:

         void CheckChanged(bool force = false);
         HistogramRec  *fRecord;

         TimeStamp_t fLastTm; // last time when histogram was updated
         double fInterval; // interval for histogram update
   };
}

#endif
