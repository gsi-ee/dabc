/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#ifndef DABC_Basic
#define DABC_Basic

#ifndef DABC_string
#include "dabc/string.h"
#endif

#ifndef DABC_ConfigIO
#include "dabc/ConfigIO.h"
#endif

namespace dabc {

   class Manager;
   class CommandReceiver;
   class Folder;
   class Mutex;
   class Configuration;

   extern const char* clPoolHandle;
   extern const char* clMemoryPool;

   extern const char* xmlInputQueueSize;
   extern const char* xmlOutputQueueSize;

   extern const char* xmlPoolName;
   extern const char* xmlWorkPool;
   extern const char* xmlFixedLayout;
   extern const char* xmlSizeLimitMb;
   extern const char* xmlCleanupTimeout;
   extern const char* xmlBufferSize;
   extern const char* xmlNumIncrement;
   extern const char* xmlNumBuffers;
   extern const char* xmlHeaderSize;
   extern const char* xmlNumSegments;
   extern const char* xmlAlignment;

   extern const char* xmlNumInputs;
   extern const char* xmlNumOutputs;
   extern const char* xmlUseAcknowledge;
   extern const char* xmlFlashTimeout;
   extern const char* xmlTrThread;

   extern const char* xmlTrueValue;
   extern const char* xmlFalseValue;

   extern const char* typeWorkingThread;
   extern const char* typeSocketDevice;
   extern const char* typeSocketThread;
   extern const char* typeNullTransport;
   extern const char* typeApplication;

   class Basic {
      friend class Folder;
      friend class Manager;

      public:
         Basic(Basic* parent, const char* name);
         virtual ~Basic();

         // these three methods describes look of element in xml file
         virtual const char* MasterClassName() const { return "Basic"; }
         virtual const char* ClassName() const { return "Basic"; }
         virtual bool UseMasterClassName() const { return false; }

         // here we listed all thread-safe methods

         inline Mutex* GetMutex() const { return fMutex; }
         inline Basic* GetParent() const { return fParent; }
         virtual CommandReceiver* GetCmdReceiver() { return 0; }

         virtual void AddChild(Basic* obj);
         virtual void RemoveChild(Basic* obj);

         virtual void DependendDestroyed(Basic*) {}

         void MakeFullName(std::string &fullname, Basic* upto = 0) const;
         std::string GetFullName(Basic* upto = 0) const;

         // operations with object name (and info) are not thread-safe
         // therefore, in the case when object name must be changed,
         // locking should be applied by any other means

         const char* GetName() const { return fName.c_str(); }
         bool IsName(const char* str) const { return fName.compare(str)==0; }
         virtual void FillInfo(std::string& info);

         int GetAppId() const { return fAppId; }
         void SetAppId(int id = 0) { fAppId = id; }

         virtual bool Store(ConfigIO &cfg) { return true; }
         virtual bool Find(ConfigIO &cfg);
         virtual bool Read(ConfigIO &cfg) { return true; }

         static long NumInstances() { return gNumInstances; }

      protected:
         void SetName(const char* name);
         void DeleteThis();

         virtual void _SetParent(Mutex* mtx, Basic* parent);

         void _MakeFullName(std::string &fullname, Basic* upto) const;

      private:
         Mutex*        fMutex;    // direct pointer on main mutex
         Basic*        fParent;   // parent object
         std::string   fName;     // object name
         bool          fCleanup;  // indicates if object should inform manager about its destroyment
         int           fAppId;    // indicates id of application, to which object belongs to (0 - default application)

         static long gNumInstances;
   };
};

#endif
