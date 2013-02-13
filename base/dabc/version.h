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

#ifndef DABC_version
#define DABC_version

/* DABC Version information */

/*
 * These macros can be used in the following way:
 *
 *    #if DABC_VERSION_CODE >= DABC_VERSION(1,9,1)
 *       #include "newinclude.h"
 *    #else
 *       #include "oldinclude.h"
 *    #endif
 *
*/

#define DABC_RELEASE "1.9.3"
#define DABC_VERSION_CODE 67843
#define DABC_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

#endif
