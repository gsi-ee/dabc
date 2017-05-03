#include "ltsm/FileInterface.h"



#include <string.h>

#include "dabc/Url.h"
#include "dabc/logging.h"
#include "dabc/timing.h"


ltsm::FileInterface::FileInterface() :
   dabc::FileInterface()
//, fTsmFilehandle(0)
//   fRemote(0),
//   fOpenedCounter(0),
//   fDataMoverIndx(0)
{
 //  memset(fDataMoverName, 0, sizeof(fDataMoverName));
	 DOUT0("tsm::FileInterface::FileInterface() ctor starts...");
	 api_msg_set_level(API_MSG_DEBUG);
	 DOUT0("tsm::FileInterface::FileInterface() ctor set api message level to %d",API_MSG_DEBUG);
	tsm_init(DSM_MULTITHREAD); // do we need multithread here?
	 DOUT0("tsm::FileInterface::FileInterface() ctor leaving...");
}

ltsm::FileInterface::~FileInterface()
{
//	if (fTsmFilehandle!=0) {
//	      DOUT2("Close existing connection to LTSM");
//	      tsm_file_close(fTsmFilehandle);
//	      fTsmFilehandle = 0;
//	}

	// todo: do we need cleanup of all known handles here?

   dsmCleanUp(DSM_MULTITHREAD);

}


dabc::FileInterface::Handle ltsm::FileInterface::fopen(const char* fname, const char* mode, const char* opt)
{

	 //TODO: create on heap struct tsm_filehandle_t fTsmFilehandle;
	 // do the open according to options
	 // return pointer on this handle as dabc Handle
	DOUT0("ltsm::FileInterface::fopen ... ");
	struct tsm_filehandle_t* theHandle=0;
	theHandle= (struct tsm_filehandle_t*) malloc(sizeof(struct tsm_filehandle_t)); // todo: may we use new instead?
	memset(theHandle, 0, sizeof(struct tsm_filehandle_t));
	std::string servername="lxltsm01-tsm-server";
	std::string node="LTSM_TEST01";
	std::string password="LTSM_TEST01";
	std::string owner="";
	std::string fsname=DEFAULT_FSNAME;
	dabc::DateTime dt;

	std::string description=dabc::format("This file was created by DABC ltsm interface at %s",dt.GetNow().AsJSString().c_str());
	//std::string description=dabc::format("This file was created by DABC ltsm interface");
	DOUT0("ltsm::FileInterface::fopen before options with options %s \n",opt);
	 dabc::Url url;
     url.SetOptions(opt);
	 if (url.HasOption("ltsmServer")) {
		 servername = url.GetOptionStr("ltsmServer", servername);
	 }
	 if (url.HasOption("ltsmNode")) {
	 		 node = url.GetOptionStr("ltsmNode", node);
	 	 }
	 if (url.HasOption("ltsmPass")) {
		 password = url.GetOptionStr("ltsmPass", password);
		 	 }
	 if (url.HasOption("ltsmOwner")) {
			  owner= url.GetOptionStr("ltsmOwner", owner);
	 }
	 if (url.HasOption("ltsmFsname")) {
	 			  fsname= url.GetOptionStr("ltsmFsname", fsname);
	 	 }
	 if (url.HasOption("ltsmDescription")) {
		 description= url.GetOptionStr("ltsmDescription", description);
	 	 	 }

	 DOUT0("Prepare open LTSM file for writing -  "
	 				                  "File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s\n",
	 				                  fname, servername.c_str(), node.c_str(), password.c_str(), owner.c_str(),
	 								  fsname.c_str(), description.c_str());

	struct login_t tsmlogin;

	login_fill(&tsmlogin, servername.c_str(),
				   node.c_str(), password.c_str(),
				   owner.c_str(), LINUX_PLATFORM,
				   fsname.c_str(), DEFAULT_FSTYPE);


	if(strstr(mode,"w")!=0)
	{
	int rc = tsm_file_open(theHandle, &tsmlogin, (char*) fname, (char*) description.c_str(), TSM_FILE_MODE_WRITE);
	if (rc) {
			    EOUT("Fail to create LTSM file for writing, using following arguments"
			                  "File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s\n",
			                  fname, servername.c_str(), node.c_str(), password.c_str(), owner.c_str(),
							  fsname.c_str(), description.c_str());
			    free(theHandle);
			    return 0;
		}
	DOUT0("Opened LTSM file for writing: "
				                  "File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s\n",
				                  fname, servername.c_str(), node.c_str(), password.c_str(), owner.c_str(),
								  fsname.c_str(), description.c_str());
	}
	else if(strstr(mode,"r")!=0)
	{
		char descriptionbuffer[DSM_MAX_DESCR_LENGTH + 1]; // avoid that retrieved description probably crashes our strin
		int rc = tsm_file_open(theHandle, &tsmlogin, (char*) fname, descriptionbuffer, TSM_FILE_MODE_READ);
		if (rc) {
					    EOUT("Fail to create LTSM file for reading, using following arguments"
					                  "File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s \n",
					                  fname, servername.c_str(), node.c_str(), password.c_str(), owner.c_str(),
									  fsname.c_str());
					    free(theHandle);
					    return 0;
				}
		DOUT0("Opened LTSM file for reading: "
						                  "File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s\n",
						                  fname, servername.c_str(), node.c_str(), password.c_str(), owner.c_str(),
										  fsname.c_str(), descriptionbuffer);
	}


	//


return theHandle;
}


int ltsm::FileInterface::GetFileIntPar(Handle, const char* parname)
{
	// TODO: meaningful info for HADES epics display?

   //if (strcmp(parname, "RFIO")==0) return 8; // return RFIO version number
   //if (fRemote && strcmp(parname, "DataMoverIndx")==0) return fDataMoverIndx;

	return 0;
}

bool ltsm::FileInterface::GetFileStrPar(Handle, const char* parname, char* sbuf, int sbuflen)
{
	// TODO: meaningful info for HADES epics display?

//   if (fRemote && strcmp(parname, "DataMoverName")==0)
//      if (strlen(fDataMoverName) < (unsigned) sbuflen) {
//         strcpy(sbuf, fDataMoverName);
//         return true;
//      }

   return false;
}


void ltsm::FileInterface::fclose(Handle f)
{
//   if ((fRemote!=0) && (f==fRemote)) {
//      ltsm_fendfile((RFILE*) fRemote);
//      fOpenedCounter--;
//      if (fOpenedCounter < 0) EOUT("Too many close operations - counter (%d) is negative", fOpenedCounter);
//      return;
//   }
//
//   if (fRemote!=0) EOUT("Get RFIO::fclose with unexpected argument when fRemote!=0 cnt %d", fOpenedCounter);
//
//   if (f!=0) ltsm_fclose((RFILE*)f);

	if(f==0) return;
	struct tsm_filehandle_t* theHandle=(tsm_filehandle_t*) f;
	tsm_file_close(theHandle);
	free(theHandle);
	}


size_t ltsm::FileInterface::fwrite(const void* ptr, size_t sz, size_t nmemb, Handle f)
{

   //return ((f==0) || (ptr==0) || (sz==0)) ? 0 : ltsm_fwrite((const char*)ptr, sz, nmemb, (RFILE*) f) / sz;

	if ((f==0) || (ptr==0) || (sz==0)) return 0;
	struct tsm_filehandle_t* theHandle=(tsm_filehandle_t*) f;
	int rc = tsm_file_write(theHandle, (void*) ptr, sz, nmemb);
	if (rc) {
		EOUT("tsm_file_write failed, handle:0x%x, size:%d, nmemb:%d",f, sz,nmemb);
		return 0;
	}
	DOUT2("ltsm::FileInterface::fwrite: successfull - handle:0x%x, size:%d, nmemb:%d",f, sz,nmemb);
	return nmemb; // return value is count of written elements (buffers) - fwrite convention
}

size_t ltsm::FileInterface::fread(void* ptr, size_t sz, size_t nmemb, Handle f)
{
   //return ((f==0) || (ptr==0) || (sz==0)) ? 0 : ltsm_fread((char*) ptr, sz, nmemb, (RFILE*) f) / sz;
	if ((f==0) || (ptr==0) || (sz==0)) return 0;
	struct tsm_filehandle_t* theHandle=(tsm_filehandle_t*) f;
	size_t bytes_read = 0;
	int rc = tsm_file_read(theHandle, ptr, sz,  nmemb, &bytes_read);
	if (rc && rc!=DSM_RC_MORE_DATA) {
		EOUT("tsm_file_read failed with return code %d - handle:0x%x, size:%d, nmemb:%d",rc, f, sz, nmemb);
		return 0;
	}
	DOUT2("ltsm::FileInterface::fread:  read %d bytes - handle:0x%x, size:%d, nmemb:%d",bytes_read, f, sz,nmemb);
	return nmemb; // return value is count of written elements (buffers) - fread convention



}

bool ltsm::FileInterface::fseek(Handle f, long int offset, bool relative)
{
   if (f==0) return false;
   //TODO: do we have such thing

   return false;
//#ifdef ORIGIN_ADSM
//   int fileid = -1;
//   printf("ltsm::FileInterface::fseek not working with original version of ADSM library\n");
//#else
//   int fileid = ltsm_ffileid((RFILE*)f);
//#endif
//
//   if (fileid<0) return false;
//
//   return ltsm_lseek(fileid, offset, relative ? SEEK_CUR : SEEK_SET) >= 0;
}


bool ltsm::FileInterface::feof(Handle f)
{
	  if (f==0) return false;
	  return false;

 //  return f==0 ? false : (ltsm_fendfile((RFILE*)f) > 0);
}

bool ltsm::FileInterface::fflush(Handle f)
{
   if (f==0) return false;

   return true;

   // return f==0 ? false : ::fflush((FILE*)f)==0;
}

dabc::Object* ltsm::FileInterface::fmatch(const char* fmask, bool select_files)
{
   return 0;
}
