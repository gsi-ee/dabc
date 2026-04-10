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
#include "pex/Febex4.h"
#include "pex/Device.h"
#include "pexor/PexorTwo.h"

#include "dabc/logging.h"
#include "dabc/string.h"
#include <unistd.h>
#include <string>

namespace pex
{
const char *xmlAdcSamplesSkipping = "SkippedADCSamples";
const char *xmlEnableEtfTrigger = "EnableEtfTrigger";
const char *xmlChannelEtfThreshold = "ChannelEtfThresholds_";
}


pex::Febex4::Febex4 (const std::string &name, dabc::Command cmd) :
    pex::Febex3::Febex3 (name, cmd)
{
  DOUT2("Created new pex::Febex4...\n");

  fType = FEB_FEBEX4;    // overwrite type of base class febex3

  // only new parameters: skip events, ivans new code thresholds
  fSkippedSamples = Cfg (pex::xmlAdcSamplesSkipping, cmd).AsInt (FEB4_ADC_SAMPLES_SKIPPING) & 0xFF;

#ifdef FEB4_IVANS_NEWCODE
  fUseEtfTrigger = Cfg (pex::xmlEnableEtfTrigger, cmd).AsBool (false);

  // more configuration bits from arrays:
  std::vector<long int> arr;
  for (int sfp = 0; sfp < PEX_NUMSFP; sfp++)
  {
    for (int slave = 0; slave < PEX_MAXSLAVES; slave++)
    {
      fEtfThreshold[sfp][slave].clear ();
      arr.clear ();
      arr = Cfg (dabc::format ("%s%d_%X", pex::xmlChannelEtfThreshold, sfp, slave), cmd).AsIntVect ();
      for (size_t i = 0; i < arr.size (); ++i)
      {
        fEtfThreshold[sfp][slave].push_back (arr[i]);
      }
    }    // slave
  }    // sfp
#endif
}

pex::Febex4::~Febex4 ()
{

}

int pex::Febex4::Disable (int sfp, int slave)
{
//  int rev = 0;
//  rev = fKinpex->WriteBus(DATA_FILT_CONTROL_REG, 0x00, sfp, slave);
//  usleep(4000);
//  if (rev)
//  {
//    EOUT("\n\nError %d FEBEX disable failed at sfp:%d,dev:%d\n ", rev, sfp, slave);
//    return rev;
//  }
  return pex::Febex3::Disable (sfp, slave);    // anything else here?
}

int pex::Febex4::Enable (int sfp, int slave)
{
  int rev = 0;
  rev=pex::Febex3::Enable (sfp, slave);    // common with febex3
  if(rev) return rev;
  rev = fKinpex->WriteBus(FEB4_GOSIP_DATA_WRITE, 0x7d000000, sfp, slave);
//  l_stat = f_pex_slave_wr (sfp, dev, 0x208010, 0x7d000000);
    usleep (1000);
return rev;
}

int pex::Febex4::Configure (int sfp, int sl)
{
  int rev = 0;
  int value = 0;
  // get submemory addresses to check setup:
  //unsigned long base_dbuf0 = 0, base_dbuf1 = 0;
  //unsigned long num_submem = 0, submem_offset = 0;
  unsigned long feb_spi = 0;


  // new for febex4 is kind of preconfiguration via spi:
  // SPI Speed:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x22000010, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_SPEED_REG failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  // Activating of SPI core with accss to both ADCs:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x2100008f, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_CTRL_REG failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  // Both ADCs in standby:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23000803, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_CTRL_REG failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);
  // Both ADCs in standby:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23000800, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_CTRL_REG failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  // Turns duty cycle stabilizer on:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23800901, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_ADC_Coding failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23001955, sfp, sl);
  usleep (1000);
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23001a55, sfp, sl);
  usleep (1000);
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23001b55, sfp, sl);
  usleep (1000);
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23001c55, sfp, sl);
  usleep (1000);

  // Write fixed code into ADC's:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, FEB4_ADC_FIX_CODE, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_ADC_Write fix adc code failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  // ADC's coding to binary output mode (write):
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23001400, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_ADC_Coding failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);
  // ADC's coding to binary output mode (read):
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23801400, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_ADC_Coding failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  rev = fKinpex->ReadBus (FEB4_GOSIP_DATA_READ, feb_spi, sfp, sl);
  if (rev || (feb_spi != FEB4_CODING))
  {
    EOUT("\n\nFebex4 adc coding is not set to offset binary %lx for slave (%d,%d), return value %d \n", feb_spi, sfp, sl, rev);
    return -1;
  }
  usleep (1000);

  // SPI access to ADC0:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x21000083, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_ADC 0_Coding failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  // For ADC0 - phase of DCO to data:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, FEB4_ADC0_CLK_PHASE, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_ADC 0 phase failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  // SPI access to ADC1:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x2100008c, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_ADC 1_Coding failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  // For ADC1 - phase of DCO to data:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, FEB4_ADC1_CLK_PHASE, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_ADC 1 phase failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  // SPI access to both ADCs:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x2100008f, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_ADC access failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  // Serial output data control - DDR two-lane, bitwise:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23002120, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_CTRL_REG failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  // Resolution/sample rate override - 14 bits / 105 MSPS (command 1):
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23010055, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_CTRL_REG  Resolution/sample rate override 1 failed for slave (%d,%d) \n ", rev,
        sfp, sl);
    return rev;
  }
  usleep (1000);

  // Resolution/sample rate override - 14 bits / 105 MSPS (command 2):
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x2300ff01, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_CTRL_REG  Resolution/sample rate override 2 failed for slave (%d,%d) \n ", rev,
        sfp, sl);
    return rev;
  }
  usleep (1000);

  // For both ADCs - chip run:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23000800, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_CTRL_REG chip run failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  // For ADCs channel output reset - only for LDVS outputs:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23002202, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_CTRL_REG channel output reset failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  //  For ADCs channel output reset - clean:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x23002200, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_CTRL_REG channel output reset clean failed for slave (%d,%d) \n ", rev, sfp,
        sl);
    return rev;
  }
  usleep (1000);

  //SPI de-activating:
  rev = fKinpex->WriteBus (FEB4_GOSIP_DATA_WRITE, 0x21000000, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write SPI_CTRL_REG de-activating failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }
  usleep (1000);

  ///////////////////////////////////////////////////////////////////////////////////////////////
  /// JAM 10-04-2026: here comes a config block mostly common with febex3. Use superclass method:
  rev = pex::Febex3::Configure (sfp, sl);
  if (rev)
    return rev;
  //////////////////////////////////////////////////////////////////////////////////////////////

  //additiona febex4 features below:

  // specify ADC' samples to skip:
  rev = fKinpex->WriteBus (FEB4_REG_ADC_SAMPLES_SKIPPING, fSkippedSamples & 0xff, sfp, sl);
  if (rev)
  {
    EOUT("\n\nError %d FEBEX4 write REG_ADC_SAMPLES_SKIPPING failed for slave (%d,%d) \n ", rev, sfp, sl);
    return rev;
  }

#ifdef FEB4_IVANS_NEWCODE
  if (fUseEtfTrigger)
  {
    //////////////////////////////////////////////
    //-- I. Rusanov - etf_thresh - 13.02.2025        --//
    //-- Using ETF Filter for self-triggering (Uwe!) --//
    //////////////////////////////////////////////

    for (int k = 0; k < FEBEX_CH; k++)
    {
      int thres = FEBEX_ETFTHRESH_DEFAULT;
      size_t kanal = k;
      if (fEtfThreshold[sfp][sl].size () > kanal)    // check if we got a configuration from file
      {
        thres = fEtfThreshold[sfp][sl].at (kanal);
      }
      value = (k << 24) | thres;
      rev = fKinpex->WriteBus (FEB4_TH_REG_FOR_ETF_TRIGGER, value, sfp, sl);
      if (rev)
      {
        EOUT("\n\nError %d FEBEX write FEB4_TH_REG_FOR_ETF_TRIGGER failed for slave (%d,%d), channel:%d \n ", rev, sfp,
            sl, k);
        return rev;
      }
    }    // kanal

  }

#endif

return rev;

}

