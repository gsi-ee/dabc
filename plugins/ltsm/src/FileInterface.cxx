#include "ltsm/FileInterface.h"

#include <string.h>

#include "dabc/Url.h"
#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/Manager.h"

ltsm::FileInterface::FileInterface() :
   dabc::FileInterface(), fSession(0), fMaxFilesPerSession(10), fSessionConnectRetries(5), fIsClosing(false), fSessionFileCount(0)
#ifdef LTSM_USE_FSD
   ,fUseFileSystemDemon(false), fServernameFSD("lxcopytool01.gsi.de"),fPortFSD(7625)
#endif
{
   DOUT3("tsm::FileInterface::FileInterface() ctor starts...");
   api_msg_set_level(API_MSG_ERROR);
   DOUT0("tsm::FileInterface::FileInterface() ctor set api message level to %d",
         API_MSG_ERROR);
   tsm_init (DSM_MULTITHREAD); // do we need multithread here?
   DOUT3("tsm::FileInterface::FileInterface() ctor leaving...");
}

ltsm::FileInterface::~FileInterface()
{
   DOUT3("ltsm::FileInterface::DTOR ... ");
   CloseTSMSession();
   tsm_cleanup (DSM_MULTITHREAD);
}

dabc::FileInterface::Handle ltsm::FileInterface::fopen(const char* fname,
   const char* mode, const char* opt)
    {

    DOUT0("ltsm::FileInterface::fopen with options %s",opt);
    dabc::Url url;
    url.SetOptions(opt);


    //here optionally close current session if we exceed file counter:

    if (url.HasOption("ltsmMaxSessionFiles"))
        {
          fMaxFilesPerSession = url.GetOptionInt("ltsmMaxSessionFiles", fMaxFilesPerSession);
     DOUT0("tsm::FileInterface::fopen uses %d max session files from url options.",fMaxFilesPerSession);
    }
 else
    {
        DOUT0("tsm::FileInterface::fopen uses %d max session files from DEFAULTS.",fMaxFilesPerSession);
    }


    if(fSessionFileCount >= fMaxFilesPerSession)
    {
      CloseTSMSession();
    }

    // open session before first file is written, or if we have closed previous session to start tape migration on server
    if (fSession == 0)
    {
       if(!OpenTSMSession(opt)) return 0;
    } // if fSesssion==0

    // default description is per file, not per session:
    dabc::DateTime dt;
    fDescription = dabc::format(
       "This file was created by DABC ltsm interface at %s",
              dt.GetNow().AsJSString().c_str());


    if (url.HasOption("ltsmDescription"))
           {
           fDescription = url.GetOptionStr("ltsmDescription", fDescription);
           }

    if (strstr(mode, "w") != 0)
   {
     int rc;
 #ifdef LTSM_USE_FSD   
    if (fUseFileSystemDemon)
      { 
      rc = fsd_tsm_fopen(fFsname.c_str(), (char*) fname,
			 (char*) fDescription.c_str(), fSession);
      // kludge for first test JAM:
      fSession->tsm_file=(struct tsm_file_t*)(42);
      }
    else
 #endif
      rc = tsm_fopen(fFsname.c_str(), (char*) fname,
		     (char*) fDescription.c_str(), fSession);
   if (rc)
       {
       EOUT(
          "Fail to open LTSM file for writing, using following arguments"
             "File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s",
          fname, fServername.c_str(), fNode.c_str(),
          fPassword.c_str(), fOwner.c_str(), fFsname.c_str(),
          fDescription.c_str());
       free(fSession);
       fSession = 0; // on failure we retry open the session. Or keep it?
       return 0;
       }
   fSessionFileCount++;
   DOUT0("Opened LTSM file (session count %d) for writing: "
      "File=%s, Servername=%s,  Description=%s", fSessionFileCount, fname,
      fServername.c_str(), fDescription.c_str());
   }
    else if (strstr(mode, "r") != 0)
   {
   EOUT("tsm_fread is not yet supported!");
   return 0;
   }

    fCurrentFile = fname;
    fIsClosing = false;
    return fSession->tsm_file; // pointer to file structure is the handle    
    }

int ltsm::FileInterface::GetFileIntPar(Handle, const char* parname)
{
   // TODO: meaningful info for HADES epics display?

   if (strcmp(parname, "RFIO") == 0)
      return -1; // return RFIO version number
   if (strcmp(parname, "DataMoverIndx") == 0)
   {
      int index = 42;
      std::string suffix = fNode.substr(fNode.size() - 2);
      index = atoi(suffix.c_str());
      return index; //take first number from node name.
      // this works for LTSM_TEST01, but not for lxbkhebe.
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
    DOUT3("ltsm::FileInterface::fclose with handle 0x%x...", f);
    if (f == 0)
   return;
    if (fIsClosing)
   {
   DOUT0(
      "ltsm::FileInterface::fclose is called during closing - ignored!");
   return;
   }
    fIsClosing = true;

   

    int rc;
    struct tsm_file_t* theHandle = (struct tsm_file_t*) f;
 // todo: is this the right file handle here? fSession->tsm_file ?  
    if (theHandle != fSession->tsm_file)
   {

   EOUT(
      "Inconsistent file  handles (0x%x != 0x%x) when closing LTSM file: "
         "File=%s, Servername=%s, Node=%s, Fsname=%s .... try to close most recent file in session",
      theHandle, fSession->tsm_file, fCurrentFile.c_str(),
      fServername.c_str(), fNode.c_str(), fFsname.c_str());
   }

    
 #ifdef LTSM_USE_FSD   
    if (fUseFileSystemDemon)
      rc = fsd_tsm_fclose(fSession);
    else
 #endif      
      rc = tsm_fclose(fSession);

    if (rc)
   {
   EOUT("Failedto close LTSM file: "
      "File=%s, Servername=%s, Node=%s, Fsname=%s",
      fCurrentFile.c_str(), fServername.c_str(), fNode.c_str(),
      fFsname.c_str());
   }
    DOUT0("ltsm::FileInterface has closed file %s", fCurrentFile.c_str());
    fIsClosing = false; // do we need such protection anymore? keep it for signalhandler tests maybe.
    DOUT3("ltsm::FileInterface::fclose END ");
    }

size_t ltsm::FileInterface::fwrite(const void* ptr, size_t sz, size_t nmemb,
   Handle f)
    {

    if ((f == 0) || (ptr == 0) || (sz == 0))
   return 0;

    if (fIsClosing)
   {
   DOUT0(
      "ltsm::FileInterface::fwrite is called during closing - ignored!");
   //tsm_cleanup (DSM_MULTITHREAD); // workaround JAM
   return 0;
   }
    struct tsm_file_t* theHandle = (struct tsm_file_t*) f;
    if (theHandle != fSession->tsm_file)
   {

   EOUT(
      "Inconsistent tsm_file_t handles (0x%x != 0x%x) when writing to LTSM: "
         "File=%s, Servername=%s, Node=%s, Fsname=%s .... something is wrong!",
      theHandle, fSession->tsm_file, fCurrentFile.c_str(),
      fServername.c_str(), fNode.c_str(), fFsname.c_str());
   return 0;
   }
    int rc;
 #ifdef LTSM_USE_FSD   
    if (fUseFileSystemDemon)
      rc = fsd_tsm_fwrite(ptr, sz, nmemb, fSession);
    else
 #endif
      rc = tsm_fwrite(ptr, sz, nmemb, fSession);
    
    if (rc < 0)
   {
     EOUT("tsm_fwrite failed, handle:0x%x, size:%d, nmemb:%d, rc=%d", f, sz, nmemb,rc);
   return 0;
   }

    if (rc != int(sz * nmemb))
   {
   EOUT("tsm_fwrite size mismatch, wrote %d bytes from requested %d bytes",
      rc, sz * nmemb);
   }

    return nmemb; // return value is count of written elements (buffers) - fwrite convention
    }

size_t ltsm::FileInterface::fread(void* ptr, size_t sz, size_t nmemb, Handle f)
    {
    EOUT("ltsm::FileInterface::fread is not yet supported!");
    return 0;

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



bool ltsm::FileInterface::OpenTSMSession(const char* opt)
{
  DOUT3("ltsm::FileInterface::OpenTSMSession ... ");
  dabc::Url url;
  url.SetOptions(opt);
  // open session before first file is written.
  fCurrentFile = "none";
  fServername = "lxltsm01-tsm-server";
  fNode = "LTSM_TEST01";
  fPassword = "LTSM_TEST01";
  fOwner = "";
  fFsname = DEFAULT_FSNAME;

   DOUT1("ltsm::FileInterface::fOpenTSMSession before options with options %s", opt);

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

      if (url.HasOption("ltsmSessionConnectRetries"))
        {
          fMaxFilesPerSession = url.GetOptionInt("ltsmMaxSessionFiles", fSessionConnectRetries);
     DOUT0("tsm::FileInterface::OpenTSMSession uses %d session connect retries from url options.",fSessionConnectRetries);
    }
      else
       {
        DOUT0("tsm::FileInterface::OpenTSMSession uses %d session connect retries from DEFAULTS.", fSessionConnectRetries);
       }

#ifdef LTSM_USE_FSD
    if (url.HasOption("ltsmUseFSD"))
        {
          fUseFileSystemDemon = url.GetOptionBool("ltsmUseFSD", fUseFileSystemDemon);
	  DOUT0("tsm::FileInterface::OpenTSMSession will use %s  from url options.",fUseFileSystemDemon ? "file system demon" : "direct TSM connection");
        }
     else
       {
          DOUT0("tsm::FileInterface::OpenTSMSession will use %s  from defaults.",fUseFileSystemDemon ? "file system demon" : "direct TSM connection");
       }

  if (fUseFileSystemDemon)
    {
      if (url.HasOption("ltsmFSDServerName"))
        {
          fServernameFSD = url.GetOptionStr("ltsmFSDServerName", fServernameFSD);
	  DOUT0("tsm::FileInterface::OpenTSMSession with FSD server %s  from url options.",fServernameFSD.c_str());
        }
      else
	{
          DOUT0("tsm::FileInterface::OpenTSMSession with FSD server %s  from defaults.",fServernameFSD.c_str());
	}
      if (url.HasOption("ltsmFSDServerPort"))
        {
          fPortFSD = url.GetOptionInt("ltsmFSDServerPort", fPortFSD);
	  DOUT0("tsm::FileInterface::OpenTSMSession with FSD server port %d  from url options.",fPortFSD);
        }
      else
	{
          DOUT0("tsm::FileInterface::OpenTSMSession with FSD server port %d  from defaults.",fPortFSD);
	}

    } //  if (fUseFileSystemDemon)
#endif    

   DOUT2(
      "Prepare open LTSM file for writing -  "
      "File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s",
      fname, fServername.c_str(), fNode.c_str(), fPassword.c_str(),
      fOwner.c_str(), fFsname.c_str(), fDescription.c_str());

   struct login_t tsmlogin;
#ifdef LTSM_USE_FSD
   fsd_login_fill(&tsmlogin, fServername.c_str(), fNode.c_str(),
      fPassword.c_str(), fOwner.c_str(), LINUX_PLATFORM,
		  fFsname.c_str(), DEFAULT_FSTYPE,
		  fServernameFSD.c_str(), fPortFSD);
#else   
   login_fill(&tsmlogin, fServername.c_str(), fNode.c_str(),
      fPassword.c_str(), fOwner.c_str(), LINUX_PLATFORM,
      fFsname.c_str(), DEFAULT_FSTYPE); 
#endif
   fSession = (struct session_t*) malloc(sizeof(struct session_t)); // todo: may we use new instead?
   int connectcount=0;
   int rc=0;
   while (connectcount++ <fSessionConnectRetries)
   {
       memset(fSession, 0, sizeof(struct session_t));

          
#ifdef LTSM_USE_FSD   
       if (fUseFileSystemDemon)
	 rc = fsd_tsm_fconnect(&tsmlogin, fSession);
       else
#endif
	 rc = tsm_fconnect(&tsmlogin, fSession);

       if (rc==0)
       {
         break;
       }
       else
       {
         EOUT("Fail to connect LTSM session using following arguments"
          "Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s , retry again %d time...",
          fServername.c_str(), fNode.c_str(), fPassword.c_str(),
          fOwner.c_str(), fFsname.c_str(), connectcount);
         dabc::mgr.Sleep(1);
       }

   } // while
   if (rc)
       {
       EOUT("Finally FAILED to connect LTSM session after %d retries!! -  using following arguments"
          "Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s", connectcount,
          fServername.c_str(), fNode.c_str(), fPassword.c_str(),
          fOwner.c_str(), fFsname.c_str());
       free(fSession);
       fSession=0;
       return false;
       }

     DOUT0("Successfully conncted LTSM session with parameter: "
          "Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s",
          fServername.c_str(), fNode.c_str(), fPassword.c_str(),
          fOwner.c_str(), fFsname.c_str());

     #ifdef LTSM_USE_FSD   
       if (fUseFileSystemDemon)
	DOUT0("Using FSD at server %s ,port %d",
          fServernameFSD.c_str(), fPortFSD);
       #endif

 return true;
}


bool ltsm::FileInterface::CloseTSMSession()
{
   if (fSession) {
      //fclose(fSession->tsm_file);
#ifdef LTSM_USE_FSD   
    if (fUseFileSystemDemon)
      fsd_tsm_fdisconnect(fSession);
    else
#endif     
      tsm_fdisconnect(fSession);
    free(fSession);
    fSession = 0;
    fSessionFileCount=0;
    return true;
   }
   return false;
}
