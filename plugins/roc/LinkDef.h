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

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclasses;

#pragma link C++ namespace nxyter;
#pragma link C++ class nxyter::Data;
#pragma link C++ class nxyter::Sorter;
#pragma link C++ class nxyter::I2Cbus;
#pragma link C++ class nxyter::Chip;

#pragma link C++ namespace roc;
#pragma link C++ class roc::Board;
#pragma link C++ class roc::UdpBoard;
#pragma link C++ class roc::Peripheral;
#pragma link C++ class roc::FEBboard;
#pragma link C++ class roc::FEB1nxC;
#pragma link C++ class roc::FEB2nx;
#pragma link C++ class roc::FEB4nx;
#pragma link C++ class roc::Test;
#pragma link C++ class roc::Calibrate;

#endif
