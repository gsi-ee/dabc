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

#ifndef DABC_Parameter
#define DABC_Parameter

#ifndef DABC_Record
#include "dabc/Record.h"
#endif

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#ifndef DABC_threads
#include "dabc/threads.h"
#endif

#include <string.h>

namespace dabc {

   class Worker;
   class Command;
   class Manager;
   class Parameter;

   enum EParamEvent {
      parCreated = 0,      ///< produced once when parameter is created
      parConfigured = 1,   ///< event only for manager, used to react on reconfiguration of parameter
      parModified = 2,     ///< produced when parameter value modified. Either every change or after time interval (default = 1 sec)
      parDestroy = 3       ///< produced once when parameter is destroyed
   };

   /** \brief Container for parameter object
    *
    * \ingroup dabc_all_classes
    */

   class ParameterContainer : public RecordContainer {
      friend class Parameter;
      friend class Worker;
      friend class Manager;

      protected:

         enum EStatistic { kindNone, kindRate, kindAverage };

         std::string   fKind;         ///< specified kind of parameter (int, double, info, ratemeter), to be used by reference to decide if object can be assigned
         TimeStamp     fLastChangeTm; ///< last time when parameter was modified
         double        fInterval;     ///< how often modified events are produced TODO: should we move it in the normal record field?
         bool          fAsynchron;    ///< indicates if parameter can produce events asynchronous to the modification of parameter itself
                                      ///< it is a case for ratemeters
         EStatistic    fStatistic;    ///< indicates if statistic is calculated: 0 - off, 1 - rate, 2 - average
         double        fRateValueSum; ///< sum of values
         double        fRateTimeSum;  ///< sum of time
         double        fRateNumSum;   ///< sum of accumulated counts
         bool          fMonitored;    ///< if true parameter change event will be delivered to the worker
         bool          fAttrModified; ///< indicate if attribute was modified since last parameter event
         bool          fDeliverAllEvents; ///< if true, any modification event will be delivered, default off
         int           fRateWidth;    ///< display width of rate variable
         int           fRatePrec;     ///< display precision of rate variable

         virtual bool SetField(const std::string& name, const char* value, const char* kind);
         virtual const char* GetField(const std::string& name, const char* dflt = 0);

         /** Method could be used to reject changes of some fields of the parameter
          * Method called under object mutex */
         virtual bool _CanChangeField(const std::string&) { return true; }

         /** Method called from manager thread when parameter configured as asynchronous.
          * It is done intentionally to avoid situation that in non-deterministic way event processing
          * per-timeout invoked from the worker thread.
          * TODO: should be invocation of parModified events generally done from the manager thread, or keep it only for asynchron parameters? */
         void ProcessTimeout(double last_dif);

         ParameterContainer(Reference worker, const std::string& name, const std::string& parkind = "", bool hidden = false);
         virtual ~ParameterContainer();

         inline void Modified() { FireParEvent(parModified); }

         void FireParEvent(int id);

         virtual void ObjectCleanup();

         bool _CalcRate(std::string& value);

         /** Specifies that parameter produce 'modified' events synchronous with changes of parameter.
          * If on=false (asynchronous), events are produced by timeout from manager (with granularity of 1 sec).
          * Interval specifies how often event should be produced - if 0 specified every parameter change will call parameter modified event.
          */
         void SetSynchron(bool on, double interval = 1., bool everyevnt = false);

         Worker* GetWorker() const;

         int GetDebugLevel() const { return Field("debug").AsInt(-1); }

         const std::string GetActualUnits() const;

         /** If true, all events must be delivered to the consumer */
         bool IsDeliverAllEvents() const;

         /** Internal method, used to inform system that parameter is modified
          * If configured, also debug output will be produced */
         void FireModified(const char* value);

         /** \brief Save parameter attributes into container */
         virtual void BuildHierarchy(HierarchyContainer* cont);

      public:

         virtual const char* ClassName() const { return "Parameter"; }

         const std::string& Kind() const;
   };

   // _______________________________________________________________________

   /** \brief %Parameter class
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    * Allows to define parameter object in \ref dabc::Worker
    * Can deliver events, which can be monitored by any external instances.
    * Main component for implementing interface to slow-control
    */


   class Parameter : public Record {

      friend class Worker;
      friend class Manager;

      protected:

         virtual const char* ParReferenceKind() { return 0; }

         /** Specifies that parameter produce 'modified' events synchronous with changes of parameter.
          * If on=false (asynchronous), events are produced by timeout from manager (with granularity of 1 sec).
          * Interval specifies how often event should be produced - if 0 specified every parameter change will call parameter modified event.
          */
         // These method is set parameter fields, specified in the command
         int ExecuteChange(Command cmd);

         /** Returns true if any parameter attribute was modified since last call to this method */
         bool TakeAttrModified();

         /** Fire parConfigured event for parameter */
         void FireConfigured();

      public:

         /** \brief Method used in reference constructor/assignments to verify is object is suitable */
         template<class T>
         bool verify_object(Object* src, T* &tgt)
         {
            // TODO: should we implement this method ???
            tgt = dynamic_cast<T*>(src);
            if (tgt==0) return false;
            const char* refkind = ParReferenceKind();
            if (refkind==0) return true;
            return tgt->Kind() == refkind;
         }


         DABC_REFERENCE(Parameter, Record, ParameterContainer)

         bool NeedTimeout();

         /** Returns true when parameter event should be delivered to the worker */
         bool IsMonitored();

         /** Returns reference on the worker */
         Reference GetWorker() const;

         /** Indicate if parameter is should generate events synchron with code which modified it*/
         Parameter& SetSynchron(bool on, double interval = 1., bool everyevnt = false);

         /** Converts parameter in ratemeter - all values will be summed up and divided on specified interval.
          * Result value will have units/second dimension */
         Parameter& SetRatemeter(bool synchron = false, double interval = 1.0);

         /** Returns true if rate measurement is activated */
         bool IsRatemeter() const;

         /** \brief Disable ratemeter functionality */
         Parameter& DisableRatemeter();

         /** \brief Converts parameter in statistic variable.
          * All values will be summed up and average over interval will be calculated.
          * Result will have units of original variable */
         Parameter& SetAverage(bool synchron = false, double interval = 1.0);

         Parameter& DisableAverage();

         /** Returns true if average calculation is active */
         bool IsAverage() const;

         /** Specify if parameter event should be delivered to the worker */
         Parameter& SetMonitored(bool on = true);

         /** Enable/disable debug output when parameter value is changed */
         Parameter& SetDebugOutput(bool on = true, int level = 1) { return SetDebugLevel(on ? level : -1); }
         Parameter& SetDebugLevel(int level = 1) { Field("debug").SetInt(level); return *this; }
         int GetDebugLevel() const;

         /** Set parameter to convert double values to the string - used for ratemeter */
         Parameter& SetWidthPrecision(unsigned width, unsigned prec);

         Parameter& SetUnits(const std::string& unit) { Field("units").SetStr(unit); return *this; }

         /** Return units of parameter value */
         const std::string GetUnits() const { return Field("units").AsStdStr(); }

         /** Return actual units of parameter value, taking into account rate (1/s) unit when enabled */
         const std::string GetActualUnits() const;

         void SetLowerLimit(double low) { Field("low").SetDouble(low); }
         double GetLowerLimit() const { return Field("low").AsDouble(); }

         void SetUpperLimit(double up) { Field("up").SetDouble(up); }
         double GetUpperLimit() const { return Field("up").AsDouble(); }

         Parameter& SetLimits(double low, double up) { SetLowerLimit(low); SetUpperLimit(up); return *this; }

         const std::string Kind() const;

         /** Can be called by user to signal framework that parameter was modified.
          * Normally happens automatically or by time interval, but with synchronous parameter
          * last update may be lost. */
         void FireModified();
   };

   // ___________________________________________________________________________________

   /** \brief Special info parameter class
    *
    * \ingroup dabc_all_classes
    *
    * This parameter class can only be used with parameters,
    * created with CreatePar(name, "info") call
    */

   class InfoParameter : public Parameter {

      protected:
         // by this we indicate that only parameter specified as info can be referenced
         virtual const char* ParReferenceKind() { return infokind(); }

      public:

         DABC_REFERENCE(InfoParameter, Parameter, ParameterContainer)

         void SetInfo(const std::string& info) { SetStr(info); }
         std::string GetInfo() const { return AsStdStr(); }

         void SetColor(const std::string& name) { Field("color").SetStr(name); }
         std::string GetColor() const { return Field("color").AsStdStr("Green"); }

         void SetVerbosity(int level) { Field("verbosity").SetInt(level); }
         int GetVerbosity() const { return Field("verbosity").AsInt(1); }

         static const char* infokind() { return "info"; }
   };

   // _______________________________________________________________________________

   /** \brief Special command definition parameter class
    *
    * \ingroup dabc_all_classes
    *
    * This parameter class can only be used with parameters,
    * created with CreatePar(name, "cmddef") call or CreateCmdDef(name)
    */

   class CommandDefinition : public Parameter {

      protected:
         // by this we indicate that only parameter specified as info can be referenced
         virtual const char* ParReferenceKind() { return cmddefkind(); }

         std::string ArgName(int n) const;

      public:

         DABC_REFERENCE(CommandDefinition, Parameter, ParameterContainer)

         CommandDefinition& AddArg(const std::string& name, const std::string& kind = "string", bool required = true, const std::string& dflt = "");

         int NumArgs() const;

         bool HasArg(const std::string& name) const;

         bool GetArg(int n, std::string& name, std::string& kind, bool& required, std::string& dflt) const;

         /** Create command according command definition,
          * all default and required parameters will be specified */
         Command MakeCommand() const;

         static const char* cmddefkind() { return "cmddef"; }

   };

}

#endif
