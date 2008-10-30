#include "mbs/MbsDataInput.h"

#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#include "dabc/logging.h"
#include "dabc/Buffer.h"

void PrintSocketError(int errnum, const char* info) 
{
   char sbuf[256];
   strerror_r(errnum, sbuf, sizeof(sbuf));
   
   EOUT(("%s error %d : %s", info, errnum, sbuf));
}


ssize_t my_stc_read(int fd, void* buffer, ssize_t len, int timeout)
{
   fd_set          xrmask, xwmask, xemask;
   struct timeval  read_timeout;
   char*           buffer_tmp = (char*) buffer;
   ssize_t         bytes_to_read = len;
   ssize_t         retval;
   int             retry_cnt(0);
   
   while (bytes_to_read > 0) {
      
      if (retry_cnt>100000) {
         EOUT(("Too many retries"));
         return -1;   
      }

      FD_ZERO(&xrmask);
      FD_ZERO(&xemask);
      FD_ZERO(&xwmask);
      FD_SET(fd, &xrmask);
      FD_SET(fd, &xemask);

      read_timeout.tv_sec  = timeout;
      read_timeout.tv_usec = 0;

      retval = select(fd+1, &xrmask, &xwmask, &xemask, timeout>0 ? &read_timeout : 0);
      
      // this is error
      if (retval<0) {
         if (errno==EINTR) continue;
         PrintSocketError(errno, "Calling select on socket");
         return -1;
      }
      
      // this is timeout
      if (retval==0) {
         return 0;   
      }
      
      if (retval!=1) {
         EOUT(("Too many file descriptors modified %d", retval));
         return -1;   
      }
       
      retval = recv(fd, buffer_tmp, bytes_to_read, 0);
      
      if (retval>0) {
         bytes_to_read -= retval;   
         buffer_tmp += retval;
         retry_cnt++;
         continue; 
      }
      
      if (retval==0) {
         EOUT(("Socket is closed on other side"));   
         return -1; 
      }
      
      if (retval<0) {
         PrintSocketError(errno, "Reading from socket");
         return -1;  
        /*  
        switch( errno ) {
          case EBADF      : return STC__INVSOCK;
          case EFAULT     : return STC__INVBUF;
          case EINVAL     : return STC__NGBUFSIZE;
          case EINTR      : return STC__EINTR;
          case ECONNRESET : return STC__ECONNRES;
          default         :  sprintf(c_msg,"STC read error channel %d",i_channel);
                             perror(c_msg);
                             return STC__FAILURE;
        }
        */
      }
   }
   
   return len;
}

mbs::MbsDataInput::MbsDataInput() :
   dabc::DataInput(),
   fSocket(-1),
   fTimeout(-1),
   fSwapping(false)
{
}

mbs::MbsDataInput::~MbsDataInput()
{
   Close(); 
}

bool mbs::MbsDataInput::Open(const char* server, int port)
{
   if (fSocket>=0) {
      EOUT(("Socket %d is already opened and used", fSocket));   
      return false; 
   }

   int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

   if (fd==-1) {
      EOUT(("Error while opening new socket err = %d", errno));
      return false;
      /*
        case EAFNOSUPPORT    : return STC__INVADDRF;
        case ESOCKTNOSUPPORT : return STC__SOCKNSUP;
        case EPROTONOSUPPORT : return STC__INVPROTO;
        case EMFILE          : return STC__SOCKTABF;
        case ENOBUFS         : return STC__SOCKSPAF;
        default              : return STC__FAILURE;
      */
   } 
   
   struct hostent *host = ::gethostbyname(server);
   if (host==0) {
      ::close(fd);
      return false;
   }
   
   struct sockaddr_in serv_addr;
   serv_addr.sin_family = host->h_addrtype;
   serv_addr.sin_port   = htons(port);
   serv_addr.sin_addr = * ((struct in_addr *) host->h_addr);
   
   int retval = ::connect(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
   
   if (retval==-1) {
      ::close(fd);
      return false;
      
      /*
             case EBADF       : return STC__INVSOCK;
       case ENOTSOCK    : return STC__NOTSOCK;
       case EISCONN     : return STC__SOCKISC;
       case ETIMEDOUT   : return STC__CONNTOUT;
       case ENETUNREACH : return STC__NETUNREA;
       case EADDRINUSE  : return STC__PORTINUS;
            case ECONNREFUSED : return STC__ECONNREF;
       default          : return STC__FAILURE;
       */
      
   } 
   
   retval = recv(fd, &fInfo, sizeof(fInfo), MSG_WAITALL);
   if (retval!=sizeof(fInfo)) {
      EOUT(("Cannot recieve first info from transport"));
      return false;
   }
   
   fSwapping = false;
   
   DOUT1(("Endian = %d", fInfo.iEndian));
   
   if (fInfo.iEndian != 1) {
      SwapMbsData(&fInfo, sizeof(fInfo));
      fSwapping = true;
   }

   if (fInfo.iEndian != 1) {
      EOUT(("Problem reading info from transport server"));
      return false;
   }
   
   DOUT1(("Open server %s port %d swapping %s bufsize %d nstream %d", 
      server, port, DBOOL(fSwapping), fInfo.iMaxBytes, fInfo.iBuffers));
 
   fSocket = fd;
    
   return true; 
}

bool mbs::MbsDataInput::Close()
{
   if (fSocket<=0) return true; 
   
   DOUT3(("Close MbsDataInput"));
   
   ::shutdown(fSocket, SHUT_RDWR);
   fSocket = -1;
   fSwapping = false;
   return true;
}
         
unsigned mbs::MbsDataInput::Read_Size()
{
   if (fSocket<=0) return di_EndOfStream;
   
   ssize_t res = my_stc_read(fSocket, &fBufHdr, sizeof(fBufHdr), fTimeout);
   
   // this is timeout, nothing to do, try later
   if (res==0) { DOUT1(("Header reading timedout")); return di_RepeatTimeOut; }
   
   // this is error
   if (res != sizeof(sMbsBufferHeader)) {
      EOUT(("Close socket while error")); 
      Close();
      return di_Error;
   }
   
   if (fSwapping) fBufHdr.Swap();
   
   if (fBufHdr.BufferLength() == sizeof(sMbsBufferHeader)) {
      DOUT4(("Keep alive buffer"));
      return di_Repeat;   
   }
      
   // this is total length of the buffer, including buffer header   
   return fBufHdr.BufferLength();
}

unsigned mbs::MbsDataInput::Read_Complete(dabc::Buffer* buf)
{
   if ((fSocket<=0) || (buf==0)) return di_Error;

   unsigned bufsize = fBufHdr.BufferLength();
   if (bufsize > buf->GetDataSize()) {
      EOUT(("Cannot use buffer %d (need %d) for reading from server", buf->GetDataSize(), bufsize));
      return di_Error;  
   }
    
   memcpy(buf->GetDataLocation(), &fBufHdr, sizeof(sMbsBufferHeader));
    
   void* tmp_buf = (char*) buf->GetDataLocation() + sizeof(sMbsBufferHeader);
   ssize_t tmp_len = bufsize - sizeof(sMbsBufferHeader);
   
   ssize_t res = my_stc_read(fSocket, tmp_buf, tmp_len, fTimeout); 

   if (res != tmp_len) {
      EOUT(("Error during buffer read"));
      Close();
      return di_Error;
   }

   if (fSwapping)
      SwapMbsData(tmp_buf, tmp_len);
      
   buf->SetDataSize(fBufHdr.UsedBufferLength());
   
   if (fBufHdr.iType == MBS_TYPE(10,1)) buf->SetTypeId(mbt_Mbs10_1); else
   if (fBufHdr.iType == MBS_TYPE(100,1)) buf->SetTypeId(mbt_Mbs100_1); else
      EOUT(("Cannot identify Mbs buffer type"));
   
   DOUT3(("Read buffer typ %d:%d completed size %d subev %d", fBufHdr.i_type, fBufHdr.i_subtype, bufsize, fBufHdr.iNumEvents));
      
   return di_Ok;   
}
