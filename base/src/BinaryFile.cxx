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
#include <stdlib.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "dabc/Object.h"

#include "dabc/logging.h"

dabc::Object* dabc::FileInterface::fmatch(const char* fmask)
{
   if ((fmask==0) || (*fmask==0)) return 0;

   std::string pathname;
   const char* fname(0);

   const char* slash = strrchr(fmask, '/');

   if (slash==0) {
      pathname = ".";
      fname = fmask;
   } else {
      pathname.assign(fmask, slash - fmask + 1);
      fname = slash + 1;
   }

   struct dirent **namelist;
   int len = scandir(pathname.c_str(), &namelist, 0, alphasort);
   if (len < 0) return 0;

   Object* res = 0;
   struct stat buf;

   for (int n=0;n<len;n++) {
      const char* item = namelist[n]->d_name;

      if ((fname==0) || (fnmatch(fname, item, FNM_NOESCAPE)==0)) {
         std::string fullitemname;
         if (slash) fullitemname += pathname;
         fullitemname += item;
         if (stat(fullitemname.c_str(), &buf)==0)
            if (!S_ISDIR(buf.st_mode) && (access(fullitemname.c_str(), R_OK)==0)) {
               if (res==0) res = new dabc::Object(0, "FilesList");
               new dabc::Object(res, fullitemname);
            }
      }

      free(namelist[n]);
   }

   free(namelist);

   return res;
}
