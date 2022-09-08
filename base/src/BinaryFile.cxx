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

#include "dabc/BinaryFile.h"

#include <unistd.h>
#include <cstdlib>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>

#include "dabc/Object.h"
#include "dabc/logging.h"

bool dabc::FileInterface::mkdir(const char* path)
{
   if (!path || (*path == 0)) return false;

   const char* part = path;

   // we should ensure that part is exists
   while (part) {
      part = strchr(part+1, '/');

      std::string subname;
      if (!part)
         subname = path;
      else
         subname.append(path, part - path);

      struct stat buf;
      if (stat(subname.c_str(), &buf) < 0) {
         if (::mkdir(subname.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) return false;
      } else {
         if (!S_ISDIR(buf.st_mode)) {
            EOUT("Existing path %s is not a directory", subname.c_str());
            return false;
         }
      }
   }

   return true;
}


dabc::Object* dabc::FileInterface::fmatch(const char* fmask, bool select_files)
{
   if (!fmask || (*fmask == 0)) return nullptr;

   std::string pathname;
   const char* fname = nullptr;

   const char* slash = strrchr(fmask, '/');

   if (!slash) {
      pathname = ".";
      fname = fmask;
   } else {
      pathname.assign(fmask, slash - fmask + 1);
      fname = slash + 1;
   }

   struct dirent **namelist;
   int len = scandir(pathname.c_str(), &namelist, 0, alphasort);
   if (len < 0) return nullptr;

   Object* res = nullptr;
   struct stat buf;

   for (int n=0;n<len;n++) {
      const char* item = namelist[n]->d_name;
      // exclude all files/directory names starting with .
      // we exclude all hidden files or directory likes '.' and '..'
      if (!item || (*item == '.')) continue;

      if (!fname || (fnmatch(fname, item, FNM_NOESCAPE) == 0)) {
         std::string fullitemname;
         if (slash) fullitemname += pathname;
         fullitemname += item;
         if (stat(fullitemname.c_str(), &buf) != 0) continue;

         if ((select_files && !S_ISDIR(buf.st_mode) && (access(fullitemname.c_str(), R_OK) == 0)) ||
            (!select_files && S_ISDIR(buf.st_mode) && (access(fullitemname.c_str(), R_OK | X_OK) == 0))) {
            if (!res) res = new dabc::Object(nullptr, "FilesList");
            new dabc::Object(res, fullitemname);
         }
      }

      std::free(namelist[n]);
   }

   std::free(namelist);

   return res;
}
