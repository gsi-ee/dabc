#ifndef DABC_timing
#define DABC_timing

namespace dabc {
    
#if defined (__x86_64__) || defined(__i386__)
/* Note: only x86 CPUs which have rdtsc instruction are supported. */
   typedef unsigned long long cycles_t;
   inline cycles_t GetFastClock()
   {
      unsigned low, high;
      unsigned long long val;
      asm volatile ("rdtsc" : "=a" (low), "=d" (high));
      val = high;
      val = (val << 32) | low;
      return val;
   }
#elif defined(__PPC__) || defined(__PPC64__)
/* Note: only PPC CPUs which have mftb instruction are supported. */
/* PPC64 has mftb */
   typedef unsigned long cycles_t;
   inline cycles_t GetFastClock()
   {
      cycles_t ret;

      asm volatile ("mftb %0" : "=r" (ret) : );
      return ret;
   }
#elif defined(__ia64__)
/* Itanium2 and up has ar.itc (Itanium1 has errata) */
   typedef unsigned long cycles_t;
   inline cycles_t GetFastClock()
   {
      cycles_t ret;
      asm volatile ("mov %0=ar.itc" : "=r" (ret));
      return ret;
   }

#else
   #include <stdint.h>
   #define DABC_SLOWTIMING
   typedef uint64_t cycles_t;
   extern cycles_t GetFastClock();
#endif
 
   extern cycles_t GetSlowClock();

   typedef double TimeStamp_t;
   
   extern const TimeStamp_t NullTimeStamp;

   class TimeSource {
      public:
         TimeSource(bool usefast = true);
         TimeSource(const TimeSource &src);
         TimeSource& operator=(const TimeSource& rhs);
         virtual ~TimeSource() {}

         bool IsUseFastClock() const { return fUseFast; }

         inline cycles_t GetClock() const
            { return fUseFast ? GetFastClock() : GetSlowClock(); }

         inline TimeStamp_t GetTimeStamp() const 
            { return (GetClock() - fClockShift) * fTimeScale + fTimeShift; }
            
         inline TimeStamp_t CalcTimeStamp(cycles_t clock) 
            { return (clock - fClockShift) * fTimeScale + fTimeShift; }
        
         void SetCalibr(double scale, cycles_t shift) 
         {
            fTimeScale = scale; 
            fTimeShift = shift; 
         }
         
         void AdjustTimeStamp(TimeStamp_t stamp);
         void AdjustCalibr(double dshift = 0., double dscale = 1.);
         
         static void SetForceSlowTiming(bool on = true) { gForceSlowTiming = on; }

      protected:
         bool      fUseFast;
         cycles_t  fClockShift;
         double    fTimeScale;
         double    fTimeShift;
         
         static bool gForceSlowTiming;
   };
   
   extern TimeSource gStamping;
   
   extern void MicroSleep(int microsec = 0);
   extern void LongSleep(int sec = 0);
   extern void ShowLongSleep(const char* title, int sec);
   
   // different functions to operate with time stamps
   
   // calculates distance in seconds between time stamps
   inline double TimeDistance(TimeStamp_t tm1, TimeStamp_t tm2)
   {
      return (tm2-tm1)*1e-6;
   }
   
   // shifts value of time stamp on given interval in seconds
   inline TimeStamp_t TimeShift(TimeStamp_t tm, double shift_sec)
   {
      return tm + shift_sec * 1e6;
   }
   
   inline bool IsNullTime(TimeStamp_t tm)
   {
      return tm <= 0.;
   }
}

#define TimeStamp(arg) dabc::gStamping.GetTimeStamp()

#endif

