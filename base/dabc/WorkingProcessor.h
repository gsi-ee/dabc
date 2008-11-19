#ifndef DABC_WorkingProcessor
#define DABC_WorkingProcessor

#ifndef DABC_CommandClient
#include "dabc/CommandClient.h"
#endif

#ifndef DABC_WorkingThread
#include "dabc/WorkingThread.h"
#endif

namespace dabc {

   class Basic;
   class Folder;
   class Parameter;

   class WorkingProcessor : public CommandReceiver {

      friend class WorkingThread;
      friend class Parameter;

      public:

         enum { evntFirstUser = 1 };

         WorkingProcessor();
         virtual ~WorkingProcessor();

         /** Method returns name of required thread class for processor.
           * If returns 0 (default) any thread class is sufficient. */
         virtual const char* RequiredThrdClass() const { return 0; }

         bool AssignProcessorToThread(WorkingThread* thrd, bool sync = true);
         void RemoveProcessorFromThread(bool forget_thread);
         WorkingThread* ProcessorThread() const { return fProcessorThread; }
         const char* ProcessorThreadName() const { return fProcessorThread ? fProcessorThread->GetName() : 0; }

         // call this method when you want to be called after specified time period
         // 0 means to get called as soon as possible
         // <0 means cancel (deactivate) if possible (when it is not too late in time) previousely scheduled timeout
         // >0 activate after specified interval
         void ActivateTimeout(double tmout_sec);

         void SetProcessorPriority(int nq) { fProcessorPriority = nq; }
         inline int ProcessorPriority() const { return fProcessorPriority; }

         uint32_t ProcessorId() const { return fProcessorId; }

         void DestroyProcessor();

         virtual bool Submit(Command* cmd);


         // this all about parameters list, which can be managed for any working processor

         Parameter* FindPar(const char* name) const;

         std::string GetParStr(const char* name, const std::string& defvalue = "") const;
         int GetParInt(const char* name, int defvalue = 0) const;
         double GetParDouble(const char* name, double defvalue = 0.) const;

         Folder* GetTopParsFolder();

      protected:

         // Method is called when requested time point is reached
         // Rewrite method in derived class to react on this event
         // return value specifies time interval to the next timeout
         // Argument identifies time distance to previous timeout
         // Return value: <0 - no new timeout is required
         //               =0 - provide timeout as soon as possible
         //               >0 - activate timeout after this interval

         virtual double ProcessTimeout(double last_diff) { return -1.; }

         inline void FireEvent(uint16_t evid)
         {
            if (fProcessorThread && (fProcessorId>0))
               fProcessorThread->Fire(CodeEvent(evid, fProcessorId), fProcessorPriority);
         }

         inline void FireEvent(uint16_t evid, uint32_t arg)
         {
            if (fProcessorThread && (fProcessorId>0))
               fProcessorThread->Fire(CodeEvent(evid, fProcessorId, arg), fProcessorPriority);
         }

         virtual void ProcessEvent(EventId);

         bool ActivateMainLoop();
         void ExitMainLoop();
         void SingleLoop(double tmout) { ProcessorThread()->SingleLoop(this, tmout); }

         virtual void DoProcessorMainLoop() {}
         virtual void DoProcessorAfterMainLoop() {}

         virtual bool IsExecutionThread();

         // method called immediately after processor was assigned to thread
         // called comes from the thread context
         virtual void OnThreadAssigned() {}

         virtual int PreviewCommand(Command* cmd);

         // some protected method for parameters handling
         Parameter* CreatePar(int kind, const char* name, const char* initvalue = 0);
         Parameter* CreateParStr(const char* name, const char* initvalue = 0);
         Parameter* CreateParInt(const char* name, int initvalue = 0);
         Parameter* CreateParDouble(const char* name, double initvalue = 0.);

         void DestroyAllPars();
         void DestroyPar(const char* name) { DestroyPar(FindPar(name)); }
         bool DestroyPar(Parameter* par);
         bool InvokeParChange(Parameter* par, const char* value, Command* cmd);

         bool SetParStr(const char* name, const char* value);
         bool SetParStr(const char* name, const std::string& value) { return SetParStr(name, value.c_str()); }
         bool SetParInt(const char* name, int value);
         bool SetParDouble(const char* name, double value);

         bool SetParFixed(const char* name, bool on = true);
         void LockUnlockPars(bool on);

         Folder* MakeFolderForParam(const char* parname);
         void SetParsHolder(Folder* holder, const char* subfolder = 0);
         void SetParDflts(int visibility = 1, bool fixed = false);

         // this method is called after parameter is changed
         // user may add its reaction on this event, but cannot
         // refuse parameter changing
         virtual void ParameterChanged(Parameter* par) {}

         WorkingThread*   fProcessorThread;
         uint32_t         fProcessorId;
         int              fProcessorPriority;
         CommandsQueue    fProcessorCommands;

         Folder*          fParsHolder;
         std::string      fParsTopFolderName;

         Mutex            fProcessorMutex;

         int              fParsDfltVisibility;
         bool             fParsDfltFixed;

      private:
         bool TakeActivateData(TimeStamp_t& mark, double& interval);

         bool             fProcessorActivateTmout; // used in activate to deliver timestamp to thread, locked by mutex
         TimeStamp_t      fProcessorActivateMark; // used in activate to deliver timestamp to thread, locked by mutex
         double           fProcessorActivateInterv; // used in activate to deliver timestamp to thread, locked by mutex

         TimeStamp_t      fProcessorPrevFire; // used in thread
         TimeStamp_t      fProcessorNextFire; // used in thread
   };
}

#endif
