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
#include "pex/FrontendBoard.h"
#include "pex/Device.h"

#include "dabc/logging.h"

pex::FrontendBoard::FrontendBoard (const std::string &name, dabc::Command cmd) :
dabc::Worker(MakePair(name.empty() ? cmd.GetStr("Name","module") : name)),
fPexorDevice (nullptr),fBoard(nullptr)
{
  DOUT2 ("Created new pex::FrontendBoard\n");


}

void pex::FrontendBoard::SetDevice(pex::Device* dev)
 {
	  fPexorDevice=dev;
	  if(dev)
		  fBoard=dev->GetkinpexHandle();
 }


pex::FrontendBoard::~FrontendBoard()
{

}

