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

#ifndef DABC_Command
#define DABC_Command

#ifndef DABC_Record
#include "dabc/Record.h"
#endif

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#include <list>


namespace dabc {

   class CommandsQueue;
   class Worker;
   class Thread;

   enum CommandRes {
      cmd_false = 0,
      cmd_true = 1,

      cmd_timedout = -1,
      cmd_ignore = -2,
      cmd_postponed = -3,
      cmd_noresult = -4,
      cmd_canceled = -5
   };


   /** \brief Container object for command which should not been seen to normal user
    *
    * \ingroup dabc_all_classes
    *
    * All operations should be performed with dabc::Command class
    * which is reference to this container */

   class CommandContainer : public RecordContainer {

      friend class Command;

      protected:

         struct CallerRec {
            Reference  worker;
            bool*      exe_ready;
            CallerRec() : worker(), exe_ready(0) {}
            CallerRec(Reference w, bool* e) : worker(w), exe_ready(e) {}
            CallerRec(const CallerRec& src) : worker(src.worker), exe_ready(src.exe_ready) {}

            CallerRec& operator=(CallerRec& src)
            {
               worker = src.worker;
               exe_ready = src.exe_ready;
               return *this;
            }
         };

         typedef std::list<CallerRec> CallersList;

         CallersList   fCallers;     ///< list of callers
         TimeStamp     fTimeout;     ///< absolute time when timeout will be expired
         bool          fCanceled;    ///< indicate if command was canceled ant not need to be executed further

         // make destructor protected that nobody can delete command directly
         CommandContainer(const std::string& name = "Command");

         virtual ~CommandContainer();

         virtual const char* ClassName() const { return "Command"; }
   };

   /** \brief Represents command with its arguments
    *
    * \ingroup dabc_core_classes
    */

   class Command : public Record {

      DABC_REFERENCE(Command, Record, CommandContainer)

      friend class CommandContainer;
      friend class CommandsQueue;
      friend class Worker;
      friend class Thread;

      private:

         void AddCaller(Reference worker, bool* exe_ready = 0);
         void RemoveCaller(Worker* worker, bool* exe_ready = 0);

         bool IsLastCallerSync();

         static Reference MakeRef(const std::string& buf);

      public:
         /** \brief Default constructor, creates empty reference on the command */
//         Command() {}

         Command(const std::string& name) throw();

         /** Compare operator, returns true when command contains pointer on same object */
         friend int operator==(const Command& cmd1, const Command& cmd2)
                         { return cmd1() == cmd2(); }

         bool HasField(const std::string& name) const { return GetObject() ? GetObject()->Fields().HasField(name) : false; }
         bool RemoveField(const std::string& name) { return GetObject() ? GetObject()->Fields().RemoveField(name) : false; }

         unsigned NumFields() const { return GetObject() ? GetObject()->Fields().NumFields() : 0; }
         std::string FieldName(unsigned cnt) const { return GetObject() ? GetObject()->Fields().FieldName(cnt) : std::string(); }

         RecordField& Field(const std::string& name) { return GetObject()->Fields().Field(name); }
         const RecordField& Field(const std::string& name) const { return GetObject()->Fields().Field(name); }

         std::string SaveToXml(bool compact=true);
         bool ReadFromXml(const char* xmlcode);

         // set of methods to keep old interface, it is preferable to use field methods

         bool SetStr(const std::string& name, const char* value) { return Field(name).SetStr(value); }
         bool SetStr(const std::string& name, const std::string& value) { return Field(name).SetStr(value); }
         std::string GetStr(const std::string& name, const std::string& dflt = "") const { return Field(name).AsStr(dflt); }
         // TODO: remove after a while
         std::string GetStdStr(const std::string& name, const std::string& dflt = "") const { return Field(name).AsStr(dflt); }

         bool SetInt(const std::string& name, int v) { return Field(name).SetInt(v); }
         int GetInt(const std::string& name, int dflt = 0) const { return Field(name).AsInt(dflt); }

         bool SetBool(const std::string& name, bool v) { return Field(name).SetBool(v); }
         bool GetBool(const std::string& name, bool dflt = false) const { return Field(name).AsBool(dflt); }

         bool SetDouble(const std::string& name, double v) { return Field(name).SetDouble(v); }
         double GetDouble(const std::string& name, double dflt = 0.) const { return Field(name).AsDouble(dflt); }

         bool SetUInt(const std::string& name, unsigned v) { return Field(name).SetUInt(v); }
         unsigned GetUInt(const std::string& name, unsigned dflt = 0) const { return Field(name).AsUInt(dflt); }

         /** \brief Set pointer argument for the command */
         void SetPtr(const std::string& name, void* p);
         /** \brief Get pointer argument from the command */
         void* GetPtr(const std::string& name, void* deflt = 0) const;

         /** Set reference to the command */
         void SetRef(const std::string& name, Reference ref);

         /** \brief Returns reference from the command, can be called only once */
         Reference GetRef(const std::string& name);

         /** \brief Set raw data to the command, which can be transported also between nodes */
         bool SetRawData(Reference rawdata);

         /** \brief Returns reference on raw data
          * Can be called only once - raw data reference will be cleaned */
         Reference GetRawData();

         void AddValuesFrom(const Command& cmd, bool canoverwrite = true);

         void SetResult(int res) { Field(ResultParName()).SetInt(res); }
         int GetResult() const { return Field(ResultParName()).AsInt(0); }
         void ClearResult() { RemoveField(ResultParName()); }
         bool HasResult() const { return HasField(ResultParName()); }

         /** \brief Set maximum time which can be used for command execution.
          *
          * If command is timed out, it will be replied with 'cmd_timedout' value.
          * \param[in] tm  timeout value, if <=0, timeout for command will be disabled
          * \returns      reference on command itself */
         Command& SetTimeout(double tm);

         /** \brief Return true if timeout was specified */
         bool IsTimeoutSet() const;

         /** \brief Returns time which remains until command should be timed out.
          * If returns positive value, timeout was specified and not yet expired (value is time in seconds till timeout)
          * If returns 0 timeout is happened. If returns negative value, timeout was not specified for the command. */
         double TimeTillTimeout() const;

         /** \brief Returns true if command timeout is expired */
         bool IsTimedout() const { return TimeTillTimeout() == 0.; }

         /** \brief Disable timeout for the command */
         void ResetTimeout() { SetTimeout(-1); }


         /** \brief Read command from string, which is typed in std output
          *  \details in simple case string is just command name
          *  One could specify arguments as list parameters, separated by spaces
          *  One also can specify arguments name with syntax arg_name=arg_value.
          *  Valid syntax is:
          *     ping
          *     ping lxg0534
          *     ping addr=lxg0534 port=2233
          */
         bool ReadFromCmdString(const std::string& str);

         /** Set command priority, defines how fast command should be treated
          *  In special cases priority allows to execute command also in worker which is not active */
         void SetPriority(int prio) { Field(PriorityParName()).SetInt(prio); }

         /** Returns command priority */
         int GetPriority() const;

         /** \brief Show on debug output content of command */
         void Print(int lvl=0, const char* from = 0) const;

         /** Replied on the command.
          * Only non-negative value should be set as result value.
          * There are several special negative values:
          *   cmd_timedout = -1 - command was timed-out
          * If result value is already set, one can call cmd.Reply() to just
          * inform receiver that command is executed.
          * After call of Reply() method command is no longer available to the user.
          */
         void Reply(int res = cmd_noresult);

         /** Reply on the command with false (cmd_false==0) value */
         void ReplyFalse() { Reply(cmd_false); }

         /** Reply on the command with true (cmd_true==1) value */
         void ReplyTrue() { Reply(cmd_true); }

         /** Reply on the command with true or false value */
         void ReplyBool(bool res) { Reply(res ? cmd_true : cmd_false); }

         /** Reply on the command with timeout value */
         void ReplyTimedout() { Reply(cmd_timedout); }

         /** Method used to clean command -
          * all internal data will be cleaned, command container will be released */
         void Release();

         /** \brief Call this method to cancel command execution.
          * Method must be used by initiator of the command.
          * As far as possible command execution will be canceled */
         void Cancel();

         /** \brief Return true if command was canceled. */
         bool IsCanceled();

         /** These methods prepare command so, that one can submit
           * command to the manager like:
           *    dabc::mgr.Submit(Command("Start").SetReceiver("Generator"));
           * Command will be queued in manager queue and than submitted to specified
           * object. If object has own thread, it will be used for command execution */

         Command& SetReceiver(const std::string& itemname);
         Command& SetReceiver(int nodeid, const std::string& itemname = "");
         Command& SetReceiver(Object* rcv);
         Command& SetReceiver(int nodeid, Object* rcv);
         std::string GetReceiver() const { return Field(ReceiverParName()).AsStr(); }
         void RemoveReceiver() { RemoveField(ReceiverParName()); }

         /** Name of the parameter, used to specified command receiver.
          * It can contain full url, including node name and item name  */
         static const char* ReceiverParName() { return "_Receiver_"; }

         /** Name of the parameter, used to keep command result */
         static const char* ResultParName() { return "_Result_"; }

         /** Name of the parameter, used to keep command priority */
         static const char* PriorityParName() { return "_Priority_"; }

   };


#define DABC_COMMAND(cmd_class, cmd_name) \
   public: \
      static const char* CmdName() { return cmd_name; } \
      cmd_class() : dabc::Command(CmdName()) {} \
      cmd_class(const dabc::Command& src) throw() : dabc::Command(src) \
        { if (!src.IsName(CmdName())) throw dabc::Exception(dabc::ex_Command, "Wrong command name in assignment constructor", src.GetName()); } \
      cmd_class& operator=(const cmd_class& cmd) throw() { dabc::Command::operator=(cmd); return *this; } \
      cmd_class& operator=(const dabc::Command& cmd) throw() { \
         if (!cmd.IsName(CmdName())) throw dabc::Exception(dabc::ex_Command, "Wrong command name in assignment operator", cmd.GetName()); \
         dabc::Command::operator=(cmd); return *this;  \
      }

}

#endif
