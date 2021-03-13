#include "ltsm/FileInterface.h"

#include <cstring>

#include "dabc/Url.h"
#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/Manager.h"

ltsm::FileInterface::FileInterface() :
  dabc::FileInterface(), fSession(0), fMaxFilesPerSession(10), fSessionConnectRetries(5), fIsClosing(false), fSessionFileCount(0), fUseDaysubfolders(false)
#ifdef LTSM_USE_FSD
   ,fUseFileSystemDemon(false), fServernameFSD("lxcopytool01.gsi.de"),fPortFSD(7625),fSessionFSD(0)
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

 if (url.HasOption("ltsmDaysubfolders"))
    {
      fUseDaysubfolders = url.GetOptionBool("ltsmDaysubfolders", fUseDaysubfolders);
      DOUT0("tsm::FileInterface::fopen uses day prefix in path: %d from url options.",fUseDaysubfolders);
    }
  else
    {
      DOUT0("tsm::FileInterface::fopen uses  uses day prefix in path: %d from DEFAULTS.",fUseDaysubfolders);
    }




  if(fSessionFileCount >= fMaxFilesPerSession)
    {
      CloseTSMSession();
    }

  // here optionally modify file path to contain year/day paths:
  std::string fileName=fname;
  // TODO 2020
  if(fUseDaysubfolders)
    {
      // JAM feb-2020: this is similar to the filename evaluation for hld files
      char buf[128];
      struct timeval tv;
      struct tm tm_res;
      gettimeofday(&tv, NULL);
      strftime(buf, 128, "%y/%j/", localtime_r(&tv.tv_sec, &tm_res));
      DOUT0("ltsm::FileInterface uses year day path %s",buf);
      std::string insertpath(buf);

  size_t slash = fileName.rfind("/");
      if (slash == std::string::npos)
   slash=0;
      fileName.insert(slash+1,insertpath);

      DOUT0("ltsm::FileInterface uses new filename %s",fileName.c_str());

     }
  // open session before first file is written, or if we have closed previous session to start tape migration on server
#ifdef LTSM_USE_FSD
  if (fUseFileSystemDemon)
    {
      if (fSessionFSD == 0)
   {
     if(!OpenTSMSession(opt)) return 0;
   } // if fSesssion==0
    }
  else
#endif
    {
      if (fSession == 0)
   {
           if(!OpenTSMSession(opt)) return 0;
        } // if fSesssion==0
    }

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
     rc = fsd_fopen(fFsname.c_str(), (char*) fileName.c_str(),
          (char*) fDescription.c_str(), fSessionFSD);
     if (rc)
       {
         EOUT(
         "Fail to open LTSM file for writing to FSD (returncode:%d), using following arguments"
         "File=%s, FSD Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s",rc,
         fileName.c_str(), fServernameFSD.c_str(), fNode.c_str(),
         fPassword.c_str(), fOwner.c_str(), fFsname.c_str(),
         fDescription.c_str());

         free(fSessionFSD);
         fSessionFSD = 0; // on failure we retry open the session. Or keep it?
         return 0;
       }
       fCurrentFile = fileName.c_str();
       fSessionFileCount++;
       DOUT0("Opened LTSM file (session count %d) for writing to FSD: "
       "File=%s, FSD Servername=%s,  Description=%s", fSessionFileCount, fname,
       fServernameFSD.c_str(), fDescription.c_str());
       return fSessionFSD; // no file structure here, use current session as handle
   }
      else
#endif
   {
     rc = tsm_fopen(fFsname.c_str(), (char*) fileName.c_str(),
          (char*) fDescription.c_str(), fSession);
     if (rc)
       {
         EOUT(
         "Fail to open LTSM file for writing (returncode:%d), using following arguments"
         "File=%s, Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s",rc,
         fileName.c_str(), fServername.c_str(), fNode.c_str(),
         fPassword.c_str(), fOwner.c_str(), fFsname.c_str(),
         fDescription.c_str());

         free(fSession);
         fSession = 0; // on failure we retry open the session. Or keep it?
         return 0;
       }
     fCurrentFile = fileName.c_str();
       fSessionFileCount++;
       DOUT0("Opened LTSM file (session count %d) for writing: "
        "File=%s, Servername=%s,  Description=%s", fSessionFileCount, fileName.c_str(),
       fServername.c_str(), fDescription.c_str());
      return fSession->tsm_file; // pointer to file structure is the handle
   }

    }
  else if (strstr(mode, "r") != 0)
    {
      EOUT("tsm_fread is not yet supported!");
      return 0;
    }
  return 0;
}

int ltsm::FileInterface::GetFileIntPar(Handle, const char* parname)
{
   // TODO: meaningful info for HADES epics display?

   if (strcmp(parname, "RFIO") == 0)
      return -1; // return RFIO version number
   if (strcmp(parname, "DataMoverIndx") == 0)
   {
      std::string suffix = fNode.substr(fNode.size() - 2);
      return std::stoi(suffix); //take first number from node name.
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
    // JAM2020: this one will pass the real filename upward for display in logfile and gui
    if (strcmp(parname, "RealFileName")==0)
     {
       strncpy(sbuf,fCurrentFile.c_str(),sbuflen);
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
  int rc=0;

#ifdef LTSM_USE_FSD
  if (fUseFileSystemDemon)
    {
       struct fsd_session_t* theHandleFSD = (struct fsd_session_t*) f;
       if (theHandleFSD != fSessionFSD)
    {

      EOUT(
      "Inconsistent file  handles (0x%x != 0x%x) when closing LTSM file: "
      "File=%s, FSD Servername=%s, Node=%s, Fsname=%s .... try to close most recent file in session",
      theHandleFSD, fSessionFSD, fCurrentFile.c_str(),
      fServernameFSD.c_str(), fNode.c_str(), fFsname.c_str());
    }
      rc = fsd_fclose(fSessionFSD);
      if (rc)
   {
     EOUT("Failedto close LTSM file on FSD server: "
          "File=%s, FSD Servername=%s, Node=%s, Fsname=%s",
          fCurrentFile.c_str(), fServernameFSD.c_str(), fNode.c_str(),
          fFsname.c_str());
     return;
   }
    }
  else
#endif
    {
      struct tsm_file_t* theHandle = (struct tsm_file_t*) f;
      if (theHandle != fSession->tsm_file)
   {

     EOUT(
          "Inconsistent file  handles (0x%x != 0x%x) when closing LTSM file: "
          "File=%s, Servername=%s, Node=%s, Fsname=%s .... try to close most recent file in session",
          theHandle, fSession->tsm_file, fCurrentFile.c_str(),
          fServername.c_str(), fNode.c_str(), fFsname.c_str());
   }


      rc = tsm_fclose(fSession);

      if (rc)
   {
     EOUT("Failedto close LTSM file: "
          "File=%s, Servername=%s, Node=%s, Fsname=%s",
          fCurrentFile.c_str(), fServername.c_str(), fNode.c_str(),
          fFsname.c_str());
     return;
   }
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

    int rc;
 #ifdef LTSM_USE_FSD
    if (fUseFileSystemDemon)
      {
   struct fsd_session_t* theHandleFSD = (struct fsd_session_t*) f;
   if (theHandleFSD != fSessionFSD)
     {

       EOUT(
       "Inconsistent file  handles (0x%x != 0x%x) when writing to LTSM: "
       "File=%s, FSD Servername=%s, Node=%s, Fsname=%s .... something is wrong!",
       theHandleFSD, fSessionFSD, fCurrentFile.c_str(),
       fServernameFSD.c_str(), fNode.c_str(), fFsname.c_str());
       return 0;
     }
      rc = fsd_fwrite(ptr, sz, nmemb, fSessionFSD);
    }
 else
#endif
   {
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
     rc = tsm_fwrite(ptr, sz, nmemb, fSession);
   }
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
    //TODO: do we have such thing?
    return false;
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
      "Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s Description=%s",
      fServername.c_str(), fNode.c_str(), fPassword.c_str(),
      fOwner.c_str(), fFsname.c_str(), fDescription.c_str());


#ifdef LTSM_USE_FSD
    struct fsd_login_t fsdlogin;
    // TODO: function fsd_login_init
    strncpy(fsdlogin.node,  fNode.c_str(), DSM_MAX_NODE_LENGTH);
    strncpy(fsdlogin.password,  fPassword.c_str(), DSM_MAX_VERIFIER_LENGTH);
    strncpy(fsdlogin.hostname,  fServernameFSD.c_str(), DSM_MAX_NODE_LENGTH);
    fsdlogin.port=fPortFSD;

    fSessionFSD = (struct fsd_session_t*) malloc(sizeof(struct fsd_session_t));
#endif
    struct login_t tsmlogin;
    login_init(&tsmlogin, fServername.c_str(), fNode.c_str(),
               fPassword.c_str(), fOwner.c_str(), LINUX_PLATFORM,
               fFsname.c_str(), DEFAULT_FSTYPE);
    fSession = (struct session_t*) malloc(sizeof(struct session_t));
    if (!fSession) {
       EOUT("Memory allocation error");
       return false;
    }

    int connectcount=0;
    int rc=0;
    while (connectcount++ <fSessionConnectRetries)
   {
#ifdef LTSM_USE_FSD
     if (fUseFileSystemDemon)
       {
    memset(fSessionFSD, 0, sizeof(struct fsd_session_t));
    rc = fsd_fconnect(&fsdlogin, fSessionFSD);
       }
     else
#endif
       {
    memset(fSession, 0, sizeof(struct session_t));
    rc = tsm_fconnect(&tsmlogin, fSession);
       }
     if (rc==0)
       {
    break;
       }
     else
       {
    EOUT("Fail to connect LTSM session (returncode:%d) using following arguments"
         "Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s , retry again %d time...",rc,
         fServername.c_str(), fNode.c_str(), fPassword.c_str(),
         fOwner.c_str(), fFsname.c_str(), connectcount);
    dabc::mgr.Sleep(1);
       }
   } // while

   if (rc)
       {
       EOUT("Finally FAILED to connect LTSM session (returncode%d))after %d retries!! -  using following arguments"
       "Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s", rc, connectcount,
          fServername.c_str(), fNode.c_str(), fPassword.c_str(),
          fOwner.c_str(), fFsname.c_str());
#ifdef LTSM_USE_FSD
       if (fUseFileSystemDemon)
    {
      free(fSessionFSD);
      fSessionFSD=0;
    }
       else
#endif
    {
      free(fSession);
      fSession=0;
    }
       return false;
       } // if(rc)

     DOUT0("Successfully conncted LTSM session with parameter: "
          "Servername=%s, Node=%s, Pass=%s, Owner=%s,Fsname=%s",
          fServername.c_str(), fNode.c_str(), fPassword.c_str(),
          fOwner.c_str(), fFsname.c_str());

   #ifdef LTSM_USE_FSD
     if (fUseFileSystemDemon)
        DOUT0("Using FSD at server %s ,port %d", fServernameFSD.c_str(), fPortFSD);
   #endif

 return true;
}


bool ltsm::FileInterface::CloseTSMSession()
{
#ifdef LTSM_USE_FSD
  if (fUseFileSystemDemon)
    {
      if(fSessionFSD==0) return false;
      fsd_fdisconnect(fSessionFSD);
      free(fSessionFSD);
      fSessionFSD = 0;
    }
  else
#endif
    {
      if(fSession==0) return false;
      tsm_fdisconnect(fSession);
      free(fSession);
      fSession = 0;
    }
  fSessionFileCount=0;
  return true;
}


