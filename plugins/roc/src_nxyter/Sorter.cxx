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

#include "nxyter/Sorter.h"

#include "nxyter/Data.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

nxyter::Sorter::Sorter(unsigned max_inp_buf, unsigned intern_out_buf, unsigned intbuf)
{
   fRocId = 0;

   fIntBufMaxSize = intbuf;
   fExtBufMaxSize = max_inp_buf;

   if (sizeof(nxyter::Data) != 6) {
      printf("nxyter::Sorter will not work on platform where sizeof(nxyter::Data) = %u\n", sizeof(nxyter::Data));
      exit(1);
   }

   fIntBuf = (nxyter::Data*) malloc(fIntBufMaxSize*6);
   fIntBufTimes = new uint32_t[fIntBufMaxSize + fExtBufMaxSize];

   fIntBufSize = 0;
   fIntBufFrontEpoch = 0;
   fIntBufCurrEpoch = 0;
   fIntBufCurrRealEpoch = 0;
   fIntBuffCurrMaxTm = 0;

   fAccumMissed = 0;
   fLastOutEpoch = 0;

   fInternOutBufSize = intern_out_buf;
   fInternOutBuf = fInternOutBufSize==0 ? 0 : malloc(fInternOutBufSize*6);

   fFillBuffer = (nxyter::Data*) fInternOutBuf;
   fFillTotalSize = fInternOutBufSize;
   fFillSize = 0;

   fDoDataCheck = true;
   fDoDebugOutput = false;
}

nxyter::Sorter::~Sorter()
{
   free(fIntBuf); fIntBuf = 0;
   free(fInternOutBuf); fInternOutBuf = 0;
   delete [] fIntBufTimes;
}

bool nxyter::Sorter::startFill(void* buf, unsigned totalsize)
{
   if (fFillSize!=0) {
      printf("There is %u messages in output, cannot start !!!\n", fFillSize);
      return false;
   }

   if (totalsize < 243*6) {
      printf("nxyter::Sorter::startFill provided buffer size %u too small\n", totalsize);
      return false;
   }

   fFillBuffer = (nxyter::Data*) buf;
   fFillTotalSize = totalsize / 6;
   fFillSize = 0;

   return true;
}

void nxyter::Sorter::cleanBuffers()
{
   // anything wich was in internal buffer, is lost
   fIntBufSize = 0;

   // all messages in output are gone
   fFillSize = 0;

   // reset all internal variables
   fIntBufFrontEpoch = 0;
   fIntBufCurrEpoch = 0;
   fIntBufCurrRealEpoch = 0;
   fIntBuffCurrMaxTm = 0;

   fAccumMissed = 0;
   fLastOutEpoch = 0;
}

void nxyter::Sorter::stopFill()
{
   fFillBuffer = (nxyter::Data*) fInternOutBuf;
   fFillTotalSize = fInternOutBufSize;
   fFillSize = 0;
}


bool nxyter::Sorter::addData(nxyter::Data* new_data, unsigned num_msg, bool flush_data)
{
   if (fFillBuffer==0) return false;
   if (num_msg == 0) return true;

   if (fIntBufSize + num_msg > fIntBufMaxSize) {
      printf("One always need enough internal buffer space to be able store complete data buffer now:%u add:%u total:%u\n",
               fIntBufSize, num_msg, fIntBufMaxSize);
      return false;
   }

   nxyter::Data* data = new_data;

   uint32_t diff = 0;

   unsigned total_data_size = fIntBufSize + num_msg;

   // here we filling time stamp value for all hits, found in data
   for (unsigned indx = fIntBufSize; indx < total_data_size; indx++, data++) {

      switch (data->getMessageType()) {
         case ROC_MSG_HIT:
            if (data->getLastEpoch()==0) {
               fIntBufTimes[indx] = fIntBufCurrEpoch | data->getNxTs();

//               if (data->getNxTs() > 0x3fe0) printf("CANBE REALLY???\n");

            } else
            if (fIntBufCurrEpoch > 0) {
               // here we must check that last epoch bit set correctly
               // lets suppose that hit time is correct, try to estimate when we should detect him

               bool canbe = true;

               if (fDoDataCheck)
               if (indx==0)
                  canbe = false;
               else {
                  uint8_t lastid = data->getId();
                  unsigned revindx = indx;

                  unsigned loopcnt = 0;
                  unsigned fifocnt = 1; // we suppose that fifo had only item

                  uint32_t hittime = (fIntBufCurrEpoch - 0x4000) | data->getNxTs();
                  // 16 ns?  - time needed to get hit ready in pipe
                  uint32_t candetecttime = hittime + 48;
                  uint32_t maxhittime = 0;

                  while (revindx-->0) {
                     nxyter::Data* rev = revindx < fIntBufSize ?
                                             (fIntBuf + revindx) :
                                             (new_data + (revindx - fIntBufSize));

                     // in any case hit will be detected in 1 epoch
                     if (hittime > fIntBufTimes[revindx] + 0x4400) break;

                     // take only hits which were
                     if (!rev->isHitMsg() || (rev->getNxNumber() != data->getNxNumber())) continue;

                     // define time of any message, that was where
                     if (fIntBufTimes[revindx] > maxhittime) maxhittime = fIntBufTimes[revindx];

                     candetecttime += 32; // we need 32 ns to read out hit

                     if (rev->getId() >= lastid) loopcnt++;

                     if (rev->getId() == data->getId()) fifocnt++;

                     lastid = rev->getId();

                     // our fifo cannot be more that 4 items longer
                     // or if we made more loops than piped items in channel
                     if ((fifocnt==4) || (loopcnt > fifocnt)) break;
                  }

                  // if last hit in the nXYTER comes later than time when with guarantee we can detect our hit,
                  // it is not a normal and cannot be
                  // set security factor
                  if (maxhittime > candetecttime) {
                     if (fDoDebugOutput) {
                        data->printData(3);
                        printf("Suppress lastepoch bit while other hit appears later %x ns than this hit MUST be in FPGA\n", maxhittime - candetecttime);
                     }
//                     if (maxhittime - candetecttime < 32) printf("CHECK DIFF\n!!!");
                     canbe = false;
                  } else
                  // why to have last epoch if hit can be detected in same epoch??
                  if (candetecttime >> 14 == hittime >> 14) {
                     if (fDoDebugOutput) {
                        data->printData(3);
                        printf("Suppress lastepoch bit while can be detected in same epoch as appear hit:%x det:%x\n", hittime, candetecttime);
                     }
                     canbe = false;
//                     if (data->getNxTs() > 0x3f00) printf("CHECK BONDARY\n!!!");
                  }
               }

               fIntBufTimes[indx] = fIntBufCurrEpoch | data->getNxTs();
               if (canbe) { fIntBufTimes[indx] -= 0x4000; /*printf("!!!!!!! CANBE !!!!!!!!\n"); */}
            } else {
               fIntBufTimes[indx] = tmFailure;
               printf("HIT Last epoch err - should not happen !!!\n");
            }
            break;
         case ROC_MSG_EPOCH:
            fRocId = data->rocNumber();

            fIntBufCurrRealEpoch = data->getEpoch();
            // when we start filling from indx==0, there is no data in buffer
            // and one can reset front epoch, take it from past to avoid any problem with "jumping" messages
            if (indx==0) {
               fIntBufFrontEpoch = fIntBufCurrRealEpoch - 0x10;
               fIntBuffCurrMaxTm = 0;
            }

            diff = fIntBufCurrRealEpoch - fIntBufFrontEpoch; // no problem with overflow, epoch has 32 bit

            // epoch difference from current epoch too much - try flush and starts from beginning
            if (diff >= 0x20000) {
               printf("Epoch diff too much - try to repair\n");

               unsigned hitsprocessed = indx - fIntBufSize;

               if (!flushBuffer(new_data, hitsprocessed, true)) {
                   printf ("Cannot flush buffer - error!!!\n");
                   return false;
               }

               return addData(new_data + hitsprocessed, num_msg - hitsprocessed);
            }

            fIntBufCurrEpoch = diff << 14; // store current epoch in shifted manner
            fIntBufTimes[indx] = fIntBufCurrEpoch;

            break;
         case ROC_MSG_SYNC:
            if (data->getSyncEpochLowBit() == (fIntBufCurrRealEpoch & 0x1))
               fIntBufTimes[indx] = fIntBufCurrEpoch | data->getSyncTs();
            else
            if (fIntBufCurrEpoch > 0) {
               fIntBufTimes[indx] = (fIntBufCurrEpoch - 0x4000) | data->getSyncTs();
//               printf("SYNC marker in previous epoch cur:%x \n", fIntBufCurrRealEpoch);
            } else {
               printf("SYNC Last epoch err - should not happen !!!\n");
               fIntBufTimes[indx] = tmFailure;
            }
            break;
         case ROC_MSG_AUX:
            if (data->getAuxEpochLowBit() == (fIntBufCurrRealEpoch & 0x1))
               fIntBufTimes[indx] = fIntBufCurrEpoch | data->getAuxTs();
            else
            if (fIntBufCurrEpoch > 0)
               fIntBufTimes[indx] = (fIntBufCurrEpoch - 0x4000) | data->getAuxTs();
            else {
               printf("AUX Last epoch err - should not happen !!!\n");
               fIntBufTimes[indx] = tmFailure;
            }
            break;
         case ROC_MSG_SYS:
            fIntBufTimes[indx] = fIntBuffCurrMaxTm;
            if (data->isStopDaqMsg()) {
               if (fDoDebugOutput)
                  printf(("Found stop daq msg, force flush!!!\n"));
               flush_data = true;
            }
            break;
         default:
            fIntBufTimes[indx] = tmEmpty;
      }

//      if (fIntBufCurrRealEpoch == 0x16346e7a) {
//          printf("Item from interesting epoch has tm %x\n", fIntBufTimes[indx]);
//          data->printData(3);
//      }


      if ((fIntBufTimes[indx] <= tmLastValid) && (fIntBufTimes[indx]>fIntBuffCurrMaxTm))
         fIntBuffCurrMaxTm = fIntBufTimes[indx];
   }

   if (!flushBuffer(new_data, num_msg, flush_data)) {
      // copy everything into internal buffer
      printf("flushBuffer error, just copy it in the internal buffer\n");
      memcpy(fIntBuf + fIntBufSize, new_data, num_msg*6);
      fIntBufSize += num_msg;
   }

   if ((fIntBufSize > 0) && (fIntBufTimes[0] > tmLastValid)) {
      printf("Hard internal error !!!!\n");
      exit(1);
   }

//   printf("At the end we have %u msg intern\n", fIntBufSize);

   if (fIntBufCurrEpoch >= tmFrontShift + tmBoundary * 16)
      // be sure that no any other hits from past can come
      if ((fIntBufSize==0) || (fIntBufTimes[0] > tmFrontShift + tmBoundary * 2)) {
//         printf("shift front epoch on %x  size = %u\n", tmFrontShift, fIntBufSize);

         fIntBufFrontEpoch += tmFrontShift >> 14;
         fIntBuffCurrMaxTm -= tmFrontShift;
         fIntBufCurrEpoch -= tmFrontShift;

         for (unsigned n=0; n < fIntBufSize; n++)
            if (fIntBufTimes[n] <= tmLastValid)
               fIntBufTimes[n] -= tmFrontShift;
      }

   return true;
}

bool nxyter::Sorter::flushBuffer(nxyter::Data* new_data, unsigned num_msg, bool force_flush)
{
   if (fFillSize == fFillTotalSize) return false;

   unsigned tail_indx(0), head_indx(fIntBufSize);

   uint32_t boundary = 0;
   unsigned nless = 0;
   unsigned pmin = 0;
   unsigned indx = 0;
   uint32_t item_real_epoch = 0;

   nxyter::Data *src_data(0), *out_data(fFillBuffer + fFillSize);

   unsigned total_data_size = fIntBufSize + num_msg;
   if (total_data_size==0) return true;

   unsigned loop_limit = total_data_size;
   // if one force flushing of the buffer, make sorting loop for all items
   if (force_flush) {
     // add fake maximum time that all other hits will be flushed from buffer
      if (total_data_size >= fIntBufMaxSize + fExtBufMaxSize) {
         printf("Something completely wrong !!!!\n");
         exit(1);
      }
      fIntBufTimes[total_data_size] = tmLastValid;
      loop_limit++;
   }

   bool output_full = false;

   // from here one start to fill output buffers, doing sorting
   for (;head_indx < loop_limit; head_indx++) {

      // in case of flush last index correspond to non-existing index
      boundary = fIntBufTimes[head_indx];

      // not valid times take off from consideration
      // too small time - not enougth to start sorting
      if ((boundary > tmLastValid) || (boundary < tmBoundary)) continue;

      boundary -= tmBoundary;

      do {

         nless = 0;
         pmin = 0;

         for(indx = tail_indx; indx < head_indx; indx++)
            if (fIntBufTimes[indx] < boundary) {
               if ((nless==0) || (fIntBufTimes[indx] < fIntBufTimes[pmin])) pmin = indx;
               nless++;
            }

         if (nless==0) break;

         src_data = pmin < fIntBufSize ? fIntBuf + pmin : new_data + (pmin - fIntBufSize);

         item_real_epoch = fIntBufFrontEpoch + (fIntBufTimes[pmin] >> 14);

         // add extra epoch to output if some timed values should be added
         if ((item_real_epoch != fLastOutEpoch) && (src_data->isHitMsg() || src_data->isSyncMsg() ||  src_data->isAuxMsg())) {

            out_data->setMessageType(ROC_MSG_EPOCH);
            out_data->setRocNumber(fRocId);
            out_data->setEpoch(item_real_epoch);
            out_data->setMissed(fAccumMissed > 255 ? 255 : fAccumMissed);

            fLastOutEpoch = item_real_epoch;
            fAccumMissed = 0;

            fFillSize++;
            out_data++;

            // cannot write item iteslf, it still marked as used
            if (fFillSize == fFillTotalSize) {
               output_full = true;
               break;
            }

         }

//         if (item_real_epoch == 0x16346e7a) {
//            printf("Item pmin:%u Time:%x\n", pmin, fIntBufTimes[pmin]);
//            src_data->printData(3);
//         }

         switch (src_data->getMessageType()) {
            case ROC_MSG_NOP:
               break;
            case ROC_MSG_HIT:
               out_data->assignData(src_data->getData());
               out_data->setLastEpoch(0);
               fFillSize++; out_data++;
               break;
            case ROC_MSG_EPOCH:
               // we do not copy epoch directly, but remember value of missed hits
//               printf("get out epoch %x\n", src_data->getEpoch());

               fAccumMissed += src_data->getMissed();
               break;
            case ROC_MSG_SYNC:
               out_data->assignData(src_data->getData());
               out_data->setSyncEpochLowBit(fLastOutEpoch & 1);
               fFillSize++; out_data++;
               break;
            case ROC_MSG_AUX:
               out_data->assignData(src_data->getData());
               out_data->setAuxEpochLowBit(fLastOutEpoch & 1);
               fFillSize++; out_data++;
               break;
            case ROC_MSG_SYS:
               out_data->assignData(src_data->getData());
               fFillSize++; out_data++;
               break;
            default:
               printf("Absolutly strange message type %u\n", src_data->getMessageType());
               break;
         }

         // mark message as done
         fIntBufTimes[pmin] = tmEmpty;

         // cannot necessary to continue - output buffer is full
         if (fFillSize == fFillTotalSize) output_full = true;

      } while ((nless>1) && !output_full);

      // "eat" all items which no longer in use
      while ((tail_indx < head_indx) && (fIntBufTimes[tail_indx] > tmLastValid)) tail_indx++;

      if (output_full) break;
   }

   // now store rest of the data in internal buffers

   if (tail_indx == total_data_size) {

      fIntBufSize = 0; // everything empty !!!

   } else {

      nxyter::Data* tgt = fIntBuf;

      nxyter::Data* src = 0;

      unsigned copy_size = total_data_size - tail_indx;

      // copy data inside internal buffer first
      if (tail_indx < fIntBufSize) {
         memmove(tgt, fIntBuf + tail_indx, (fIntBufSize - tail_indx) * 6);
         tgt += (fIntBufSize - tail_indx);

         src = new_data;
         copy_size = num_msg;
      } else {
         src = new_data + (tail_indx - fIntBufSize);
      }

      memcpy(tgt, src, copy_size * 6);

      // copy calculated timing information

      fIntBufSize = total_data_size - tail_indx;

      memmove(fIntBufTimes, fIntBufTimes + tail_indx, fIntBufSize * sizeof(uint32_t));
   }

   return true;
}

bool nxyter::Sorter::shiftFilledData(unsigned num)
{
   if (num>=fFillSize) {
      fFillSize = 0;
      return true;
   } else
   if (num==0) return true;

   fFillSize -= num;

   memmove(fFillBuffer, fFillBuffer + num, fFillSize*6);

   return true;
}
