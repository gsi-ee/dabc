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
#include "pex/Poland.h"

#include "dabc/logging.h"

pex::Poland::Poland (const std::string &name, dabc::Command cmd) :
pex::FrontendBoard::FrontendBoard(name,  FEB_POLAND, cmd)
{
  DOUT2 ("Created new pex::Poland\n");





}

pex::Poland::~Poland()
{

}

int pex::Poland::Disable(int sfp, int slave) {

	return sfp+slave;
}

int pex::Poland::Enable(int sfp, int slave) {

	return sfp+slave;
}

int pex::Poland::Configure(int sfp, int slave) {


	return sfp+slave;
}




