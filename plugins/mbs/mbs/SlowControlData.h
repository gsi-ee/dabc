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

#ifndef MBS_SlowControlData
#define MBS_SlowControlData

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>


namespace mbs {

   /** \brief Record for manipulation with slow control data
    *
    * Used in EPICS, DIM and FESA plugins to deliver data to analysis */

   class SlowControlData {
      protected:

         uint32_t   fEventId;      ///<  event number
         uint32_t   fEventTime;    ///<   unix time in seconds

         std::vector<std::string> fLongRecords;  ///< names of long records
         std::vector<int64_t> fLongValues;       ///< values of long records

         std::vector<std::string> fDoubleRecords; ///< names of double records
         std::vector<double> fDoubleValues;       ///< values of double records

         std::string  fDescriptor;                ///< descriptor

         void BuildDescriptor()
         {
            fDescriptor.clear();

            fDescriptor.append(dabc::format("%u ", (unsigned) fLongRecords.size()));
            fDescriptor.append(1,'\0');

            for (unsigned ix = 0; ix < fLongRecords.size(); ++ix) {
               // record the name of just written process variable:
               fDescriptor.append(fLongRecords[ix]);
               fDescriptor.append(1,'\0');
            }

            fDescriptor.append(dabc::format("%u ", (unsigned) fDoubleRecords.size()));
            fDescriptor.append(1,'\0');

            for (unsigned ix = 0; ix < fDoubleRecords.size(); ix++) {
               // record the name of just written process variable:
               fDescriptor.append(fDoubleRecords[ix]);
               fDescriptor.append(1,'\0');
            }

            while (fDescriptor.size() % 4 != 0) fDescriptor.append(1,'\0');
         }


      public:

         SlowControlData() :
            fEventId(0),
            fEventTime(0),
            fLongRecords(),
            fLongValues(),
            fDoubleRecords(),
            fDoubleValues(),
            fDescriptor()
         {
         }

         virtual ~SlowControlData() {}

         void Clear()
         {
            fEventId = 0;
            fEventTime = 0;
            fLongRecords.clear();
            fLongValues.clear();
            fDoubleRecords.clear();
            fDoubleValues.clear();
            fDescriptor.clear();
         }

         bool Empty() const { return fLongRecords.empty() && fDoubleRecords.empty(); }

         void SetEventId(uint32_t id) { fEventId = id; }
         uint32_t GetEventId() const { return fEventId; }

         void SetEventTime(uint32_t tm) { fEventTime = tm; }
         uint32_t GetEventTime() const { return fEventTime; }

         /** Method add long record. If specified, name duplication will be checked */
         void AddLong(const std::string &name, int64_t value, bool checkduplicate = false)
         {
            if (checkduplicate)
               for (unsigned n=0;n<fLongRecords.size();n++)
                  if (fLongRecords[n]==name) {
                     fLongValues[n] = value;
                     return;
                  }

            fLongRecords.push_back(name);
            fLongValues.push_back(value);
         }

         /** Method add double record. If specified, name duplication will be checked  */
         void AddDouble(const std::string &name, double value, bool checkduplicate = false)
         {
            if (checkduplicate)
               for (unsigned n=0;n<fDoubleRecords.size();n++)
                  if (fDoubleRecords[n]==name) {
                     fDoubleValues[n] = value;
                     return;
                  }
            fDoubleRecords.push_back(name);
            fDoubleValues.push_back(value);
         }

         unsigned NumLongs() const { return fLongRecords.size(); }
         int64_t GetLongValue(unsigned indx) { return indx < fLongValues.size() ? fLongValues[indx] : 0; }
         std::string GetLongName(unsigned indx) { return indx < fLongRecords.size() ? fLongRecords[indx] : std::string(); }

         unsigned NumDoubles() const { return fDoubleRecords.size(); }
         double GetDoubleValue(unsigned indx) { return indx < fDoubleValues.size() ? fDoubleValues[indx] : 0.; }
         std::string GetDoubleName(unsigned indx) { return indx < fDoubleRecords.size() ? fDoubleRecords[indx] : std::string(); }


         unsigned GetRawSize()
         {
            BuildDescriptor();

            return sizeof(uint32_t) +    // event number
                   sizeof(uint32_t) +    // time
                   sizeof(uint64_t) +    // numlongs
                   fLongValues.size()*sizeof(int64_t) + // longs
                   sizeof(uint64_t) +    // numdoubles
                   fDoubleValues.size()*sizeof(double) + // doubles
                   fDescriptor.size();
         }

         unsigned Write(void* buf, unsigned buflen)
         {
            unsigned size = GetRawSize();
            if (size > buflen) return 0;

            char* ptr = (char*) buf;

            memcpy(ptr, &fEventId, sizeof(fEventId)); ptr+=sizeof(fEventId);

            memcpy(ptr, &fEventTime, sizeof(fEventTime)); ptr+=sizeof(fEventTime);

            uint64_t num64 = fLongValues.size();
            memcpy(ptr, &num64, sizeof(num64)); ptr+=sizeof(num64);

            for (unsigned ix = 0; ix < fLongValues.size(); ++ix) {
               int64_t val = fLongValues[ix]; // machine independent representation here
               memcpy(ptr, &val, sizeof(val)); ptr+=sizeof(val);
            }

            // header with number of double records
            num64 = fDoubleValues.size();
            memcpy(ptr, &num64, sizeof(num64)); ptr+=sizeof(num64);

            // data values for double records:
            for (unsigned ix = 0; ix < fDoubleValues.size(); ix++) {
               double val = fDoubleValues[ix]; // should be always 8 bytes
               memcpy(ptr, &val, sizeof(val)); ptr+=sizeof(val);
            }

            // copy description of record names at subevent end:
            memcpy(ptr, fDescriptor.c_str(), fDescriptor.size());

            return size;
         }

         bool Read(void* buf, unsigned bufsize)
         {
            if ((buf==0) || (bufsize<24)) return false;

            Clear();

            char* ptr = (char*) buf;
            char* theEnd = ptr + bufsize;

            memcpy(&fEventId, ptr, sizeof(fEventId)); ptr+=sizeof(fEventId);

            memcpy(&fEventTime, ptr, sizeof(fEventTime)); ptr+=sizeof(fEventTime);

            uint64_t num64 = 0;
            memcpy(&num64, ptr, sizeof(num64)); ptr+=sizeof(num64);
            fLongValues.reserve(num64);
            for (uint64_t n=0;n<num64;n++) {
               int64_t val(0);
               memcpy(&val, ptr, sizeof(val)); ptr+=sizeof(val);
               fLongValues.push_back(val);
            }

            num64 = 0;
            memcpy(&num64, ptr, sizeof(num64)); ptr+=sizeof(num64);
            fDoubleValues.reserve(num64);
            for (uint64_t n=0;n<num64;n++) {
               double val = 0.;
               memcpy(&val, ptr, sizeof(val)); ptr+=sizeof(val);
               fDoubleValues.push_back(val); // should be always 8 bytes
            }

            unsigned num = 0;
            if (sscanf(ptr, "%u", &num)!=1) {
               printf("cannot get number of long names\n");
               return false;
            }
            if (num != fLongValues.size()) {
               printf("mismatch between count long names %u and values %u\n", num, (unsigned) fLongValues.size());
               return false;
            }
            ptr  = ptr + strlen(ptr) + 1;

            fLongRecords.reserve(num);
            for (unsigned n=0;n<num;n++) {
               if (ptr>=theEnd) {
                  printf("decoding error\n");
                  return false;
               }
               fLongRecords.push_back(ptr);
               ptr  = ptr + strlen(ptr) + 1;
            }

            num = 0;
            if (sscanf(ptr, "%u", &num)!=1) {
               printf("cannot get number of double names\n");
               return false;
            }
            if (num != fDoubleValues.size()) {
               printf("mismatch between count double names %u and values %u\n", num, (unsigned) fDoubleValues.size());
               return false;
            }
            ptr  = ptr + strlen(ptr) + 1;

            fDoubleRecords.reserve(num);
            for (unsigned n=0;n<num;n++) {
               if (ptr>=theEnd) {
                  printf("decoding error\n");
                  return false;
               }
               fDoubleRecords.push_back(ptr);
               ptr  = ptr + strlen(ptr) + 1;
            }

            return true;
         }
   };
}

#endif
