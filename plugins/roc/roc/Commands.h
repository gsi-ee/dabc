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

#ifndef ROC_COMMANDS_H
#define ROC_COMMANDS_H


#ifndef DABC_Command
#include "dabc/Command.h"
#endif

#ifndef DABC_Exception
#include "dabc/Exception.h"
#endif

#include <stdint.h>

namespace base {
   class OperList;
}

namespace roc {

   extern const char* AddrPar;
   extern const char* ValuePar;
   extern const char* ErrNoPar;
   extern const char* TmoutPar;
   extern const char* RawDataPar;
   extern const char* OperListPar;

   extern const char* xmlRawReadout;

   extern const char* xmlNumRocs;
   extern const char* xmlRocIp;
   extern const char* xmlRocFebs;
   extern const char* xmlRawFile;
   extern const char* xmlReadoutAppClass;

   extern const char* xmlNumMbs;
   extern const char* xmlMbsAddr;
   extern const char* xmlSyncSubeventId;

   extern const char* xmlNumSusibo;
   extern const char* xmlSusiboAddr;

   extern const char* xmlNumSlaves;
   extern const char* xmlSlaveAddr;

   extern const char* xmlNumTRBs;
   extern const char* xmlTRBAddr;

   extern const char* xmlUseDLM;
   extern const char* xmlMeasureADC;

   extern const char* xmlEpicsNode;


   class CmdPut : public dabc::Command {

      DABC_COMMAND(CmdPut, "CmdPut")

      CmdPut(uint32_t address, uint32_t value, double tmout = 5.) :
          dabc::Command(CmdName())
      {
         SetUInt(AddrPar, address);
         SetUInt(ValuePar, value);
         SetDouble(TmoutPar, tmout);
      }
   };

  class CmdGet : public dabc::Command {

     DABC_COMMAND(CmdGet, "CmdGet")

     CmdGet(uint32_t address, double tmout = 5.) :
         dabc::Command(CmdName())
     {
        SetUInt(AddrPar, address);
        SetDouble(TmoutPar, tmout);
     }
   };

  class CmdNOper : public dabc::Command {

     DABC_COMMAND(CmdNOper, "CmdNOper")

     CmdNOper(base::OperList* lst, double tmout = 5.) :
         dabc::Command(CmdName())
     {
        SetPtr(OperListPar, lst);
        SetDouble(TmoutPar, tmout);
     }
  };


  class CmdDLM : public dabc::Command {
     DABC_COMMAND(CmdDLM,"CmdDLM")

     CmdDLM(unsigned num, double tmout = 5.) :
         dabc::Command(CmdName())
     {
        SetUInt(AddrPar, num);
        SetDouble(TmoutPar, tmout);
     }
  };

  class CmdCalibration : public dabc::Command {

     DABC_COMMAND(CmdCalibration, "RocCalibration")

     static const char* FlagName() { return "CalibrFlag"; }

     CmdCalibration(bool on) :
        dabc::Command(CmdName())
     {
        SetBool(FlagName(), on);
     }
  };

  class CmdGlobalCalibration : public dabc::Command {

     DABC_COMMAND(CmdGlobalCalibration, "RocGlobalCalibration")

     static const char* FlagName() { return "CalibrFlag"; }

     CmdGlobalCalibration(bool on) :
        dabc::Command(CmdName())
     {
        SetBool(FlagName(), on);
     }
   };

  class CmdGetBoardPtr : public dabc::Command {

     DABC_COMMAND(CmdGetBoardPtr, "GetBoardPtr")

     static const char* Board() { return "Board"; }
   };

  class CmdReturnBoardPtr : public dabc::Command {

     DABC_COMMAND(CmdReturnBoardPtr, "ReturnBoardPtr")

     static const char* Board() { return "Board"; }
   };

  class CmdMessagesVector : public dabc::Command {

     DABC_COMMAND(CmdMessagesVector, "MessagesVector")

     static const char* Vector() { return "Vecotr"; }

     CmdMessagesVector(void* vect) :
        dabc::Command(CmdName())
     {
        SetPtr(Vector(), vect);
     }
   };

  class CmdAddSlave : public dabc::Command {

     DABC_COMMAND(CmdAddSlave, "AddSlave")

     static const char* Master() { return "Master"; }
     static const char* RocId() { return "RocId"; }

     CmdAddSlave(unsigned master, unsigned rocid) :
        dabc::Command(CmdName())
     {
        SetUInt(Master(), master);
        SetUInt(RocId(), rocid);
     }
  };

}

#endif

