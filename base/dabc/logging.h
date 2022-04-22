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

#ifndef DABC_logging
#define DABC_logging

#include <cstdio>

#include <cstdint>

#ifndef DABC_string
#include "dabc/string.h"
#endif

#ifndef DABC_defines
#include "dabc/defines.h"
#endif

#ifndef DEBUGLEVEL
#define DEBUGLEVEL 1
#endif

namespace dabc {

   class LoggerEntry;
   class LoggerLineEntry;
   class Mutex;

   /** \brief Logging class
    *
    * \ingroup dabc_all_classes
    *
    * Accessible via dabc::lgr() function.
    *
    */

   class Logger {
      public:
         enum EShowInfo {
            lPrefix  = 0x0001,  // show configured debug prefix
            lDate    = 0x0002,  // show current date
            lTime    = 0x0004,  // show current time
            lFile    = 0x0008,  // show source file
            lFunc    = 0x0010,  // show source func
            lLine    = 0x0020,  // show line number
            lLevel   = 0x0040,  // show message level
            lMessage = 0x0080,  // show debug message itself
            lNoDrop  = 0x0100,  // disable drop of frequent messages
            lNoPrefix= 0x0200,  // disable prefix output (superior to lPrefix)
            lTStamp  = 0x0400,  // show TimeStamp (ms precision)
            lSyslgLvl= 0x0800   // show messege level in syslog format
         };

         Logger(bool withmutex = true);
         virtual ~Logger();

         // Debug mask sets fields, which are displayed for normal debug messages
         void SetDebugMask(unsigned mask) { fDebugMask = mask; }
         unsigned GetDebugMask() const  { return fDebugMask; }

         // Error mask sets fields, which are displayed for error messages
         void SetErrorMask(unsigned mask) { fErrorMask = mask; }
         unsigned GetErrorMask() const  { return fErrorMask; }

         // File mask can only add extra fields to the output
         void SetFileMask(unsigned mask) { fFileMask = mask; }
         unsigned GetFileMask() const  { return fFileMask; }

         void SetDebugLevel(int level = 0);
         void SetFileLevel(int level = 0);
         void SetSyslogLevel(int level = 0);

         void SetLogLimit(unsigned limit = 100) { fLogLimit = limit; }
         inline int GetDebugLevel() const { return fDebugLevel; }
         inline int GetFileLevel() const { return fFileLevel; }

         void SetPrefix(const char* prefix = nullptr) { fPrefix = prefix ? prefix : ""; }
         const char* GetPrefix() { return fPrefix.c_str(); }

         virtual void LogFile(const char* fname);
         virtual void Syslog(const char* prefix);

         void ShowStat(bool tofile = true);

         /** \brief Close any file open by logger */
         void CloseFile();

         static void CheckTimeout();

         static void DisableLogReopen();

         static inline Logger* Instance() { return gDebug; }

         static inline void Debug(int level, const char* filename, unsigned linenumber, const char* funcname, const char* message)
         {
            if (Instance() && (level <= Instance()->fLevel))
               Instance()->DoOutput(level, filename, linenumber, funcname, message);
         }

      protected:

         static Logger* gDebug;

         virtual void DoOutput(int level, const char* filename, unsigned linenumber, const char* funcname, const char* message);

         void _ExtendLines(unsigned max);

         void _FillString(std::string& str, unsigned mask, LoggerEntry* entry);

         virtual void _DoCheckTimeout();

      private:
         Logger*           fPrev{nullptr};
         LoggerLineEntry **fLines{nullptr};
         unsigned          fMaxLine{0};
         Mutex            *fMutex{nullptr};
         FILE             *fFile{nullptr};            // output file for log messages
         std::string       fSyslogPrefix;             // if specified, send message to syslog
         unsigned          fDebugMask{0};             // mask for debug output
         unsigned          fErrorMask{0};             // mask for error output
         unsigned          fFileMask{0};              // mask for file output
         int               fDebugLevel{0};            // level of debug output on terminal
         int               fFileLevel{0};             // level of debug output to file
         int               fSyslogLevel{0};           // level of syslog message
         int               fLevel{0};                 // used to define max
         std::string       fPrefix;                   // prefix of all messages
         std::string       fLogFileName;              // name of logfile
         double            fLogReopenTime{0};         // last time when logfile was reopened
         bool              fLogFileModified{false};   // true if any string was written into file
         unsigned          fLogLimit{0};              // maximum number of log messages before drop
         bool              fLogReopenDisabled{false}; // disable file reopen when doing shutdown
   };

   #define DOUT(level, args ... ) \
     dabc::Logger::Debug(level, __FILE__, __LINE__, __func__, dabc::format( args ).c_str())

   #if DEBUGLEVEL > -2
      #define EOUT( args ... ) DOUT(-1, args )
   #else
      #define EOUT( args ... ) {}
   #endif

   #if DEBUGLEVEL > -1
      #define DOUT0( args ... ) DOUT(0, args )
   #else
      #define DOUT0( args ... ) {}
   #endif

   #if DEBUGLEVEL > 0
      #define DOUT1( args ... ) DOUT(1, args )
   #else
      #define DOUT1( args ... ) {}
   #endif

   #if DEBUGLEVEL > 1
      #define DOUT2( args ... ) DOUT( 2, args )
   #else
      #define DOUT2( args ... ) {}
   #endif

   #if DEBUGLEVEL > 2
      #define DOUT3( args ... ) DOUT(3, args )
   #else
      #define DOUT3( args ... ) {}
   #endif

   #if DEBUGLEVEL > 3
      #define DOUT4( args ... ) DOUT(4, args )
   #else
      #define DOUT4( args ... ) {}
   #endif

   #if DEBUGLEVEL > 4
      #define DOUT5( args ... ) DOUT(5, args )
   #else
      #define DOUT5( args ... )
   #endif

   #define DBOOL(arg) (arg ? "true" : "false")

   #define DNAME(arg) (arg ? arg->GetName() : "---")

   extern void SetDebugLevel(int level = 0);
   extern void SetFileLevel(int level = 0);
   extern void SetDebugPrefix(const char* prefix = nullptr);

   extern Logger* lgr();
};

#endif

