#include "roc/FileInput.h"

#include "dabc/Buffer.h"
#include "dabc/logging.h"
#include "roc/Iterator.h"

/*!
 * \class roc::FileInput
 * \brief This class provide read access to lmd files via roc::Board interface
 *
 * No any put/get/startDaq/stopDaq methods are functional.
 * Only getNextBuffer() is implemented. Main motivation is to provide access to
 * lmd files via roc::Iterator interface in the form:
 *
 * roc::Iterator iter("file://calibr_*.lmd");
 *
 * while (iter.next()) {
 *    iter.msg().printData();
 * }
 *
 * Iterator reads data from lmd file for the only roc. By default, first available roc number
 * is used. If required, roc id can be selected in the constructor like:
 *
 * roc::Iterator iter("file://calibr_*.lmd&rocid=3");
 *
 * From raw lmd files, produced by DABC, iterator excludes duplicated sync and
 * epoch messages from the end.
 *
 * \todo add class docu
 */

roc::FileInput::FileInput(mbs::LmdInput* inp, int selectroc) :
   roc::Board(),
   fInput(inp),
   fHdr(0),
   fSubev(0),
   fSelectRoc(selectroc)
{
   setRole(base::roleDAQ);
}

roc::FileInput::~FileInput()
{
   if (fInput) { delete fInput; fInput = 0; }
}

bool roc::FileInput::getNextBuffer(void* &buf, unsigned& len, double tmout)
{
   if (fInput==0) return false;

   while (true) {
      if (fHdr==0) {
         fHdr = fInput->ReadEvent();
         if (fHdr==0) return false;
         fSubev = 0;
      }

      fSubev = fHdr->NextSubEvent(fSubev);
      if (fSubev!=0) {
         buf = fSubev->RawData();
         len = fSubev->RawDataSize();

         if ( (fSubev->iProcId == roc::proc_ErrEvent) ||
              (fSubev->iProcId==roc::proc_RocEvent) ||
              (fSubev->iProcId==roc::proc_MergedEvent) ||
              (fSubev->iProcId==roc::proc_RawData) ) {

            if ((fSelectRoc>=0) && (fSelectRoc != fSubev->iSubcrate)) continue;

            //if (fSelectRoc<0) fSelectRoc = fSubev->iSubcrate;

            // bool calibrated = (fSubev->iProcId == proc_MergedEvent);

            roc::Iterator iter(fSubev->iControl);

            iter.assign(buf, len);

            if (!iter.last()) {
               EOUT("Cannot jump to end of raw buffer len = %d", len);
               continue;
            }

            // exclude last SYNC message
            if (!iter.msg().isSyncMsg()) {
               EOUT("Not an sync message in the end");
               continue;
            }

            unsigned msg_size = roc::Message::RawSize(iter.getFormat());

            len -= msg_size;

            if (!iter.prev()) {
               EOUT("Cannot roll back one message");
               continue;
            }

            // exclude last EPOCH message, belonging to SYNC
            if (iter.msg().isEpochMsg()) len -= msg_size;
         } else
           continue; // do not take non-nXYTER event

         fBrdNumber = fSubev->iSubcrate;

         // check if not an empty event
         // check in calibration that empty event not produced at all
         if (len>0) return true;
      }

      fHdr = 0;
   }

   return false;
}

int roc::FileInput::getTransportKind() const
{
   return roc::kind_File;

/*   roc::MessageFormat fmt = getMsgFormat();
   
   return ((fmt == formatOptic1) || (fmt==formatOptic2)) ? roc::kind_ABB : roc::kind_UDP;
*/
}


int roc::FileInput::getMsgFormat() const
{
   return fSubev ? fSubev->iControl : roc::formatEth1;
}
