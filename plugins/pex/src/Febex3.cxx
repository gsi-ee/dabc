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
#include "pex/Device.h"
#include "pexor/PexorTwo.h"

#include "dabc/logging.h"
#include "dabc/string.h"
#include <unistd.h>
#include <string>

namespace pex
{
const char *xmlTracelen = "Tracelength";
const char *xmlTrigDelay = "TriggerDelay";
const char *xmlDataFilterControl = "DataFilterControl";
const char *xmlTrigSumA = "TriggerSumA";
const char *xmlTrigGap = "TriggerGap";
const char *xmlTrigSumB = "TriggerSumB";
const char *xmlEnergySumA = "EnergySumA";
const char *xmlEnergyGap = "EnergyGap";
const char *xmlEnergySumB = "EnergySumB";
const char *xmlClockSourceSFP = "ClockSourceSFP";
const char *xmlClockSourceSlave = "ClockSourceSlave";
const char *xmlFebex14bit = "Is14bit_";
const char *xmlNegativePolarity = "NegativePolarity_";
const char *xmlTriggerMode = "TriggerMode_";
const char *xmlEvenOddOr = "EvenOddOr_";
const char *xmlDisableMask = "ChannelsDisabledMask_";
const char *xmlSparsifyingMask = "ChannelsSparsifyingMask_";
const char *xmlTriggerMask = "ChannelsTriggerMask_";
const char *xmlChannelThreshold = "ChannelThresholds_";
}

pex::Febex3::Febex3(const std::string &name, dabc::Command cmd) :
    pex::FrontendBoard::FrontendBoard(name, FEB_FEBEX3, cmd)
{
  DOUT2("Created new pex::Febex3...\n");

// TODO: here all configurable parameter in dabc style, like in murx
  fTracelen = Cfg(pex::xmlTracelen, cmd).AsInt(FEB_TRACE_LEN);
  fTrigDelay = Cfg(pex::xmlTrigDelay, cmd).AsInt(FEB_TRIG_DELAY);
  fDataFilterControl = Cfg(pex::xmlDataFilterControl, cmd).AsInt(DATA_FILT_CONTROL_DAT);
  fTrigSumA = Cfg(pex::xmlTrigSumA, cmd).AsInt(TRIG_SUM_A);
  fTrigGap = Cfg(pex::xmlTrigGap, cmd).AsInt(TRIG_GAP);
  fTrigSumB = Cfg(pex::xmlTrigSumB, cmd).AsInt(TRIG_SUM_B);
  fEnergySumA = Cfg(pex::xmlEnergySumA, cmd).AsInt(ENERGY_SUM_A);
  fEnergyGap = Cfg(pex::xmlEnergyGap, cmd).AsInt(ENERGY_GAP);
  fEnergySumB = Cfg(pex::xmlEnergySumB, cmd).AsInt(ENERGY_SUM_B);
  fClockSourceSFP = Cfg(pex::xmlClockSourceSFP, cmd).AsInt(-1);
  fClockSourceSlave = Cfg(pex::xmlClockSourceSlave, cmd).AsInt(-1);
  DOUT1("Febex3 is using tracelen %d, triggerdelay:%d, data filter control: 0x%x\n", fTracelen, fTrigDelay,
      fDataFilterControl);
  DOUT1("\t  Trigger: sumA:%d, gap:%d, sumB:%d  Energy: sumA:%d, gap:%d, sumBB:%d\n", fTrigSumA, fTrigGap, fTrigSumB,
      fEnergySumA, fEnergyGap, fEnergySumB);
  DOUT1("\t  Clock source: (sfp %d, slave:%d)\n", fClockSourceSFP, fClockSourceSlave);

  // more configuration bits from arrays:
  std::vector<long int> arr;
  for (int sfp = 0; sfp < PEX_NUMSFP; sfp++)
  {
    f12_14[sfp].clear();
    arr.clear();
    arr = Cfg(dabc::format("%s%d", pex::xmlFebex14bit, sfp), cmd).AsIntVect();
    for (size_t i = 0; i < arr.size(); ++i)
    {
      f12_14[sfp].push_back(arr[i]);
    }

    fPolarity[sfp].clear();
    arr.clear();
    arr = Cfg(dabc::format("%s%d", pex::xmlNegativePolarity, sfp), cmd).AsIntVect();
    for (size_t i = 0; i < arr.size(); ++i)
    {
      fPolarity[sfp].push_back(arr[i]);
    }

    fTriggerMod[sfp].clear();
    arr.clear();
    arr = Cfg(dabc::format("%s%d", pex::xmlTriggerMode, sfp), cmd).AsIntVect();
    for (size_t i = 0; i < arr.size(); ++i)
    {
      fTriggerMod[sfp].push_back(arr[i]);
    }

    fEvenOddOr[sfp].clear();
    arr.clear();
    arr = Cfg(dabc::format("%s%d", pex::xmlEvenOddOr, sfp), cmd).AsIntVect();
    for (size_t i = 0; i < arr.size(); ++i)
    {
      fEvenOddOr[sfp].push_back(arr[i]);
    }

    fDisableChannel[sfp].clear();
    arr.clear();
    arr = Cfg(dabc::format("%s%d", pex::xmlDisableMask, sfp), cmd).AsIntVect();
    for (size_t i = 0; i < arr.size(); ++i)
    {
      fDisableChannel[sfp].push_back(arr[i]);
    }

    fDataReduction[sfp].clear();
    arr.clear();
    arr = Cfg(dabc::format("%s%d", pex::xmlSparsifyingMask, sfp), cmd).AsIntVect();
    for (size_t i = 0; i < arr.size(); ++i)
    {
      fDataReduction[sfp].push_back(arr[i]);
    }

    fEnableTrigger[sfp].clear();
    arr.clear();
    arr = Cfg(dabc::format("%s%d", pex::xmlTriggerMask, sfp), cmd).AsIntVect();
    for (size_t i = 0; i < arr.size(); ++i)
    {
      fEnableTrigger[sfp].push_back(arr[i]);
    }

    for (int slave = 0; slave < PEX_MAXSLAVES; slave++)
    {
      arr.clear();
      arr = Cfg(dabc::format("%s%d_%X", pex::xmlChannelThreshold, sfp, slave), cmd).AsIntVect();
      for (size_t i = 0; i < arr.size(); ++i)
      {
        fThreshold[sfp][slave].push_back(arr[i]);
      }
    }    // slave
  }    // sfp
//

}

pex::Febex3::~Febex3()
{

}

int pex::Febex3::Disable(int sfp, int slave)
{
  int rev = 0;
  rev = fKinpex->WriteBus(DATA_FILT_CONTROL_REG, 0x00, sfp, slave);
  usleep(4000);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX disable failed at sfp:%d,dev:%d\n ", rev, sfp, slave);
    return rev;
  }
  return 0;
}

int pex::Febex3::Enable(int sfp, int slave)
{
  int rev = 0;
  rev = fKinpex->WriteBus(DATA_FILT_CONTROL_REG, (fDataFilterControl & 0xFF), sfp, slave);
  usleep(4000);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX enable failed at sfp:%d,dev:%d\n ", rev, sfp, slave);
    return rev;
  }
  return 0;
}

int pex::Febex3::Configure(int sfp, int sl)
{
  int rev = 0;
  int value = 0;
  // get submemory addresses to check setup:
  unsigned long base_dbuf0 = 0, base_dbuf1 = 0;
  unsigned long num_submem = 0, submem_offset = 0;
  unsigned long feb_ctrl = 0, feb_time = 0;
  //  unsigned long qfw_ctrl = 0;

  // needed for check of meta data, read it in any case
//    printm ("SFP: %d, FEBEX3: %d \n", sfp, dev);
//    // get address offset of febex buffer 0,1 for each febex/exploder
  rev = fKinpex->ReadBus(REG_BUF0, base_dbuf0, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x (double buffer 0 address)\n", rev, sfp, sl, REG_BUF0);
    return -1;
  }
  DOUT1("FEBEX at sfp %d slave %d: Base address for Double Buffer 0  0x%lx  \n", sfp, sl, base_dbuf0);
  rev = fKinpex->ReadBus(REG_BUF1, base_dbuf1, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x (double buffer 0 address)\n", rev, sfp, sl, REG_BUF1);
    return -1;
  }
  DOUT1("FEBEX at sfp %d slave %d: Base address for Double Buffer 1  0x%lx  \n", sfp, sl, base_dbuf1);

//
//    // get nr. of channels per febex
  rev = fKinpex->ReadBus(REG_SUBMEM_NUM, num_submem, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x ((num submem)\n", rev, sfp, sl, REG_SUBMEM_NUM);
    return -1;
  }
  DOUT1("FEBEX at sfp %d slave %d: Number of Channels: 0x%lx  \n", sfp, sl, num_submem);

//    // get buffer per channel offset
  rev = fKinpex->ReadBus(REG_SUBMEM_OFF, submem_offset, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d in ReadBus: sfp:%d slave %x addr %x (submem_offset)\n", rev, sfp, sl, REG_SUBMEM_OFF);
    return -1;
  }
  DOUT1("FEBEX at sfp %d slave %d:  Offset of Channel to the Base address: 0x%lx  \n", sfp, sl, submem_offset);

//    // disable test data length
  rev = fKinpex->WriteBus(REG_DATA_LEN, 0x10000000, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX disabling test data length failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

//    // specify trace length in slices
  rev = fKinpex->WriteBus(REG_FEB_TRACE_LEN, fTracelen, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX write tracelength failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

  //
//    // specify trigger delay in slices
  rev = fKinpex->WriteBus(REG_FEB_TRIG_DELAY, fTrigDelay, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX write trigger delay  failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

//
//    // disable trigger acceptance in febex
  rev = fKinpex->WriteBus(REG_FEB_CTRL, 0, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX write REG_FEB_CTRL failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  rev = fKinpex->ReadBus(REG_FEB_CTRL, feb_ctrl, sfp, sl);
  if (rev || (feb_ctrl & 0x1) != 0)
  {
    EOUT("\n\nError  FEBEX slave(%d,%d) : disabling trigger acceptance in febex failed!\n", sfp, sl);
    return -5;
  }

//    // enable trigger acceptance in febex
  rev = fKinpex->WriteBus(REG_FEB_CTRL, 1, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX write REG_FEB_CTRL enable trigger acceptance failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  rev = fKinpex->ReadBus(REG_FEB_CTRL, feb_ctrl, sfp, sl);
  if (rev || (feb_ctrl & 0x1) != 1)
  {
    EOUT("\n\nError  FEBEX slave(%d,%d) : enabling trigger acceptance in febex failed!\n", sfp, sl);
    return -5;
  }

//    // set channels used for self trigger signal
  int ev_odd_or = FEBEX_EOR_ENAB_DEFAULT;
  size_t slave = sl;    // workaround for size warning
  if (fEvenOddOr[sfp].size() > slave)    // check if we got a configuration from file
  {
    ev_odd_or = fEvenOddOr[sfp].at(slave);
  }
  int polnegative = FEBEX_POL_NEG_DEFAULT;
  if (fPolarity[sfp].size() > slave)    // check if we got a configuration from file
  {
    polnegative = fPolarity[sfp].at(slave);
  }
  int triggermode = FEBEX_TRIG_MOD_DEFAULT;
  if (fTriggerMod[sfp].size() > slave)    // check if we got a configuration from file
  {
    triggermode = fTriggerMod[sfp].at(slave);
  }
  int enabletrigger = FEBEX_CHMASK_TRIG_DEFAULT;
  if (fEnableTrigger[sfp].size() > slave)    // check if we got a configuration from file
  {
    enabletrigger = fEnableTrigger[sfp].at(slave);
  }
  value = ((ev_odd_or << 21) | (polnegative << 20) | (triggermode << 16) | enabletrigger);
  rev = fKinpex->WriteBus(REG_FEB_SELF_TRIG, value, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX write REG_FEB_SELF_TRIG failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

//    // set the step size for self trigger and data reduction
  for (int k = 0; k < FEBEX_CH; k++)
  {
    int thres = FEBEX_THRESH_DEFAULT;
    size_t kanal = k;
    if (fThreshold[sfp][sl].size() > kanal)    // check if we got a configuration from file
    {
      thres = fThreshold[sfp][sl].at(kanal);
    }
    value = (k << 24) | thres;
    rev = fKinpex->WriteBus(REG_FEB_STEP_SIZE, value, sfp, sl);
    if (rev)
    {
      EOUT("\n\nError %d FEBEX write REG_FEB_STEP_SIZE failed for slave (%d,%d), channel:%d \n ", rev, sfp, sl, k);
      return rev;
    }
  }    // kanal

  fKinpex->WriteBus(REG_FEB_TIME, 0x0, sfp, sl);
  if (sfp == fClockSourceSFP && sl == fClockSourceSlave)
  {

    fKinpex->WriteBus(REG_FEB_TIME, 0x7, sfp, sl);
  }
  else
  {
    fKinpex->WriteBus(REG_FEB_TIME, 0x5, sfp, sl);
  }

//// enable/disable no hit in trace data suppression of channel
  int datareduction = FEBEX_CHMASK_SPARS_DEFAULT;
  if (fDataReduction[sfp].size() > slave)    // check if we got a configuration from file
  {
    datareduction = fDataReduction[sfp].at(slave);
  }
  rev = fKinpex->WriteBus(REG_DATA_REDUCTION, datareduction, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX write REG_DATA_REDUCTION failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

//    // set channels used for self trigger signal
  int disabledchannels = FEBEX_CHMASK_DISAB_DEFAULT;
  if (fDisableChannel[sfp].size() > slave)    // check if we got a configuration from file
  {
    disabledchannels = fDisableChannel[sfp].at(slave);
  }

  rev = fKinpex->WriteBus(REG_MEM_DISABLE, disabledchannels, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX write REG_MEM_DISABLE failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

//// write SFP id for channel header
  rev = fKinpex->WriteBus(REG_HEADER, sfp, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX write REG_HEADER failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

//    // set/checks fpga clocks with fixed code for adc channel 0 (adc 0)
//    //                                       and  adc channel 8 (adc 1)
// ??????
//    // set trapez parameters for trigger/hit finding
  rev = fKinpex->WriteBus(TRIG_SUM_A_REG, fTrigSumA, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX setting TRIG_SUM_A failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  rev = fKinpex->WriteBus(TRIG_GAP_REG, fTrigSumA + fTrigGap, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX setting TRIG_GAP failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

  rev = fKinpex->WriteBus(TRIG_SUM_B_REG, fTrigSumA + fTrigGap + fTrigSumB + 1 , sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX setting TRIG_SUM_B failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
//
//    // set trapez parameters for energy estimation
  rev = fKinpex->WriteBus(ENERGY_SUM_A_REG, fEnergySumA, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX setting ENERGY_SUM_A failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  rev = fKinpex->WriteBus(ENERGY_GAP_REG, fEnergySumA + fEnergyGap, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX setting ENERGY_GAP_REG failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  rev = fKinpex->WriteBus(ENERGY_SUM_B_REG, fEnergySumA + fEnergyGap + fEnergySumB + 1, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX setting ENERGY_SUM_B failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

//    // select clock source and enable trapezoidal energy filter
//    // check presence of external clock
  rev = fKinpex->ReadBus(REG_FEB_TIME, feb_time, sfp, sl);
  if (rev || (feb_time & 0x10) != 0x10)
  {
    EOUT("\n\nError  FEBEX slave(%d,%d) : external clock not present!\n", sfp, sl);
    return -6;
  }
  return rev;
}

