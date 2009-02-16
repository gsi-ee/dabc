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
/* identities for filter patterns                */
struct s_pat1
             {
             short         i_trigger;
             short         i_dummy;
             };

struct s_pat2
             {
             short         i_procid;
             char          h_subcrate;
             char          h_control;
             };

struct s_pat3
             {
             short         i_type;
             short         i_subtype;
             };
