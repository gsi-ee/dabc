#ifndef ROC_RocCommands
#define ROC_RocCommands
#include "dabc/Command.h"


#define DABC_ROC_POOLNAME "RocPool"


#define DABC_ROC_COMMAND_WRITE_REGISTER "WriteRocRegister"
#define DABC_ROC_COMMAND_READ_REGISTER "ReadRocRegister"
#define DABC_ROC_COMMAND_START_DAQ "StartRocDaq"
#define DABC_ROC_COMMAND_STOP_DAQ "StopRocDaq"
#define DABC_ROC_COMMAND_START_PULSER "StartRocPulser"
#define DABC_ROC_COMMAND_STOP_PULSER "StopRocPulser"



#define DABC_ROC_COMPAR_BUFSIZE      "BufferSize"
#define DABC_ROC_COMPAR_QLENGTH      "QueueLength"
#define DABC_ROC_COMPAR_POOL_SIZE    "PoolBuffers"
#define DABC_ROC_COMPAR_ROCSNUMBER   "NumRocs"
#define DABC_ROC_COMPAR_NUMOUTPUTS   "NumOutputs"



#define DABC_ROC_COMPAR_BOARDIP      "BoardIP"
#define DABC_ROC_COMPAR_TRANSWINDOW  "TransportWindow"

#define DABC_ROC_COMPAR_BOARDNUMBER   "BoardIdNumber"

#define DABC_ROC_COMPAR_ADDRESS       "RegisterAddress"
#define DABC_ROC_COMPAR_VALUE         "RegisterValue"
#define DABC_ROC_COMPAR_PULSE_RESETDELAY "PulseResetDelay"
#define DABC_ROC_COMPAR_PULSE_LENGTH "PulseLength"
#define DABC_ROC_COMPAR_PULSE_NUMBER "PulseNumber"



namespace roc{


/** this command sets value to roc register*/   
  class CommandWriteRegister : public dabc::Command {
      public:   
         CommandWriteRegister(unsigned int boardNum, unsigned int address, unsigned int value) :
            dabc::Command(DABC_ROC_COMMAND_WRITE_REGISTER) 
            {
               SetInt(DABC_ROC_COMPAR_BOARDNUMBER, boardNum);
               SetInt(DABC_ROC_COMPAR_ADDRESS, address);
               SetInt(DABC_ROC_COMPAR_VALUE, value); 
            }
   };  

/** this command reads value from roc register. Command reply will contain result (??).*/   
  class CommandReadRegister : public dabc::Command {
      public:   
         CommandReadRegister(unsigned int boardNum, unsigned int address) :
            dabc::Command(DABC_ROC_COMMAND_READ_REGISTER) 
            {
               SetInt(DABC_ROC_COMPAR_BOARDNUMBER, boardNum);
               SetInt(DABC_ROC_COMPAR_ADDRESS, address);
            }
   };  


/** this command starts data transfer from ROC.*/   
  class CommandStartDAQ : public dabc::Command {
      public:   
         CommandStartDAQ(unsigned int boardNum) :
            dabc::Command(DABC_ROC_COMMAND_START_DAQ) 
            {
               SetInt(DABC_ROC_COMPAR_BOARDNUMBER, boardNum);
            }
   };  

/** this command stops data transfer from ROC.*/   
  class CommandStopDAQ : public dabc::Command {
      public:   
         CommandStopDAQ(unsigned int boardNum) :
            dabc::Command(DABC_ROC_COMMAND_STOP_DAQ) 
            {
               SetInt(DABC_ROC_COMPAR_BOARDNUMBER, boardNum);
            }
   };  


/** this command starts ROC test pulser.*/   
  class CommandStartPulser : public dabc::Command {
      public:   
         CommandStartPulser(unsigned int boardNum, unsigned int pulseResetDelay, unsigned int pulseLen, unsigned int pulseNum) :
            dabc::Command(DABC_ROC_COMMAND_START_PULSER) 
            {
               SetInt(DABC_ROC_COMPAR_BOARDNUMBER, boardNum);
               SetInt(DABC_ROC_COMPAR_PULSE_RESETDELAY, pulseResetDelay);
               SetInt(DABC_ROC_COMPAR_PULSE_LENGTH, pulseLen);
               SetInt(DABC_ROC_COMPAR_PULSE_NUMBER, pulseNum);
            }
   };  



/** this command stops test pulser.*/   
  class CommandStopPulser : public dabc::Command {
      public:   
         CommandStopPulser(unsigned int boardNum) :
            dabc::Command(DABC_ROC_COMMAND_STOP_PULSER) 
            {
               SetInt(DABC_ROC_COMPAR_BOARDNUMBER, boardNum);
            }
   };  


} // end namspace roc


#endif

