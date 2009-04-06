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
   fDev(dev),
   fControlMutex(),
   fCtrlState(ctrlReady),
   fControlCond(&fControlMutex),
   fControlCmd(0),
   fControlSendSize(0),
   fPacketCounter(0)

{
   // we will react on all input packets
   SetDoingInput(true);

}

roc::UdpControlSocket::~UdpControlSocket()
{
   completeLoop(false);

   if (fDev) fDev->fCtrlCh = 0;
   fDev = 0;
}

void roc::UdpControlSocket::ProcessEvent(dabc::EventId evnt)
{
   switch (dabc::GetEventCode(evnt)) {

      case evntDoCtrl: {
         if (fControlSend==0) return;
         StartSend(fControlSend, fControlSendSize);
         double tmout = fFastMode ? (fLoopCnt++ < 4 ? 0.01 : fLoopCnt*0.1) : 1.;
         fTotalTmoutSec -= tmout;
         ActivateTimeout(tmout);
         break;
      }

      case evntSocketRead: {
         UdpMessageFull* buf = fControlRecv;
         if (buf==0) buf = &fRecvBuf;

         ssize_t len = DoRecvBuffer(buf, sizeof(UdpMessageFull));
         if (len<=0) return;

         if ((fControlRecv != 0) &&
             (fControlRecv->password == htonl(ROC_PASSWORD)) &&
             (fControlSend->id == fControlRecv->id) &&
             (fControlSend->tag == fControlRecv->tag)) {
            completeLoop(true);
         } else
         if (fDev)
            fDev->processCtrlMessage(buf, len);
         break;
      }
      default:
         dabc::SocketIOProcessor::ProcessEvent(evnt);
   }
}

double roc::UdpControlSocket::ProcessTimeout(double last_diff)
{
   if (fShowProgress) {
       std::cout << ".";
       std::cout.flush();
   }

   if (fTotalTmoutSec <= 0.) {
      // stop doing;
      completeLoop(false);
      return -1.;
   }

   if (fFastMode || (fTotalTmoutSec <=5.)) {
      FireEvent(evntDoCtrl);
      return -1.;
   }

   fTotalTmoutSec -= 1.;

   return 1.;
}

bool roc::UdpControlSocket::completeLoop(bool res)
{
   if (fShowProgress) {
       std::cout << std::endl;
       std::cout.flush();
       fShowProgress = false;
   }

   dabc::Command* cmd = 0;

   {
      dabc::LockGuard guard(fControlMutex);

      if (fCtrlState == ctrlReady) return false;

      if (fControlCmd==0) {
         fCtrlState = res ? ctrlGotReply : ctrlTimedout;
         fControlCond._DoFire();
      } else {
         cmd = fControlCmd;
         fControlCmd = 0;
         fCtrlState = ctrlReady;
      }
   }

   if (cmd) dabc::Command::Reply(cmd, res);

   return true;
}

bool roc::UdpControlSocket::startCtrlLoop(
      dabc::Command* cmd,
      UdpMessage* send_buf, unsigned sendsize,
      UdpMessageFull* recv_buf,
      double total_tmout_sec, bool show_progress)
{
   {
      dabc::LockGuard guard(fControlMutex);

      if ((send_buf==0) || (recv_buf==0) || (sendsize==0)) return false;

      if (fCtrlState!=ctrlReady) {
         EOUT(("cannot start operation - somebody else uses control loop"));
         return false;
      }

      fCtrlState = ctrlWaitReply;
   }

   fControlCmd = cmd;

   fControlSend = send_buf;
   fControlRecv = recv_buf;
   fControlSendSize = sendsize;

   fControlSend->password = htonl(ROC_PASSWORD);
   fControlSend->id = htonl(fPacketCounter++);
   fControlRecv->id = ~fControlSend->id;

   // send aligned to 4 bytes packet to the ROC

   while ((fControlSendSize < MAX_UDP_PAYLOAD) &&
          (fControlSendSize + UDP_PAYLOAD_OFFSET) % 4) fControlSendSize++;

   fTotalTmoutSec = total_tmout_sec;
   fShowProgress = show_progress;

   if (fTotalTmoutSec>20.) fShowProgress = true;

   // in fast mode we will try to resend as fast as possible
   fFastMode = (fTotalTmoutSec < 10.) && !fShowProgress;
   fLoopCnt = 0;

   FireEvent(evntDoCtrl);

   return true;
}

bool roc::UdpControlSocket::waitCtrlLoop(double total_tmout_sec)
{
   dabc::LockGuard guard(fControlMutex);

   fControlCond._DoWait(total_tmout_sec + 1.);

   bool res = (fCtrlState == ctrlGotReply);

   fCtrlState = ctrlReady;

   return res;
}

// __________________________________________________________


roc::UdpDevice::UdpDevice(dabc::Basic* parent, const char* name, const char* thrdname, dabc::Command* cmd) :
   dabc::Device(parent, name),
   roc::UdpBoard(),
   fConnected(false),
   fRocIp(),
   fCtrlPort(0),
   fCtrlCh(0),
   fDataPort(0),
   fDataCh(0),
   controlSendSize(0),
   isBrdStat(false),
   displayConsoleOutput_(false)
{
   fRocIp = GetCfgStr(roc::xmlBoardIP, "", cmd);
   if (fRocIp.length()==0) return;

   if (!dabc::mgr()->MakeThreadFor(this, thrdname)) return;

   int nport = 8725;

   int fd = dabc::SocketThread::StartUdp(nport, nport, nport+1000);

   DOUT1(("Socket %d port %d  remote %s", fd, nport, fRocIp.c_str()));

   fd = dabc::SocketThread::ConnectUdp(fd, fRocIp.c_str(), ROC_DEFAULT_CTRLPORT);

   if (fd<=0) return;

   fCtrlCh = new UdpControlSocket(this, fd);

   fCtrlCh->AssignProcessorToThread(ProcessorThread());

   DOUT0(("Create control socket %d for port %d connected to host %s", fd, nport, fRocIp.c_str()));

   fCtrlPort = nport;
   fConnected = true;
}

roc::UdpDevice::~UdpDevice()
{
   if (fCtrlCh) {
      fCtrlCh->fDev = 0;
      fCtrlCh->DestroyProcessor();
   }
   fCtrlCh = 0;

   if (fDataCh) {
      fDataCh->fDev = 0;
      fDataCh->DestroyProcessor();
   }
   fDataCh = 0;
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

   if (fCtrlCh==0) return false;

   if ((role==roleMaster) || (role == roleDAQ)) {
      if (!poke(ROC_MASTER_LOGIN, 0)) return false;

      int nport = fCtrlPort + 1;

      int fd = dabc::SocketThread::StartUdp(nport, nport, nport+1000);

      fd = dabc::SocketThread::ConnectUdp(fd, fRocIp.c_str(), ROC_DEFAULT_DATAPORT);

      if (fd<=0) return false;

      if (!poke(ROC_MASTER_DATAPORT, nport)) {
         close(fd);
         return false;
      }

      fDataPort = nport;

      fDataCh = new UdpDataSocket(this, fd);
      fDataCh->AssignProcessorToThread(ProcessorThread());

      DOUT0(("Create data socket %d for port %d connected to host %s", fd, nport, fRocIp.c_str()));
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

   if (fCtrlCh==0) return false;

   if (!fCtrlCh->startCtrlLoop(0,
         &controlSend, controlSendSize,
         &controlRecv, total_tmout_sec, show_progress)) return false;

   return fCtrlCh->waitCtrlLoop(total_tmout_sec);
}

void roc::UdpDevice::processCtrlMessage(UdpMessageFull* pkt, unsigned len)
{
   // process PEEK or POKE reply messages from ROC

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
   }
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

