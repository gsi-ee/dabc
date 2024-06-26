// $Id$
//-----------------------------------------------------------------------
//       The GSI Online Offline Object Oriented (Go4) Project
//         Experiment Data Processing at EE department, GSI
//-----------------------------------------------------------------------
// Copyright (C) 2000- GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
//                     Planckstr. 1, 64291 Darmstadt, Germany
// Contact:            http://go4.gsi.de
//-----------------------------------------------------------------------
// This software can be used under the license agreements as stated
// in Go4License.txt file which is part of the distribution.
//-----------------------------------------------------------------------

#ifndef F_UT_STATUS_H
#define F_UT_STATUS_H

#include "typedefs.h"
#include "s_daqst.h"
#include "s_setup.h"
#include "s_set_ml.h"
#include "s_set_mo.h"

INTS4 f_ut_setup(s_setup *, INTU4 *, INTS4);
INTS4 f_ut_set_ml(s_set_ml *, INTU4 *, INTS4);
INTS4 f_ut_status(s_daqst *, INTS4);
INTS4 f_ut_setup_r(s_setup *, INTS4);
INTS4 f_ut_set_ml_r(s_set_ml *, INTS4);
INTS4 f_ut_status_r(s_daqst *, INTS4);
INTS4 f_ut_setup_ini(s_setup *);
INTS4 f_ut_set_ml_ini(s_set_ml *);
INTS4 f_ut_set_mo_ini(s_set_mo *);
INTS4 f_ut_status_ini(s_daqst *);
INTS4 f_ut_set_mo(s_set_mo *, INTS4);
INTS4 f_ut_set_mo_r(s_set_mo *, INTS4);

#endif
