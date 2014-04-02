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

#include <stdint.h>
#include <string>
#include <vector>


namespace mbs {

   /** \brief Record for manipulation with slow control data
    *
    * Used in EPICS, DIM and FESA plugins to deliver data to analysis */

   class SlowControlData {
      protected:
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

         SlowControlData() {}
         virtual ~SlowControlData() {}

         void Clear()
         {
            fLongRecords.clear();
            fLongValues.clear();
            fDoubleRecords.clear();
            fDoubleValues.clear();
         }

         bool Empty() const { return fLongRecords.empty() && fDoubleRecords.empty(); }

         void AddLong(const std::string& name, int64_t value)
         {
            fLongRecords.push_back(name);
            fLongValues.push_back(value);
         }

         void AddDouble(const std::string& name, double value)
         {
            fDoubleRecords.push_back(name);
            fDoubleValues.push_back(value);
         }

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

         unsigned Write(void* buf, unsigned buflen, uint32_t evnumber=0, uint32_t currtime = 0)
         {
            unsigned size = GetRawSize();
            if (size > buflen) return 0;

            char* ptr = (char*) buf;

            memcpy(ptr, &evnumber, sizeof(evnumber)); ptr+=sizeof(evnumber);

            memcpy(ptr, &currtime, sizeof(currtime)); ptr+=sizeof(currtime);

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




   };
}

#endif
