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

#include "roc/CombinerModule.h"

#include "roc/Board.h"
#include "roc/Iterator.h"
#include "roc/Commands.h"

#include "dabc/logging.h"
#include "dabc/Pointer.h"
#include "dabc/MemoryPool.h"
#include "dabc/Command.h"
#include "dabc/Pointer.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"

#include "mbs/LmdTypeDefs.h"
#include "mbs/MbsTypeDefs.h"

//#include "bnet/common.h"

const char* roc::xmlSkipErrorData     = "SkipErrorData";
const char* roc::xmlIgnoreMissingEpoch= "IgnoreMissingEpoch";
const char* roc::xmlSyncNumber        = "SyncNumber";
const char* roc::xmlSyncScaleDown     = "SyncScaleDown";
const char* roc::xmlSpillRoc          = "SpillRoc";
const char* roc::xmlSpillAux          = "SpillAux";
const char* roc::xmlCalibrationPeriod = "CalibrationPeriod";
const char* roc::xmlCalibrationLength = "CalibrationLength";
const char* roc::xmlThrottleAux       = "ThrottleAux";
const char* roc::xmlGet4ResetPeriod   = "Get4ResetPeriod";
const char* roc::xmlGet4ResetLimit    = "Get4ResetLimit";


bool InitIterator(roc::Iterator& iter, dabc::Buffer& buf, unsigned shift = 0, bool isudp = false, unsigned rocid = 0)
{
   if (buf.null()) return false;

   if ((buf.GetTypeId() < roc::rbt_RawRocData) ||
       (buf.GetTypeId() > roc::rbt_RawRocData + roc::formatNormal)) return false;

   int fmt = buf.GetTypeId() - roc::rbt_RawRocData;

   iter.setFormat(fmt);
   if (isudp) iter.setRocNumber(rocid);

   void* ptr = buf.SegmentPtr();
   unsigned size = buf.SegmentSize();

   if (shift>=size) return false;

   iter.assign((uint8_t*) ptr + shift, size - shift);

   return true;
}


roc::CombinerModule::CombinerModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fOutBuf(),
   f_outptr(),
   fSpillRoc(-1),
   fSpillAux(-1),
   fSpillState(true),
   fCalibrationPeriod(-1.),
   fCalibrationLength(1.),
   fLastCalibrationTime(),
   fExtraMessages()
{
//   dabc::SetDebugLevel(5);

   fSkipErrorData = Cfg(roc::xmlSkipErrorData, cmd).AsBool(false);

   fIgnoreMissingEpoch = Cfg(roc::xmlIgnoreMissingEpoch, cmd).AsBool(false);
   fSyncScaleDown = Cfg(roc::xmlSyncScaleDown, cmd).AsInt(-1);
   fSyncNumber = Cfg(roc::xmlSyncNumber, cmd).AsInt(0);
   if ((fSyncNumber<0) || (fSyncNumber>1)) fSyncNumber = 0;

   if ((fSyncScaleDown>=0) && (fSyncScaleDown<32))
      fSyncScaleDown = 1 << fSyncScaleDown;
   else
   if (fSyncScaleDown == 77) {
      EOUT("Mode 77 was required by FEET readout - take data as is, pack into MBS buffer and store");
      EOUT("Please use raw readout instead");
      fSyncScaleDown = -1;
   } else {
      EOUT("Sync scale down factor not specified. Use 1 as default");
      fSyncScaleDown = -1;
   }

   fSpillRoc = Cfg(roc::xmlSpillRoc, cmd).AsInt(-1);
   fSpillAux = Cfg(roc::xmlSpillAux, cmd).AsInt(-1);
   fCalibrationPeriod = Cfg(roc::xmlCalibrationPeriod, cmd).AsDouble(-1.);
   fCalibrationLength = Cfg(roc::xmlCalibrationLength, cmd).AsDouble(0.5);

   fGet4ResetPeriod = Cfg(roc::xmlGet4ResetPeriod, cmd).AsDouble(-1.);

   // limit used no for periodic, but for intended reset
   fGet4ResetLimit = Cfg(roc::xmlGet4ResetLimit, cmd).AsDouble(-1);
   fLastGet4ResetTm = dabc::Now();
   fDetectGet4Problem = false;

   if (fGet4ResetLimit>0)
      DOUT0("Enable GET4 reset at maximum every %5.1f s", fGet4ResetLimit);

   double flushTime = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(-1.);

   if (flushTime>0.)
      CreateTimer("FlushTimer", flushTime, false);
   fFlushFlag = false;

   DOUT1("CombinerModule name:%s numinp = %u SyncScaleDown = %d flushtime = %5.1f", GetName(), NumInputs(), fSyncScaleDown, flushTime);

   fLastCalibrationTime.Reset();

   fLastSpillTime = 0;

   CreatePar("SpillState").SetInt(-1);

   if (fSpillAux>=0) {
      if (fCalibrationPeriod < 5.) fCalibrationPeriod = 5.;
      if (fCalibrationLength < 0.1) fCalibrationLength = 0.1;

      DOUT1("CombinerModule spill detection ROC:%d aux:%d period:%4.1f length:%3.1f", fSpillRoc, fSpillAux, fCalibrationPeriod, fCalibrationLength);

      CreatePar("SpillRate").SetRatemeter(false, 10).SetUnits("spill");

      CreateTimer("CalibrTimer", -1., false);
   }

   if (fGet4ResetPeriod>0) {
      CreateTimer("Get4Timer", fGet4ResetPeriod, false);
   }

   CreatePar("RocData").SetRatemeter(false, 3.);

   CreatePar("RocEvents").SetRatemeter(false, 3.).SetUnits("ev");

   CreatePar("RocErrors").SetRatemeter(false, 3.).SetUnits("ev");

   if (Par("RocData").GetDebugLevel()<0) Par("RocData").SetDebugLevel(1);
   if (Par("RocEvents").GetDebugLevel()<0) Par("RocEvents").SetDebugLevel(1);


   fThrottleAux = Cfg(roc::xmlThrottleAux, cmd).AsInt(-1);

   for(unsigned num=0; num < NumInputs(); num++) {
      SetPortRatemeter(InputName(num), Par("RocData"));

      DOUT2("Create input %s queue capacity %u", num, InputName(num).c_str(), InputQueueCapacity(num));

      fInp.push_back(InputRec());
      fInp[num].use = false;
      if (fThrottleAux>=0)
         CreatePar( dabc::format("Throttle%u", num)).SetRatemeter(false, 3.).SetLimits(0., 100.);
   }

   CreatePar("RocInfo", "info").SetSynchron(true, 2., false);
   SetInfo(dabc::format("ROC combiner module ready. NumInputs = %u" , NumInputs()), true);
}

roc::CombinerModule::~CombinerModule()
{
   DOUT3("roc::CombinerModule::~CombinerModule");

}

void roc::CombinerModule::ModuleCleanup()
{
   dabc::ModuleAsync::ModuleCleanup();

   DOUT3("roc::CombinerModule::ModuleCleanup()");
   while (fExtraMessages.size()>0) {
      delete fExtraMessages.front();
      fExtraMessages.pop_front();
   }

   f_outptr.reset();
   fOutBuf.Release();
}


void roc::CombinerModule::SetInfo(const std::string& info, bool forceinfo)
{
   Par("RocInfo").SetStr(info);

   if (forceinfo) Par("RocInfo").FireModified();
}




void roc::CombinerModule::ProcessInputEvent(unsigned port)
{
   // check events in the buffers queues
   FindNextEvent(port);

   FillBuffer();
}


void roc::CombinerModule::BeforeModuleStart()
{
   if (fSpillAux==77) {
      DOUT0("Shoot timer for the first time");
      ShootTimer("CalibrTimer", fCalibrationPeriod);
   }
}

void roc::CombinerModule::ProcessTimerEvent(unsigned timer)
{
   if (TimerName(timer) == "CalibrTimer") {

      if (fSpillAux==77) {

         fSpillState = !fSpillState;

         dabc::mgr.GetApp().Submit(roc::CmdCalibration(fSpillState));

         ShootTimer("CalibrTimer", fSpillState ? fCalibrationLength : fCalibrationPeriod);
      } else {
         dabc::mgr.GetApp().Submit(roc::CmdCalibration(false));
      }
   } else

   if (TimerName(timer) == "FlushTimer") {
      if (fFlushFlag) FlushOutputBuffer();
      fFlushFlag = true;
   } else

   if (TimerName(timer) == "Get4Timer") {
      InvokeAllGet4Reset();
   }
}

bool roc::CombinerModule::FindNextEvent(unsigned recid)
{
   if (recid>=fInp.size()) return false;

   InputRec* rec = &(fInp[recid]);

   if (!rec->isrocid()) {
      EOUT("Not configured input %u !!!!", recid);
      exit(4);
   }

//   DOUT5("FindNextEvent REC:%p ROCID:%d KIND:%d", rec, rec->rocid, rec->isudp);

   // if one already found events for specified roc, return
   if (rec->isready) return true;

   while (rec->curr_nbuf < NumCanRecv(recid)) {

      dabc::Buffer buf = RecvQueueItem(recid, rec->curr_nbuf);

      if (rec->curr_indx >= buf.GetTotalSize()) {
         if (rec->isprev || rec->isnext) {
            rec->curr_nbuf++;
            if (rec->curr_nbuf == InputQueueCapacity(recid)) {
               SetInfo(dabc::format("Skip all data while we cannot find two events in complete queue for input %u nbuf %u isprev %s isnext %s prevnbuf %u", recid, rec->curr_nbuf, DBOOL(rec->isprev), DBOOL(rec->isnext), rec->prev_nbuf));
               DOUT0("Skip all data while we cannot find two events in complete queue for input %u nbuf %u isprev %s isnext %s prevnbuf %u prevevnt %u buftotalsize %u", recid, rec->curr_nbuf, DBOOL(rec->isprev), DBOOL(rec->isnext), rec->prev_nbuf, rec->prev_evnt, buf.GetTotalSize());

               rec->isprev = false;
               rec->isnext = false;
               SkipInputBuffers(recid, rec->curr_nbuf);
               rec->curr_nbuf = 0;
            }
         } else {
            // no need to keep this buffer in place if no epoch was found
            SkipInputBuffers(recid, rec->curr_nbuf+1);
            rec->curr_nbuf = 0;
         }
         rec->curr_indx = 0;
         continue;
      }

//      bool dodump = (rec->curr_indx == 0);

      roc::Iterator iter;

      InitIterator(iter, buf, rec->curr_indx, rec->isudp, rec->rocid);

      bool iserr = false;
      roc::Message* data = 0;

      while (iter.next()) {

         data = & iter.msg();

         if (rec->IsDifferentRocId(data->getRocNumber(), iserr)) {
            if (iserr)
               EOUT("Input:%u Kind:%s Mismatch in ROC numbers %u %u", recid, (rec->isudp ? "UDP" : "Optic"), rec->rocid, data->getRocNumber());
            rec->curr_indx += iter.getMsgSize();
            continue;
         }

         switch (data->getMessageType()) {

         case roc::MSG_HIT: { // nXYTER message, not interesting here
            break;
         }

         case roc::MSG_EPOCH: { // epoch marker
            rec->curr_epoch = data->getEpochNumber();
            rec->iscurrepoch = true;
            break;
         }

         case roc::MSG_SYNC: { // SYCN message

            if (data->getSyncChNum()==fSyncNumber) {
               bool isepoch = rec->iscurrepoch;

               if (!isepoch && fIgnoreMissingEpoch) {
                  // workaround for the corrupted FEET readout,
                  // where no epoch created for SYNC markers

                  // increment faked epoch number for each next sync marker
                  rec->curr_epoch++;
                  isepoch = true;
               }

               if (!isepoch) {
                  SetInfo(dabc::format("Found SYNC marker %6x without epoch", data->getSyncData()));
               } else
               if ((fSyncScaleDown>0) && (data->getSyncData() % fSyncScaleDown != 0)) {
                  SetInfo(dabc::format("Roc%u SYNC marker %06x not in expected sync step %02x",
                        rec->rocid, data->getSyncData(), fSyncScaleDown));
               } else {
                  if (!rec->isprev) {
                     rec->prev_epoch = rec->curr_epoch;
                     rec->isprev = true;
                     rec->prev_nbuf = rec->curr_nbuf;
                     rec->prev_indx = rec->curr_indx;
                     rec->prev_evnt = data->getSyncData();
                     rec->prev_stamp = data->getSyncTs();
                     rec->data_length = 0;
                  } else {
                     rec->next_epoch = rec->curr_epoch;
                     rec->isnext = true;
                     rec->next_nbuf = rec->curr_nbuf;
                     rec->next_indx = rec->curr_indx;
                     rec->next_evnt = data->getSyncData();
                     rec->next_stamp = data->getSyncTs();
                  }
               }
            }
            break;
         }

         case roc::MSG_AUX: { // AUX message

            if ((fThrottleAux>=0) &&  (data->getAuxChNum()==(unsigned)fThrottleAux)) {
               uint64_t tm = data->getMsgFullTime(rec->curr_epoch);
               bool state = data->getAuxFalling() == 0;

               if ((rec->last_thottle_tm != 0) && (rec->last_throttle_state != state)) {
                  uint64_t dist = roc::Message::CalcDistance(rec->last_thottle_tm, tm);

                  if (rec->last_throttle_state)
                     Par( dabc::format("Throttle%u", data->getRocNumber())).SetDouble(dist * 1e-7); // maximum is 100 % can be
               }

               rec->last_thottle_tm = tm;
               rec->last_throttle_state = state;
            } else
            if ((fSpillRoc>=0) && ((unsigned)fSpillRoc==data->getRocNumber())
                 && ((unsigned)fSpillAux == data->getAuxChNum())) {

               uint64_t tm = data->getMsgFullTime(rec->curr_epoch);

               bool faraway = (roc::Message::CalcDistance(fLastSpillTime, tm) > 10000000);
               if (tm < fLastSpillTime) faraway = true;

               bool changed = false;

               if ((data->getAuxFalling()!=0) && fSpillState && faraway) {
                  DOUT1("DETECT SPILL OFF");
                  fSpillState = false;
                  fLastSpillTime = tm;
                  changed = true;
               } else
               if ((data->getAuxFalling()==0) && !fSpillState && faraway) {
                  DOUT1("DETECT SPILL ON");
                  fSpillState = true;
                  fLastSpillTime = tm;
                  changed = true;
                  Par("SpillRate").SetInt(1);
               }

               if (changed) Par("SpillState").SetInt(fSpillState ? 1 : 0);

               // DOUT0("Period %f changed %d state %d", fCalibrationPeriod, changed, fSpillState);

               // if spill is off and calibration period is specified, try to start calibration
               if ((fCalibrationPeriod>0) && changed && !fSpillState) {
                  dabc::TimeStamp now = dabc::Now();
                  double dist = fCalibrationPeriod + 1000;

                  if (!fLastCalibrationTime.null())
                     dist = now - fLastCalibrationTime;

                  // DOUT0("Distane %6.1f", dist);

                  if (dist > fCalibrationPeriod) {
                     fLastCalibrationTime = now;
                     DOUT0("Invoke autocalibr mode after %5.1f s", dist);

                     dabc::mgr.GetApp().Submit(roc::CmdCalibration(true));
                     ShootTimer("CalibrTimer", fCalibrationLength);
                  }
               }

            }
            break;
         }

         case roc::MSG_EPOCH2:  { // EPOCH2 message
            uint32_t get4 = data->getEpoch2ChipNumber();
            uint32_t epoch2 = data->getEpoch2Number();

            if (get4<MaxGet4 && rec->canCheckAnyGet4) {
               if ((rec->lastEpoch2[get4]!=0) && (epoch2!=0) && (epoch2 > rec->lastEpoch2[get4])) {
                  uint32_t diff = epoch2 - rec->lastEpoch2[get4];
                  // try to exclude very far lost of epochs - most probably, transport failure
                  if ((diff!=1) && (diff<5) && rec->canCheckGet4[get4]) {
                     DOUT0("Detect error epoch2 %u  shift = %u on ROC:%u Get4:%u", epoch2, diff, rec->rocid, get4);
                     fDetectGet4Problem = true;
                  }
               }

               rec->lastEpoch2[get4] = epoch2;
               if (data->getEpoch2Sync()) {

                  if (epoch2 > 0xff000000) {
                     DOUT0("GET4 epoch2 %u number on ROC:%u Get4:%u too large", epoch2, rec->rocid, get4);
                     if (epoch2>0xfffff000) dabc::mgr.StopApplication();
                  }

                  if (rec->iscurrepoch && (rec->curr_epoch > 0xf0000000)) {
                     DOUT0("250 MHz epoch = %u on ROC:%u with GET4 readout closer to overflow ", rec->curr_epoch, rec->rocid);
                     if (rec->curr_epoch > 0xffff0000) dabc::mgr.StopApplication();
                  }

                  if (!rec->canCheckGet4[get4]) {
                     DOUT0("Enable checking of GET4:%u on ROC:%u", get4, rec->rocid);
                     rec->canCheckGet4[get4] = true;
                  }

                  unsigned mod = epoch2 % 25;
                  if (mod!=0) {
                     fDetectGet4Problem = true;

                     if (rec->lastEpoch2SyncErr[get4]!=mod)
                        DOUT0("Detect wrong epoch2 : %u (mod=%u) value when sync=1 ROC:%u Get4:%u", epoch2, mod, rec->rocid, get4);

                     rec->lastEpoch2SyncErr[get4] = mod;
                  }

                  // check about every 0.625 s
                  if (epoch2 % 25000 == 0) {
                      // check if during such long period significant part of the distance between edges approx equal to epoch length
                     for (int nch=0;nch<MaxGet4Ch;nch++) {
                        if ((rec->get4AllCnt[get4][nch] > 10) &&
                            (rec->get4ErrCnt[get4][nch] > 0.2 * rec->get4AllCnt[get4][nch])) {
                               DOUT0("Suspicious distance on ROC:%d Get4:%u Channel:%u fullcnt:%d errcnt:%d\n", rec->rocid, get4, nch, rec->get4AllCnt[get4][nch], rec->get4ErrCnt[get4][nch]);
                               fDetectGet4Problem = true;
                            }
                        rec->get4AllCnt[get4][nch]=0;
                        rec->get4ErrCnt[get4][nch]=0;

                        rec->get4EdgeErrs[get4][nch]=0;
                     }
                     rec->lastEpoch2SyncErr[get4]=0;
                  }
               }
            } else
            if (!rec->canCheckAnyGet4 && (dabc::Now() > fLastGet4ResetTm + 1.)) {
               DOUT0("Enable GET4 checking again for ROC%u", rec->rocid);
               rec->canCheckAnyGet4 = true;
            }

            if (fDetectGet4Problem && (fGet4ResetLimit>0.)) {
               if (dabc::Now() > fLastGet4ResetTm + fGet4ResetLimit)
                  InvokeAllGet4Reset();
            }

            break;
         }

         case roc::MSG_GET4: {  // GET4 message
            unsigned g4id = data->getGet4Number();

            if (rec->canCheckGet4[g4id] && rec->canCheckAnyGet4) {

               unsigned g4ch = data->getGet4ChNum();
               unsigned g4fl = data->getGet4Edge();

               uint64_t fulltm = data->getMsgFullTime(rec->lastEpoch2[g4id]);

               bool change_edge = ((rec->get4EdgeCnt[g4id][g4ch]>0) ^ g4fl);

               if (change_edge) {
                  rec->get4AllCnt[g4id][g4ch]++;
                  uint64_t diff2 = (fulltm - rec->get4LastTm[g4id][g4ch]);

                  // check if difference between two edges close to epoch length - 26214.4 ns
                  for (unsigned k=1;k<4;k++)
                     if ((diff2 > (k*26214 - 500)) && (diff2 < (k*26214 + 500))) {
                        rec->get4ErrCnt[g4id][g4ch]++;
                        break;
                     }

                  // check if there are many same edges one after another
                  if (abs(rec->get4EdgeCnt[g4id][g4ch])>3) {
                     // if there are few errors, just inform about them, otherwise invoke failure
                     if (rec->get4EdgeErrs[g4id][g4ch]++<3)
                        DOUT0("EDGE failure ROC:%u Get4:%u Channel:%u EdgeCnt:%d Last epoch2:%u", rec->rocid, g4id, g4ch, rec->get4EdgeCnt[g4id][g4ch], rec->lastEpoch2[g4id]);
                     else
                        fDetectGet4Problem = true;
                  }

                  // reset counter when changing sign
                  rec->get4EdgeCnt[g4id][g4ch] = 0;
               }

               rec->get4EdgeCnt[g4id][g4ch] += (g4fl ? +1 : -1);

               rec->get4LastTm[g4id][g4ch] = fulltm;

            }

            break;
         }

         case roc::MSG_SYS:  { // SYS message
            switch (data->getSysMesType()) {
               case roc::SYSMSG_SYNC_PARITY:
                  SetInfo(dabc::format("Roc%u Sync parity", rec->rocid));
                  break;
               case SYSMSG_USER:
                  SetInfo(dabc::format("Roc%u user sys message %u", rec->rocid, data->getSysMesData()));
                  if (data->getSysMesData()==roc::SYSMSG_USER_RECONFIGURE) {
                     // SetInfo("One could start checking of GET4 messages again");
                     DOUT2("One could start checking of GET4 messages again");
                     rec->canCheckAnyGet4 = true;
                  }
                  break;
               default:
                  SetInfo(dabc::format("Roc%u SysMsg type = %u", rec->rocid, data->getSysMesType()));
            }
            break;
         }

         } // end of the switch

         rec->curr_indx += iter.getMsgSize();

         if (rec->isprev) rec->data_length += iter.getMsgSize();

         if (rec->isprev && rec->isnext) {
             DOUT5("ROCID:%u Find sync events %u - %u fmt:%u", rec->rocid, rec->prev_evnt, rec->next_evnt, rec->format);

//            DOUT1("Tm: %7.5f Rocid:%u Find sync events %u - %u between %u:%u and %u:%u",
//                    TimeStamp()*1e-6, rec->rocid, rec->prev_evnt, rec->next_evnt,
//                    rec->prev_nbuf, rec->prev_indx, rec->next_nbuf, rec->next_indx);

            rec->isready = true;
            return true;
         }
      }
   }

   return false;
}


void roc::CombinerModule::InvokeAllGet4Reset()
{
   for (unsigned ninp = 0; ninp < fInp.size(); ninp++) {
      fInp[ninp].canCheckAnyGet4 = false;
      for (int n=0;n<MaxGet4;n++) {
         fInp[ninp].canCheckGet4[n] = false;
         fInp[ninp].lastEpoch2SyncErr[n] = 0;
         fInp[ninp].lastEpoch2[n] = 0;
         for(int ch=0;ch<MaxGet4Ch;ch++) {
            fInp[ninp].get4EdgeCnt[n][ch] = 0;
            fInp[ninp].get4EdgeErrs[n][ch] = 0;
            fInp[ninp].get4AllCnt[n][ch] = 0;
            fInp[ninp].get4ErrCnt[n][ch] = 0;
            fInp[ninp].get4LastTm[n][ch] = 0;
         }
      }
   }

   fLastGet4ResetTm = dabc::Now();
   fDetectGet4Problem = false;
   DOUT0("Submit GET4 reset command");
   dabc::mgr.GetApp().Submit(dabc::Command("ResetAllGet4"));
}


void roc::CombinerModule::ProcessOutputEvent(unsigned indx)
{
   FillBuffer();
}


bool roc::CombinerModule::SkipEvent(unsigned recid)
{
   if (recid>=fInp.size()) return false;

   InputRec* rec = &(fInp[recid]);

   if ((rec==0) || !rec->isprev || !rec->isnext) return false;

   rec->isready     = false;
   rec->isprev      = true;
   rec->prev_epoch  = rec->next_epoch;
   rec->prev_nbuf   = rec->next_nbuf;
   rec->prev_indx   = rec->next_indx;
   rec->prev_evnt   = rec->next_evnt;
   rec->prev_stamp  = rec->next_stamp;
   rec->data_length = 0;

   rec->isnext     = false;

   rec->nummbssync = 0;
   rec->firstmbssync = 0;

   unsigned can_skip = rec->can_skip_buf();

   if (can_skip>0) {
//      DOUT0("On input %u Skip buffers %u", recid, master_skip);

      SkipInputBuffers(recid, can_skip);

      rec->did_skip_buf(can_skip);
   }

//   DOUT4("Skip event done, search next from %u:%u",
//            rec->curr_nbuf, rec->curr_indx);

   return FindNextEvent(recid);
}

uint32_t CalcEventDistanceNew(uint32_t prev_ev, uint32_t next_ev)
{
   return (next_ev>=prev_ev) ? next_ev - prev_ev : next_ev + 0x1000000 - prev_ev;
}

uint32_t CalcAbsEventDistanceNew(uint32_t ev1, uint32_t ev2)
{
   uint32_t diff = ev1>ev2 ? ev1-ev2 : ev2-ev1;
   if (diff > 0x800000) diff = 0x1000000 - diff;
   return diff;
}


bool roc::CombinerModule::FillBuffer()
{
   // method fill output buffer with complete sync event from all available sources

   bool doagain = true;
   uint32_t evnt_cut = 0;

   while (doagain) {

      doagain = false;

      bool is_data_selected(false);

      // first check that all inputs has at least first event
      for (unsigned ninp = 0; ninp < fInp.size(); ninp++) {
         InputRec* inp = &(fInp[ninp]);
         if (!inp->isprev) return false;
         inp->use = false;
         inp->data_err = false;
      }

      // than check that if two event are defined, not too big difference between them
      for (unsigned ninp = 0; ninp < fInp.size(); ninp++) {
         InputRec* inp = &(fInp[ninp]);

         if (!inp->isready) continue;

         uint32_t diff = CalcEventDistanceNew(inp->prev_evnt, inp->next_evnt);
         if (fSyncScaleDown>0) {
            if (diff != (unsigned) fSyncScaleDown) {
               DOUT0("ROC%u: Events shift %06x -> %06x = %02x, expected %02x, ignore", inp->rocid, inp->prev_evnt, inp->next_evnt, diff, fSyncScaleDown);
               inp->data_err = true;
            }
         } else {
            if (diff > 0x100000) {
               inp->data_err = true;
               EOUT("Too large event shift %06x -> %06x = %06x on ROC%u", inp->prev_evnt, inp->next_evnt, diff, inp->rocid);
            }
         }

/*         if ((!inp->data_err) && (ninp==0)) {
            if  (inp->prev_evnt != (inp->prev_epoch & 0xffffff)) {
                EOUT("Mismatch in epoch %8x and event number %6x for ROC%u", inp->prev_epoch, inp->prev_evnt, inp->rocid);
                inp->data_err = true;
            }

            if  (inp->next_evnt != (inp->next_epoch & 0xffffff)) {
                EOUT("Mismatch in epoch %8x and event number %6x for ROC%u", inp->next_epoch, inp->next_evnt, inp->rocid);
                inp->data_err = true;
            }
         }
*/
         if (inp->data_err) {
            inp->use = true;
            is_data_selected = true;
            break;
         }
      }

      uint32_t min_evnt(0xffffff), max_evnt(0); // 24 bits

      if (!is_data_selected) {

         // try select data on the basis of the event number

         for (unsigned ninp = 0; ninp < fInp.size(); ninp++) {
            // one should have at least epoch mark define
            InputRec* inp = &(fInp[ninp]);

            if (!inp->isprev) return false;

            if (inp->prev_evnt < evnt_cut) continue;

            if (inp->prev_evnt < min_evnt) min_evnt = inp->prev_evnt;
            if (inp->prev_evnt > max_evnt) max_evnt = inp->prev_evnt;
         }

         if (min_evnt > max_evnt) {
            EOUT("Nothing found!!!");
            return false;
         }

         // try to detect case of upper border crossing
         if (min_evnt != max_evnt)
            if ((min_evnt < 0x10000) && (max_evnt > 0xff0000L)) {
               evnt_cut = 0x800000;
               doagain = true;
               continue;
            }

         evnt_cut = 0;

         for (unsigned ninp = 0; ninp < fInp.size(); ninp++) {

            InputRec* inp = &(fInp[ninp]);

            inp->use = (inp->prev_evnt == min_evnt);
            inp->data_err = false;

            if (!inp->use) continue;

            // one should have next event completed to use it for combining
            if (!inp->isready) return false;

            is_data_selected = true;

            if (ninp==0) continue;

            if (!fInp[0].use) // if first roc is not used, anyway error data
               inp->data_err = true;
            else
            if (inp->next_evnt != fInp[0].next_evnt) {
               inp->data_err = true;
               uint32_t diff = CalcAbsEventDistanceNew(inp->next_evnt , fInp[0].next_evnt);

               if (diff > 0x1000)
                  EOUT("Next event mismatch between ROC%u:%06x and ROC%u:%06x  diff = %06x",
                         fInp[0].rocid, fInp[0].next_evnt, inp->rocid, inp->next_evnt, diff);
            }
         }

      }

      if (!is_data_selected) {
         EOUT("Data not selected here - why");
         return false;
      }

      dabc::BufferSize_t totalsize = sizeof(mbs::EventHeader);
      unsigned numused(0);
      bool skip_evnt(false);

//      DOUT3("Try to prepare for event %u", min_evnt);

      for (unsigned ninp = 0; ninp < fInp.size(); ninp++) {

         InputRec* inp = &(fInp[ninp]);

         if (!inp->use) continue;

         if (inp->data_err && fSkipErrorData) skip_evnt = true;

         totalsize += sizeof(mbs::SubeventHeader);

         // one need always epoch for previous event
         totalsize += 2*roc::Message::RawSize(inp->format);

         totalsize += inp->data_length;

         numused++;
      }

      unsigned extra_size(0);

      unsigned grand_totalsize = totalsize + extra_size;

      // if not all inputs are selected in bnet mode, skip data while we need complete events

      // if we producing normal event, try to add extra messages if we have them
      if (!skip_evnt && (numused==fInp.size()) && fExtraMessages.size()>0)
         extra_size = fExtraMessages.front()->size() * roc::Message::RawSize(roc::formatNormal);

      // check if we should send previously filled data - reason that new event does not pass into rest of buffer
      if (!skip_evnt && (grand_totalsize > f_outptr.fullsize()))
         if (!FlushOutputBuffer()) return false;

      // take new buffer if required
      if (!skip_evnt && (fOutBuf.null() || (grand_totalsize > f_outptr.fullsize()))) {
         if (fOutBuf.null())
            fOutBuf = TakeBuffer();
         if (fOutBuf.null()) {
            EOUT("Cannot get buffer from pool");
            return false;
         }

         // if we cannot put extra messages, try to skip them at this event
         if (fOutBuf.GetTotalSize() < grand_totalsize)
            extra_size = 0;

         // if anyway size too big, skip event totally
         if (fOutBuf.GetTotalSize() < totalsize) {
            SetInfo( dabc::format("Cannot put event (sz:%u) in buffer (sz:%u) - skip event", totalsize, fOutBuf.GetTotalSize()) );
            skip_evnt = true;
         }
         f_outptr = fOutBuf;
      }

      if (!skip_evnt) {
         dabc::Pointer old(f_outptr);

//         DOUT5("Start MBS event at pos %u", fOutBuf.Distance(fOutBuf, f_outptr);

//         DOUT0("Building event %u", min_evnt);

         mbs::EventHeader* evhdr = (mbs::EventHeader*) f_outptr();
         evhdr->Init(min_evnt);
         f_outptr.shift(sizeof(mbs::EventHeader));

//         DOUT1("Fill event %7u  ~size %6u outsize:%u",
//            min_evnt, totalsize, f_outptr.fullsize());

//         DOUT1("Distance to data %u", fOutBuf.Distance(fOutBuf, f_dataptr);

         MessagesVector* extra = 0;

         if (extra_size>0) {
            extra = fExtraMessages.front();
            fExtraMessages.pop_front();
         }

         unsigned filled_sz = FillRawSubeventsBuffer(f_outptr, extra);

         SetInfo(dabc::format("Fill event %7u sz %u", min_evnt, filled_sz));

         //DOUT1("Build event %u", filled_sz);

         if (filled_sz == 0) {
             EOUT("Event data not filled - event skipped!!!");
             f_outptr = old;
         } else {

//            DOUT0("Calculated size = %u  filled size = %u", totalsize, filled_sz + sizeof(mbs::EventHeader));

            evhdr->SetSubEventsSize(filled_sz);
         }
      } else {
         SetInfo(dabc::format("Skip event %7u", min_evnt));
      }

      doagain = true;
      for (unsigned ninp = 0; ninp<fInp.size(); ninp++)
         if (fInp[ninp].use)
            if (!SkipEvent(ninp)) doagain = false;
   }

   return false;
}

/*! send to output all data, which is now filled into output buffer
 */

bool roc::CombinerModule::FlushOutputBuffer()
{
   // if no buffer, nothing to flush - exit normally
   if (fOutBuf.null()) return true;

   dabc::BufferSize_t usedsize = fOutBuf.GetTotalSize() - f_outptr.fullsize();
   if (usedsize==0) return true;

   // all outputs must be able to get buffer for sending
   if (!CanSendToAllOutputs()) {
      return false;
   }

//   DOUT1("Filled size = %u", usedsize);

   f_outptr.reset();

   fOutBuf.SetTotalSize(usedsize);

   fOutBuf.SetTypeId(mbs::mbt_MbsEvents);

/*
   DOUT0("Flush output buffer %u of size %u", fOutBuf.SegmentId(), (unsigned) fOutBuf.GetTotalSize());
   unsigned numevents=0;
   mbs::ReadIterator iter(fOutBuf);
   while (iter.NextEvent()) {
      DOUT0("   Flush event %u size %u", iter.evnt()->EventNumber(), iter.evnt()->FullSize());
      numevents++;
   }
   DOUT0("Flush output buffer numevents %u", numevents);
*/

   SendToAllOutputs(fOutBuf);

   fOutBuf.Release();

   fFlushFlag = false;

   return true;
}

unsigned roc::CombinerModule::FillRawSubeventsBuffer(dabc::Pointer& outptr, roc::MessagesVector* extra)
{
   unsigned filled_size = 0;

   bool iserr = false;

   for (unsigned ninp = 0; ninp < fInp.size(); ninp++) {
      InputRec* rec = &(fInp[ninp]);
      if (!rec->use) continue;

      mbs::SubeventHeader* subhdr = (mbs::SubeventHeader*) outptr();
      subhdr->Init();
      subhdr->iProcId = rec->data_err ? roc::proc_ErrEvent : roc::proc_RocEvent;
      subhdr->iSubcrate = rec->rocid;
      subhdr->iControl = rec->format; // roc::formatEth1 == 0, compatible with older lmd files

      if (rec->data_err) iserr = true;

      outptr.shift(sizeof(mbs::SubeventHeader));
      filled_size+=sizeof(mbs::SubeventHeader);

      unsigned msg_size = roc::Message::RawSize(rec->format);

      unsigned subeventsize = msg_size;

      roc::Message msg;
      msg.setMessageType(roc::MSG_EPOCH);
      msg.setRocNumber(rec->rocid);
      msg.setEpochNumber(rec->prev_epoch);
      msg.setEpochMissed(0);
      msg.copyto(outptr(), rec->format);

      outptr.shift(msg_size);

      unsigned nbuf = rec->prev_nbuf;

      bool firstmsg = true;

      while (nbuf<=rec->next_nbuf) {
         dabc::Buffer inbuf = RecvQueueItem(ninp, nbuf);
         if (inbuf.null()) {
            EOUT("Internal error");
            return 0;
         }

         // if next epoch in the same buffer, limit its size by next_indx + 6 (with next event)
         dabc::Pointer ptr = inbuf;
         if (nbuf == rec->next_nbuf) ptr.setfullsize(rec->next_indx + msg_size);
         if (nbuf == rec->prev_nbuf) ptr.shift(rec->prev_indx);

         // DOUT0("Copy to output %u has %u", ptr.fullsize(), outptr.fullsize());
         if (firstmsg && extra) {
            // if we have extra messages, put them right after first sync message
            outptr.copyfrom_shift(ptr, msg_size);
            subeventsize += msg_size;
            ptr.shift(msg_size);
            AddExtraMessagesToSubevent(extra, outptr, subeventsize, rec);
         }

         // copy rest of the data
         outptr.copyfrom_shift(ptr, ptr.fullsize());
         subeventsize += ptr.fullsize();
         firstmsg = false;

         nbuf++;
      }

      filled_size += subeventsize;
      subhdr->SetRawDataSize(subeventsize);
   }

   Par("RocEvents").SetInt(1);

   if (iserr) Par("RocErrors").SetInt(1);

   if (extra!=0) delete extra;

   return filled_size;
}

void roc::CombinerModule::AddExtraMessagesToSubevent(roc::MessagesVector* extra, dabc::Pointer& outptr, unsigned& subeventsize, InputRec* rec)
{
   unsigned msg_size = roc::Message::RawSize(rec->format);

//   int cnt = 0;

   for (unsigned n=0;n<extra->size();n++)
      if (extra->at(n).getRocNumber() == rec->rocid) {
         extra->at(n).copyto(outptr(), rec->format);
         outptr.shift(msg_size);
         subeventsize += msg_size;
//         cnt++;
      }

//   if (cnt>0) DOUT0("Add %d extra messages for ROC%u", cnt, rec->rocid);
}

void roc::CombinerModule::DumpData(dabc::Buffer& buf)
{
   roc::Iterator iter;
   if (!InitIterator(iter, buf)) return;

   while (iter.next())
      iter.msg().printData(3);
}

int roc::CombinerModule::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("ConfigureInput")) {
      int recid = cmd.GetInt("Input", 0);

      if ((recid<0) || (recid>=(int)fInp.size())) {
         EOUT("Something wrong with input configurations");
         return dabc::cmd_false;
      }
      InputRec* rec = &(fInp[recid]);

      if (!rec->isrocid()) {
         rec->isudp = cmd.GetBool("IsUdp", true);
         rec->rocid = cmd.GetInt("ROCID", 0);
         rec->format = cmd.GetInt("Format", 0);

         SetInfo(dabc::format("Configure input %u with ROC:%d Kind:%s", recid, rec->rocid, (rec->isudp ? "UDP" : "Optic")), true);
      }

      return dabc::cmd_true;

   } else

   if (cmd.IsName(CmdMessagesVector::CmdName())) {
      MessagesVector* vect = (MessagesVector*) cmd.GetPtr(CmdMessagesVector::Vector());

      if (vect!=0)
         fExtraMessages.push_back(vect);
      else
         EOUT("Zero vector with extra messages");

      if (fExtraMessages.size()>10) {
         EOUT("Too many extra messages, remove part of them");
         delete fExtraMessages.front();
         fExtraMessages.pop_front();
      }

      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}


