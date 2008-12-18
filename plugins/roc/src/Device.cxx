#include "roc/Device.h"
#include "roc/Transport.h"
#include "roc/Commands.h"
#include "dabc/Port.h"
#include "dabc/Manager.h"
#include "dabc/Module.h"
#include "dabc/MemoryPool.h"

#include <exception>
#include <stdexcept>

const char* roc::xmlNumRocs         = "NumRocs";
const char* roc::xmlRocPool         = "RocPool";
const char* roc::xmlTransportWindow = "TransportWindow";
const char* roc::xmlBoardIP         = "BoardIP";

roc::Device::Device(Basic* parent, const char* name) :
   dabc::Device(parent, name),
   SysCoreControl()
{
   DOUT3(("Constructing ROC device %s", name));

   dabc::Manager::Instance()->MakeThreadFor(this, "RocDeviceThrd");
}

roc::Device::~Device()
{
   DOUT3(("Destroy ROC device %s", GetName()));
   DoDeviceCleanup(true);
}

int roc::Device::AddBoard(const char* address, unsigned ctlPort)
{
   DOUT1(("roc::Device::AddBoard %s",  address));

   return SysCoreControl::addBoard(address,ctlPort);
}

bool roc::Device::DoDeviceCleanup(bool full)
{
   DOUT1(("DoDeviceCleanup: %s class:%s  full = %s", GetName(), ClassName(), DBOOL(full)));
   // remove boards from controller here:
   // NOTE: this should be internal method of controller, TODO!
   // need to empty the socket data here before removing the board instance?

   bool res = dabc::Device::DoDeviceCleanup(full);

   if (full) SysCoreControl::deleteBoards(true);

   return res;
}


int roc::Device::ExecuteCommand(dabc::Command* cmd)
{
   int cmd_res = cmd_false;

   unsigned rocid = cmd->GetUInt(DABC_ROC_COMPAR_BOARDNUMBER, 0);

   if (cmd->IsName(DABC_ROC_COMMAND_START_DAQ)) {
      // todo: check acknowledge value and set cmd_res accordingly
      if (GetBoard(rocid))
         cmd_res = cmd_bool(GetBoard(rocid)->startDaq());
      DOUT1(("Command %s  starts data taking from ROC %u res:%d", DABC_ROC_COMMAND_START_DAQ, rocid, cmd_res));
   } else
   if (cmd->IsName(DABC_ROC_COMMAND_STOP_DAQ)) {
      if (GetBoard(rocid))
         cmd_res = cmd_bool(GetBoard(rocid)->stopDaq());
      DOUT1(("Command %s  stops data taking from ROC %u, res:%d",DABC_ROC_COMMAND_STOP_DAQ, rocid, cmd_res));
   } else
   if (cmd->IsName(DABC_ROC_COMMAND_WRITE_REGISTER)) {
      unsigned address = cmd->GetUInt(DABC_ROC_COMPAR_ADDRESS,0);
      unsigned value = cmd->GetUInt(DABC_ROC_COMPAR_VALUE, 0);
      int messack = 6;
      if (GetBoard(rocid))
         messack = GetBoard(rocid)->poke(address,value);
      cmd_res = cmd_bool((messack==0) || (messack==2));
      DOUT1(("Command %s  writes value:%d to register:%d of ROC %d, result=%d",
             DABC_ROC_COMMAND_WRITE_REGISTER,value,address, rocid, messack));
   } else
   if (cmd->IsName(DABC_ROC_COMMAND_READ_REGISTER)) {
      unsigned address = cmd->GetUInt(DABC_ROC_COMPAR_ADDRESS,0);
      unsigned value = 0;
      int messack = 6;
      if (GetBoard(rocid))
         messack = GetBoard(rocid)->peek(address,value);
      cmd_res = cmd_bool(messack==0);
      DOUT1(("Command %s  reads value:%u from register:%u of ROC %u, result=%d",DABC_ROC_COMMAND_READ_REGISTER,value,address, rocid, messack));
      // how to transfer result back to command user?
   } else
   if (cmd->IsName(DABC_ROC_COMMAND_START_PULSER)) {
      unsigned resetdelay = cmd->GetUInt(DABC_ROC_COMPAR_PULSE_RESETDELAY,100);
      unsigned plength = cmd->GetUInt(DABC_ROC_COMPAR_PULSE_LENGTH,500);
      unsigned pnumber = cmd->GetUInt(DABC_ROC_COMPAR_PULSE_NUMBER,10);
      int messack = 6;
      if (GetBoard(rocid)) {
         messack = GetBoard(rocid)->poke(ROC_TESTPULSE_RESET_DELAY,resetdelay);
         messack += GetBoard(rocid)->poke(ROC_TESTPULSE_LENGTH, plength);
         messack += GetBoard(rocid)->poke(ROC_TESTPULSE_NUMBER, pnumber);
         messack += GetBoard(rocid)->poke(ROC_TESTPULSE_START, 1);
      }
      cmd_res = cmd_bool(messack==0);
      DOUT1(("Command %s  started pulser of ROC %u with delay:%u, pulselength:%u, pulsenumber:%u",DABC_ROC_COMMAND_START_PULSER, rocid, resetdelay, plength, pnumber));
   } else
   if (cmd->IsName(DABC_ROC_COMMAND_STOP_PULSER)) {
      int messack = 6;
      if (GetBoard(rocid))
         messack =  GetBoard(rocid)->poke(ROC_TESTPULSE_RESET,1);
      cmd_res = cmd_bool(messack==0);
      DOUT1(("Command %s  resets pulser of ROC %u",DABC_ROC_COMMAND_STOP_PULSER, rocid));
   } else
      cmd_res = dabc::Device::ExecuteCommand(cmd);

   return cmd_res;
}

SysCoreBoard* roc::Device::GetBoard(unsigned id)
{
   if (!SysCoreControl::isValidBoardNum(id)) {
      EOUT(("ROC id:%u is not existing!", id));
      return 0;
   }

   return &SysCoreControl::board(id);
}


int roc::Device::CreateTransport(dabc::Command* cmd, dabc::Port* port)
{
   if (port==0) {
      EOUT(("roc::Device::CreateTransport FAILED, port %s not specified!"));
      return cmd_false;
   }
   // take these from command parameters here:
   std::string ip = port->GetCfgStr(xmlBoardIP, "localhost", cmd);

   int TransWindow = port->GetCfgInt(xmlTransportWindow, 30, cmd);

   int bufsize = port->GetCfgInt(dabc::xmlBufferSize, 2048, cmd);

   Transport* transport = new Transport(this, port, bufsize, ip.c_str(), TransWindow);
   DOUT1(("Device::CreateTransport ip %s win:%d buf:%d ", ip.c_str(), TransWindow, bufsize));

   transport->AssignProcessorToThread(ProcessorThread());

   return cmd_true;
}

void roc::Device::DataCallBack(SysCoreBoard* brd)
{
   Transport* tr = 0;

   {
      dabc::LockGuard guard(fDeviceMutex);
      for (unsigned n=0; n<fTransports.size(); n++) {
         tr = (Transport*) fTransports[n];
         if ((tr!=0) && (tr->GetBoard() == brd)) break;
         tr = 0;
      }
   }

//   DOUT1(("DataCallBack tr = %p", tr));

   if (tr) tr->ComplteteBufferReading();
}

