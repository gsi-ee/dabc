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

#include "mbs/TextInput.h"

#include <string.h>
#include <stdlib.h>
#include <sstream>

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "dabc/Manager.h"
#include "dabc/BinaryFile.h"

#include "mbs/MbsTypeDefs.h"
#include "mbs/Iterator.h"

mbs::TextInput::TextInput(const dabc::Url& url) :
   dabc::FileInput(url),
   fDataFormat("uint32_t"),
   fNumData(8),
   fNumHeaderLines(1),
   fCharBufferLength(1024),
   fFile(),
   fCharBuffer(0),
   fEventCounter(0),
   fFormatId(0),
   fDataUnitSize(4)
{
   fDataFormat = url.GetOptionStr("format", fDataFormat);
   fNumData = url.GetOptionInt("numdata", fNumData);
   fNumHeaderLines = url.GetOptionInt("header", fNumHeaderLines);
   fCharBufferLength = url.GetOptionInt("buflen", fCharBufferLength);
}

mbs::TextInput::~TextInput()
{
   CloseFile();

   delete [] fCharBuffer; fCharBuffer = 0;
}

bool mbs::TextInput::Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   if (!dabc::FileInput::Read_Init(wrk, cmd)) return false;

   fDataFormat = wrk.Cfg(mbs::xmlTextDataFormat, cmd).AsStr(fDataFormat);
   fNumData = wrk.Cfg(mbs::xmlTextNumData, cmd).AsInt(fNumData);
   fNumHeaderLines = wrk.Cfg(mbs::xmlTextHeaderLines, cmd).AsInt(fNumHeaderLines);
   fCharBufferLength = wrk.Cfg(mbs::xmlTextCharBuffer, cmd).AsInt(fCharBufferLength);

   if (fDataFormat=="float") { fFormatId = 0; fDataUnitSize = sizeof(float); } else
   if (fDataFormat=="int32_t") { fFormatId = 1; fDataUnitSize = 4; } else
   if (fDataFormat=="int") { fFormatId = 1; fDataUnitSize = 4; } else
   if (fDataFormat=="uint32_t") { fFormatId = 2; fDataUnitSize = 4; } else
   if (fDataFormat=="unsigned") { fFormatId = 2; fDataUnitSize = 4; } else {
      EOUT("Unsupported data format %s", fDataFormat.c_str());
      return false;
   }

   if (fCharBufferLength < 1024) fCharBufferLength = 1024;
   fCharBuffer = new char[fCharBufferLength];

   return OpenNextFile();
}

bool mbs::TextInput::OpenNextFile()
{
   CloseFile();

   if (!TakeNextFileName()) return false;

   fFile.open(CurrentFileName().c_str());

   int cnt = fNumHeaderLines;
   while (cnt-- > 0) fFile.getline(fCharBuffer, fCharBufferLength);

   if (!fFile.good()) {
      EOUT("Cannot open file %s for reading", CurrentFileName().c_str());
      fFile.close();
      return false;
   }

   return true;
}

bool mbs::TextInput::CloseFile()
{
	if (fFile.is_open()) fFile.close();
	ClearCurrentFileName();
   return true;
}

unsigned mbs::TextInput::Read_Size()
{
   // get size of the buffer which should be read from the file

   if (!fFile.good())
      if (!OpenNextFile()) return dabc::di_EndOfStream;

   return dabc::di_DfltBufSize;
}

unsigned mbs::TextInput::Read_Complete(dabc::Buffer& buf)
{
	unsigned rawdatasize = RawDataSize();

   mbs::WriteIterator iter(buf);
   while (iter.NewEvent(fEventCounter)) {
   	// check if we have enough place for next subevent,
      if (!iter.NewSubevent(rawdatasize, 1, 0)) break;

      // fill raw data iter.rawdata() with 0
      memset(iter.rawdata(), 0, rawdatasize);

      // read next nonempty line into buffer

      const char* sbuf = 0;
      do {
         if (fFile.eof())
         	if (!OpenNextFile()) return dabc::di_EndOfStream;

         *fCharBuffer = 0;

      	fFile.getline(fCharBuffer, fCharBufferLength);

      	if ((strlen(fCharBuffer) == 0) && fFile.eof()) {
      		DOUT1("empty line in end of file");
      		continue;
      	}

         if (fFile.bad()) {
          	EOUT("File: %s reading error", CurrentFileName().c_str());
         	return dabc::di_Error;
         }

         sbuf = fCharBuffer;
         while ((*sbuf!=0) && ((*sbuf==' ') || (*sbuf=='\t'))) sbuf++;

         if (strlen(sbuf)==0) DOUT1("Empty string eof fail = %d %d", fFile.eof(), fFile.fail());

      } while (strlen(sbuf)==0);

      unsigned filledsize = FillRawData(fCharBuffer, iter.rawdata(), rawdatasize);
      if (filledsize==0) return dabc::di_Error;

      iter.FinishSubEvent(filledsize);

      if (!iter.FinishEvent()) {
      	EOUT("Problem with iterator");
      	return dabc::di_Error;
      }

      fEventCounter++;

      if (fFile.eof())
      	if (!OpenNextFile()) return dabc::di_EndOfStream;
   }

   buf = iter.Close();

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
              EOUT("Fail to decode stream into float, cnt = %d", n);
              EOUT("Error Line %s", str);
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
            EOUT("Fail to decode stream into int32_t, cnt = %d", n);
            EOUT("Error Line %s", str);
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
            EOUT("Fail to decode stream into uint32_t, cnt = %d", n);
            EOUT("Error Line %s", str);
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
