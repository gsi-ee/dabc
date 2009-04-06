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

#include "roc/UdpDevice.h"

#include <iostream>
#include <fstream>

#include "dabc/Manager.h"

#include "roc/Defines.h"

#define SC_BITFILE_BUFFER_SIZE 4194304

roc::UdpControlSocket::UdpControlSocket(UdpDevice* dev, int fd) :
   dabc::SocketIOProcessor(fd),
   fDev(dev)
{
   // we will react on all input packets
   SetDoingInput(true);
}

roc::UdpControlSocket::~UdpControlSocket()
{
   if (fDev) fDev->fCtrl = 0;
   fDev = 0;
}

void roc::UdpControlSocket::ProcessEvent(dabc::EventId evnt)
{
   switch (dabc::GetEventCode(evnt)) {
      case evntSendCtrl:
         DOUT0(("Send packet of size %u", fDev->controlSendSize));
         StartSend(&(fDev->controlSend), fDev->controlSendSize);
         break;
      case evntSocketRead: {
         ssize_t len = DoRecvBuffer(&fRecvBuf, sizeof(fRecvBuf));
         if ((len>0) && fDev)
            fDev->processCtrlMessage(&fRecvBuf, len);
         break;
      }
      default:
         dabc::SocketIOProcessor::ProcessEvent(evnt);
   }
}

// __________________________________________________________


roc::UdpDevice::UdpDevice(dabc::Basic* parent, const char* name, const char* thrdname, dabc::Command* cmd) :
   dabc::Device(parent, name),
   roc::UdpBoard(),
   fCtrl(0),
   fCond(),
   ctrlState_(ctrlReady),
   controlSendSize(0),
   isBrdStat(false),
   displayConsoleOutput_(false)
{
   fConnected = false;

   std::string remhost = GetCfgStr(roc::xmlBoardIP, "", cmd);
   if (remhost.length()==0) return;

   if (!dabc::mgr()->MakeThreadFor(this, thrdname)) return;

   int nport = 8725;

   int fd = dabc::SocketThread::StartUdp(nport, nport, nport+1000);

   DOUT0(("Socket %d port %d  remote %s", fd, nport, remhost.c_str()));

   fd = dabc::SocketThread::ConnectUdp(fd, remhost.c_str(), ROC_DEFAULT_CTRLPORT);

   if (fd<=0) return;

   fCtrl = new UdpControlSocket(this, fd);

   fCtrl->AssignProcessorToThread(ProcessorThread());

   DOUT1(("Create control socket for port %d connected to host %s", nport, remhost.c_str()));

   fConnected = true;
}

roc::UdpDevice::~UdpDevice()
{
   if (fCtrl) {
      fCtrl->fDev = 0;
      fCtrl->DestroyProcessor();
   }
   fCtrl = 0;
}

int roc::UdpDevice::ExecuteCommand(dabc::Command* cmd)
{
   if (cmd->IsName("GetBoardPtr")) {

      roc::Board* brd = static_cast<roc::Board*> (this);
      cmd->SetStr("BoardPtr", dabc::format("%p", brd));
      return cmd_true;
   }

   return dabc::Device::ExecuteCommand(cmd);
}


bool roc::UdpDevice::initialise(BoardRole role)
{
   DOUT0(("Starting initialize"));

   if ((role==roleMaster) || (role == roleDAQ)) {
      if (!poke(ROC_MASTER_LOGIN, 0)) return false;
   }

   fRocNumber = peek(ROC_NUMBER);
   if (errno()!=0) return false;

   uint32_t sw_ver = peek(ROC_SOFTWARE_VERSION);
   if (errno()!=0) return false;

   if((sw_ver >= 0x01070000) || (sw_ver < 0x01060000)) {
      EOUT(("The ROC you want to access has software version %x \n", sw_ver));
      EOUT(("This C++ access class only supports boards with major version 1.6 == %x\n", 0x01060000));
   }
   DOUT0(("ROC software version is: 0x%x\n", sw_ver));

   uint32_t hw_ver = peek(ROC_HARDWARE_VERSION);
   if (errno()!=0) return false;

   if((hw_ver >= 0x01070000) || (hw_ver < 0x01060000)) {
      EOUT(("The ROC you want to access has hardware version %x \n", hw_ver));
      EOUT(("Please update your hardware to major version 1.6 == %x\n", 0x01060000));
   }

   DOUT0(("ROC hardware version is: 0x%x\n", hw_ver));

   return true;
}


int roc::UdpDevice::CreateTransport(dabc::Command* cmd, dabc::Port* port)
{
   return dabc::Device::CreateTransport(cmd, port);
}

bool roc::UdpDevice::poke(uint32_t addr, uint32_t value, double tmout)
{
   DOUT0(("Starting poke"));

   fErrNo = 0;

   bool show_progress = false;

   // define operations, which takes longer time as usual one
   switch (addr) {
      case ROC_CFG_WRITE:
      case ROC_CFG_READ:
      case ROC_OVERWRITE_SD_FILE:
      case ROC_DO_AUTO_DELAY:
      case ROC_DO_AUTO_LATENCY:
      case ROC_FLASH_KIBFILE_FROM_DDR:
         if (tmout < 10) tmout = 10.;
         show_progress = true;
         break;
   }

   controlSend.tag = ROC_POKE;
   controlSend.address = htonl(addr);
   controlSend.value = htonl(value);
   controlSendSize = sizeof(UdpMessage);

   fErrNo = 6;

   if (performCtrlLoop(tmout, show_progress))
      fErrNo = ntohl(controlRecv.value);

   DOUT0(("Roc:%s Poke(0x%04x, 0x%04x) res = %d\n", GetName(), addr, value, fErrNo));

   return fErrNo == 0;
}

uint32_t roc::UdpDevice::peek(uint32_t addr, double tmout)
{
   DOUT0(("Starting peek"));

   controlSend.tag = ROC_PEEK;
   controlSend.address = htonl(addr);
   controlSendSize = sizeof(UdpMessage);

   fErrNo = 6;

   uint32_t res = 0;

   if (performCtrlLoop(tmout, false)) {
      res = ntohl(controlRecv.value);
      fErrNo = ntohl(controlRecv.address);
   }

   DOUT0(("Roc:%s Peek(0x%04x, 0x%04x) res = %d\n", GetName(), addr, res, fErrNo));

   return res;
}

bool roc::UdpDevice::performCtrlLoop(double total_tmout_sec, bool show_progress)
{
   // normal reaction time of the ROC is 0.5 ms, therefore one can resubmit command
   // as soon as several ms without no reply
   // At the same time there are commands which takes seconds (like SD card write)
   // Therefore, one should try again and again if command is done

   if (IsExecutionThread()) {
      EOUT(("Cannot perform control from our own thread !!!"));
      return false;
   }

   {
      // before we start sending, indicate that we now in control loop
      // and can accept replies from ROC
      dabc::LockGuard guard(fCond.CondMutex());
      if (fCtrl==0) return false;
      if (ctrlState_!=ctrlReady) {
         EOUT(("cannot start operation - somebody else uses control loop"));
         return false;
      }

      ctrlState_ = ctrlWaitReply;
   }

   controlSend.password = htonl(ROC_PASSWORD);
   controlSend.id = htonl(currentMessagePacketId++);

   bool res = false;

   // send aligned to 4 bytes packet to the ROC

   while ((controlSendSize < MAX_UDP_PAYLOAD) &&
          (controlSendSize + UDP_PAYLOAD_OFFSET) % 4) controlSendSize++;

   if (total_tmout_sec>20.) show_progress = true;

   // if fast mode we will try to resend as fast as possible
   bool fast_mode = (total_tmout_sec < 10.) && !show_progress;
   int loopcnt = 0;
   bool wasprogressout = false;
   bool doresend = true;

   do {
      if (doresend) {
         if (fCtrl==0) break;
         fCtrl->FireEvent(roc::UdpControlSocket::evntSendCtrl);
         doresend = false;
      }

      double wait_tm = fast_mode ? (loopcnt++ < 4 ? 0.01 : loopcnt*0.1) : 1.;

      dabc::LockGuard guard(fCond.CondMutex());

      fCond._DoWait(wait_tm);

      // resend packet in fast mode always, in slow mode only in the end of interval
      doresend = fast_mode ? true : total_tmout_sec <=5.;

      total_tmout_sec -= wait_tm;

      if (ctrlState_ == ctrlGotReply)
         res = true;
      else
      if (show_progress) {
          std::cout << ".";
          std::cout.flush();
          wasprogressout = true;
      }
   } while (!res && (total_tmout_sec>0.));

   if (wasprogressout) std::cout << std::endl;

   dabc::LockGuard guard(fCond.CondMutex());
   ctrlState_ = ctrlReady;
   return res;
}

void roc::UdpDevice::processCtrlMessage(UdpMessageFull* pkt, unsigned len)
{
   // procees PEEK or POKE reply messages from ROC

   if(pkt->tag == ROC_CONSOLE) {
      pkt->address = ntohl(pkt->address);

      switch (pkt->address) {
         case ROC_STATBLOCK:
            setBoardStat(pkt->rawdata, displayConsoleOutput_);
            break;

         case ROC_DEBUGMSG:
            if (displayConsoleOutput_)
               DOUT0(("\033[0;%dm Roc:%d %s \033[0m", 36, 0, (const char*) pkt->rawdata));
            break;
         default:
            if (displayConsoleOutput_)
               DOUT0(("Error addr 0x%04x in cosle message\n", pkt->address));
      }

      return;
   }

   dabc::LockGuard guard(fCond.CondMutex());

   // check first that user waits for reply
   if (ctrlState_ != ctrlWaitReply) return;

   // if packed id is not the same, do not react
   if(controlSend.id != pkt->id) return;

   // if packed tag is not the same, do not react
   if(controlSend.tag != pkt->tag) return;

   memcpy(&controlRecv, pkt, len);

   ctrlState_ = ctrlGotReply;
   fCond._DoFire();
}

void roc::UdpDevice::setBoardStat(void* rawdata, bool print)
{
   UdpStatistic* src = (UdpStatistic*) rawdata;

   if (src!=0) {
      brdStat.dataRate = ntohl(src->dataRate);
      brdStat.sendRate = ntohl(src->sendRate);
      brdStat.recvRate = ntohl(src->recvRate);
      brdStat.nopRate = ntohl(src->nopRate);
      brdStat.frameRate = ntohl(src->frameRate);
      brdStat.takePerf = ntohl(src->takePerf);
      brdStat.dispPerf = ntohl(src->dispPerf);
      brdStat.sendPerf = ntohl(src->sendPerf);
      isBrdStat = true;
   }

   if (print && isBrdStat)
      DOUT0(("\033[0;%dm Roc:%u  Data:%6.3f MB/s Send:%6.3f MB/s Recv:%6.3f MB/s NOPs:%2u Frames:%2u Data:%4.1f%s Disp:%4.1f%s Send:%4.1f%s \033[0m",
         36, 0, brdStat.dataRate*1e-6, brdStat.sendRate*1e-6, brdStat.recvRate*1e-6,
             brdStat.nopRate, brdStat.frameRate,
             brdStat.takePerf*1e-3,"%", brdStat.dispPerf*1e-3,"%", brdStat.sendPerf*1e-3,"%"));
}

int roc::UdpDevice::pokeRawData(uint32_t address, const void* rawdata, uint32_t rawdatelen, double tmout)
{
   controlSend.tag = ROC_POKE;
   controlSend.address = htonl(address);
   controlSend.value = htonl(rawdatelen);
   memcpy(controlSend.rawdata, rawdata, rawdatelen);
   controlSendSize = sizeof(UdpMessage) + rawdatelen;

   int res = 6;

   if (performCtrlLoop(tmout, tmout > 3.)) res = ntohl(controlRecv.value);

   DOUT1(("Roc:%s PokeRaw(0x%04x, 0x%04x) res = %d\n", GetName(), address, rawdatelen, res));

   return res;
}

bool roc::UdpDevice::sendConsoleCommand(const char* cmd)
{
   unsigned length = cmd ? strlen(cmd) + 1 : 0;

   if ((length<2) || (length > sizeof(controlSend.rawdata))) return false;

   return pokeRawData(ROC_CONSOLE_CMD, cmd, length) == 1;
}

bool roc::UdpDevice::saveConfig(const char* filename)
{
   uint32_t len = filename ? strlen(filename) + 1 : 0;

   DOUT1(("Save config file %s on ROC\n",filename ? filename : ""));

   return pokeRawData(ROC_CFG_WRITE, filename, len, 10.) == 0;
}

bool roc::UdpDevice::loadConfig(const char* filename)
{
   uint32_t len = filename ? strlen(filename) + 1 : 0;

   DOUT1(("Load config file %s on ROC\n",filename ? filename : ""));

   return pokeRawData(ROC_CFG_READ, filename, len, 10.) == 0;
}


int roc::UdpDevice::parseBitfileHeader(char* pBuffer, unsigned int nLen)
{
   char* binBuffer;

   if (nLen < 16) {
      EOUT(("Invalid filesize\n"));
      return -1;
   }

   if (*((unsigned short*) pBuffer) == 0x0900 && *((unsigned int*) (pBuffer + 2)) == 0xF00FF00F && *((unsigned int*) (pBuffer + 6)) == 0xF00FF00F) {
      DOUT1(("Bitfile found, skipping header...\n"));
      if (*((unsigned int*) (pBuffer + 10)) != 0x61010000)
      {
         EOUT(("Corrupt file\n"));
         return -1;
      }

      unsigned int tmpLen = ntohs(*((unsigned short*) (pBuffer + 14)));
      unsigned int tmpPos = 13;

      for (int i = 0;i < 4;i++)
      {
         if (nLen < tmpPos + 5 + tmpLen)
         {
            EOUT(("Invalid filesize\n"));
            return(-1);
         }

         tmpPos += 3 + tmpLen;
         if (*((unsigned char*) (pBuffer + tmpPos)) != 0x62 + i) {
            EOUT(("Corrupt Bitfile\n"));
            return -1;
         }
         tmpLen = ntohs(*((unsigned short*) (pBuffer + tmpPos + 1)));
      }

      binBuffer = pBuffer + tmpPos + 5;
   } else {
      EOUT(("No Bitfile header found\n"));
      binBuffer = pBuffer;
   }

   if (*((unsigned int*) (binBuffer)) != 0xFFFFFFFF || *((unsigned int*) (binBuffer + 4)) != 0x665599AA) {
      EOUT(("Corrupt Binfile\n"));
      return -1;
   }
   DOUT1(("Binfile checked\n"));

   return binBuffer - pBuffer -1;  //-1 bugfix stefan Mueller-Klieser
}

uint8_t roc::UdpDevice::calcBinXOR(const char* filename)
{
   char* bitfileBuffer = new char[SC_BITFILE_BUFFER_SIZE]; //bitfile is a binfile + header
   unsigned int size=0;
   int i = 0;

   DOUT0(("Loading bitfile!\n"));

   std::ifstream bitfile;
   bitfile.open(filename, std::ios_base::in);
   if(!bitfile) {
      EOUT(("Cannot open file: %s", filename));
      return 0;
   }

   uint8_t XORCheckSum = 0;

   while(!bitfile.eof())
   {
      bitfile.read(bitfileBuffer + size++, 1);
   }
   size--;//eof is +1

   int nBinPos = parseBitfileHeader(bitfileBuffer, size);
   if (nBinPos == -1)
      EOUT(("File does not seem to be a bit or bin file\n"));

   DOUT0(("Bitfilesize: %u + 512 bytes for the KIB header minus %d bytes for removed bitfile header\n", size, nBinPos));

   int bytes = 0;
   for(i = 0; i < 6;i++)//(int)(size - nBinPos); i++)
   {
      XORCheckSum ^= (uint8_t)bitfileBuffer[nBinPos + i];
      bytes++;
   }
   DOUT0(("check started at %d\n", nBinPos));
   DOUT0(("XOR is %u bytes checkes %d\n", XORCheckSum, bytes));

   delete [] bitfileBuffer;

   return XORCheckSum;
}

bool roc::UdpDevice::uploadDataToRoc(char* buf, unsigned datalen)
{
   poke(ROC_CLEAR_FILEBUFFER, 1);

   uint32_t maxsendsize = sizeof(controlSend.rawdata);

   unsigned pos = 0;

   while (pos < datalen) {
      uint32_t sendsize = datalen - pos;
      if (sendsize > maxsendsize) sendsize = maxsendsize;

      if (pokeRawData(ROC_FBUF_START + pos, buf + pos, sendsize)!=0) {
         EOUT(("Failed data packet with addr %u", pos));
         return false;
      }

      pos += sendsize;
   }
   return true;
}


bool roc::UdpDevice::uploadBitfile(const char* filename, int position)
{
   DOUT1(("Start upload of file %s, position: %d ...\n", filename, position));

   //bitfile is a binfile + header
   if(position > 1) {
      EOUT(("Only fileposition 0 and 1 is supported, sorry!\n"));
      return false;
   }

   std::ifstream bitfile;
   bitfile.open(filename, std::ios_base::in);

   if(!bitfile) {
      EOUT(("Cannot open file: %s\n", filename));
      return false;
   }

   ISEBinfileHeader hdr;
   memset(&hdr, 0, sizeof(hdr));
   strcpy((char*)hdr.ident, "XBF");
   hdr.headerSize = htonl(sizeof(hdr));
   memcpy(hdr.bitfileName, filename, 64);
   hdr.timestamp = 0;
   hdr.XORCheckSum = 0;

   bitfile.seekg(0, std::ios::end);
   unsigned bitfileSize  = bitfile.tellg();
   bitfile.seekg(0, std::ios::beg);
   char* bitfileBuffer = new char [bitfileSize];
   if (bitfileBuffer == 0) {
      EOUT(("Memory allocation error!\n"));
      return false;
   }
   memset(bitfileBuffer, 0, bitfileSize);
   bitfile.read(bitfileBuffer, bitfileSize);
   bitfile.close();
   //cout << "Bitfile loaded, size is " << bitfileSize << " bytes." << endl;

   int nBinPos = parseBitfileHeader(bitfileBuffer, bitfileSize);
   if (nBinPos == -1) {
      EOUT(("File does not seem to be a bit or bin file\n"));
      delete [] bitfileBuffer;
      return false;
   }

   //cout << "Bitfilesize: " << bitfileSize <<  " + 512 bytes for the KIB header minus " << nBinPos << " bytes for removed bitfile header" << endl;
   unsigned binfileSize = bitfileSize - nBinPos;
   DOUT0(("Binfile loaded, size is %u bytes.\n", binfileSize));

   for(unsigned i = 0; i < binfileSize; i++)
      hdr.XORCheckSum ^= bitfileBuffer[nBinPos + i];
   //cout << "XOR 8bit over bin file is " << (int)hdr.XORCheckSum << endl;

   hdr.binfileSize = htonl(binfileSize);

   uint32_t uploadBufferSize = sizeof(hdr) + binfileSize;

   char* buffer = new char[uploadBufferSize];
   if (buffer == 0) {
      delete [] bitfileBuffer;
      return false;
   }
   memset(buffer, 0, uploadBufferSize);

   memcpy(buffer, &hdr, sizeof(hdr));
   memcpy(buffer + sizeof(hdr), bitfileBuffer + nBinPos, binfileSize);

   uint8_t XORbitfileCheckSum = 0;
   for(unsigned n = 0; n < uploadBufferSize; n++)
      XORbitfileCheckSum ^= buffer[n];

   DOUT0(("XOR 8bit over Kib file is %u\n", XORbitfileCheckSum));

   DOUT0(("Now uploading Bitfile to ROC.\n"));

   ISEBinfileHeader* testhdr = (ISEBinfileHeader*) buffer;

   uint32_t testlen = ntohl(testhdr->binfileSize) + ntohl(testhdr->headerSize);
   if (testlen != uploadBufferSize)
      EOUT(("Size missmatch %u %u\n", testlen, uploadBufferSize));

   if (!uploadDataToRoc(buffer, uploadBufferSize)) {
      EOUT(("Fail to uplad file to the ROC\n"));
      delete [] bitfileBuffer;
      delete [] buffer;
      return false;
   }

   uint8_t ROCchecksum = peek(ROC_CHECK_BITFILEBUFFER);

   if(ROCchecksum != XORbitfileCheckSum) {
      DOUT0(("--- Upload failed !!!---\n"));
      delete [] bitfileBuffer;
      delete [] buffer;
      return false;
   }

   DOUT1(("Start flashing - wait ~200 s.\n"));

   ROCchecksum = ~XORbitfileCheckSum;

   if (poke(ROC_FLASH_KIBFILE_FROM_DDR, position, 300.)) {
      if(position == 0)
         ROCchecksum = peek(ROC_CHECK_BITFILEFLASH0);
      else
         ROCchecksum = peek(ROC_CHECK_BITFILEFLASH1);
   }

   delete [] bitfileBuffer;
   delete [] buffer;

   if(ROCchecksum != XORbitfileCheckSum) {
      EOUT(("--- Flashing returned an error! ---\n"));
      return false;
   }

   DOUT0(("--- Flashing finished successfully! ---\n"));
   return true;
}

bool roc::UdpDevice::uploadSDfile(const char* filename, const char* sdfilename)
{
   if (sdfilename == 0) sdfilename = filename;

   if ((filename==0) || (strlen(filename)==0)) return false;

   std::ifstream bfile;
   bfile.open(filename, std::ios_base::in);

   if(!bfile) {
      EOUT(("Cannot open file: %s\n", filename));
      return false;
   }

   bfile.seekg(0, std::ios::end);
   unsigned bufSize = bfile.tellg();
   if (bufSize == 0) {
      EOUT(("File is empty: %s\n", filename));
      return false;
   }

   bfile.seekg(0, std::ios::beg);
   char* buffer = new char [bufSize + 1024];
   if (buffer == 0) {
      EOUT(("Memory allocation error!\n"));
      return false;
   }
   memset(buffer, 0, bufSize + 1024);
   bfile.read(buffer + 1024, bufSize);
   bfile.close();

   *((uint32_t*) buffer) = htonl(bufSize);

   memcpy(buffer + 4, sdfilename, strlen(sdfilename) + 1);

   bufSize+=1024;

   uint8_t XORCheckSum = 0;

   for(unsigned i = 0; i < bufSize; i++)
      XORCheckSum ^= buffer[i];

   if (!uploadDataToRoc(buffer, bufSize)) {
      EOUT(("Fail to uplad file %s to the ROC\n", filename));
      delete [] buffer;
      return false;
   }

   uint8_t ROCchecksum = peek(ROC_CHECK_FILEBUFFER);

   if (ROCchecksum != XORCheckSum) {
      EOUT(("File XOR checks sum %u differ from uploaded %u\n",
             XORCheckSum, ROCchecksum));
      delete[] buffer;

      return false;
   }

   // one need about 1.5 sec to write single 4K block on SD-card :((
   double waittime = ((bufSize - 1024) / 4096 + 1.) * 1.52 + 0.5;

   DOUT0(("Start %s file writing, please wait ~%2.0f sec\n", sdfilename, waittime));

   // take 30 sec more for any unexpectable delays
   int res = poke(ROC_OVERWRITE_SD_FILE, 1, waittime + 30.);
//   int res = 0;

   if (res==0)
      DOUT0(("File %s was overwritten on SD card\n",sdfilename));
   else
      EOUT(("Fail to overwrite file %s on SD card\n", sdfilename));

   delete[] buffer;

   return res == 0;
}

