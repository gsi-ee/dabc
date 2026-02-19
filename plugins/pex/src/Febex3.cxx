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
#include "pex/Febex3.h"
//#include "pex/Device.h"
//#include "pexor/PexorTwo.h"

#include "dabc/logging.h"

pex::Febex3::Febex3 (const std::string &name, dabc::Command cmd) :
pex::FrontendBoard::FrontendBoard(name,  FEB_FEBEX3, cmd)
{
  DOUT2 ("Created new pex::Febex3\n");


// TODO: here all configurable parameter in dabc style, like in murx
// but now from xml tags
//  MU_SETUP_GET_UI_PRINT("FEBEX_TRACE_LEN", feb_tracelen);
//  MU_SETUP_GET_UI_PRINT("FEBEX_TRIG_DELAY", feb_trig_delay);
//  MU_SETUP_GET_UI_PRINT("FEBEX_DATA_FILTER_CONTROL", data_filter_control);
//  MU_SETUP_GET_UI_PRINT("FEBEX_TRIG_SUMA", trig_sum_a);
//  MU_SETUP_GET_UI_PRINT("FEBEX_TRIG_GAP", trig_gap);
//  MU_SETUP_GET_UI_PRINT("FEBEX_TRIG_SUMB", trig_sum_b);
//  MU_SETUP_GET_UI_PRINT("FEBEX_ENERGY_SUMA", energy_sum_a);
//  MU_SETUP_GET_UI_PRINT("FEBEX_ENERGY_GAP", energy_gap);
//  MU_SETUP_GET_UI_PRINT("FEBEX_ENERGY_SUMB", energy_sum_b);
//  MU_SETUP_GET_UI("FEBEX_CLK_SOURCE_ID",clk_source[0],rev);
//  if (rev)
//    {
//      clk_source[0] = clk_source[0] & 0xFF;
//      clk_source[1] = ui_get_mu[1] & 0xFF;
//      printm ("Febex3 clock source setup has been  modified to (0x%x,%d) from %s.\n", clk_source[0],  clk_source[1],MURX_USF);
//  }
  ////////////////////////



}

pex::Febex3::~Febex3()
{

}

int pex::Febex3::Disable(int sfp, int slave) {

	return sfp+slave;
}

int pex::Febex3::Enable(int sfp, int slave) {

	return sfp+slave;
}

int pex::Febex3::Configure(int sfp, int slave) {


	return sfp+slave;
}




