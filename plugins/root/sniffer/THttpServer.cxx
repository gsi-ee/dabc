// $Id$
// Author: Sergey Linev   21/12/2013

#include "THttpServer.h"

#include "THttpEngine.h"
//#include "TMongoose.h"
//#include "TFastCgi.h"
#include "TRootSniffer.h"
#include "TTimer.h"
#include "TSystem.h"
#include "TImage.h"

#include <string>
#include <cstdlib>

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// THttpCallArg                                                         //
//                                                                      //
// Contains arguments for single HTTP call                              //
// Must be used in THttpEngine to process icomming http requests        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


THttpCallArg::THttpCallArg() :
   TObject(),
   fPathName(),
   fFileName(),
   fQuery(),
   fCond(),
   fContentType(),
   fContent(),
   fBinData(0),
   fBinDataLength(0)
{
   // constructor
}

THttpCallArg::~THttpCallArg()
{
   // destructor

   if (fBinData) {
      free(fBinData);
      fBinData = 0;
   }
}

void THttpCallArg::SetPathAndFileName(const char* fullpath)
{
   // set complete path of requested http element
   // For instance, it could be "/folder/subfolder/get.bin"
   // Here "/folder/subfolder/" is element path and "get.bin" requested file.
   // One could set path and file name separately

   fPathName.Clear();
   fFileName.Clear();

   if (fullpath==0) return;

   const char* rslash = strrchr(fullpath,'/');
   if (rslash==0) {
      fFileName = fullpath;
   } else {
      fPathName.Append(fullpath, rslash - fullpath);
      if (fPathName=="/") fPathName.Clear();
      fFileName = rslash+1;
   }
}


// ====================================================================

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// THttpTimer                                                           //
//                                                                      //
// Specialized timer for THttpServer                                    //
// Main aim - provide regular call of THttpServer::ProcessRequests()    //
// method                                                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


class THttpTimer : public TTimer {
   public:

      THttpServer* fServer;

      THttpTimer(Long_t milliSec, Bool_t mode, THttpServer* serv) :
         TTimer(milliSec, mode),
         fServer(serv)
      {
         // construtor
      }

      virtual ~THttpTimer()
      {
         // destructor
      }

      virtual void Timeout()
      {
         // timeout handler
         // used to process http requests in main ROOT thread

         if (fServer) fServer->ProcessRequests();
      }

};

// =======================================================

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// THttpServer                                                          //
//                                                                      //
// Online server for arbitrary ROOT analysis                            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

THttpServer::THttpServer(const char* engine) :
   TNamed("http","ROOT http server"),
   fEngines(),
   fTimer(0),
   fSniffer(0),
   fMainThrdId(0),
   fHttpSys(),
   fRootSys(),
   fJSRootIOSys(),
   fMutex(),
   fCallArgs()
{
   // constructor

   // Checks where ROOT sources (via $ROOTSYS variable)
   // Sources are required to locate files and scripts,
   // which will be provided to the web clients by request

   fMainThrdId = TThread::SelfId();

   const char* rootsys = gSystem->Getenv("ROOTSYS");
   fRootSys = (rootsys!=0) ? rootsys : ".";

   fHttpSys = TString::Format("%s/net/http", fRootSys.Data());

   const char* jsrootiosys = gSystem->Getenv("JSROOTIOSYS");
   if (jsrootiosys!=0)
      fJSRootIOSys = jsrootiosys;
   else
      fJSRootIOSys = fHttpSys + "/JSRootIO";

   fDefaultPage = fHttpSys + "/files/main.htm";
   fDrawPage = fHttpSys + "/files/single.htm";

   fSniffer = new TRootSniffer("sniff");

   // start timer
   SetTimer(100, kTRUE);

   CreateEngine(engine);
}

THttpServer::~THttpServer()
{
   // destructor
   // delete all http engines and sniffer

   fEngines.Delete();

   if (fSniffer) delete fSniffer;
   fSniffer = 0;

   SetTimer(0);
}


Bool_t THttpServer::CreateEngine(const char* engine)
{
   // factory method to create different http engine
   // At the moment two engine kinds are supported:
   //  mongoose (default) and fastcgi
   // Examples:
   //   "mongoose:8090" or ":8090" - creates mongoose web server with http port 8090
   //   "fastcgi:9000" - creates fastcgi server with port 9000

   if (engine==0) return kFALSE;

   THttpEngine* eng = 0;

   //if (*engine == ':') eng = new TMongoose(this, engine); else
   //if (strncmp(engine,"mongoose:", 9)==0) eng = new TMongoose(this, engine+8); else
   //if (strncmp(engine,"fastcgi:", 8)==0) eng = new TFastCgi(this, engine+7);

   if (eng==0) return kFALSE;


   if (!eng->IsReady()) {
      delete eng;
      return kFALSE;
   }

   fEngines.Add(eng);
   return kTRUE;
}


void THttpServer::SetTimer(Long_t milliSec, Bool_t mode)
{
   // create timer which will invoke ProcessRequests() function periodically
   // Timer is required to perform all actions in main ROOT thread
   // Method arguments are the same as for TTimer constructor
   // By default, sync timer with 100 ms period is created
   //
   // If milliSec == 0, no timer will be created.
   // In this case application should regularly call ProcessRequests() method.

   if (fTimer) {
      fTimer->Stop();
      delete fTimer;
      fTimer = 0;
   }
   if (milliSec>0) {
      fTimer = new THttpTimer(milliSec, mode, this);
      fTimer->TurnOn();
   }
}


Bool_t THttpServer::IsFileRequested(const char* uri, TString& res) const
{
   // verifies that request just file name
   // File names typically contains prefix like "httpsys/" or "jsrootiosys/"
   // If true, method returns real name of the file,
   // which should be delivered to the client
   // Method is thread safe and can be called from any thread

   if ((uri==0) || (strlen(uri)==0)) return kFALSE;

   std::string fname = uri;
   size_t pos = fname.rfind("httpsys/");
   if (pos!=std::string::npos) {
      fname.erase(0, pos+7);
      res = fHttpSys + fname.c_str();
      return kTRUE;
   }

   if (!fRootSys.IsNull()) {
      pos = fname.rfind("rootsys/");
      if (pos!=std::string::npos) {
         fname.erase(0, pos+7);
         res = fRootSys + fname.c_str();
         return kTRUE;
      }
   }

   if (!fJSRootIOSys.IsNull()) {
      pos = fname.rfind("jsrootiosys/");
      if (pos!=std::string::npos) {
         fname.erase(0, pos+11);
         res = fJSRootIOSys + fname.c_str();
         return kTRUE;
      }
   }

   return kFALSE;
}

Bool_t THttpServer::ExecuteHttp(THttpCallArg* arg)
{
   // Executes http request, specified in THttpCallArg structure
   // Method can be called from any thread
   // Actual execution will be done in main ROOT thread, where analysis code is running.

   if (fMainThrdId == TThread::SelfId()) {
      // should not happen, but one could process requests directly without any signaling

      ProcessRequest(arg);

      return kTRUE;
   }

   // add call arg to the list
   fMutex.Lock();
   fCallArgs.Add(arg);
   fMutex.UnLock();

   // and now wait until request is processed
   arg->fCond.Wait();

   return kTRUE;
}


void THttpServer::ProcessRequests()
{
   // Process requests, submitted for execution
   // Regularly invoked by THttpTimer, when somewhere in the code
   // gSystem->ProcessEvents() is called.
   // User can call serv->ProcessRequests() directly, but only from main analysis thread.

   if (fMainThrdId != TThread::SelfId()) {
      Error("ProcessRequests", "Should be called only from main ROOT thread");
      return;
   }

   while (true) {
      THttpCallArg* arg = 0;

      fMutex.Lock();
      if (fCallArgs.GetSize()>0) {
         arg = (THttpCallArg*) fCallArgs.First();
         fCallArgs.RemoveFirst();
      }
      fMutex.UnLock();

      if (arg==0) return;

      ProcessRequest(arg);

      arg->fCond.Signal();
   }

   // regularly call Process() method of engine to let perform actions in ROOT context
   TIter iter(&fEngines);
   THttpEngine* engine = 0;
   while ((engine = (THttpEngine*)iter()) != 0)
      engine->Process();
}

void THttpServer::ProcessRequest(THttpCallArg* arg)
{
   // Process single http request
   // Depending from requested path and filename different actions will be performed.
   // In most cases information is provided by TRootSniffer class

   if (arg->fFileName.IsNull() || (arg->fFileName == "index.htm")) {

      Bool_t usedefaultpage = kTRUE;

      if (!fSniffer->CanExploreItem(arg->fPathName.Data()) &&
            fSniffer->CanDrawItem(arg->fPathName.Data())) usedefaultpage = kFALSE;

      if (usedefaultpage) arg->fContent = fDefaultPage;
                     else arg->fContent = fDrawPage;

      arg->SetFile();

      return;
   }

   TString buf = arg->fFileName;

   if (IsFileRequested(arg->fFileName.Data(), buf)) {
      arg->fContent = buf;
      arg->SetFile();
      return;
   }

   if ((arg->fFileName == "h.xml") || (arg->fFileName == "get.xml"))  {

      // Info("ProcessRequest", "Produce hierarchy %s", arg->fPathName.Data());

      arg->fContent.Form(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<dabc version=\"2\" xmlns:dabc=\"http://dabc.gsi.de/xhtml\" path=\"%s\">\n", arg->fPathName.Data());

      fSniffer->ScanXml(arg->fPathName.Data(), arg->fContent);

      arg->fContent.Append("</dabc>\n");
      arg->SetXml();

      return;
   }

   if (arg->fFileName == "h.json")  {

      // Info("ProcessRequest", "Produce hierarchy %s", arg->fPathName.Data());

      arg->fContent.Append("{\n");

      fSniffer->ScanJson(arg->fPathName.Data(), arg->fContent);

      arg->fContent.Append("\n}\n");
      arg->SetJson();

      return;
   }


   if (arg->fFileName == "get.bin") {
      if (fSniffer->ProduceBinary(arg->fPathName.Data(), arg->fQuery.Data(), arg->fBinData, arg->fBinDataLength)) {
         arg->SetBin();
         return;
      }
   }

   if (arg->fFileName == "get.png") {
      if (fSniffer->ProduceImage(TImage::kPng, arg->fPathName.Data(), arg->fQuery.Data(), arg->fBinData, arg->fBinDataLength)) {
         arg->SetPng();
         return;
      }
   }

   if (arg->fFileName == "get.jpeg") {
      if (fSniffer->ProduceImage(TImage::kJpeg, arg->fPathName.Data(), arg->fQuery.Data(), arg->fBinData, arg->fBinDataLength)) {
         arg->SetJpeg();
         return;
      }
   }

   arg->Set404();
}


Bool_t THttpServer::Register(const char* subfolder, TObject* obj)
{
   // Register object in folders hierarchy
   //
   // See TRootSniffer::RegisterObject() for more details

   return fSniffer->RegisterObject(subfolder, obj);
}

Bool_t THttpServer::Unregister(TObject* obj)
{
   // Unregister object in folders hierarchy
   //
   // See TRootSniffer::UnregisterObject() for more details

   return fSniffer->UnregisterObject(obj);
}
