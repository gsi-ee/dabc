#include "dabc/logging.h"

#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <list>

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <sys/time.h>
#include <unistd.h>

#include "dabc/timing.h"
#include "dabc/threads.h"

dabc::Logger* dabc::Logger::gDebug = 0;

dabc::Logger mydebug; // here is an instance of default debug output

namespace dabc {

   class LoggerEntry {
      public:
         String      fFileName;
         String      fFuncName;
         unsigned    fLine;
         int         fLevel;
         unsigned    fCounter;
         time_t      fMsgTime; // normal time when message will be output
         String      fLastMsg; // last shown message
         TimeStamp_t fLastTm;  // dabc (fast) time of last output
         unsigned    fDropCnt; // number of dropped messages
         bool        fShown;   // used in statistic output

         LoggerEntry(const char* fname, const char* funcname, unsigned line, int lvl) :
            fFileName(fname),
            fFuncName(funcname),
            fLine(line),
            fLevel(lvl),
            fCounter(0),
            fLastMsg(),
            fLastTm(0),
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
               if ((*iter)->fFileName.compare(filename)==0) return *iter;
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
   fPrefix = "";

   fDebugMask = lPrefix | lMessage;
   fErrorMask = lPrefix | lFile | lFunc | lLine | lMessage;
   fFileMask = lNoPrefix | lTime; // disable prefix and enable time

   fMutex = withmutex ? new Mutex : 0;
   fMaxLine = 0;
   fLines = 0;

   fFile = 0;

   LockGuard lock(fMutex);
   _ExtendLines(1024);

   gDebug = this;
}

dabc::Logger::~Logger()
{
   gDebug = fPrev;

   {
     LockGuard lock(fMutex);
     for (unsigned n=0; n<fMaxLine; n++)
        if (fLines[n]) delete fLines[n];
     _ExtendLines(0);

     if (fFile) fclose(fFile);
     fFile = 0;
   }

   delete fMutex; fMutex = 0;
}

void dabc::Logger::_ExtendLines(unsigned max)
{
   if ((max>0) && (max<fMaxLine)) return;

   LoggerLineEntry **lin = 0;
   if (max>0) {
      lin = new LoggerLineEntry* [max];
      unsigned n = 0;

      while (n < fMaxLine) { lin[n] = fLines[n]; n++; }
      while (n < max) { lin[n] = 0; n++; }
   }

   if (fLines) delete[] fLines;

   fLines = lin;
   fMaxLine = max;
}

void dabc::Logger::LogFile(const char* fname)
{
   LockGuard lock(fMutex);

   if (fFile) fclose(fFile);

   fFile = 0;

   if ((fname!=0) && (strlen(fname)>0))
      fFile = fopen(fname, "a");
}

void dabc::Logger::_FillString(String& str, unsigned mask, LoggerEntry* entry)
{
   str.clear();
   if (mask==0) return;

   if ((mask & lPrefix) && ((mask & lNoPrefix) == 0))
      if (fPrefix.length() > 0) {
         str = fPrefix;
         str += ":";
      }

   if (mask & lLevel) {
      if (str.length() > 0) str+=" ";
      if (entry->fLevel<0)
         str += "EOUT";
      else
         str += dabc::format("DOUT%d", entry->fLevel);
   }

   if (mask & (lDate | lTime)) {
      struct tm * inf = localtime ( &(entry->fMsgTime) );
      if (mask & lDate) {
         if (str.length() > 0) str+=" ";
         str += dabc::format("%d-%02d-%d", inf->tm_mday, inf->tm_mon, inf->tm_year + 1900);
      }

      if (mask & lTime) {
         if (str.length() > 0) str+=" ";
         str += dabc::format("%02d:%02d:%02d", inf->tm_hour, inf->tm_min, inf->tm_sec);
      }
   }

   if (mask & lFile) {
      if (str.length() > 0) str+=" ";
      str += "File:";
      str += entry->fFileName;
   }

   if (mask & lFunc) {
      if (str.length() > 0) str+=" ";
      str += "Func:";
      str += entry->fFuncName;
   }

   if (mask & lLine) {
      if (str.length() > 0) str+=" ";
      str += dabc::format("Line:%u", entry->fLine);
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
   TimeStamp_t now = TimeStamp();

   LockGuard lock(fMutex);

   if (linenumber>=fMaxLine)
      _ExtendLines((linenumber/1024 + 1) * 1024);

   LoggerLineEntry* lentry = fLines[linenumber];

   if (lentry == 0) {
      lentry = new LoggerLineEntry(linenumber);
      fLines[linenumber] = lentry;
   }

   LoggerEntry* entry = lentry->GetFile(filename, funcname, level);
   entry->fCounter++;

   unsigned mask = level>=0 ? fDebugMask : fErrorMask;
   unsigned fmask = mask | (fFile ? fFileMask : 0);
   bool drop_msg = (entry->fCounter > 100) &&
                   (dabc::TimeDistance(entry->fLastTm, now) < 0.5);

   if (drop_msg) entry->fDropCnt++;

   if (drop_msg && ((mask & lNoDrop) == 0) && ((fmask & lNoDrop) == 0)) return;

   entry->fMsgTime = time(0);
   entry->fLastMsg = message;

   if (!drop_msg || (mask & lNoDrop)) {
      dabc::String str;
      _FillString(str, mask, entry);

      if (str.length() > 0) {
         FILE* out = level < 0 ? stderr : stdout;
         fprintf(out, str.c_str());
         fprintf(out, "\n");
         fflush(out);
      }
   }

   if (fFile && (!drop_msg || (fmask & lNoDrop))) {
      dabc::String str;
      _FillString(str, fmask, entry);
      if (str.length()>0) {
         fprintf(fFile, str.c_str());
         fprintf(fFile, "\n");
      }
   }

   if (!drop_msg) {
      entry->fDropCnt = 0;
      entry->fLastTm = now;
   }
}

void dabc::Logger::ShowStat()
{
   LockGuard lock(fMutex);

   fprintf(stdout,"\n=======  Begin debug statistic =============\n");
   if (fFile)
      fprintf(fFile,"\n=======  Begin debug statistic =============\n");

   for (unsigned n=0; n<fMaxLine;n++) {
      LoggerLineEntry* entry = fLines[n];
      if (entry==0) continue;

      LoggerLineEntry::flist::iterator iter = entry->fFiles.begin();

      while (iter != entry->fFiles.end()) {
         (*iter)->fShown = false;
         iter++;
      }
   }

   String* currfile = 0;

   do {

      currfile = 0;

      for (unsigned n=0; n<fMaxLine;n++) {
         LoggerLineEntry* entry = fLines[n];
         if (entry==0) continue;

         LoggerLineEntry::flist::iterator iter = entry->fFiles.begin();

         while (iter != entry->fFiles.end()) {
            LoggerEntry* fentry = *iter++;

            if (fentry->fShown) continue;

            if ((currfile!=0) && (fentry->fFileName.compare(*currfile) != 0)) continue;

            if (currfile==0) {
               currfile = &(fentry->fFileName);
               fprintf(stdout, "\nFile: %s\n", currfile->c_str());
               if (fFile)
                  fprintf(fFile, "\nFile: %s\n", currfile->c_str());
            }

            fprintf(stdout,"  Line:%4u Lvl:%2d Cnt:%4u Func:%s Msg:'%s'\n",
                       n, fentry->fLevel, fentry->fCounter, fentry->fFuncName.c_str(), fentry->fLastMsg.c_str());

            if (fFile)
               fprintf(fFile,"  Line:%4u Lvl:%2d Cnt:%4u Func:%s Msg:'%s'\n",
                          n, fentry->fLevel, fentry->fCounter, fentry->fFuncName.c_str(), fentry->fLastMsg.c_str());

            fentry->fShown = true;
         }
      }
   } while (currfile != 0);

   fprintf(stdout,"\n=======  Stop debug statistic =============\n");
   if (fFile)
      fprintf(fFile,"\n=======  Stop debug statistic =============\n");

   fflush(stdout);
   if (fFile) fflush(fFile);
}

void dabc::SetDebugLevel(int level)
{
   dabc::Logger::Instance()->SetDebugLevel(level);
}

void dabc::SetDebugPrefix(const char* prefix)
{
    dabc::Logger::Instance()->SetPrefix(prefix);
}
