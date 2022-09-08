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

#include "dabc/logging.h"

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <iostream>
#include <list>

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <sys/time.h>
#include <unistd.h>
#include <syslog.h>

#include "dabc/timing.h"
#include "dabc/threads.h"

dabc::Logger* dabc::Logger::gDebug = nullptr;

dabc::Logger mydebug; // here is an instance of default debug output

namespace dabc {

   class LoggerEntry {
      public:
         std::string      fFileName;
         std::string      fFuncName;
         unsigned         fLine;
         int              fLevel;
         unsigned         fCounter;
         time_t           fMsgTime; // normal time when message will be output
         std::string      fLastMsg; // last shown message
         TimeStamp        fLastTm;  // dabc (fast) time of last output
         unsigned         fDropCnt; // number of dropped messages
         bool             fShown;   // used in statistic output

         LoggerEntry(const char* fname, const char* funcname, unsigned line, int lvl) :
            fFileName(fname),
            fFuncName(funcname),
            fLine(line),
            fLevel(lvl),
            fCounter(0),
            fMsgTime(),
            fLastMsg(),
            fLastTm(),
            fDropCnt(0),
            fShown(false)
         {
         }

         ~LoggerEntry() {}
   };

   class LoggerLineEntry {
      public:

         typedef std::list<LoggerEntry*> flist;

         flist fFiles;
         unsigned  fLine;

         LoggerLineEntry(unsigned line) : fFiles(), fLine(line) {}

         ~LoggerLineEntry()
         {
            while (fFiles.size()>0) {
               delete fFiles.front();
               fFiles.pop_front();
            }
         }

         LoggerEntry* GetFile(const char* filename, const char* funcname, int lvl)
         {
            flist::iterator iter = fFiles.begin();
            while (iter != fFiles.end()) {
               if ((*iter)->fFileName.compare(filename) == 0) return *iter;
               iter++;
            }

            LoggerEntry* f = new LoggerEntry(filename, funcname, fLine, lvl);
            fFiles.push_front(f);
            return f;
         }
   };

}

// ____________________________________________________________


dabc::Logger::Logger(bool withmutex)
{
   fPrev = gDebug;

   fDebugLevel = 1;
   fFileLevel = 1;
   fSyslogLevel = 0;
   fLevel = 1;
   fPrefix = "";

   fDebugMask = lPrefix | lTStamp  | lMessage;
   fErrorMask = lPrefix | lTStamp  | lFile | lFunc | lLine | lMessage;
   fFileMask = lNoPrefix | lTime; // disable prefix and enable time

   fMutex = withmutex ? new Mutex(true) : nullptr;
   fMaxLine = 0;
   fLines = nullptr;

   fLogFileName = "";
   fFile = nullptr;
   fSyslogPrefix = "";
   fLogReopenTime = 0.;
   fLogFileModified = false;
   fLogLimit = 100;
   fLogReopenDisabled = false;

   LockGuard lock(fMutex);
   _ExtendLines(1024);

   gDebug = this;
}

dabc::Logger::~Logger()
{
   gDebug = fPrev;

   CloseFile();

   Syslog(0);

   {
     LockGuard lock(fMutex);

//     printf("Destroy logger %p lines %u\n", this, fMaxLine);

     for (unsigned n=0; n<fMaxLine; n++)
        if (fLines[n]) { /*printf("  destroy line %3u %p\n", n, fLines[n]); */ delete fLines[n]; }
     _ExtendLines(0);
   }

   delete fMutex; fMutex = nullptr;
}

void dabc::Logger::CloseFile()
{
   LockGuard lock(fMutex);

   if (fFile) fclose(fFile);
   fFile = nullptr;
}


void dabc::Logger::SetDebugLevel(int level)
{
   fDebugLevel = level;
   fLevel = fDebugLevel < fFileLevel ? fFileLevel : fDebugLevel;
}

void dabc::Logger::SetFileLevel(int level)
{
   fFileLevel = level;
   fLevel = fDebugLevel < fFileLevel ? fFileLevel : fDebugLevel;
}

void dabc::Logger::SetSyslogLevel(int level)
{
   fSyslogLevel = level;
}

void dabc::Logger::_ExtendLines(unsigned max)
{
   if ((max > 0) && (max < fMaxLine)) return;

   LoggerLineEntry **lin = nullptr;
   if (max > 0) {

//      printf("Extend logger lines %u\n", max);

      lin = new LoggerLineEntry* [max];
      unsigned n = 0;

      while (n < fMaxLine) { lin[n] = fLines[n]; n++; }
      while (n < max) { lin[n] = nullptr; n++; }
   }

   if (fLines) delete[] fLines;

   fLines = lin;
   fMaxLine = max;
}

void dabc::Logger::LogFile(const char* fname)
{
   LockGuard lock(fMutex);

   if (fFile) fclose(fFile);
   fFile = nullptr;

   if (fname && (strlen(fname) > 0)) {
      fLogFileName = fname;
      fFile = fopen(fname, "a");
      fLogReopenTime = dabc::Now().AsDouble();
      fLogFileModified = false;
   }
}

void dabc::Logger::Syslog(const char* prefix)
{
   LockGuard lock(fMutex);

   fSyslogPrefix = prefix ? prefix : "";
}


void dabc::Logger::_FillString(std::string& str, unsigned mask, LoggerEntry* entry)
{
   str.clear();
   if (mask == 0) return;

   if ((mask & lPrefix) && ((mask & lNoPrefix) == 0))
      if (fPrefix.length() > 0) {
         str = fPrefix;
         //str += ":";
      }

   if (mask & lLevel) {
      if (str.length() > 0) str+=" ";
      if (entry->fLevel<0)
         str += "EOUT";
      else
         str += dabc::format("DOUT%d", entry->fLevel);
   }

   if (mask & lSyslgLvl) {
      if (str.length() > 0) str+=" ";
      if (entry->fLevel<0)
         str += "  <E>";
      else
         str += "  <I>";
   }

   if (mask & (lDate | lTime)) {
      struct tm tm_res;
      struct tm * inf = localtime_r ( &(entry->fMsgTime), &tm_res );
      if (mask & lDate) {
         if (str.length() > 0) str+=" ";
         str += dabc::format("%d-%02d-%d", inf->tm_mday, inf->tm_mon, inf->tm_year + 1900);
      }

      if (mask & lTime) {
         if (str.length() > 0) str+=" ";
         str += dabc::format("%02d:%02d:%02d", inf->tm_hour, inf->tm_min, inf->tm_sec);
      }
   }

   if ((mask & lTStamp) && !(mask & lSyslgLvl)) {
      double tm = dabc::Now().AsDouble();
      if (str.length() > 0) str+=" ";
      str += dabc::format("%10.6f", tm);
   }

   if (mask & lFile) {
      if (str.length() > 0) str+=" ";
//      str += "File:";
      str += entry->fFileName;
   }

   if (mask & lFunc) {
      if (str.length() > 0) str+=":";
//      str += "Func:";
      str += entry->fFuncName;
   }

   if (mask & lLine) {
      if (str.length() > 0) str+=":";
      str += dabc::format("%u", entry->fLine);
   }

   if (mask & lMessage) {
      if (str.length() > 0) str+=" ";
      str += entry->fLastMsg;
   }

   if ((mask & lNoDrop) == 0)
      if (entry->fDropCnt > 0)
         str += dabc::format(" [Drop %u]", entry->fDropCnt);
}

void dabc::Logger::DoOutput(int level, const char* filename, unsigned linenumber, const char* funcname, const char* message)
{
   TimeStamp now = dabc::Now();

   std::string syslogout;

   {
   LockGuard lock(fMutex);

   if (linenumber>=fMaxLine)
      _ExtendLines((linenumber/1024 + 1) * 1024);

   LoggerLineEntry* lentry = fLines[linenumber];

   if (!lentry) {
      lentry = new LoggerLineEntry(linenumber);
      fLines[linenumber] = lentry;
   }

   LoggerEntry* entry = lentry->GetFile(filename, funcname, level);
   entry->fCounter++;

   unsigned mask = level>=0 ? fDebugMask : fErrorMask;
   unsigned fmask = mask | (fFile ? fFileMask : 0);
   bool drop_msg = (entry->fCounter > fLogLimit) &&
                   ((now - entry->fLastTm) < 0.5);

   if (drop_msg) entry->fDropCnt++;

   if (drop_msg && ((mask & lNoDrop) == 0) && ((fmask & lNoDrop) == 0)) return;

   entry->fMsgTime = time(nullptr);
   entry->fLastMsg = message;

   if ((!drop_msg || (mask & lNoDrop)) && (level<=fDebugLevel)) {
      std::string str;
      _FillString(str, mask, entry);

      if (str.length() > 0) {
         FILE* out = level < 0 ? stderr : stdout;
         fprintf(out, "%s\n", str.c_str());
         fflush(out);
      }
   }

   if (!fSyslogPrefix.empty() && (!drop_msg || (mask & lNoDrop)) && (level<=fSyslogLevel)) {
      _FillString(syslogout, mask | lSyslgLvl, entry);
   }

   if (fFile && (!drop_msg || (fmask & lNoDrop)) && (level<=fFileLevel)) {
      std::string str;
      _FillString(str, fmask, entry);
      if (str.length()>0) {
         fprintf(fFile, "%s\n", str.c_str());
         fflush(fFile);
         fLogFileModified = true;
      }
      _DoCheckTimeout();
   }

   if (!drop_msg) {
      entry->fDropCnt = 0;
      entry->fLastTm = now;
   }

   } // end of LockGuard

   if (!syslogout.empty()) {
      openlog(fSyslogPrefix.c_str(), LOG_ODELAY, LOG_LOCAL1);
      syslog(level < 0 ? LOG_ERR : LOG_INFO, "%s", syslogout.c_str());
      closelog();
   }

}

void dabc::Logger::_DoCheckTimeout()
{
   if (fLogReopenDisabled || !fFile || fLogFileName.empty() || !fLogFileModified) return;

   double now = dabc::Now().AsDouble();

   if ((now - fLogReopenTime) > 3.) {
      fLogReopenTime = now;
      fclose(fFile);
      fFile = fopen(fLogFileName.c_str(), "a");
      fLogFileModified = false;
   }
}

void dabc::Logger::CheckTimeout()
{
   if (!Instance() || Instance()->fLogReopenDisabled) return;
   LockGuard lock(Instance()->fMutex);
   Instance()->_DoCheckTimeout();
}

void dabc::Logger::DisableLogReopen()
{
   if (Instance()) Instance()->fLogReopenDisabled = true;
}

void dabc::Logger::ShowStat(bool tofile)
{
   LockGuard lock(fMutex);

   FILE* out = tofile ? fFile : stdout;
   if (!out) out = stdout;

   fprintf(out,"\n=======  Begin debug statistic =============\n");

   for (unsigned n=0; n<fMaxLine;n++) {
      LoggerLineEntry* entry = fLines[n];
      if (!entry) continue;

      LoggerLineEntry::flist::iterator iter = entry->fFiles.begin();

      while (iter != entry->fFiles.end()) {
         (*iter)->fShown = false;
         iter++;
      }
   }

   std::string* currfile = nullptr;

   do {

      currfile = nullptr;

      for (unsigned n=0; n<fMaxLine;n++) {
         LoggerLineEntry* entry = fLines[n];
         if (!entry) continue;

         LoggerLineEntry::flist::iterator iter = entry->fFiles.begin();

         while (iter != entry->fFiles.end()) {
            LoggerEntry* fentry = *iter++;

            if (fentry->fShown) continue;

            if (currfile && (fentry->fFileName.compare(*currfile) != 0)) continue;

            if (!currfile) {
               currfile = &(fentry->fFileName);
               fprintf(out, "\nFile: %s\n", currfile->c_str());
            }

            fprintf(out,"  Line:%4u Lvl:%2d Cnt:%4u Func:%s Msg:'%s'\n",
                       n, fentry->fLevel, fentry->fCounter, fentry->fFuncName.c_str(), fentry->fLastMsg.c_str());

            fentry->fShown = true;
         }
      }
   } while (currfile);

   fprintf(out,"\n=======  Stop debug statistic =============\n");

   fflush(out);
}

void dabc::SetDebugLevel(int level)
{
   dabc::Logger::Instance()->SetDebugLevel(level);
}

void dabc::SetFileLevel(int level)
{
   dabc::Logger::Instance()->SetFileLevel(level);
}

void dabc::SetDebugPrefix(const char* prefix)
{
    dabc::Logger::Instance()->SetPrefix(prefix);
}

dabc::Logger* dabc::lgr()
{
   return dabc::Logger::Instance();
}
