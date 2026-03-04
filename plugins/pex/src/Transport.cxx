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
#include "pex/Transport.h"
#include "pex/Input.h"
#include "pex/Device.h"

#include "dabc/Port.h"
#include "dabc/logging.h"
#include "dabc/statistic.h"

pex::Transport::Transport(pex::Device* dev, pex::Input* inp, dabc::Command cmd, const dabc::PortRef& inpport)
   :  dabc::InputTransport(cmd, inpport, inp, true), fPexorDevice(dev), fPexorInput(inp)
{

}



pex::Transport::~Transport()
{

}

bool pex::Transport::StartTransport()
{
	DOUT1("StartTransport() %p\n", thread().GetObject());
	// start/stop acquisition (trigger) is independent of dabc running state!
	//fPexorDevice->StartAcquisition();
	// but we may initialize DAQ here at first startup JAM 02-mar-26:

	fPexorDevice->InitDAQ();
	return dabc::InputTransport::StartTransport();

}

bool pex::Transport::StopTransport()
{
	DOUT1("StopTransport() %p\n", thread().GetObject());
	//fPexorDevice->StopAcquisition();
	// start/stop acquisition (trigger) is independent of dabc running state!
	bool rev=dabc::InputTransport::StopTransport();
	DOUT1("\npex::Transport stopped with result %d. Total number of token errors: %d in %e s\n",  rev, fPexorInput->fErrorRate.GetNumOper(),  fPexorInput->fErrorRate.GetTotalTime());
	return rev;

}



void pex::Transport::ProcessPoolChanged(dabc::MemoryPool* pool)
{
   // TODO: check if this has any effect. Replace by other method to map dabc pool for sg dma
	DOUT1("############## pex::Transport::ProcessPoolChanged for memory pool %p",pool);
	fPexorDevice->MapDMAMemoryPool(pool);

}



