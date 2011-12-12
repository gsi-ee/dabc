#include "ezca/EpicsInput.h"

#include "dabc/Port.h"
#include "dabc/Pointer.h"
#include "mbs/MbsTypeDefs.h"
#include "ezca/Definitions.h"

#include <sys/timeb.h>

 #include <stdint.h>

//#include "ezcalib.h"



ezca::EpicsInput::EpicsInput(const char* name, uint32_t bufsize) :
dabc::DataInput(),
fName(name ? name : "EpicsMonitorInput"),
fBufferSize(bufsize),
fTimeout(1),
fSubeventId(8)
{
}

ezca::EpicsInput::~EpicsInput()
{
   Close();
}

bool ezca::EpicsInput::Read_Init(dabc::Command* cmd, dabc::WorkingProcessor* port)
{
	DOUT1(("EpicsInput::Read_Init"));
	dabc::ConfigSource cfg(cmd, port);

   fName = cfg.GetCfgStr(ezca::xmlEpicsName, fName);
   fBufferSize = cfg.GetCfgInt(dabc::xmlBufferSize, fBufferSize);
   fTimeout = cfg.GetCfgDouble(ezca::xmlTimeout, fTimeout);
   fSubeventId= cfg.GetCfgInt(ezca::xmlEpicsSubeventId, fSubeventId);
   fInfoDescr.SetUpdateRecord(cfg.GetCfgStr(ezca::xmlUpdateFlagRecord, "flag"));
   fInfoDescr.SetIDRecord(cfg.GetCfgStr(ezca::xmlEventIDRecord, "id"));
   int numlongs=cfg.GetCfgInt(ezca::xmlNumLongRecords, 0);
   for(int t=0;t<numlongs;++t)
      fInfoDescr.AddLongRecord(cfg.GetCfgStr(FORMAT(("%s%d", ezca::xmlNameLongRecords, t)),"dummy"));
   int numdubs = cfg.GetCfgInt(ezca::xmlNumDoubleRecords, 0);
   for(int t=0;t<numdubs;++t)
      fInfoDescr.AddDoubleRecord(cfg.GetCfgStr(FORMAT(("%s%d", ezca::xmlNameDoubleRecords, t)),"dummy"));

   DOUT1(("EpicsInput %s - BufferSize = %d byte, Timeout = %e s, subevtid:%d, update flag:%s id:%s ",
         fName.c_str(), fBufferSize, fTimeout,fSubeventId,fInfoDescr.GetUpdateRecord(),fInfoDescr.GetIDRecord()));

   return true;
}



bool ezca::EpicsInput::Close()
{
   fInfoDescr.Reset();
   DOUT1(("EpicsInput::Close"));
   return true;
}

unsigned ezca::EpicsInput::Read_Size()
{
   // evaluate real expected size from infodescr entries
   unsigned int totalsize=sizeof(mbs::EventHeader)+sizeof(mbs::SubeventHeader)+2*sizeof(unsigned int) + 2*sizeof(unsigned int64_t); // header length
#ifdef EZCA_useDOUBLES
   totalsize+= fInfoDescr.NumLongRecords()*sizeof(int64_t)+ fInfoDescr.NumDoubleRecords()*sizeof(double);

#else
   totalsize+= fInfoDescr.NumLongRecords()*sizeof(int64_t)+ fInfoDescr.NumDoubleRecords()*sizeof(int64_t);
#endif
   // account for process variable names list here:
   unsigned int descsize=fInfoDescr.SizeofDoubleRecords()+ fInfoDescr.SizeofLongRecords();
   descsize += fInfoDescr.NumDoubleRecordsString().size()+1 + fInfoDescr.NumLongRecordsString().size()+1;
   // note: we have to add extra size for the '\0' separators to be put into payload buffer
   DOUT3(("EpicsInput:: descriptor size is %d",descsize));
   totalsize+= descsize;

   // string sizes may use fraction of words, adjust to upper bounds:
   int itotalsize=totalsize/sizeof(int);
   if(itotalsize*sizeof(int)<totalsize)
      totalsize=(itotalsize+1)*sizeof(int);
   if(totalsize>fBufferSize) totalsize=fBufferSize;
   DOUT3(("EpicsInput::Read_Size-%d",totalsize));
   return totalsize;
}

unsigned ezca::EpicsInput::Read_Complete(dabc::Buffer* buf)
{
   static long evcount=0;
   long evtnumber=0;
   long flag = 0;
   static long oldflag = -1;
   bool useflag=true;
   if (std::string(fInfoDescr.GetUpdateRecord()) == "") {
      useflag = false;
   }
   DOUT3(("EpicsInput:useflag is %d",useflag));
   if (useflag) {
      // first check the flag record:
      if (CA_GetLong((char*) fInfoDescr.GetUpdateRecord(), flag) != 0)
         return dabc::di_RepeatTimeOut; DOUT3(("EpicsInput:ReadComplete flag record %s = %d ",fInfoDescr.GetUpdateRecord(), flag));
      if (flag != 0 || flag == oldflag) {
         oldflag = flag;
         return dabc::di_RepeatTimeOut;
      }
      oldflag = flag;

      if (CA_GetLong((char*) fInfoDescr.GetIDRecord(), evtnumber) != 0)
         return dabc::di_RepeatTimeOut; DOUT3(("EpicsInput:ReadComplete id record %s = %d ",fInfoDescr.GetIDRecord(), evtnumber));
      // if this is nonzero, read other records and write buffer
   }
   else
      {
           if(oldflag){
                   oldflag=0;
                   return dabc::di_RepeatTimeOut;
           }
           else
           {
                   oldflag=1;
           }
      }



   std::string descriptor; // this contains process variable names description
   dabc::Pointer ptr(buf);
   DOUT3(("EpicsInput:Got pointer 0x%x, buf size:%d",ptr(),buf.GetTotalSize()));
   unsigned totallen = 0;
   unsigned rawlen = 0;
   // prepare mbs event with subevent
   mbs::EventHeader* evhdr = (mbs::EventHeader*) ptr();
   evhdr->Init(evcount++);
   //evhdr->Init(evtnumber);
   ptr.shift(sizeof(mbs::EventHeader));
   totallen += sizeof(mbs::EventHeader);
   mbs::SubeventHeader* subhdr = (mbs::SubeventHeader*) ptr();
   subhdr->Init();
   subhdr->fFullId=fSubeventId;
   //	subhdr->iProcId = ezca::proc_EPICS_Mon;
   //	subhdr->iSubcrate = 0; // TODO: configuration of id numbers
   //	subhdr->iControl = 0; //ezca::packageStruct;
   ptr.shift(sizeof(mbs::SubeventHeader));
   totallen += sizeof(mbs::SubeventHeader);
   rawlen=0;
   //first payload: id number
   DOUT3(("EpicsInput:Got pointer after shift:0x%x",ptr()));
   *((unsigned int*) ptr()) = evtnumber;
   ptr.shift(sizeof(unsigned int));
   rawlen += sizeof(unsigned int);
   totallen += sizeof(unsigned int);
   //secondly: put time
   // system time expression
   struct timeb s_timeb;
   ftime(&s_timeb);
   *((unsigned int*) ptr()) = s_timeb.time;
   ptr.shift(sizeof(unsigned int));
   rawlen += sizeof(unsigned int);
   totallen += sizeof(unsigned int);
   // first payload: header with number of long records. All aligned to 8 bytes now JAM
   *((unsigned int64_t*) ptr()) = fInfoDescr.NumLongRecords();
      ptr.shift(sizeof(unsigned int64_t));
      rawlen += sizeof(unsigned int64_t);
      totallen += sizeof(unsigned int64_t);

      // put number of longs into variable description:
   descriptor.append(fInfoDescr.NumLongRecordsString());
   descriptor.append(1,'\0');

   // now the data values for each record in order:
   for (unsigned int ix = 0; ix < fInfoDescr.NumLongRecords(); ++ix)
   {
      long tmp=0;
      if (CA_GetLong((char*) fInfoDescr.GetLongRecord(ix), tmp) != 0)
         continue;
      int64_t val=tmp; // machine independent representation here
      DOUT3(("EpicsInput LongRecord:%s - val= %d ",fInfoDescr.GetLongRecord(ix), val));
      DOUT3(("EpicsInput:Got pointer after shift:0x%x",ptr()));
      *((int64_t*) ptr()) = val;
      ptr.shift(sizeof(val));
      rawlen += sizeof(val);
      totallen += sizeof(val);

      // record the name of just written process variable:
      descriptor.append(std::string(fInfoDescr.GetLongRecord(ix)));
      descriptor.append(1,'\0');
   }

   // header with number of double records
   *((unsigned int64_t*) ptr()) = fInfoDescr.NumDoubleRecords();
        ptr.shift(sizeof(unsigned int64_t));
        rawlen += sizeof(unsigned int64_t);
        totallen += sizeof(unsigned int64_t);


   // put number of double into variable description:
     descriptor.append(fInfoDescr.NumDoubleRecordsString());
     descriptor.append(1,'\0');

   // data values for double records:
   for (unsigned int ix = 0; ix < fInfoDescr.NumDoubleRecords(); ++ix)
   {
      double val = 0;
      if (CA_GetDouble((char*) fInfoDescr.GetDoubleRecord(ix), val) != 0)
         continue;
      //val=3.14159e-42; // for testing
      DOUT3(("EpicsInput DoubleRecord:%s - val= %f ",fInfoDescr.GetDoubleRecord(ix), val));
      DOUT3(("EpicsInput:Got pointer after shift:0x%x",ptr()));
#ifdef EZCA_useDOUBLES
      double tmpval=val; // should be always 8 bytes
      *((double*) ptr()) = tmpval;
#else
      // workaround: instead of doubles, we store integers scaled by a factor JAM
      int64_t tmpval= val;
      tmpval *= int64_t(EZCA_DOUBLESCALE);
      *((int64_t*) ptr()) = tmpval;
#endif
      ptr.shift(sizeof(tmpval));
      rawlen += sizeof(tmpval);
      totallen += sizeof(tmpval);
      // record the name of just written process variable:
      descriptor.append(std::string(fInfoDescr.GetDoubleRecord(ix)));
      descriptor.append(1,'\0');

   }

   // TODO: add other record types here?

   // copy description of record names at subevent end:

   size_t textlen=descriptor.size();
   DOUT3(("EpicsInput:Copied description of length %d\n",textlen));
   ptr.copyfrom((void*) descriptor.c_str(), textlen);
   ptr.shift(textlen);
   rawlen += textlen;
   totallen += textlen;

   // finalize this subevent:
   int irawlen=rawlen/sizeof(int);
   if(irawlen*sizeof(int)<rawlen)
      rawlen=(irawlen+1)*sizeof(int);

   int itotallen=totallen/sizeof(int);
      if(itotallen*sizeof(int)<totallen)
         totallen=(itotallen+1)*sizeof(int);

   subhdr->SetRawDataSize(rawlen);
   evhdr->SetSubEventsSize(sizeof(mbs::SubeventHeader) + rawlen);
   buf->SetTypeId(mbs::mbt_MbsEvents);
   DOUT3(("EpicsInput:: totallen = %d, evnum=%d",totallen,evhdr->iEventNumber));
   buf->SetDataSize(totallen);
   return totallen > 0 ? dabc::di_Ok : dabc::di_RepeatTimeOut;
}


int ezca::EpicsInput::CA_GetLong(char* name, long& val)
{
   int rev=ezcaGet(name, ezcaLong, 1, &val);
   if(rev!=EZCA_OK)
   {
      EOUT(("EpicsInput:: Error %d in ezcaGet for record %s",rev, name));
   }
   DOUT3(("EpicsInput::CA_GetLong(%s)=%d",name,val));
   return rev;
}

int ezca::EpicsInput::CA_GetDouble(char* name, double& val)
{
   int rev=ezcaGet(name, ezcaDouble, 1, &val);
   if(rev!=EZCA_OK)
   {
      EOUT(("EpicsInput:: Error %d in ezcaGet for record %s",rev, name));
   }
   DOUT3(("EpicsInput::CA_GetDouble(%s)=%d",name,val));
   return rev;
}

