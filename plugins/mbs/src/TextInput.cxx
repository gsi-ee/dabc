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

#include "mbs/TextInput.h"

#include <string.h>
#include <stdlib.h>
#include <sstream>

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "dabc/FileIO.h"
#include "dabc/Manager.h"
#include "dabc/Port.h"

#include "mbs/MbsTypeDefs.h"
#include "mbs/Iterator.h"

mbs::TextInput::TextInput(const char* fname, uint32_t bufsize) :
   dabc::DataInput(),
   fFileName(fname ? fname : ""),
   fBufferSize(bufsize),
   fDataFormat("uint32_t"),
   fNumData(8),
   fNumHeaderLines(1),
   fCharBufferLength(1024),
   fFilesList(0),
   fCurrentFileName(),
   fFile(),
   fCharBuffer(0),
   fEventCounter(0),
   fFormatId(0),
   fDataUnitSize(4)
{
}

mbs::TextInput::~TextInput()
{
   CloseFile();
   if (fFilesList) {
      delete fFilesList;
      fFilesList = 0;
   }
   delete [] fCharBuffer; fCharBuffer = 0;
}

bool mbs::TextInput::Read_Init(dabc::Command* cmd, dabc::WorkingProcessor* port)
{
   dabc::ConfigSource cfg(cmd, port);

   fFileName = cfg.GetCfgStr(mbs::xmlFileName, fFileName);
   fBufferSize = cfg.GetCfgInt(dabc::xmlBufferSize, fBufferSize);
   fDataFormat = cfg.GetCfgStr(mbs::xmlTextDataFormat, fDataFormat);
   fNumData = cfg.GetCfgInt(mbs::xmlTextNumData, fNumData);
   fNumHeaderLines = cfg.GetCfgInt(mbs::xmlTextHeaderLines, fNumHeaderLines);
   fCharBufferLength = cfg.GetCfgInt(mbs::xmlTextCharBuffer, fCharBufferLength);

   return Init();
}

bool mbs::TextInput::Init()
{
	DOUT0(("File name = %s", fFileName.c_str()));

   if (fFileName.length()==0) return false;

   if (fFilesList!=0) {
      EOUT(("Files list already exists"));
      return false;
   }

   if (fBufferSize==0) {
      EOUT(("Buffer size not specified !!!!"));
      return false;
   }

   if (fDataFormat=="float") { fFormatId = 0; fDataUnitSize = sizeof(float); } else
   if (fDataFormat=="int32_t") { fFormatId = 1; fDataUnitSize = 4; } else
   if (fDataFormat=="int") { fFormatId = 1; fDataUnitSize = 4; } else
   if (fDataFormat=="uint32_t") { fFormatId = 2; fDataUnitSize = 4; } else
   if (fDataFormat=="unsigned") { fFormatId = 2; fDataUnitSize = 4; } else {
   	EOUT(("Unsupported data format %s", fDataFormat.c_str()));
   	return false;
   }

   if (strpbrk(fFileName.c_str(),"*?")!=0)
      fFilesList = dabc::Manager::Instance()->ListMatchFiles("", fFileName.c_str());
   else {
      fFilesList = new dabc::Folder(0, "FilesList", true);
      new dabc::Basic(fFilesList, fFileName.c_str());
   }
   if (fCharBufferLength < 1024) fCharBufferLength = 1024;
   fCharBuffer = new char[fCharBufferLength];

   return OpenNextFile();
}

bool mbs::TextInput::OpenNextFile()
{
   CloseFile();

   if ((fFilesList==0) || (fFilesList->NumChilds()==0)) return false;

   const char* nextfilename = fFilesList->GetChild(0)->GetName();

   fFile.open(nextfilename);

   int cnt = fNumHeaderLines;
   while (cnt-- > 0) fFile.getline(fCharBuffer, fCharBufferLength);

   bool res = fFile.good();

   if (!res) {
      EOUT(("Cannot open file %s for reading", nextfilename));
      fFile.close();
   } else
      fCurrentFileName = nextfilename;

   delete fFilesList->GetChild(0);

   return res;
}

bool mbs::TextInput::CloseFile()
{
	if (fFile.is_open()) fFile.close();
   fCurrentFileName = "";
   return true;
}

unsigned mbs::TextInput::Read_Size()
{
   // get size of the buffer which should be read from the file

   if (!fFile.good())
      if (!OpenNextFile()) return dabc::di_EndOfStream;

   return fBufferSize;
}

unsigned mbs::TextInput::Read_Complete(dabc::Buffer* buf)
{
	unsigned rawdatasize = RawDataSize();

   mbs::WriteIterator iter(buf);
   while (iter.NewEvent(fEventCounter)) {
   	// check if we have enough place for next subevent,
      if (!iter.NewSubevent(rawdatasize, 1, 0)) break;

      // fill raw data iter.rawdata() with 0
      memset(iter.rawdata(), 0, rawdatasize);

      // read next nonempty line into buffer

      const char* buf = 0;
      do {
         if (fFile.eof())
         	if (!OpenNextFile()) return dabc::di_EndOfStream;

         *fCharBuffer = 0;

      	fFile.getline(fCharBuffer, fCharBufferLength);

      	if ((strlen(fCharBuffer) == 0) && fFile.eof()) {
      		DOUT1(("empty line in end of file"));
      		continue;
      	}

         if (fFile.bad()) {
          	EOUT(("File: %s reading error", fCurrentFileName.c_str()));
         	return dabc::di_Error;
         }

         buf = fCharBuffer;
         while ((*buf!=0) && ((*buf==' ') || (*buf=='\t'))) buf++;

         if (strlen(buf)==0) DOUT1(("Empty string eof fail = %d %d", fFile.eof(), fFile.fail()));

      } while (strlen(buf)==0);

      unsigned filledsize = FillRawData(fCharBuffer, iter.rawdata(), rawdatasize);
      if (filledsize==0) return dabc::di_Error;

      iter.FinishSubEvent(filledsize);

      if (!iter.FinishEvent()) {
      	EOUT(("Problem with iterator"));
      	return dabc::di_Error;
      }

      fEventCounter++;

      if (fFile.eof())
      	if (!OpenNextFile()) return dabc::di_EndOfStream;
   }

   return dabc::di_Ok;
}

unsigned mbs::TextInput::RawDataSize()
{
   /** Return raw data size (upper limit), required for single event */

   return fNumData * fDataUnitSize;
}

unsigned mbs::TextInput::FillRawData(const char* str, void* rawdata, unsigned maxsize)
{
   /** Decode raw data from text string into subevent,
    * Return actual filled event size, 0 - error */

   std::istringstream src(str);

   switch (fFormatId) {
      case 0: {
         float* tgt = (float*) rawdata;
         for(int n=0;n<fNumData;n++) {
            src >> *tgt;
            if (src.fail()) {
              EOUT(("Fail to decode stream into float, cnt = %d", n));
              EOUT(("Error Line %s", str));
              return 0;
            }
            tgt++;
         }
         break;
      }

      case 1: {
         int32_t* tgt = (int32_t*) rawdata;
         for(int n=0;n<fNumData;n++) {
          src >> *tgt;
          if (src.fail()) {
            EOUT(("Fail to decode stream into int32_t, cnt = %d", n));
            EOUT(("Error Line %s", str));
            return 0;
          }
          tgt++;
         }
         break;
      }

     case 2: {
        uint32_t* tgt = (uint32_t*) rawdata;
        for(int n=0;n<fNumData;n++) {
          src >> *tgt;
          if (src.fail()) {
            EOUT(("Fail to decode stream into uint32_t, cnt = %d", n));
            EOUT(("Error Line %s", str));
            return 0;
          }
          tgt++;
        }
        break;
     }

     default:
        return 0;
   }

   return maxsize;
}
