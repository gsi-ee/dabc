// $Id$
// Author: Sergey Linev   21/12/2013

#ifndef ROOT_THttpServer
#define ROOT_THttpServer

#ifndef ROOT_TNamed
#include "TNamed.h"
#endif

#ifndef ROOT_TList
#include "TList.h"
#endif

#ifndef ROOT_TMutex
#include "TMutex.h"
#endif

#ifndef ROOT_TCondition
#include "TCondition.h"
#endif


// this class is used to deliver http request arguments to main process
// and also to return back results of processing


class THttpEngine;
class THttpTimer;
class TRootSniffer;
class THttpServer;


class THttpCallArg : public TObject {

protected:
   friend class THttpServer;

   TString fTopName;            //! top item name
   TString fPathName;           //! item path
   TString fFileName;           //! file name
   TString fQuery;              //! additional arguments

   TCondition fCond;            //! condition used to wait for processing

   TString fContentType;        //! type of content
   TString fHeader;             //! response header like ContentEncoding, Cache-Control and so on
   TString fContent;            //! text content (if any)
   Int_t   fZipping;            //! indicate if content should be zipped

   void *fBinData;              //! binary data, assigned with http call
   Long_t fBinDataLength;       //! length of binary data

   Bool_t IsBinData() const
   {
      return fBinData && fBinDataLength > 0;
   }

public:

   THttpCallArg();
   ~THttpCallArg();

   // these methods used to set http request arguments

   void SetTopName(const char *topname)
   {
      fTopName = topname;
   }
   void SetPathAndFileName(const char *fullpath);
   void SetPathName(const char *p)
   {
      fPathName = p;
   }
   void SetFileName(const char *f)
   {
      fFileName = f;
   }
   void SetQuery(const char *q)
   {
      fQuery = q;
   }
   const char *GetTopName() const
   {
      return fTopName.Data();
   }
   const char *GetPathName() const
   {
      return fPathName.Data();
   }
   const char *GetFileName() const
   {
      return fFileName.Data();
   }
   const char *GetQuery() const
   {
      return fQuery.Data();
   }

   // these methods used in THttpServer to set results of request processing

   void SetContentType(const char *typ)
   {
      fContentType = typ;
   }
   void Set404()
   {
      SetContentType("_404_");
   }

   // indicate that http request should response with file content
   void SetFile(const char* filename = 0)
   {
      SetContentType("_file_");
      if (filename!=0) fContent = filename;
   }

   void SetXml()
   {
      SetContentType("text/xml");
   }

   void SetJson()
   {
      SetContentType("application/json");
   }

   void AddHeader(const char* name, const char* value)
   {
      fHeader.Append(TString::Format("%s: %s\r\n", name, value));
   }

   // Set encoding like gzip
   void SetEncoding(const char *typ)
   {
      AddHeader("Content-Encoding", typ);
   }

   // Set content directly
   void SetContent(const char *c)
   {
      fContent = c;
   }

   // Compress content with gzip, provides appropriate header
   Bool_t CompressWithGzip();

   // Set kind of content zipping
   // 0 - none
   // 1 - only when supported in request header
   // 2 - if supported and content size bigger than 10K
   // 3 - always
   void SetZipping(Int_t kind)
   {
      fZipping = kind;
   }

   // return zipping
   Int_t GetZipping() const
   {
      return fZipping;
   }

   void SetExtraHeader(const char* name, const char* value)
   {
      AddHeader(name, value);
   }

   // Fill http header
   void FillHttpHeader(TString &buf, const char* header = 0);

   // these methods used to return results of http request processing

   Bool_t IsContentType(const char *typ) const
   {
      return fContentType == typ;
   }

   Bool_t Is404() const
   {
      return IsContentType("_404_");
   }

   Bool_t IsFile() const
   {
      return IsContentType("_file_");
   }

   const char *GetContentType() const
   {
      return fContentType.Data();
   }

   void SetBinData(void* data, Long_t length);

   Long_t GetContentLength() const
   {
      return IsBinData() ? fBinDataLength : fContent.Length();
   }

   const void *GetContent() const
   {
      return IsBinData() ? fBinData : fContent.Data();
   }

   ClassDef(THttpCallArg, 0) // Arguments for single HTTP call
};


class THttpServer : public TNamed {

protected:

   TList        fEngines;     //! engines which runs http server
   THttpTimer  *fTimer;       //! timer used to access main thread
   TRootSniffer *fSniffer;    //! sniffer provides access to ROOT objects hierarchy

   Long_t       fMainThrdId;  //! id of the main ROOT process

   TString      fJsRootSys;   //! location of Root JS (if any)
   TString      fTopName;     //! name of top folder, default - "ROOT"

   TString      fDefaultPage; //! file name for default page name
   TString      fDefaultPageCont; //! content of the file content
   TString      fDrawPage;    //! file name for drawing of single element
   TString      fDrawPageCont; //! content of draw page

   TMutex       fMutex;       //! mutex to protect list with arguments
   TList        fCallArgs;    //! submitted arguments

   // Here any request can be processed
   virtual void ProcessRequest(THttpCallArg *arg);

   static Bool_t VerifyFilePath(const char* fname);

public:

   THttpServer(const char *engine = "civetweb:8080");
   virtual ~THttpServer();

   Bool_t CreateEngine(const char *engine);

   TRootSniffer *GetSniffer() const
   {
      return fSniffer;
   }

   void SetSniffer(TRootSniffer *sniff);

   void SetTopName(const char *top)
   {
      fTopName = top;
   }

   const char *GetTopName() const
   {
      return fTopName.Data();
   }

   void SetTimer(Long_t milliSec = 100, Bool_t mode = kTRUE);

   /** Check if file is requested, thread safe */
   Bool_t  IsFileRequested(const char *uri, TString &res) const;

   /** Execute HTTP request */
   Bool_t ExecuteHttp(THttpCallArg *arg);

   /** Process submitted requests, must be called from main thread */
   void ProcessRequests();

   /** Register object in subfolder */
   Bool_t Register(const char *subfolder, TObject *obj);

   /** Unregister object */
   Bool_t Unregister(TObject *obj);

   /** Guess mime type base on file extension */
   static const char *GetMimeType(const char *path);

   /** Reads content of file from the disk */
   static char* ReadFileContent(const char* filename, Int_t& len);

   ClassDef(THttpServer, 0) // HTTP server for ROOT analysis
};

#endif
