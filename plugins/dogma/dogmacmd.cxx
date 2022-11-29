// $Id$

/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/

#include <cstdio>
#include <cstring>
#include <map>
#include <vector>
#include <algorithm>
#include <ctime>

#include "dabc/Manager.h"

int usage(const char* errstr = nullptr)
{
   if (errstr)
      printf("Error: %s\n\n", errstr);

   printf("Utility for dogma\n");

   return errstr ? 1 : 0;
}

int main(int argc, char* argv[])
{
   if ((argc < 2) || !strcmp(argv[1],"-help") || !strcmp(argv[1],"?"))
      return usage();

   return 0;
}
