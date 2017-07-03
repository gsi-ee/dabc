#include "ltsm/FileInterface.h"

#include <string.h>

#include "dabc/Url.h"
#include "dabc/logging.h"
#include "dabc/timing.h"

ltsm::FileInterface::FileInterface() :
  dabc::FileInterface(),fIsClosing(false)
    {
    DOUT3("tsm::FileInterface::FileInterface() ctor starts...");
    api_msg_set_level(API_MSG_NORMAL);
    DOUT0(
	    "tsm::FileInterface::FileInterface() ctor set api message level to %d",
	   API_MSG_NORMAL);
    tsm_init (DSM_MULTITHREAD); // do we need multithread here?
    DOUT3("tsm::FileInterface::FileInterface() ctor leaving...");
    }

ltsm::FileInterface::~FileInterface()
    {

    DOUT0("ltsm::FileInterface::DTOR ... ");
    #ifdef LTSM_OLD_FILEAPI
    dsmCleanUp(DSM_MULTITHREAD);
    #else
    tsm_cleanup (DSM_MULTITHREAD);
    #endif

    }

dabc::FileInterface::Handle ltsm::FileInterface::fopen(const char* fname,
	const char* mode, const char* opt)
    {

    //TODO: create on heap struct tsm_filehandle_t fTsmFilehandle;
    // do the open according to options
    // return pointer on this handle as dabc Handle
    DOUT3("ltsm::FileInterface::fopen ... ");


    // workaround for cleanup problem: do init in open, cleanup in close
    //tsm_init (DSM_MULTITHREAD);
    fCurrentFile = "none";
    fServername = "lxltsm01-tsm-server";
    fNode = "LTSM_TEST01";
    fPassword = "LTSM_TEST01";
    fOwner = "";
    fFsname = DEFAULT_FSNAME;
    dabc::DateTime dt;
    fDescription = dabc::format(
	    "This file was created by DABC ltsm interface at %s",
	    dt.GetNow().AsJSString().c_str());

    DOUT3("ltsm::FileInterface::fopen before options with options %s \n", opt);
    dabc::Url url;
    url.SetOptions(opt);
    if (url.HasOption("ltsmServer"))
	{
	fServername = url.GetOptionStr("ltsmServer", fServername);
	}
    if (url.HasOption("ltsmNode"))
	{
	fNode = url.GetOptionStr("ltsmNode", fNode);
	}
    if (url.HasOption("ltsmPass"))
	{
	fPassword = url.GetOptionStr("ltsmPass", fPassword);
	}
    if (url.HasOption("ltsmOwner"))
	{
	fOwner = url.GetOptionStr("ltsmOwner", fOwner);
	}
    if (url.HasOption("ltsmFsname"))
	{
	fFsname = url.GetOptionStr("ltsmFsname", fFsname);
	}
    if (url.HasOption("ltsmDescription"))
	{
	fDescription = url.GetOptionStr("ltsmDescription", fDescription);
	}

    DOUT2(
	    "Prepare open LTSM file for writing -  "
		    "File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s\n",
	    fname, fServername.c_str(), fNode.c_str(), fPassword.c_str(),
	    fOwner.c_str(), fFsname.c_str(), fDescription.c_str());

    struct login_t tsmlogin;

    login_fill(&tsmlogin, fServername.c_str(), fNode.c_str(), fPassword.c_str(),
	    fOwner.c_str(), LINUX_PLATFORM, fFsname.c_str(), DEFAULT_FSTYPE);

#ifdef LTSM_OLD_FILEAPI

    struct tsm_filehandle_t* theHandle=0;
    theHandle= (struct tsm_filehandle_t*) malloc(sizeof(struct tsm_filehandle_t)); // todo: may we use new instead?
    memset(theHandle, 0, sizeof(struct tsm_filehandle_t));

    if(strstr(mode,"w")!=0)
	{
	int rc = tsm_file_open(theHandle, &tsmlogin, (char*) fname, (char*) fDescription.c_str(), TSM_FILE_MODE_WRITE);
	if (rc)
	    {
	    EOUT("Fail to create LTSM file for writing, using following arguments"
		    "File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s\n",
		    fname, fServername.c_str(), fNode.c_str(), fPassword.c_str(), fOwner.c_str(),
		    fFsname.c_str(), fDescription.c_str());
	    free(theHandle);
	    return 0;
	    }
	DOUT0("Opened LTSM file for writing: "
		"File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s\n",
		fname, fServername.c_str(), fNode.c_str(), fPassword.c_str(), fOwner.c_str(),
		fFsname.c_str(), fDescription.c_str());
	}
    else if(strstr(mode,"r")!=0)
	{
	char descriptionbuffer[DSM_MAX_DESCR_LENGTH + 1]; // avoid that retrieved description probably crashes our string
	int rc = tsm_file_open(theHandle, &tsmlogin, (char*) fname, descriptionbuffer, TSM_FILE_MODE_READ);
	if (rc)
	    {
	    EOUT("Fail to create LTSM file for reading, using following arguments"
		    "File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s \n",
		    fname, fServername.c_str(), fNode.c_str(), fPassword.c_str(), fOwner.c_str(),
		    fFsname.c_str());
	    free(theHandle);
	    return 0;
	    }
	DOUT0("Opened LTSM file for reading: "
		"File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s\n",
		fname, fServername.c_str(), fNode.c_str(), fPassword.c_str(), fOwner.c_str(),
		fFsname.c_str(), descriptionbuffer);
	}

    //
#else



    // for the moment, the pointer to the session object is the file handle
    // when we keep the session handle open for subsequent files, we may use the session->tsm_file
    // pointer as dabc handle and keep the session pointer as FileInterface member
    struct session_t* theHandle = 0;
    theHandle = (struct session_t*) malloc(sizeof(struct session_t)); // todo: may we use new instead?
    memset(theHandle, 0, sizeof(struct session_t));
    if (strstr(mode, "w") != 0)
	{
	int rc = tsm_fopen(fFsname.c_str(), (char*) fname,
		(char*) fDescription.c_str(), &tsmlogin, theHandle);
	if (rc)
	    {
	    EOUT(
		    "Fail to create LTSM file for writing, using following arguments"
			    "File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s\n",
		    fname, fServername.c_str(), fNode.c_str(), fPassword.c_str(),
		    fOwner.c_str(), fFsname.c_str(), fDescription.c_str());
	    free(theHandle);
	    return 0;
	    }
	DOUT0(
		"Opened LTSM file for writing: "
			"File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s\n",
		fname, fServername.c_str(), fNode.c_str(), fPassword.c_str(),
		fOwner.c_str(), fFsname.c_str(), fDescription.c_str());
	}
    else if(strstr(mode,"r")!=0)
   	{
	EOUT("tsm_fread is not yet supported!");
   	return 0;
   	}

#endif
    fCurrentFile = fname;
    fIsClosing=false;
    return theHandle;
    }

int ltsm::FileInterface::GetFileIntPar(Handle, const char* parname)
    {
    // TODO: meaningful info for HADES epics display?

    if (strcmp(parname, "RFIO")==0) return -1; // return RFIO version number
    if (strcmp(parname, "DataMoverIndx")==0) {
	int index=42;
	std::string suffix=fNode.substr(fNode.size() - 2);
	index=atoi (suffix.c_str());
	return index; //take first number from node name.
    
}
    return 0;
    }

bool ltsm::FileInterface::GetFileStrPar(Handle, const char* parname, char* sbuf,
	int sbuflen)
    {
    // some info for HADES epics display - backward compatibility
    if (strcmp(parname, "DataMoverName") == 0)
	{
	strncpy(sbuf, fNode.c_str(), sbuflen);
	return true;
	}

    return false;
    }

void ltsm::FileInterface::fclose(Handle f)
    {
      DOUT0("ltsm::FileInterface::fclose with handle 0x%x... ",f);
    if (f == 0)
	return;
    if(fIsClosing)
      {
	 DOUT0("ltsm::FileInterface::fclose is called during closing - ignored!");
	 //tsm_cleanup (DSM_MULTITHREAD); // workaround JAM
	return;
      }
    fIsClosing=true;
#ifdef LTSM_OLD_FILEAPI
    struct tsm_filehandle_t* theHandle=(tsm_filehandle_t*) f;
    tsm_file_close(theHandle);
    free(theHandle);
    //dsmCleanUp(DSM_MULTITHREAD); // workaround JAM
#else

    struct session_t* theHandle = (struct session_t*) f;
    int rc = tsm_fclose(theHandle);
    if (rc)
	{
	EOUT("Failed to close LTSM file: "
		"File=%s, Servername=%s, Node=%s, Fsname=%s \n",
		fCurrentFile.c_str(), fServername.c_str(), fNode.c_str(),
		fFsname.c_str());
	}
    DOUT0("ltsm::FileInterface::fclose after tsm_fclose with rc %d... ",rc);
    free(theHandle);
    //tsm_cleanup (DSM_MULTITHREAD); // workaround JAM

#endif


    DOUT0("ltsm::FileInterface::fclose END ");
    }

size_t ltsm::FileInterface::fwrite(const void* ptr, size_t sz, size_t nmemb,
	Handle f)
    {


    if ((f == 0) || (ptr == 0) || (sz == 0))
	return 0;

     if(fIsClosing)
      {
	 DOUT0("ltsm::FileInterface::fwrite is called during closing - ignored!");
	 //tsm_cleanup (DSM_MULTITHREAD); // workaround JAM
	return 0;
      }


#ifdef LTSM_OLD_FILEAPI
    struct tsm_filehandle_t* theHandle=(tsm_filehandle_t*) f;
    int rc = tsm_file_write(theHandle, (void*) ptr, sz, nmemb);
    if (rc)
	{
	EOUT("tsm_file_write failed, handle:0x%x, size:%d, nmemb:%d",f, sz,nmemb);
	return 0;
	}
    DOUT2("ltsm::FileInterface::fwrite: successfull - handle:0x%x, size:%d, nmemb:%d",f, sz,nmemb);
#else
    struct session_t* theHandle = (struct session_t*) f;
    int rc = tsm_fwrite(ptr, sz, nmemb, theHandle);
    if (rc < 0)
	{
	EOUT("tsm_fwrite failed, handle:0x%x, size:%d, nmemb:%d", f, sz, nmemb);
	return 0;
	}

    if(rc != int(sz*nmemb))
	{
	EOUT("tsm_fwrite size mismatch, wrote %d bytes from requested %d bytes", rc, sz*nmemb);
	}

#endif

    return nmemb; // return value is count of written elements (buffers) - fwrite convention
    }

size_t ltsm::FileInterface::fread(void* ptr, size_t sz, size_t nmemb, Handle f)
    {
    //return ((f==0) || (ptr==0) || (sz==0)) ? 0 : ltsm_fread((char*) ptr, sz, nmemb, (RFILE*) f) / sz;
    if ((f == 0) || (ptr == 0) || (sz == 0))
	return 0;

#ifdef LTSM_OLD_FILEAPI

    struct tsm_filehandle_t* theHandle=(tsm_filehandle_t*) f;
    size_t bytes_read = 0;
    int rc = tsm_file_read(theHandle, ptr, sz, nmemb, &bytes_read);
    if (rc && rc!=DSM_RC_MORE_DATA)
	{
	EOUT("tsm_file_read failed with return code %d - handle:0x%x, size:%d, nmemb:%d",rc, f, sz, nmemb);
	return 0;
	}
    DOUT2("ltsm::FileInterface::fread:  read %d bytes - handle:0x%x, size:%d, nmemb:%d",bytes_read, f, sz,nmemb);

#else
    EOUT("tsm_fread is not yet supported!");
    return 0;
#endif

    return nmemb; // return value is count of written elements (buffers) - fread convention
    }

bool ltsm::FileInterface::fseek(Handle f, long int offset, bool relative)
    {
    if (f == 0)
	return false;
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
    if (f == 0)
	return false;
    return false;

    //  return f==0 ? false : (ltsm_fendfile((RFILE*)f) > 0);
    }

bool ltsm::FileInterface::fflush(Handle f)
    {
    if (f == 0)
	return false;

    return true;

    // return f==0 ? false : ::fflush((FILE*)f)==0;
    }

dabc::Object* ltsm::FileInterface::fmatch(const char* fmask, bool select_files)
    {
    return 0;
    }
