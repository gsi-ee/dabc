#include "roc/CalibrationModule.h"

#include "roc/Commands.h"
#include "roc/Device.h"

#include "dabc/logging.h"
#include "dabc/PoolHandle.h"
#include "dabc/MemoryPool.h"
#include "dabc/Command.h"
#include "dabc/Port.h"
#include "dabc/Pointer.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"

#include "SysCoreControl.h"
#include "SysCoreSorter.h"

#include "mbs/LmdTypeDefs.h"
#include "mbs/MbsTypeDefs.h"

#include <vector>
#include <math.h>

#define FullTimeStampRange 0x400000000000ULL

roc::CalibrationModule::CalibRec::CalibRec(unsigned id) :
   rocid(id),
   calibr_k(1.), calibr_b_e(0), calibr_b_f(0.), weight(0), sorter(0),
   evnt1_tm(0), evnt2_tm(0), evnt1_num(0), evnt2_num(0), evnt_len(0),
   data(0), numdata(0),
   front_epoch(0), front_shift(0.),
   last_epoch(0), is_last_epoch(false), last_stamp(0),
   stamp(0), stamp_copy(false)
{
}

void roc::CalibrationModule::CalibRec::RecalculateCalibr(double k, uint32_t b_e, double b_f)
{
   // if we have no last hit information, just take calibration as is
   if (NeedBCoef()) {
      calibr_k = k;
      calibr_b_e = b_e;
      calibr_b_f = b_f;
      weight = 1;
      return;
   }

   double new_k = (k + calibr_k*weight) / (weight + 1.);

   calibr_b_f += (calibr_k - new_k) * (last_epoch + last_stamp / 16384.);

   calibr_k = new_k;

   double fl = floor(calibr_b_f);
   if (fl >= 1.) {
     calibr_b_f -= fl;
     calibr_b_e += uint32_t(fl);
   } else
   if (fl < 0) {
     calibr_b_f -= fl;
     calibr_b_e -= uint32_t(-fl);
   }

/*
   // if old and new B are too far away from each other, one has,
   // probably, overflow and should handle it
   if (weight==0) {
      calibr_b_e = b_e;
      calibr_b_f = b_f;
   } else {

      if (b_e>calibr_b_e) b_f += (0.+ b_e - calibr_b_e);
                     else b_f -= (0.+ calibr_b_e - b_e);

      calibr_b_f = (calibr_b_f * weight + b_f) / (weight + 1.);

      while (calibr_b_f<0) { calibr_b_e--; calibr_b_f+=1.; }
      while (calibr_b_f >=1.) { calibr_b_e++; calibr_b_f-=1.; }
   }
*/
   if (weight<100) weight++;
}

double roc::CalibrationModule::CalibRec::CalibrEpoch(uint32_t& epoch)
{
//   double calibr_epoch = epoch * calibr_k + calibr_b_e + calibr_b_f;

  double fraction = epoch * (calibr_k-1.) + calibr_b_f;

  epoch += calibr_b_e;

  double fl = floor(fraction);
  if (fl >= 1.) {
    fraction -= fl;
    epoch += uint32_t(fl);
  } else
  if (fl < 0) {
    fraction -= fl;
    epoch -= uint32_t(-fl);
  }

  return fraction;
}

void roc::CalibrationModule::CalibRec::TimeStampOverflow()
{
// calibr_b += FullTimeStampRange * calibr_k;

   DOUT1(("Roc:%u Take into account overflow of epoch", rocid));

   double ep_shift = (calibr_k - 1.);
   if (ep_shift<0.) ep_shift+=1.;
   ep_shift*=0x100000000LL;

   calibr_b_e += uint32_t(floor(ep_shift));

   calibr_b_f += (ep_shift - floor(ep_shift));

   while (calibr_b_f >= 1.) {
      calibr_b_e++;
      calibr_b_f-=1.;
   }
}

roc::CalibrationModule::CalibrationModule(const char* name,
                                      dabc::Command* cmd) :
   dabc::ModuleAsync(name),
   fPool(0),
   f_inpptr(),
   fOutBuf(0),
   f_outptr()
{

   fNumRocs = GetCfgInt(roc::xmlNumRocs, 2, cmd);
   fBufferSize = GetCfgInt(dabc::xmlBufferSize, 16384, cmd);
   int numoutputs = GetCfgInt(dabc::xmlNumOutputs, 2, cmd);

   DOUT1(("new roc::CalibrationModule %s buff %d", GetName(), fBufferSize));
   fPool = CreatePool(roc::xmlRocPool, 1, fBufferSize);

   CreateInput("Input", fPool, 10);

   dabc::RateParameter* r = CreateRateParameter("CalibrRate", false, 3., "Input");
   r->SetLimits(0, 10.);
   r->SetDebugOutput(true);

   for(int n=0; n<numoutputs; n++)
      CreateOutput(FORMAT(("Output%d", n)), fPool, 10);

   fBuildEvents = 0;
   fSkippedEvents = 0;

   fFrontOutEpoch = 0;
   fLastOutEpoch = 0;
   fIsOutEpoch = false;
}

roc::CalibrationModule::~CalibrationModule()
{
   for (unsigned n=0;n<fCalibr.size();n++) {
      delete fCalibr[n].sorter;
      fCalibr[n].sorter = 0;
   }

   dabc::Buffer::Release(fOutBuf); fOutBuf = 0;
}

void roc::CalibrationModule::ProcessInputEvent(dabc::Port* inport)
{
   DoCalibration();
}

void roc::CalibrationModule::ProcessOutputEvent(dabc::Port* inport)
{
   DoCalibration();
}

#define NullStamp 0xffffffff
#define EndStamp  0xfffffffe

bool roc::CalibrationModule::DoCalibration()
{
   bool doagain = true;

/*
   if (fBuildSize > 22000000) {

      DOUT0(("Build: %ld  Skip: %ld events %5.1f",
            fBuildEvents, fSkippedEvents,
           (fBuildEvents > 0 ? 100. * fSkippedEvents / fBuildEvents : 0.)));

      dabc::mgr()->RaiseCtrlCSignal();

      return false;
   }
*/
   uint64_t fLastOutTm = 0;

   while (doagain) {
      // check that there is data in input and we can send results to all outputs
      if (Input(0)->InputBlocked()) return false;

      if (IsAnyOutputBlocked()) return false;

      // skip input buffer if it has no data
      if ((f_inpptr.buf()!=0) && (f_inpptr.fullsize() <= sizeof(mbs::EventHeader))) {
         f_inpptr.reset();
         Input(0)->SkipInputBuffers(1);
         continue;
      }

      // assign pointer on the begginnig of the first buffer
      if (f_inpptr.fullsize()==0) {
         dabc::Buffer* buf = Input(0)->InputBuffer(0);

         if (buf->GetTypeId() == dabc::mbt_EOF) {

            // check again that output not blocked
            if (FlushOutputBuffer()) continue;

            DOUT0(("Find EOF buffer - stop module !!!"));

            DOUT0(("Build: %ld  Skip: %ld events %5.1f",
               fBuildEvents, fSkippedEvents,
               (fBuildEvents > 0 ? 100. * fSkippedEvents / fBuildEvents : 0.)));

            for(unsigned n=0;n<NumOutputs();n++) {
               dabc::Buffer* eof_buf = fPool->TakeEmptyBuffer();
               eof_buf->SetTypeId(dabc::mbt_EOF);
               Output(n)->Send(eof_buf);
            }

            Input(0)->SkipInputBuffers(1);

            // stop module execution - application will react later on
            Stop();

            dabc::mgr()->GetApp()->InvokeCheckModulesCmd();

            return false;
         }

         f_inpptr.reset(buf);
      }

      if (fOutBuf==0) {
         fOutBuf = fPool->TakeBuffer(fBufferSize, false);
         if (fOutBuf==0) return false;
         fOutBuf->SetDataSize(fBufferSize);
         f_outptr.reset(fOutBuf);
         fIsOutEpoch = false; // always put epoch in begin of output buffer
      }

      mbs::EventHeader* inpevhdr = (mbs::EventHeader*) f_inpptr();

      if (f_inpptr.fullsize() < inpevhdr->FullSize()) {
         EOUT(("Missmatch in beginning !!!!!!! rest: %u  evnt: %u  buf:%u",
            f_inpptr.fullsize(), inpevhdr->FullSize(), Input(0)->InputBuffer(0)->GetDataSize()));
         f_inpptr.reset();
         Input(0)->SkipInputBuffers(1);

         fSkippedEvents++;
         continue;
      }

//      if (fSkippedEvents + fBuildEvents > 5500) exit(1);

//      DOUT1(("CALIBR: Analyse event %u of full size %u", inpevhdr->EventNumber(), inpevhdr->FullSize()));

      if (inpevhdr->FullSize() > f_outptr.fullsize())
         if (FlushOutputBuffer())
            continue;
         else {
            EOUT(("Event in input buffer too big %u, skip it", inpevhdr->FullSize()));
            Input(0)->SkipInputBuffers(1);
            f_inpptr.reset();
            fSkippedEvents++;
            continue;
         }

      // check input event - if it is ok

      mbs::SubeventHeader* subevhdr = 0;
      bool iserrdata(false);
      unsigned subevntcnt = 0;
      CalibRec* rec0 = 0;

      while ((subevhdr = inpevhdr->NextSubEvent(subevhdr)) != 0) {
         subevntcnt++;
         if (subevhdr->iProcId == roc::proc_ErrEvent) {
            iserrdata = true;
            break;
         }
         if (subevhdr->RawDataSize() < 3*6) {
            EOUT(("To small raw data size %u in event, skip", subevhdr->RawDataSize()));
            iserrdata = true;
            break;
         }
      }
      if (subevntcnt != fNumRocs) iserrdata = true;

      subevhdr = 0;
      subevntcnt = 0;
      if (!iserrdata)
      while ((subevhdr = inpevhdr->NextSubEvent(subevhdr)) != 0) {

         subevntcnt++;

         if (subevntcnt > fCalibr.size()) {
            EOUT(("Add subcrate %u", subevhdr->iSubcrate));

            fCalibr.push_back(CalibRec(subevhdr->iSubcrate));
            fCalibr[subevntcnt-1].sorter = new SysCoreSorter(fBufferSize / 6, fBufferSize / 6, 4096);
         }

         rec0 = &(fCalibr[0]);
         CalibRec *rec = &(fCalibr[subevntcnt-1]);
         if (rec->rocid != (unsigned) subevhdr->iSubcrate) {
            EOUT(("Missmatch in ROCs ids cnt:%u %u %u",  subevntcnt, rec->rocid, subevhdr->iSubcrate));
            iserrdata = true;
            continue;
         }

         // from this point we can switch to next subevent in case of error
         if (iserrdata) continue;


         SysCoreData* data = (SysCoreData*) subevhdr->RawData();

//         if (rec->rocid==1) {
//            unsigned numdata = subevhdr->RawDataSize() / 6;
//            while (numdata--) {
//               data->printData(3);
//               data++;
//            }
//            data = (SysCoreData*) subevhdr->RawData();
//         }


         if (!data->isEpochMsg()) {
            EOUT(("Should be epoch in the beginning, get %u", data->getMessageType()));
            iserrdata = true;
            continue;
         }

         uint32_t epoch1 = data->getEpoch();
         data++;

         if (!data->isSyncMsg()) {
            EOUT(("Should be sync event at the shift = 6"));
            iserrdata = true;
            continue;
         }

         rec->evnt1_tm = data->getMsgFullTime(epoch1);
         rec->evnt1_num = data->getSyncEvNum();

         if (!rec->is_last_epoch) {
            rec->is_last_epoch = true;
            rec->last_epoch = epoch1 - 8;
         }

         data = (SysCoreData*) ((char*) subevhdr->RawData() + subevhdr->RawDataSize() - 6);
         if (!data->isSyncMsg()) {
            EOUT(("Should be sync event in the end"));
            iserrdata = true;
            continue;
         }

         SysCoreData* last_sync = data--;

         bool is_extra_last_epoch = data->isEpochMsg();

         uint32_t epoch2(epoch1);

         do {
            if (data->isEpochMsg()) {
               epoch2 = data->getEpoch();
               break;
            }
         } while (--data > subevhdr->RawData());

         rec->evnt2_tm = last_sync->getMsgFullTime(epoch2);
         rec->evnt2_num = last_sync->getSyncEvNum();

         if (epoch2 - rec->last_epoch > 0x20000) {
            EOUT(("Roc:%u Too far epoch jump last:%08x evnt1:%08x evnt2:%08x",
                   rec->rocid, rec->last_epoch, epoch1, epoch2));
            iserrdata = true;
            continue;
         }

         if (epoch2==epoch1) {
//            EOUT(("Roc:%u Sync markers cannot be with the same epoch number evnt1:%08x evnt2:%08x",
//                   rec->rocid, epoch1, epoch2));
            iserrdata = true;
            continue;
         }

         rec->evnt_len = SysCoreData::CalcDistance(rec->evnt1_tm, rec->evnt2_tm);
         if (rec->evnt_len == 0) {
//            EOUT(("Roc:%u Zero event length!!!", rec->rocid));
            iserrdata = true;
            continue;
         }

         if (rec->rocid==0)
            if ((rec->evnt1_num != (epoch1 & 0xffffff)) ||
                (rec->evnt2_num != (epoch2 & 0xffffff))) {
//                    EOUT(("Sync number error for master roc %06x != %08x || %06x != %08x",
//                        rec->evnt1_num, epoch1 & 0xffffff, rec->evnt2_num, epoch2 & 0xffffff));
                    iserrdata = true;
                    continue;
                }


         DOUT1(("Roc:%u Evnt1:%6x Evnt2:%6x", rec->rocid, rec->evnt1_num, rec->evnt2_num));

         if (rec!=rec0) {

            if ((rec->evnt1_num != rec0->evnt1_num) || (rec->evnt2_num != rec0->evnt2_num)) {
               EOUT(("Events numbers missmatch !!!"));
               iserrdata = true;
               continue;
            }

            double k = 1. / rec->evnt_len * rec0->evnt_len;

            if ((k<0.99) || (k>1.01)) {
               // EOUT(("Coefficient K:%5.3f too far from 1.0, error", k));
               iserrdata = true;
               continue;
            }

//            double b = rec0->evnt1_tm - k * rec->evnt1_tm;
//            while (b<0.) b+= FullTimeStampRange;
//            b = b / 16384.;

            // recalculate koef only when we see that no epoch errors are there
            if (rec->NeedBCoef()) {
               uint32_t b_e = uint32_t(rec0->evnt1_tm >> 14) - uint32_t(rec->evnt1_tm >> 14);
               double b_f = 1.* (rec0->evnt1_tm % 16384) / 16384. - (k-1.) * (rec->evnt1_tm / 16384) - k * (rec->evnt1_tm % 16384) / 16384.;
               rec->RecalculateCalibr(k, b_e, b_f);
            } else
               rec->RecalculateCalibr(k);

//            DOUT1(("Roc:%u Calibr K:%7.5f B:%10.2f b_e:%u b_f:%6.4f   b_diff:%5.3f",
//               rec->rocid, k, b, b_e, b_f, b_e + b_f - b));
         }

         rec->data = (SysCoreData*) subevhdr->RawData();
         rec->numdata = subevhdr->RawDataSize()/6 - 1;
         if (is_extra_last_epoch) rec->numdata--;
      }

      // now try to afjust front output epoch on all channels
      if (!iserrdata) {

         uint32_t min_cal_epoch = 0xFFFFFFFF;
//         0x100000000LL;

         for(unsigned n=0;n<fCalibr.size();n++) {
            CalibRec *rec = &(fCalibr[n]);

            rec->front_epoch = rec->last_epoch;

            uint32_t cal_epoch = rec->front_epoch;

            // here one not interesting in fraction
            rec->CalibrEpoch(cal_epoch);

            if (cal_epoch < min_cal_epoch) min_cal_epoch = cal_epoch;
         }

         fFrontOutEpoch = min_cal_epoch - 8; // -8 to protect us again floating round problem

//         DOUT1(("Let's try front epoch as %x", fFrontOutEpoch));

         // now calculate front_shift values, which are correspond to selected front epochs

         for(unsigned n=0;n<fCalibr.size();n++) {
            CalibRec *rec = &(fCalibr[n]);

            uint32_t cal_epoch = rec->front_epoch;

            double fraction = rec->CalibrEpoch(cal_epoch);

            if (cal_epoch - fFrontOutEpoch < 8) {
               EOUT(("Rounding problem !!!!!!!!!!!!!!!!!!"));
               exit(1);
            }

//            DOUT1(("For roc:%u diff to front is %u %5.3f", rec->rocid, cal_epoch - fFrontOutEpoch, fraction));

            if (cal_epoch - fFrontOutEpoch > 0x20000) {
               EOUT(("Roc:%u Distances between epochs cal:%08X out:%08X tooooooo far = %08X",
                  rec->rocid, cal_epoch, fFrontOutEpoch, cal_epoch - fFrontOutEpoch));
               iserrdata = true;
               break;
            }

//            calibr_fr_epoch -= fFrontOutEpoch;
//            while (calibr_fr_epoch > 0x100000000LLU) calibr_fr_epoch -= 0x100000000LLU;
//            rec->front_shift = calibr_fr_epoch  * 0x4000;

            cal_epoch -= fFrontOutEpoch;

            rec->front_shift = (cal_epoch + fraction) * 16384.;

            if ((rec->front_shift < 0.) || (rec->front_shift > 0x80000000)) {
               EOUT(("Something completely wrong with front_shift = %9.1f!!!", rec->front_shift));
               iserrdata = true;
               break;
            }
         }
      }

      if (!iserrdata)
         for(unsigned n=0;n<fCalibr.size();n++) {

            CalibRec *rec = &(fCalibr[n]);

            if (!rec->sorter->addData(rec->data, rec->numdata)) {
                EOUT(("Cannot sort out data on roc %u size:%u", n, rec->numdata));
                iserrdata = true;
                break;
            }

            rec->data = rec->sorter->filledBuf();
            rec->numdata = rec->sorter->sizeFilled();

//            DOUT1(("After sorting Roc:%u has %u data", rec->rocid, rec->numdata));

            rec->stamp = NullStamp;
         }

      if (iserrdata) {
         fSkippedEvents++;

//         EOUT(("CALIBR: Skip event id = %u sz = %u from input iserr %s numrecs %u",
//            inpevhdr->EventNumber(), inpevhdr->FullSize(), DBOOL(iserrdata), recs.size()));

         if (f_inpptr.fullsize() < inpevhdr->FullSize()) {
            EOUT(("Corrupted MBS data rest: %u  evnt: %u", f_inpptr.fullsize(), inpevhdr->FullSize()));
            f_inpptr.reset();
         } else
            f_inpptr.shift(inpevhdr->FullSize());

         // in any case, remove all previous messages - bad lack for them
         for(unsigned n=0;n<fCalibr.size();n++) {
            fCalibr[n].sorter->cleanBuffers();
            fCalibr[n].is_last_epoch = false;
         }

         continue;
      }

//      DOUT0(("Combine sync event %06x", fCalibr[0].evnt1_num));

      fLastOutTm = 0;

      dabc::Pointer out_begin(f_outptr);

      mbs::EventHeader* evhdr = (mbs::EventHeader*) f_outptr();
      evhdr->Init(inpevhdr->EventNumber());
      f_outptr.shift(sizeof(mbs::EventHeader));

      mbs::SubeventHeader* subhdr = (mbs::SubeventHeader*) f_outptr();
      subhdr->Init();
      subhdr->iProcId = roc::proc_MergedEvent;
      subhdr->iSubcrate = rec0->rocid;
      f_outptr.shift(sizeof(mbs::SubeventHeader));

      dabc::Pointer data_begin(f_outptr);

      CalibRec *minrec(0);

      bool far_epoch_jump = false;


//      data = (SysCoreData*) f_outptr();
//      data->setMessageType(ROC_MSG_EPOCH);
//      data->setRocNumber(0);
//      data->setEpoch(rec0->evnt_epoch);
//      data->setMissed(0);
//      f_outptr.shift(6);

      // here one should fill

      do {

         minrec = 0;

         bool is_end_of_data = false;

         for (unsigned n = 0; n<fCalibr.size(); n++) {
            CalibRec *rec = &(fCalibr[n]);

            while (rec->stamp==NullStamp) {

               if (rec->numdata==0) {
                  rec->stamp = EndStamp;
                  is_end_of_data = true;
                  break;
               }

//               DOUT1(("Get msg %u in rocid %u", data->getMessageType(), rec->rocid));

               if (rec->data->isHitMsg()) {
                  rec->last_stamp = rec->data->getNxTs();
                  rec->stamp = uint32_t(rec->data->getNxTs() * rec->calibr_k + rec->front_shift);
                  rec->stamp_copy = true;
//                  DOUT1(("Roc:%u Hit stamp:%x", rec->rocid, rec->stamp));
               } else
               if (rec->data->isAuxMsg()) {
                  rec->last_stamp = rec->data->getAuxTs();
                  rec->stamp = uint32_t(rec->data->getAuxTs() * rec->calibr_k + rec->front_shift) & 0xfffffffe;
                  rec->stamp_copy = true;
//                  DOUT1(("Roc:%u Aux stamp:%x", rec->rocid, rec->stamp));
               } else
               if (rec->data->isSyncMsg()) {
                  rec->last_stamp = rec->data->getSyncTs();
                  rec->stamp = uint32_t(rec->data->getSyncTs() * rec->calibr_k + rec->front_shift) & 0xfffffffe;
                  rec->stamp_copy = false;
//                  DOUT1(("Roc:%u Sync stamp:%x", rec->rocid, rec->stamp));
               } else
               if (rec->data->isEpochMsg()) {
                  uint32_t epoch = rec->data->getEpoch();

                  if (rec->is_last_epoch && (epoch < rec->last_epoch))
                     rec->TimeStampOverflow();

                  unsigned diff = epoch - rec->last_epoch;
                  if (!rec->is_last_epoch) diff = epoch - rec->front_epoch;

                  rec->last_epoch = epoch;
                  rec->last_stamp = 0;
                  rec->is_last_epoch = true;

                  rec->front_shift += diff * rec->calibr_k * 0x4000;

                  if (rec->last_epoch - rec->front_epoch > 0x20000) {
                     EOUT(("Roc:%u Epochs front:%x epoch:%x diff:%x - should not happen !!!",
                         rec->rocid, rec->front_epoch, rec->last_epoch, rec->last_epoch - rec->front_epoch));
                     far_epoch_jump = true;
                     rec->stamp = EndStamp;
                     iserrdata = true;
                  } else
                     rec->stamp = uint32_t(rec->front_shift);

                  rec->stamp_copy = false;
               } else {
                  // skip all other messages
                  rec->data++;
                  rec->numdata--;
               }
            }

            // no any data can be taken from that stream
            if (rec->stamp==EndStamp) continue;

            if ((minrec==0) || (rec->stamp < minrec->stamp)) minrec = rec;
         }

         if (is_end_of_data) minrec = 0;

         if (minrec != 0) {

//            DOUT1(("Minumum rec has ROC:%u stamp:%x DoCopy:%d", minrec->rocid, minrec->stamp, minrec->stamp_copy));
/*
            if (minrec->stamp_copy) {
               uint64_t fulltm = (uint64_t(fFrontOutEpoch) << 14) | minrec->stamp;

               DOUT1(("Now:%llu  Prev:%llu Diff to prev:%llu %llu",  fulltm, fLastOutTm, fulltm - fLastOutTm, fLastOutTm - fulltm));

               if (fLastOutTm!=0)
                  if (fLastOutTm > fulltm) {
                     if (SysCoreData::CalcDistance(fLastOutTm, fulltm) > 16 * 0x4000) minrec->stamp_copy = false;

                     EOUT(("Missmatch in time order iscorrect = %s!!!", DBOOL(minrec->stamp_copy)));
                  }

               if (minrec->stamp_copy)
                  fLastOutTm = fulltm;
            }
*/

            if (minrec->stamp_copy) {

//               DOUT1(("Roc:%u Add hit with stamp %x to output", minrec->rocid, minrec->stamp));

               uint32_t next_epoch = fFrontOutEpoch + (minrec->stamp >> 14);

               if (!fIsOutEpoch || (fLastOutEpoch != next_epoch)) {

                  if (next_epoch < fLastOutEpoch) DOUT1(("Epoch missmatch prev:%x next:%x", fLastOutEpoch, next_epoch));

                  // at that place we need at least 12 bytes - for epoch and message
                  if (f_outptr.fullsize() < 12) {
//                     EOUT(("Not enougth place in output buffer for epoch - AAAAA"));
//                     iserrdata = true;
                     is_end_of_data = true;
                     minrec = 0;
                     break;
                  }

                  SysCoreData* data = (SysCoreData*) f_outptr();
                  data->setMessageType(ROC_MSG_EPOCH);
                  data->setRocNumber(0);
                  data->setEpoch(next_epoch);
                  data->setMissed(0);
                  f_outptr.shift(6);
//                  if (fSkippedEvents + fBuildEvents > 5100)
//                     data->printData(3);
                  fLastOutEpoch = next_epoch;
                  fIsOutEpoch = true;
               }

               // if one have no enough space in buffer, just stop event filling
               if (f_outptr.fullsize() < 6) {
                  is_end_of_data = true;
                  minrec = 0;
//                  EOUT(("Not enougth place in output buffer - AAAAA"));
//                  iserrdata = true;
                  break;
               }

               f_outptr.copyfrom(minrec->data, 6);

               SysCoreData* data = (SysCoreData*) f_outptr();
               data->setRocNumber(minrec->rocid);
               if (data->isHitMsg()) {
                  data->setLastEpoch(0);
                  data->setNxTs(minrec->stamp & 0x3fff);
               } else
               if (data->isAuxMsg()) {
                  data->setAuxTs(minrec->stamp & 0x3fff);
                  data->setAuxEpochLowBit(fLastOutEpoch & 0x1);
               } else
               if (data->isSyncMsg()) {
                  data->setSyncTs(minrec->stamp & 0x3fff);
                  data->setSyncEpochLowBit(fLastOutEpoch & 0x1);
               }

               f_outptr.shift(6);

//               if (fSkippedEvents + fBuildEvents > 5100)
//                  data->printData(3);

            }


            minrec->data++;
            minrec->numdata--;
            minrec->stamp = NullStamp;
         }

      } while (minrec!=0);


      // skip processed data from sorter at the end
      for(unsigned n=0;n<fCalibr.size();n++) {
         CalibRec *rec = &(fCalibr[n]);
//         if (rec->sorter->sizeFilled() - rec->numdata)
//            DOUT1(("Roc:%u Shift %u data from sorter", rec->rocid, rec->sorter->sizeFilled() - rec->numdata));

         if (far_epoch_jump) {
            rec->sorter->cleanBuffers();
            rec->is_last_epoch = false;
            EOUT(("Remove everything from rec %u", rec->rocid));
         } else
            rec->sorter->shiftFilledData(rec->sorter->sizeFilled() - rec->numdata);
      }


      fBuildEvents++;

      subhdr->SetRawDataSize(data_begin.distance_to(f_outptr));
      evhdr->SetSubEventsSize(subhdr->FullSize());

      if (iserrdata) {
         EOUT(("Hard error - data was filled partially, therefore they will be discarded completely"));
         f_outptr.reset(out_begin);
      } else
         fBuildSize+=evhdr->FullSize();

      f_inpptr.shift(inpevhdr->FullSize());

//      DOUT1(("Fill output event of total size %u", evhdr->FullSize()));
   }

   return true;
}


bool roc::CalibrationModule::FlushOutputBuffer()
{
   if (fOutBuf==0) return false;

   if (f_outptr.fullsize() == fOutBuf->GetDataSize()) return false;

   // send output buffer to all outputs
   fOutBuf->SetDataSize(fOutBuf->GetDataSize() - f_outptr.fullsize());
   fOutBuf->SetTypeId(mbs::mbt_MbsEvents);

   SendToAllOutputs(fOutBuf);
   fOutBuf = 0;
   f_outptr.reset();

   return true;
}
