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

#include "hadaq/CombinerModule.h"

#include <math.h>

#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Application.h"
#include "dabc/Manager.h"

#include "hadaq/HadaqTypeDefs.h"


hadaq::CombinerModule::CombinerModule(const char* name,  dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
	CreatePoolHandle("Pool1");

	CreateInput("Input", Pool(), 5);

//	DOUT1(("Start as client for node %s:%d", port->Par(hadaq::xmlServerName).AsStr(""), port->Par(hadaq::xmlServerPort).AsInt()));
}

void hadaq::CombinerModule::ProcessInputEvent(dabc::Port* port)
{
	dabc::Buffer buf = port->Recv();

//	unsigned cnt = hadaq::ReadIterator::NumEvents(buf);
//
//	DOUT1(("Found %u events in MBS buffer", cnt));

	buf.Release();
}





