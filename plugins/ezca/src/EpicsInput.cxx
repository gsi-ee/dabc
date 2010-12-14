#include "ezca/EpicsInput.h"

#include "dabc/Port.h"
#include "dabc/Pointer.h"
#include "mbs/MbsTypeDefs.h"
#include "ezca/Definitions.h"

//#include "ezcalib.h"



ezca::EpicsInput::EpicsInput(const char* name, uint32_t bufsize) :
   dabc::DataInput(),
   fName(name ? name : "EpicsMonitorInput"),
   fBufferSize(bufsize),
   fTimeout(1),
   fSubeventId(0)
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
	   {
		   fInfoDescr.AddLongRecord(cfg.GetCfgStr(FORMAT(("%s%d", ezca::xmlNameLongRecords, t)),"dummy"));
	   }
   int numdubs=cfg.GetCfgInt(ezca::xmlNumDoubleRecords, 0);
   for(int t=0;t<numdubs;++t)
	   {
		   fInfoDescr.AddDoubleRecord(cfg.GetCfgStr(FORMAT(("%s%d", ezca::xmlNameDoubleRecords, t)),"dummy"));
	   }


   DOUT1(("EpicsInput %s - BufferSize = %d byte, Timeout = %e s, update flag:%s id:%s",
		   fName.c_str(), fBufferSize, fTimeout,fInfoDescr.GetUpdateRecord(),fInfoDescr.GetIDRecord()));

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
	unsigned int totalsize=sizeof(mbs::EventHeader)+sizeof(mbs::SubeventHeader)+2*sizeof(unsigned int); // header length
	totalsize+= fInfoDescr.NumLongRecords()*sizeof(long)+ fInfoDescr.NumDoubleRecords()*sizeof(double);
	if(totalsize>fBufferSize) totalsize=fBufferSize;
	DOUT3(("EpicsInput::Read_Size-%d",totalsize));
    return totalsize;
}

unsigned ezca::EpicsInput::Read_Complete(dabc::Buffer* buf)
{
	static long evcount=0;
	// first check the flag record:
	long flag=0;
        static long oldflag=-1;
	if(CA_GetLong((char*) fInfoDescr.GetUpdateRecord(), flag) != 0) return dabc::di_RepeatTimeOut;
	DOUT3(("EpicsInput:ReadComplete flag record %s = %d ",fInfoDescr.GetUpdateRecord(), flag));
	if(flag!=0 || flag==oldflag){ 
          oldflag=flag;
          return dabc::di_RepeatTimeOut;
        }
	oldflag=flag;
	long evtnumber=0;
	if(CA_GetLong((char*) fInfoDescr.GetIDRecord(), evtnumber) != 0) return dabc::di_RepeatTimeOut;
	DOUT3(("EpicsInput:ReadComplete id record %s = %d ",fInfoDescr.GetIDRecord(), evtnumber));
	// if this is nonzero, read other records and write buffer
   dabc::Pointer ptr(buf);
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
	// first payload: header with number of long records
	*((unsigned int*) ptr()) = fInfoDescr.NumLongRecords();
	ptr.shift(sizeof(unsigned int));
	rawlen += sizeof(unsigned int);
	totallen += sizeof(unsigned int);

	// now the data values for each record in order:
	for (unsigned int ix = 0; ix < fInfoDescr.NumLongRecords(); ++ix)
	{
		long val = 0;
		if (CA_GetLong((char*) fInfoDescr.GetLongRecord(ix), val) != 0)
			continue;
		DOUT3(("EpicsInput LongRecord:%s - val= %d ",fInfoDescr.GetLongRecord(ix), val));
		*((long*) ptr()) = val;
		ptr.shift(sizeof(val));
		rawlen += sizeof(val);
		totallen += sizeof(val);
	}

	// header with number of double records
	*((unsigned int*) ptr()) = fInfoDescr.NumDoubleRecords();
	ptr.shift(sizeof(unsigned int));
	rawlen += sizeof(unsigned int);
	totallen += sizeof(unsigned int);

	// data values for double records:
	for (unsigned int ix = 0; ix < fInfoDescr.NumDoubleRecords(); ++ix)
	{
		double val = 0;
		if (CA_GetDouble((char*) fInfoDescr.GetDoubleRecord(ix), val) != 0)
			continue;
		DOUT1(("EpicsInput DoubleRecord:%s - val= %f ",fInfoDescr.GetDoubleRecord(ix), val));
		// workaround: instead of doubles, we store integers scaled by factor 1000 JAM
                int tmpval= (int) (val*1000);
                *((int*) ptr()) = tmpval;
		ptr.shift(sizeof(tmpval));
		rawlen += sizeof(tmpval);
		totallen += sizeof(tmpval);
	}

	// TODO: add other record types here?

	// finalize this subevent:
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

