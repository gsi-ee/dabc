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
#include "pex/Device.h"
#include "pexor/PexorTwo.h"

#include "dabc/logging.h"

#include <vector>

namespace pex
{
const char *xmlQfwMode = "QFWMode";
const char *xmlDACMode = "DACMode";
const char *xmlDACAllValue = "DACConstantValue";
const char *xmlPolandTriggerMasterSFP = "TriggerMasterSFP";
const char *xmlPolandTriggerMasterSlave = "TriggerMasterSlave";
const char *xmlQFWSteps = "QFWSteps";
const char *xmlQFWTimes = "QFWTimes";

}


pex::Poland::Poland (const std::string &name, dabc::Command cmd) :
pex::FrontendBoard::FrontendBoard(name,  FEB_POLAND, cmd),fQfwMode(pex::QFWMODE_PLUS_2_5pF_0_25pC), fDACMode(pex::DACMODE_TESTSTRUCTURE),fTriggerMasterSFP(0), fTriggerMasterSlave(0), fDACallvalue(0)
{
  DOUT2 ("Created new pex::Poland\n");

  for(int loop=0; loop<POLAND_QFWLOOPS; ++loop)
  {
    fQfwSteps[loop]=10;
    fQfwTimes[loop]=100.0 ;// museconds
  }

  // more configuration bits from arrays:
   std::vector<long int> arr;
   arr.clear();
   arr = Cfg(pex::xmlQFWSteps, cmd).AsIntVect();
   for (size_t i = 0; i < arr.size(); ++i)
   {
     if(i>=POLAND_QFWLOOPS) break;
     fQfwSteps[i]=arr[i];
   }
     arr.clear();
    arr = Cfg(pex::xmlQFWTimes, cmd).AsIntVect();

    for (size_t i = 0; i < arr.size(); ++i)
      {
        if(i>=POLAND_QFWLOOPS) break;
        fQfwTimes[i]=arr[i];
      }


  fQfwMode =(poland_qfwmode_t)  Cfg(pex::xmlQfwMode, cmd).AsInt(pex::QFWMODE_PLUS_2_5pF_0_25pC);
  fDACMode = (poland_dacmode_t) Cfg(pex::xmlDACMode, cmd).AsInt(pex::DACMODE_TESTSTRUCTURE);
  fTriggerMasterSFP= Cfg(pex::xmlPolandTriggerMasterSFP, cmd).AsInt(0);
  fTriggerMasterSlave= Cfg(pex::xmlPolandTriggerMasterSlave, cmd).AsInt(0);
  fDACallvalue= Cfg(pex::xmlDACAllValue, cmd).AsInt(0);

}

pex::Poland::~Poland()
{

}

int pex::Poland::Disable(int , int ) {

	return 0;
}

int pex::Poland::Enable(int sfp , int slave) {
    if(sfp==0 && slave==0) sleep(2); // impose wait time once for all chains after configure
	return 0;
}

int pex::Poland::Configure(int sfp, int sl) {


  // get submemory addresses to check setup:
       unsigned long base_dbuf0 = 0, base_dbuf1 = 0;
       unsigned long num_submem = 0, submem_offset = 0;
       unsigned long qfw_ctrl = 0;

       int rev = fKinpex->ReadBus (REG_BUF0, base_dbuf0, sfp, sl);
       if (rev == 0)
       {
         DOUT1("Slave %x: Base address for Double Buffer 0  0x%lx  \n", sl, base_dbuf0);
       }
       else
       {
         EOUT("\n\ntoken Error %d in ReadBus: slave %x addr %x (double buffer 0 address)\n", rev, sl, REG_BUF0);
         return -1;
       }
       rev = fKinpex->ReadBus (REG_BUF1, base_dbuf1, sfp, sl);
       if (rev == 0)
       {
         DOUT1("Slave %x: Base address for Double Buffer 1  0x%lx  \n", sl, base_dbuf1);
       }
       else
       {
         EOUT("\n\ntoken Error %d in ReadBus: slave %x addr %x (double buffer 1 address)\n", rev, sl, REG_BUF1);
         return -1;
       }
       rev = fKinpex->ReadBus (REG_SUBMEM_NUM, num_submem, sfp, sl);
       if (rev == 0)
       {
         DOUT1("Slave %x: Number of Channels  0x%lx  \n", sl, num_submem);
       }
       else
       {
         EOUT("\n\ntoken Error %d in ReadBus: slave %x addr %x (num submem)\n", rev, sl, REG_SUBMEM_NUM);
         return -1;
       }
       rev = fKinpex->ReadBus (REG_SUBMEM_OFF, submem_offset, sfp, sl);
       if (rev == 0)
       {
         DOUT1("Slave %x: Offset of Channel to the Base address  0x%lx  \n", sl, submem_offset);
       }
       else
       {
         EOUT("\n\ncheck_token Error %d in ReadBus: slave %x addr %x (submem offset)\n", rev, sl, REG_SUBMEM_OFF);
         return -1;
       }

       // disable test data length:
       rev = fKinpex->WriteBus (REG_DATA_LEN, 0x10000000, sfp, sl);
       if (rev)
       {
         EOUT("\n\nError %d in WriteBus disabling test data length !\n", rev);
         return -1;
       }

       // disable trigger acceptance in exploder2asleep(2);

       rev = fKinpex->WriteBus (POLAND_REG_RESET, 0, sfp, sl);
       if (rev)
       {
         EOUT("\n\nError %d in WriteBus disabling  trigger acceptance (sfp:%d, device:%d)!\n", rev, sfp, sl);
         return -1;
       }
       rev = fKinpex->ReadBus (POLAND_REG_RESET, qfw_ctrl, sfp, sl);
       if (rev == 0)
       {
         if ((qfw_ctrl & 0x1) != 0)
         {
           EOUT("Disabling trigger acceptance in qfw failed for sfp:%d device:%d!!!!!\n", sfp, sl);
           return -1;
         }
       }
       else
       {
         EOUT("\n\nError %d in ReadBus: disabling  trigger acceptance (sfp:%d, device:%d)\n", rev, sfp, sl);
         return -1;
       }

       // enable trigger acceptance in exploder2a:
       rev = fKinpex->WriteBus (POLAND_REG_RESET, 1, sfp, sl);
       if (rev)
       {
         EOUT("\n\nError %d in WriteBus enabling  trigger acceptance (sfp:%d, device:%d)!\n", rev, sfp, sl);
         return -1;
       }
       rev = fKinpex->ReadBus (POLAND_REG_RESET, qfw_ctrl, sfp, sl);
       if (rev == 0)
       {
         if ((qfw_ctrl & 0x1) != 1)
         {
           //       WriteGosip (fSFP, fSlave, POLAND_REG_QFW_PRG, 1);
                 //       WriteGosip (fSFP, fSlave, POLAND_REG_QFW_PRG, 0);
      EOUT("Enabling trigger acceptance in qfw failed for sfp:%d device:%d!!!!!\n", sfp, sl);
           return -1;
         }
         DOUT1("Trigger acceptance is enabled for sfp:%d device:%d  \n", sfp, sl);
       }
       else
       {
         EOUT("\n\nError %d in ReadBus: enabling  trigger acceptance (sfp:%d, device:%d)\n", rev, sfp, sl);
         return -1;
       }

       // write SFP id for channel header:
       rev = fKinpex->WriteBus (REG_HEADER, sfp, sfp, sl);
       if (rev)
       {
         EOUT("\n\nError %d in WriteBus writing channel header (sfp:%d, device:%d)!\n", rev, sfp, sl);
         return -1;
       }

       // enable frontend logic:
       // ? do we need setting to 0 before as in poland scripts?
       rev = fKinpex->WriteBus (REG_RST, 1, sfp, sl);
       if (rev)
       {
         EOUT("\n\nError %d in WriteBus resetting qfw (sfp:%d, device:%d)!\n", rev, sfp, sl);
         return -1;
       }


// TODO: poland specific setup here

       if(sfp==fTriggerMasterSFP && sl==fTriggerMasterSlave)
       {
         rev = fKinpex->WriteBus (POLAND_REG_MASTERMODE, 1, sfp, sl);
         if (rev)
         {
           EOUT("\n\nError %d in WriteBus setting master trigger mode (sfp:%d, device:%d)!\n", rev, sfp, sl);
           return -1;
         }
//         if(fInternalTrigger) // TODO
//         {
//         //rev = fKinpex->WriteBus (POLAND_REG_INTERNAL_TRIGGER, 1, sfp, sl);
//         }

       }



       rev = fKinpex->WriteBus (POLAND_REG_QFW_MODE, fQfwMode, sfp, sl); // TODO: check validity of mode here
        if (rev)
        {
          EOUT("\n\nError %d in WriteBus setting QFW sampling mode %d (sfp:%d, device:%d)!\n", fQfwMode, rev, sfp, sl);
          return -1;
        }
        // following is required to really activate qfw mode (thanks Sven Loechner for fixing):
        rev = fKinpex->WriteBus (POLAND_REG_QFW_PRG, 1, sfp, sl);
        rev = fKinpex->WriteBus (POLAND_REG_QFW_PRG, 0, sfp, sl);
        if (rev)
        {
          EOUT("\n\nError %d in WriteBus programming QFW sampling mode %d (sfp:%d, device:%d)!\n", fQfwMode, rev, sfp, sl);
          return -1;
        }

        for (int i = 0; i < POLAND_QFWLOOPS; ++i)
        {
          rev = fKinpex->WriteBus (POLAND_REG_STEPS_BASE+ 4 * i, fQfwSteps[i], sfp, sl);

          int timebins=fQfwTimes[i]/POLAND_TIME_UNIT;
          rev = fKinpex->WriteBus (POLAND_REG_TIME_BASE+ 4 * i, timebins, sfp, sl);
          if (rev)
          {
            EOUT("\n\nError %d in WriteBus writing QFW loop partition and time (sfp:%d, device:%d)!\n", rev, sfp, sl);
            return -1;
          }
        } // for POLAND_QFWLOOPS

//        if(fDoResetQFW)
       //       {
       //         WriteGosip (fSFP, fSlave, POLAND_REG_QFW_RESET, 0);
       //         WriteGosip (fSFP, fSlave, POLAND_REG_QFW_RESET, 1);
       //         //std::cout << "PolandGui::SetRegisters did reset QFW for ("<<fSFP<<", "<<fSlave<<")"<< std::endl;
       //       }
       //




        //DAC setup here:
        fKinpex->WriteBus (POLAND_REG_DAC_MODE, fDACMode, sfp, sl);
        switch(fDACMode)
               {

                 case pex::DACMODE_MANUAL:
                   // manual settings:
//                   for (int d = 0; d < POLAND_DAC_NUM; ++d)
//                   {
//                     WriteGosip (fSFP, fSlave, POLAND_REG_DAC_BASE_WRITE + 4 * d, theSetup->fDACValue[d]);
//                   }
                   EOUT("\nPoland DAC mode manual (%d) not implemented yet for sfp:%d, slave:%d!\n", fDACMode, sfp, sl);
                   break;
                 case pex::DACMODE_TESTSTRUCTURE:
                   // test structure:
                   // no more actions needed
                   break;
                 case pex::DACMODE_CALIBRATION:
                   // issue calibration:
//                   WriteGosip (fSFP, fSlave, POLAND_REG_DAC_CAL_STARTVAL , theSetup->fDACStartValue);
//                   WriteGosip (fSFP, fSlave, POLAND_REG_DAC_CAL_OFFSET ,  theSetup->fDACOffset);
//                   WriteGosip (fSFP, fSlave, POLAND_REG_DAC_CAL_DELTA ,  theSetup->fDACDelta);
//                   WriteGosip (fSFP, fSlave, POLAND_REG_DAC_CAL_TIME ,  theSetup->fDACCalibTime);

                   EOUT("\n Poland DAC mode calibration (%d) not implemented yet for sfp:%d, slave:%d!\n", fDACMode, sfp, sl);
                   break;
                 case pex::DACMODE_CONSTANT:
                   // all same constant value mode:
                   fKinpex->WriteBus (POLAND_REG_DAC_ALLVAL, fDACallvalue, sfp, sl);

                   break;

                 case pex::DACMODE_NONE:
                 default:
                   EOUT("\n\nError - Never come here - undefined DAC mode %d for sfp:%d, slave:%d!\n", fDACMode, sfp, sl);
                   break;

               };
        fKinpex->WriteBus (POLAND_REG_DAC_PROGRAM, 1, sfp, sl);
        fKinpex->WriteBus (POLAND_REG_DAC_PROGRAM, 0, sfp, sl);


	return 0;
}




