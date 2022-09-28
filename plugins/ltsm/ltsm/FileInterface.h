// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#ifndef LTSM_FileInterface
#define LTSM_FileInterface


#ifndef DABC_BinaryFile
#include "dabc/BinaryFile.h"
#endif

// JAM 12-jun-2019 use file system demon branch of ltsm library
#define LTSM_USE_FSD 1

// JAM 17-Sep-2021 - enable compatibility mode for changed api
#define LTSM_NEW_FSDAPI 1

extern "C"
    {
#include "ltsmapi.h"
    }

#ifdef LTSM_USE_FSD
extern "C"
    {
#ifdef LTSM_NEW_FSDAPI

#include "fsqapi.h"

#define fsd_fconnect(a,b) fsq_fconnect(a,b)
#define fsd_fdisconnect(X) fsq_fdisconnect(X)
#define fsd_fopen(a,b,c,d) fsq_fopen(a,b,c,d)
#define fsd_fclose(X) fsq_fclose(X)
#define fsd_fwrite(a,b,c,d) fsq_fwrite(a,b,c,d)

#define fsd_login_t fsq_login_t
#define fsd_session_t fsq_session_t
#define fsd_session fsd_session

#else
#include "fsdapi.h"
#endif

    }
#endif


#ifndef DABC_string
#include "dabc/string.h"
#endif



namespace ltsm {

   class FileInterface: public dabc::FileInterface
	{
    protected:

	struct session_t* fSession; //< keep session open during several files
	std::string fCurrentFile; //< remember last open file, for info output
	std::string fServername;
	std::string fNode;
	std::string fPassword;
	std::string fOwner;
	std::string fFsname;
	std::string fDescription;


	int fMaxFilesPerSession; //< set maximum number of files before re-opening session. For correct migration job on TSM server

	int fSessionConnectRetries;//< number of attempts to connect new tsm session


	bool fIsClosing; //< avoid double fclose on termination by this


	int fSessionFileCount; //< count number of files in current session

	bool fUseDaysubfolders; //< append number of day in year as subfolder if true

#ifdef LTSM_USE_FSD
	bool fUseFileSystemDemon; //< write to file system demon server instead of TSM server

	std::string fServernameFSD; //< name of the file system demon port
	int fPortFSD;  //< port of file system demon connection

	struct fsd_session_t* fSessionFSD; //< keep fsd session open during several files

#ifdef	LTSM_NEW_FSDAPI
	enum fsq_storage_dest_t fFSQdestination;
    //<- may switch here destination for data
// 	enum fsq_storage_dest_t {
//         FSQ_STORAGE_NULL  - null device, just test network,
// 	FSQ_STORAGE_LOCAL      - store to local disk of FSQ
// 	FSQ_STORAGE_LUSTRE     - copy to lustre, but not to tape
// 	FSQ_STORAGE_TSM	       - put to tsm tape archive only
// 	FSQ_STORAGE_LUSTRE_TSM - both lustre and tape - hades production mode
#endif

#endif


	/** Re-open session with parameters specified in dabc options string*/
	bool OpenTSMSession(const char* options);

	/** Shut down current TSM session*/
	bool CloseTSMSession();



    public:

	FileInterface();

	virtual ~FileInterface();

	virtual Handle fopen(const char* fname, const char* mode, const char *opt = nullptr);

	virtual void fclose(Handle f);

	virtual size_t fwrite(const void* ptr, size_t sz, size_t nmemb,
		Handle f);

	virtual size_t fread(void* ptr, size_t sz, size_t nmemb, Handle f);

	virtual bool feof(Handle f);

	virtual bool fflush(Handle f);

	virtual bool fseek(Handle f, long int offset, bool realtive = true);

	virtual dabc::Object* fmatch(const char* fmask,
		bool select_files = true);

	virtual int GetFileIntPar(Handle h, const char* parname);

	virtual bool GetFileStrPar(Handle h, const char* parname, char* sbuf,
		int sbuflen);

	};

    }

#endif
