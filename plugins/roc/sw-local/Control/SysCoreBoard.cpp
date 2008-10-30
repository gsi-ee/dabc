#include "SysCoreBoard.h"

#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <string.h>

#include <iostream>
#include <fstream>

#include "../CppSockets/UdpSocket.h"
#include "../CppSockets/SocketHandler.h"

#include "SysCoreControl.h"
#include "SysCoreSorter.h"

using namespace std;

#ifdef __SC_DEBUG__
#define DOUT(args) printf args
#else
#define DOUT(args)
#endif

#define EOUT(args) printf args
#define DOUT0(args) printf args

double get_clock()
{
   static double firsttm = 0.;

   ntptimeval val;
   ntp_gettime(&val);

   double tm = val.time.tv_sec + val.time.tv_usec * 1e-6;

   if (firsttm == 0.) firsttm = tm;

   return tm - firsttm;
}


/*****************************************************************************
 * extern global Definitions
 ****************************************************************************/

//! KIP Binfile header
/*!
 * A bitfile from Xilinx ISE consists of a binfile and a fileheader.
 * We need only a bin file for reprogramming the virtex. So
 * we use this header in front of the bin file
 * to store it on the Actel Flash. Its size is 512 bytes.
 * Behind the Header, the binfile will be written to the flash
 */

union Kib_File_Header
{
   struct
   {
      uint8_t ident[4];
      uint32_t headerSize;
      uint32_t binfileSize;
      uint8_t XORCheckSum;
      uint8_t bitfileName[65];
      uint32_t timestamp;
   };
   uint8_t bytes[512];
};



/*****************************************************************************
 * Sockets
 ****************************************************************************/

//! SysCore Control DataSocket class
/*!
 * This class encapsulates all buffermanagment and fetchment for the datastreams.
 */
class SccDataSocket : public UdpSocket {

   protected:

      SysCoreBoard* board_;

   public:
      SccDataSocket(ISocketHandler* h, SysCoreBoard* board) :
         UdpSocket(*h),
         board_(board)
      {
      }
      
      virtual ~SccDataSocket()
      {
         DOUT(("Data socket %d closed\n", GetPort()));
      }

      virtual void OnRawData(const char *p,size_t l,struct sockaddr *sa_from,socklen_t sa_len)
      {
//         if (sa_len != sizeof(struct sockaddr_in)) return;
//         struct sockaddr_in sa;
//         memcpy(&sa,sa_from,sa_len);
//         if (*((uint32_t*)&sa.sin_addr) == board_->getIpAddress())

         // probably, one can check here source address
         board_->addDataPacket(p, l);
      }
};


/*****************************************************************************
 * SysCoreBoard
 ****************************************************************************/

uint16_t SysCoreBoard::gDataPortBase = 5846;

SysCoreBoard::SysCoreBoard(SysCoreControl* controler, uint32_t address, bool asmaster, uint16_t portnum) :
   controler_(controler),
   dataSocket_(0), 
   rocCtrlPort(portnum),
   rocDataPort(ROC_DEFAULT_DATAPORT),
   hostDataPort(0),
   ipAddress(address),
   rocNumber(0),
   masterMode(asmaster),
   daqState(daqInit),
   daqActive_(false)
{
   // should be non 0 
   currentMessagePacketId = 1; 

   transferWindow = 60;

   pthread_mutex_init(&dataMutex_, NULL);
   pthread_cond_init(&dataCond_, NULL);
   dataRequired_ = 0;
   consumerMode_ = 0;
   fillingData_ = false;

   readBufSize_ = 0;
   readBufPos_ = 0;

   fTotalRecvPacket = 0;
   fTotalResubmPacket = 0;
   
   memset(&brdStat, 0, sizeof(brdStat));
   isBrdStat = false;
   
   sorter_ = 0;
   
   ringBuffer = 0; 
   ringCapacity = 0; 
   ringHead = 0;
   ringHeadId = 0; 
   ringTail = 0;
   ringSize = 0;
}

SysCoreBoard::~SysCoreBoard()
{
   DOUT(("SysCoreBoard %u dtor\n", rocNumber));
   
   resetDaq();
   
   if (sorter_!=0) {
      delete sorter_;
      sorter_ = 0;   
   }
   
   if (dataSocket_) {
      delete dataSocket_; 
      dataSocket_ = 0;
   }

   pthread_cond_destroy(&dataCond_);
   pthread_mutex_destroy(&dataMutex_);
}

void SysCoreBoard::setUseSorter(bool on)
{
   delete sorter_;
   sorter_ = 0;   

   if (on)
      sorter_ = new SysCoreSorter(1024, 0, 1024);
}


bool SysCoreBoard::startDataSocket()
{
   dataSocket_ = new SccDataSocket(controler_->socketHandler_, this);
   
   port_t port = gDataPortBase;
   if (dataSocket_->Bind(port, 1000)==0) {
      hostDataPort = dataSocket_->GetPort(); 
      DOUT(("Bind data port to %u\n", hostDataPort));
      return true;
   }

   EOUT(("Data Socket Bind failed for Ports %u .. %u \n", 
      gDataPortBase, gDataPortBase + 1000));
      
   hostDataPort = 0;
   delete dataSocket_; 
   dataSocket_ = 0;
  
   return false;
}


//!TODO: this is a dangerous function copied from David Rohr, gives back the Headersize + 1
// but we want the Header size , so i patched the return value but needs real bugfix
int SysCoreBoard::parseBitfileHeader(char* pBuffer, unsigned int nLen)
{
   char* binBuffer;

   if (nLen < 16)
   {
      EOUT(("Invalid filesize\n"));
      return(-1);
   }
   if (*((unsigned short*) pBuffer) == 0x0900 && *((unsigned int*) (pBuffer + 2)) == 0xF00FF00F && *((unsigned int*) (pBuffer + 6)) == 0xF00FF00F)
   {
      DOUT0(("Bitfile found, skipping header...\n"));
      if (*((unsigned int*) (pBuffer + 10)) != 0x61010000)
      {
         EOUT(("Corrupt file\n"));
         return(-1);
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
         if (*((unsigned char*) (pBuffer + tmpPos)) != 0x62 + i)
         {
            EOUT(("Corrupt Bitfile\n"));
            return(-1);
         }
         tmpLen = ntohs(*((unsigned short*) (pBuffer + tmpPos + 1)));
      }

      binBuffer = pBuffer + tmpPos + 5;
   }
   else
   {
      EOUT(("No Bitfile header found\n"));
      binBuffer = pBuffer;
   }

   if (*((unsigned int*) (binBuffer)) != 0xFFFFFFFF || *((unsigned int*) (binBuffer + 4)) != 0x665599AA)
   {
      EOUT(("Corrupt Binfile\n"));
      return(-1);
   }
   DOUT0(("Binfile found\n"));

   return(binBuffer - pBuffer -1);//-1 bugfix stefan Mueller-Klieser
}

uint8_t SysCoreBoard::calcBinXOR(const char* filename)
{
   char* bitfileBuffer = new char[SC_BITFILE_BUFFER_SIZE]; //bitfile is a binfile + header
   unsigned int size=0;
   int i = 0;

   DOUT0(("Loading bitfile!\n"));

   ifstream bitfile;
   bitfile.open(filename, ios_base::in);
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

bool SysCoreBoard::uploadDataToRoc(char* buf, unsigned datalen)
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


bool SysCoreBoard::uploadBitfile(const char* filename, int position)
{
   DOUT0(("Start upload of file %s, position: %d ...\n", filename, position));
   
   //bitfile is a binfile + header
   if(position > 1) {
      EOUT(("Only fileposition 0 and 1 is supported, sorry!\n"));
      return false;
   }

   ifstream bitfile;
   bitfile.open(filename, ios_base::in);

   if(!bitfile) {
      EOUT(("Cannot open file: %s\n", filename));
      return false;
   }

   Kib_File_Header hdr;
   memset(&hdr, 0, sizeof(hdr));
   strcpy((char*)hdr.ident, "XBF");
   hdr.headerSize = htonl(sizeof(hdr));
   memcpy(hdr.bitfileName, filename, 64);
   hdr.timestamp = 0;
   hdr.XORCheckSum = 0;

   bitfile.seekg(0, ios::end);
   unsigned bitfileSize  = bitfile.tellg();
   bitfile.seekg(0, ios::beg);
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

   Kib_File_Header* testhdr = (Kib_File_Header*) buffer;
   
   uint32_t testlen = ntohl(testhdr->binfileSize) + ntohl(testhdr->headerSize);
   if (testlen != uploadBufferSize)
      EOUT(("Size missmatch %u %u\n", testlen, uploadBufferSize));
      
   if (!uploadDataToRoc(buffer, uploadBufferSize)) {
      EOUT(("Fail to uplad file to the ROC\n"));
      delete [] bitfileBuffer;
      delete [] buffer;
      return false;
   }
   
   uint32_t retVal = 0;
   peek(ROC_CHECK_BITFILEBUFFER, retVal);
   uint8_t ROCchecksum = retVal;

   if(ROCchecksum != XORbitfileCheckSum) {
      DOUT0(("--- Upload failed !!!---\n"));
      delete [] bitfileBuffer;
      delete [] buffer;
      return false;
   }

   DOUT0(("Start flashing - wait ~200 s.\n"));
   
   ROCchecksum = ~XORbitfileCheckSum;

   if (poke(ROC_FLASH_KIBFILE_FROM_DDR, position, 300.) == 0) {
      retVal = 0;
      if(position == 0)
         peek(ROC_CHECK_BITFILEFLASH0, retVal);
      else
         peek(ROC_CHECK_BITFILEFLASH1, retVal);
      ROCchecksum = retVal;
   }
   
   delete [] bitfileBuffer;
   delete [] buffer;

   if(ROCchecksum != XORbitfileCheckSum) {
      DOUT0(("--- Flashing returned an error! ---\n"));
      return false;
   }

   DOUT0(("--- Flashing finished successfully! ---\n"));
   return true;
}

bool SysCoreBoard::uploadSDfile(const char* filename, const char* sdfilename)
{
   if (sdfilename == 0) sdfilename = filename;
   
   if ((filename==0) || (strlen(filename)==0)) return false; 

   ifstream bfile;
   bfile.open(filename, ios_base::in);

   if(!bfile) {
      EOUT(("Cannot open file: %s\n", filename));
      return false;
   }

   bfile.seekg(0, ios::end);
   unsigned bufSize = bfile.tellg();
   if (bufSize == 0) {
      EOUT(("File is empty: %s\n", filename));
      return false;
   }
   
   bfile.seekg(0, ios::beg);
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
   
   uint32_t retVal = 0;
   peek(ROC_CHECK_FILEBUFFER, retVal);
   uint8_t ROCchecksum = retVal;

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

void SysCoreBoard::addDataPacket(const char* p, unsigned l)
{
   fTotalRecvPacket++;

   bool dosendreq = false;
   bool docheckretr = false;
   SysCore_Data_Request_Full req;

   SysCore_Data_Packet_Full* src = (SysCore_Data_Packet_Full*) p;
   SysCore_Data_Packet_Full* tgt = 0;

//   DOUT(("Packet id:%u size %u reqid:%u\n", packet->pktid, packet->nummsg, packet->lastreqid));

   double curr_tm = get_clock();

   bool data_call_back = false;

   {
      LockGuard guard(dataMutex_);
      
      if (!daqActive_) return;

      uint32_t src_pktid = ntohl(src->pktid);

//      DOUT0(("Brd:%u  Packet id:%05u Tm:%7.5f Head:%u Need:%u\n", rocNumber, src_pktid, curr_tm, ringHeadId, dataRequired_ / MESSAGES_PER_PACKET));

      if (ntohl(src->lastreqid) == lastRequestId) lastRequestSeen = true;

      uint32_t gap = src_pktid - ringHeadId;
      
      if (gap < ringCapacity - ringSize) {
         // normal when gap == 0, meaning we get that we expecting 
          
         while (ringHeadId != src_pktid) {
            // this is indicator of lost packets 
            docheckretr = true; 
            packetsToResend.push_back(ResendPkt(ringHeadId));
            
            ringHeadId++;
            ringHead = (ringHead+1) % ringCapacity;
            ringSize++;
         }
         
         tgt = ringBuffer + ringHead;
         ringHead = (ringHead+1) % ringCapacity;
         ringHeadId++;    
         ringSize++;
      } else
      if (ringSize + gap < ringSize) {
         // this is normal resubmitted packet

         fTotalResubmPacket++;
         
         unsigned resubm = ringTail + ringSize + gap;
         while (resubm>=ringCapacity) resubm-=ringCapacity;
         tgt = ringBuffer + resubm;
         
         DOUT(("Get resubm pkt %u on position %u headid = %u\n", src_pktid, resubm, ringHeadId));

         if (tgt->lastreqid != 0) {
            DOUT0(("Roc:%u Packet %u already seen  Resubm:%u Waitsz:%u ringSize:%u mode:%u\n", 
                rocNumber, src_pktid, packetsToResend.size(), dataRequired_, ringSize, consumerMode_));
            if (src_pktid != tgt->pktid) 
               EOUT(("!!!!!!!! Roc:%u Missmtach %u %u\n",rocNumber, src_pktid , tgt->pktid));
            tgt = 0;
         } else {
            // remove id from resend list
            list<ResendPkt>::iterator iter = packetsToResend.begin();
            while (iter!=packetsToResend.end()) {
               if (iter->pktid == src_pktid) {
                  packetsToResend.erase(iter);
                  break;
               }
               iter++;
            }
         }
      } else {
         EOUT(("Packet with id %u far away from expected range %u - %u\n",
                 src_pktid, ringHeadId - ringSize, ringHeadId));
      }
      
      if (tgt!=0) {
         tgt->lastreqid = 1;
         tgt->pktid = src_pktid;
         tgt->nummsg = ntohl(src->nummsg);
         memcpy(tgt->msgs, src->msgs, tgt->nummsg*6);
      } 
      
      if (ringSize >= ringCapacity)
         EOUT(("RING BUFFER DANGER Roc:%u size:%u capacity:%u\n", rocNumber, ringSize, ringCapacity));
         
      // check if we found start/stop messages
      if ((tgt!=0) && ((daqState == daqStarting) || (daqState == daqStopping)))
         for (unsigned n=0;n<tgt->nummsg;n++) {
            SysCoreData* data = (SysCoreData*) (tgt->msgs + n*6);
            if (data->isStartDaqMsg()) {
               DOUT(("Find start message\n"));
               if (daqState == daqStarting)
                  daqState = daqRuns;
               else
                  EOUT(("Start DAQ when not expected\n"));
            } else
            if (data->isStopDaqMsg()) {
               DOUT(("Find stop message\n"));
               if (daqState == daqStopping)
                  daqState = daqStopped;
               else
                 EOUT(("Stop DAQ when not expected\n"));
            }
         }

      if (daqState == daqStopped)
         if ((packetsToResend.size()==0) && !docheckretr)
            daqActive_ = false;

      dosendreq = _checkDataRequest(&req, curr_tm, docheckretr);
      
      // one should have definite amount of valid packets in data queue
      if (consumerMode_ && _checkAvailData(dataRequired_))
         if (consumerMode_ == 1) 
            pthread_cond_signal(&dataCond_);
         else {
            consumerMode_ = 0;
            dataRequired_ = 0;
            data_call_back = true;
         }
   }

   if (dosendreq) 
      sendDataRequest(&req);
      
   if (data_call_back)
      controler_->DataCallBack(this);
}

bool SysCoreBoard::_checkAvailData(unsigned num_msg)
{
   unsigned min_num_pkts = (num_msg < MESSAGES_PER_PACKET) ? 1 : num_msg / MESSAGES_PER_PACKET;
   
   // no chance to get required number of packets
   if ((ringSize - packetsToResend.size()) < min_num_pkts) return false;
   
   unsigned ptr = ringTail;
   unsigned total_sz = 0;
   while (ptr != ringHead) {
      // check, if there is lost packets 
      if (ringBuffer[ptr].lastreqid == 0) return false;
      total_sz += ringBuffer[ptr].nummsg;
      if (total_sz >= num_msg) return true;
      ptr = (ptr+1) % ringCapacity;  
   }
   
   return false;
}

void SysCoreBoard::TestTimeoutConditions(double curr_tm)
{
   bool dosendreq = false;
   SysCore_Data_Request_Full req;

   {
      LockGuard guard(dataMutex_);

      dosendreq = _checkDataRequest(&req, curr_tm, true);
   }

   if (dosendreq)
      sendDataRequest(&req);
}

bool SysCoreBoard::_checkDataRequest(SysCore_Data_Request_Full* req, double curr_tm, bool check_retrans)
{
   // try to send credits increment anytime when 1/2 of maximum is availible

   if (!daqActive_) return false;

   
   // send request each 0.2 sec, if there is no replies on last request
   // send it much faster - every 0.01 sec. 
   bool dosend = 
      fabs(curr_tm - lastRequestTm) > (lastRequestSeen ? 0.2 : 0.01);
   
   uint32_t frontid = ringHeadId;
   if ((packetsToResend.size() < transferWindow) &&  
       (ringSize < ringCapacity - transferWindow))
         frontid += (transferWindow - packetsToResend.size());
   
   // if newly calculated front id bigger than last
   if ((frontid - lastSendFrontId) < 0x80000000) { 
       
     if ((frontid - lastSendFrontId) >= transferWindow / 3) dosend = true;
     
   } else
      frontid = lastSendFrontId;
   
   req->frontpktid = frontid;
   
   req->tailpktid = ringHeadId - ringSize;
   
   req->numresend = 0;
   
   if (!check_retrans && !dosend) return false;
   
   list<ResendPkt>::iterator iter = packetsToResend.begin();
   while (iter!=packetsToResend.end()) {
     if ((iter->numtry==0) || (fabs(curr_tm - iter->lasttm)) > 0.1) {
        iter->lasttm = curr_tm;
        iter->numtry++;
        if (iter->numtry < 8) {
            req->resend[req->numresend++] = iter->pktid;
            
            iter++;
            dosend = true;
            
            if (req->numresend >= sizeof(req->resend) / 4) {
               EOUT(("Number of resends more than one can pack in the retransmit packet\n")); 
               break;   
            }
            
        } else {

            list<ResendPkt>::iterator deliter = iter;
            
            unsigned target = ringHead + iter->pktid - ringHeadId;
            while (target >= ringCapacity) target += ringCapacity;

            EOUT(("Roc:%u Drop pkt %u tgt %u\n", rocNumber, iter->pktid, target));
            
            SysCore_Data_Packet_Full* tgt = ringBuffer + target;

            tgt->pktid = iter->pktid;
            tgt->nummsg = 1; // only one message
            tgt->lastreqid = 1; 

            SysCoreData* data = (SysCoreData*) tgt->msgs;
            data->setRocNumber(rocNumber);
            data->setMessageType(ROC_MSG_SYS);
            data->setSysMesType(5); // this is lost packet mark

            iter++;
            packetsToResend.erase(deliter);
        }
     } else
       iter++;
   }
   
   return dosend;   
}

bool SysCoreBoard::waitData(unsigned num_msgs, double wait_sec)
{
   if (num_msgs==0) {
      EOUT(("One should specify non-zero messages count\n"));
      num_msgs = 1;
   }

   LockGuard guard(dataMutex_);
   
   bool check = _checkAvailData(num_msgs);

   if (check || (wait_sec==0.)) return check;

   if (!daqActive_) {
      DOUT(("No more data can come - DAQ is not active bufsize = %d\n", ringSize));
      return false;
   }
   
   if (consumerMode_) {
      EOUT(("There is other consumer from board data\n"));
      return false;
   }

   struct timeval tp;
   gettimeofday(&tp, 0);

   long wait_microsec = long(wait_sec*1e6);

   tp.tv_sec += (wait_microsec + tp.tv_usec) / 1000000;
   tp.tv_usec = (wait_microsec + tp.tv_usec) % 1000000;

   struct timespec tsp  = { tp.tv_sec, tp.tv_usec*1000 };

   dataRequired_ = num_msgs;
   consumerMode_ = 1;
   if (wait_sec>0.)
      pthread_cond_timedwait(&dataCond_, &dataMutex_, &tsp);
   else
      pthread_cond_wait(&dataCond_, &dataMutex_);
   dataRequired_ = 0;
   consumerMode_ = 0;

   return _checkAvailData(num_msgs);
}

int SysCoreBoard::requestData(unsigned num_msgs)
{
   LockGuard guard(dataMutex_);

   if (!daqActive_) {
      DOUT(("No more data can come - DAQ is not active\n"));
      return 0;
   }
   
   if (_checkAvailData(num_msgs)) return 1;
   
   if (consumerMode_ == 1) {
      EOUT(("There is other consumer from board data\n"));
      return 0;
   }

   consumerMode_ = 2;
   dataRequired_ = num_msgs;
   
   return 2;
}

bool SysCoreBoard::fillData(void* buf, unsigned& sz)
{
   char* tgt = (char*) buf;
   unsigned fullsz = sz;
   sz = 0;
   
   unsigned start(0), stop(0), count(0);
   
   
   // in first lock take region, where we can copy data from
   {
      LockGuard guard(dataMutex_); 
       
      if (fillingData_) {
          EOUT(("Other consumer fills data from buffer !!!\n"));
          return false;
      }
      
      // no data to fill at all
      if (ringSize == 0) return false;

      fillingData_ = true;
      
      start = ringTail; stop = ringTail;
      while (stop!=ringHead) {
         if (ringBuffer[stop].lastreqid == 0) break; 
         stop = (stop + 1) % ringCapacity;
      } 
   }

   // now out of locked region start data filling
   
   if (sorter_) sorter_->startFill(buf, fullsz);

   while (start != stop) {
      SysCore_Data_Packet_Full* pkt = ringBuffer + start;
      
      if (sorter_) {
         // -1 while sorter want to put epoch, but did not do this
         if (sorter_->sizeFilled() >= (fullsz/6 - 1)) break;

         sorter_->addData((SysCoreData*) pkt->msgs, pkt->nummsg);
      } else {
         unsigned pktsize = pkt->nummsg * 6;
         if (pktsize>fullsz) {
            if (sz==0)
               EOUT(("Buffer size %u is too small for complete packet size %u\n", fullsz, pktsize));
            break;
         }

         memcpy(tgt, pkt->msgs, pktsize);

         sz+=pktsize;
         tgt+=pktsize;
         fullsz-=pktsize;
      }
      
      pkt->lastreqid = 0; // mark as no longer used 
      start = (start+1) % ringCapacity;
      count++; // how many packets taken from buffer
   }
   
   if (sorter_) {
      sz = sorter_->sizeFilled() * 6;
      sorter_->stopFill();  
   }

   bool dosendreq = false;
   SysCore_Data_Request_Full req;
   double curr_tm = get_clock();

   {
      LockGuard guard(dataMutex_);
      
      ringTail = start;
      fillingData_ = false;
      ringSize -= count;
      
      dosendreq = _checkDataRequest(&req, curr_tm, true);
   }

   if (dosendreq)
      sendDataRequest(&req);

   return sz>0;
}

bool SysCoreBoard::isDataInBuffer(double tmout)
{
   if (readBufPos_ < readBufSize_) return true;

   readBufSize_ = 0;
   readBufPos_ = 0;

   if (!waitData(1, tmout)) return false;

   readBufSize_ = sizeof(readBuf_);

   if (!fillData(readBuf_, readBufSize_)) {
      readBufSize_ = 0;
      return false;
   }
//   else printf("read of size %u %5.1f\n", readBufSize_, readBufSize_ / 6.);

   return true;
}


bool SysCoreBoard::getNextData(SysCoreData &target, double tmout)
{
   if (!isDataInBuffer(tmout)) return false;

   target.assignData(readBuf_ + readBufPos_);
   readBufPos_+=6;
   return true;
}

int SysCoreBoard::peek(uint32_t address, uint32_t &readValue, double tmout)
{
   readValue = 0;

   controlSend.tag = ROC_PEEK;
   controlSend.address = htonl(address);
   
   controlSendSize = sizeof(SysCore_Message_Packet);
   
   int res = 6;

   if (controler_->performCtrlLoop(this, tmout, false)) {
      readValue = ntohl(controlRecv.value);
      res = ntohl(controlRecv.address);
   }
   
   DOUT(("Roc:%u Peek(0x%04x, 0x%04x) res = %d\n", rocNumber, address, readValue, res));

   return res;
}

void SysCoreBoard::setBoardStat(void* rawdata, bool print)
{
   SysCore_Statistic* src = (SysCore_Statistic*) rawdata;

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
      DOUT0(("\033[0;%dm Roc:%u  Data:%6.3f MB/s Send:%6.3f MB/s Recv:%6.3f MB/s NOPs:%2u Frames:%2u Data:%4.1f%s Disp:%4.1f%s Send:%4.1f%s \n \033[0m", 
         36, rocNumber, brdStat.dataRate*1e-6, brdStat.sendRate*1e-6, brdStat.recvRate*1e-6, 
             brdStat.nopRate, brdStat.frameRate,
             brdStat.takePerf*1e-3,"%", brdStat.dispPerf*1e-3,"%", brdStat.sendPerf*1e-3,"%"));
}


SysCore_Statistic* SysCoreBoard::takeStat(double tmout, bool print)
{
   void *rawdata = 0;
   
   if (tmout > 0.) { 
      uint32_t readValue;
      int res = peek(ROC_STATBLOCK, readValue, tmout);
      if (res!=0) return 0;
      rawdata = controlRecv.rawdata;
   }
      
   setBoardStat(rawdata, print);
   
   return isBrdStat ? &brdStat : 0;
}

int SysCoreBoard::poke(uint32_t address, uint32_t writeValue, double tmout)
{
   if (!isMasterMode())
      EOUT(("Poke forbidenn for observer, but you can try!\n"));
   
   bool show_progress = false; 
   
   // define operations, which takes longer time as usual one   
   switch (address) {
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
   controlSend.address = htonl(address);
   controlSend.value = htonl(writeValue);
   controlSendSize = sizeof(SysCore_Message_Packet);
   
   int res = 6;

   if (controler_->performCtrlLoop(this, tmout, show_progress)) res = ntohl(controlRecv.value);
                           
   DOUT(("Roc:%u Poke(0x%04x, 0x%04x) res = %d\n", rocNumber, address, writeValue, res));

   return res;
}

void SysCoreBoard::sendDataRequest(SysCore_Data_Request_Full* pkt)
{
   uint32_t pkt_size = sizeof(SysCore_Data_Request) + pkt->numresend * sizeof(uint32_t);
   
   // make request always 4 byte alligned
   while ((pkt_size < MAX_UDP_PAYLOAD) && 
          (pkt_size + UDP_PAYLOAD_OFFSET) % 4) pkt_size++;
   
   lastRequestTm = get_clock(); 
   lastRequestSeen = false; 
   lastSendFrontId = pkt->frontpktid;
   lastRequestId++;

//   if (pkt->numresend)   
//      DOUT(("Request:%u request front:%u tail:%u resubm:%u\n", lastRequestId, pkt->frontpktid, pkt->tailpktid, pkt->numresend));
   pkt->password = htonl(ROC_PASSWORD);
   pkt->reqpktid = htonl(lastRequestId);
   pkt->frontpktid = htonl(pkt->frontpktid);
   pkt->tailpktid = htonl(pkt->tailpktid);
   for (uint32_t n=0; n < pkt->numresend; n++)
      pkt->resend[n] = htonl(pkt->resend[n]);
   pkt->numresend = htonl(pkt->numresend);

   if (dataSocket_)
      dataSocket_->SendToBuf(getIpAddress(), getDataPort(), (char*)pkt, pkt_size);

//   DOUT0(("Brd:%u Request id:%05u Tm:%7.5f Resend:%u First:%u\n", 
//      rocNumber, lastSendFrontId, get_clock(), ntohl(pkt->numresend),
//         (ntohl(pkt->numresend)>0 ? ntohl(pkt->resend[0]) : 0)));
}

bool SysCoreBoard::startDaq(unsigned trWin)
{
   resetDaq();

   bool res = true;

   {
      LockGuard guard(dataMutex_);
      daqActive_ = true;
      daqState = daqStarting;
      transferWindow = trWin < 4 ? 4 : trWin;
      
      ringCapacity = trWin * 10;
      if (ringCapacity<100) ringCapacity = 100;
      
      ringBuffer = new SysCore_Data_Packet_Full[ringCapacity];
      ringHead = 0;
      ringHeadId = 0; 
      ringTail = 0;
      ringSize = 0;
      
      if (ringBuffer==0) {
         EOUT(("Cannot allocate ring buffer\n"));
         res = false;
      } else
      for (unsigned n=0;n<ringCapacity;n++)
         ringBuffer[n].lastreqid = 0; // indicate that entry is invalid
   }

   if (res) 
      res = poke(ROC_START_DAQ , 1) == 0;

   if (!res) {
      EOUT(("Fail to start DAQ\n"));
      LockGuard guard(dataMutex_);
      daqActive_ = false;
      daqState = daqFails;
   }

   return res;
}

void SysCoreBoard::resetDaq()
{
   LockGuard guard(dataMutex_);

   daqActive_ = false;
   daqState = daqInit;
   
   lastRequestId = 0; 
   lastRequestTm = 0.;
   lastRequestSeen = true;
   lastSendFrontId = 0;
   
   packetsToResend.clear();
   
   if (ringBuffer) {
      delete [] ringBuffer;
      ringBuffer = 0;
   }

   ringCapacity = 0; 
   ringHead = 0;
   ringHeadId = 0; 
   ringTail = 0;
   ringSize = 0;
}

bool SysCoreBoard::suspendDaq()
{
   DOUT(("Calling suspendDAQ\n"));

   {
      LockGuard guard(dataMutex_);

      if ((daqState!=daqStarting) && (daqState!=daqRuns)) {
         EOUT(("Calling suspendDaq at wrong state\n"));
         return false;
      }

      daqState = daqStopping;
//      daqActive_ = false;
   }

   bool res = (poke(ROC_SUSPEND_DAQ , 1) == 0);

   if (!res) {
       DOUT(("DAQ suspend FAILS\n"));
       LockGuard guard(dataMutex_);
       daqState = daqFails;
       daqActive_ = false;
   }

   return res;
}

bool SysCoreBoard::stopDaq()
{
   DOUT(("Calling stopDaq\n"));

   // call reset first that no any new request will be send to ROC
   resetDaq();

   return poke(ROC_STOP_DAQ , 1) == 0;
}

int SysCoreBoard::pokeRawData(uint32_t address, const void* rawdata, uint32_t rawdatelen, double tmout)
{
   controlSend.tag = ROC_POKE;
   controlSend.address = htonl(address);
   controlSend.value = htonl(rawdatelen);
   memcpy(controlSend.rawdata, rawdata, rawdatelen);
   controlSendSize = sizeof(SysCore_Message_Packet) + rawdatelen;
 
   int res = 6;

   if (controler_->performCtrlLoop(this, tmout, tmout > 3.)) res = ntohl(controlRecv.value);
                           
   DOUT(("Roc:%u PokeRaw(0x%04x, 0x%04x) res = %d\n", rocNumber, address, rawdatelen, res));

   return res;
}

bool SysCoreBoard::sendConsoleCommand(const char* cmd)
{
   unsigned length = cmd ? strlen(cmd) + 1 : 0;

   if ((length<2) || (length > sizeof(controlSend.rawdata))) return false;
   
   return pokeRawData(ROC_CONSOLE_CMD, cmd, length) == 1;
}

bool SysCoreBoard::saveConfig(const char* filename)
{
   uint32_t len = filename ? strlen(filename) + 1 : 0;

   DOUT(("Save config file %s on ROC\n",filename ? filename : ""));
   
   return pokeRawData(ROC_CFG_WRITE, filename, len, 10.) == 0;
}

bool SysCoreBoard::loadConfig(const char* filename)
{
   uint32_t len = filename ? strlen(filename) + 1 : 0;
   
   DOUT(("Load config file %s on ROC\n",filename ? filename : ""));
   
   return pokeRawData(ROC_CFG_READ, filename, len, 10.) == 0;
}


