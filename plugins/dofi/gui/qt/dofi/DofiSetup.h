#ifndef DOFISETUP_H
#define DOFISETUP_H

#include "MuppetGui.h"

#include <vector>
#include <iostream>

/** number of input or output:*/
#define DOFI_NUM_CHANNELS 64

/** registers for invert, delay, signal length forming, start here: */
#define DOFI_SIGNALCTRL_BASE 0x0

/* signal control register bits:
 * |63......40|39.....16|15....1|0|
 * | -length- | -delay- |xxxxxxx|invert|
 *
 * 24 bit length values 0 .16777215 (TODO units!)
 * 24 bit delay valuse  0 ...16777215 (units?)
 * 1 bit inverted yes or no
 * */

/** registers to set AND relation between this output channel and input channels (bit indexed);
 * starting from this address:*/
#define DOFI_ANDCTRL_BASE 0x40

/** registers to set OR relation between this output channel and input channels (bit indexed)
 * starting from tis address*/
#define DOFI_ORCTRL_BASE 0x80

/* address of input scalers from here:*/
#define DOFI_INPUTSCALER_BASE 0xC0

/* address of output scalers from here:*/
#define DOFI_OUTPUTSCALER_BASE 0x100

class DofiSetup: public MuppetSetup
{
public:

  /* registers for input signal shaping and forming*/
  unsigned long long fSignalControl[DOFI_NUM_CHANNELS];

  /* registers for each output (array index); defining which inputs (defined by bit number) should
   * appear on output when signals fulfill AND relation*/
  unsigned long long fOutputANDBits[DOFI_NUM_CHANNELS];

  /* registers for each output (array index); defining which inputs (defined by bit number) should
   * appear on output when signals fulfill OR relation*/
  unsigned long long fOutputORBits[DOFI_NUM_CHANNELS];

  /* most recent input scaler values */
  unsigned long long fInputScalers[DOFI_NUM_CHANNELS];

  /* most recent output scaler values */
  unsigned long long fOutputScalers[DOFI_NUM_CHANNELS];


  DofiSetup () :
      MuppetSetup ()
  {
    for (int i = 0; i < DOFI_NUM_CHANNELS; ++i)
    {
      fSignalControl[i] = 0;
    }

    for (int i = 0; i < DOFI_NUM_CHANNELS; ++i)
    {
      fOutputANDBits[i] = 0;
    }
    for (int i = 0; i < DOFI_NUM_CHANNELS; ++i)
    {
      fOutputORBits[i] = 0;
    }

    for (int i = 0; i < DOFI_NUM_CHANNELS; ++i)
    {
      fInputScalers[i] = 0;
      fOutputScalers[i] = 0;
    }

  }

  void SetInputInvert (unsigned int channel, bool on)
  {
    if (channel < DOFI_NUM_CHANNELS)
    {
      on ? (fSignalControl[channel] |= 1) : (fSignalControl[channel] &= ~1);
    }
  }

  bool IsInputInvert (unsigned int channel)
  {
    if (channel >= DOFI_NUM_CHANNELS)
      return false;
    return ((fSignalControl[channel] & 0x1) == 0x1);
  }

  void SetInputDelay (unsigned int channel, int delay)
  {
    if (channel < DOFI_NUM_CHANNELS)
    {
      unsigned long long mask = (0xFFFFFF << 16);
      fSignalControl[channel] &= ~mask;
      fSignalControl[channel] |= (delay & 0xFFFFFF) << 16;
    }
  }

  int GetInputDelay (unsigned int channel)
  {
    if (channel >= DOFI_NUM_CHANNELS)
      return -1;
    int value = (fSignalControl[channel] >> 16) & 0xFFFFFF;
    return value;
  }

  void SetInputLength (unsigned int channel, int len)
  {
    if (channel < DOFI_NUM_CHANNELS)
    {
      unsigned long long mask = (0xFFFFFF << 40);
      fSignalControl[channel] &= ~mask;
      fSignalControl[channel] |= (len & 0xFFFFFF) << 40;
    }
  }

  int GetInputLength (unsigned int channel)
  {
    if (channel >= DOFI_NUM_CHANNELS)
      return -1;
    int value = (fSignalControl[channel] >> 40) & 0xFFFFFF;
    return value;
  }

  unsigned long long GetInputScaler (unsigned int channel)
  {
    if (channel >= DOFI_NUM_CHANNELS)
      return 0;
    return fInputScalers[channel];
  }

  unsigned long long GetOutputScaler (unsigned int channel)
  {
    if (channel >= DOFI_NUM_CHANNELS)
      return 0;
    return fOutputScalers[channel];
  }

  void SetOutputAND (unsigned int channel, unsigned long long mask, bool clear = false)
  {
    if (channel < DOFI_NUM_CHANNELS)
    {

      if (clear)
      {
        fOutputANDBits[channel] &= ~mask;
      }
      else
      {
        fOutputANDBits[channel] &= ~mask;
        fOutputANDBits[channel] |= mask;
      }
    }
  }

  unsigned long long GetOutputAND (unsigned int channel)
  {
    if (channel >= DOFI_NUM_CHANNELS)
      return 0;
    return fOutputANDBits[channel];
  }

  void AddOutputAND (unsigned int outchannel, unsigned int inchannel)
  {
    if (outchannel < DOFI_NUM_CHANNELS && inchannel < DOFI_NUM_CHANNELS)
    {
      unsigned long long mask = ((unsigned long long) 1 << inchannel);
      fOutputANDBits[outchannel] |= mask;
      printf("AddOutputAND -mask:0x%llx reg:0x%llx\n",mask, fOutputANDBits[outchannel]);
    }
  }

  void ClearOutputAND (unsigned int outchannel, unsigned int inchannel)
  {
    if (outchannel < DOFI_NUM_CHANNELS && inchannel < DOFI_NUM_CHANNELS)
    {
      unsigned long long mask = ((unsigned long long) 1 << inchannel);
      fOutputANDBits[outchannel] &= ~mask;
      printf("ClearOutputAND -mask:0x%llx reg:0x%llx\n",mask, fOutputANDBits[outchannel]);
    }
  }

  bool IsOutputAND (unsigned int outchannel, unsigned int inchannel)
  {
    bool rev = false;
    if (outchannel < DOFI_NUM_CHANNELS && inchannel < DOFI_NUM_CHANNELS)
    {
      unsigned long long mask = ((unsigned long long) 1 << inchannel);
      rev = ((fOutputANDBits[outchannel] & mask) == mask);
//      if(fOutputANDBits[outchannel])
//        std::cout<<"IsOutputAND out:"<<outchannel<<", in:"<<inchannel<<" register:"<< fOutputANDBits[outchannel]<<", mask:"<< mask<<"result:"<<rev <<std::endl;
    }
    return rev;
  }

  //////////////////

  void SetOutputOR (unsigned int channel, unsigned long long mask, bool clear = false)
  {
    if (channel < DOFI_NUM_CHANNELS)
    {

      if (clear)
      {
        fOutputORBits[channel] &= ~mask;
      }
      else
      {
        fOutputORBits[channel] &= ~mask;
        fOutputORBits[channel] |= mask;
      }
    }
  }

  unsigned long long GetOutputOR (unsigned int channel)
  {
    if (channel >= DOFI_NUM_CHANNELS)
      return 0;
    return fOutputORBits[channel];
  }

  void AddOutputOR (unsigned int outchannel, unsigned int inchannel)
  {
    if (outchannel < DOFI_NUM_CHANNELS && inchannel < DOFI_NUM_CHANNELS)
    {
      unsigned long long mask = ((unsigned long long) 1 << inchannel);
      fOutputORBits[outchannel] |= mask;
    }
  }

  void ClearOutputOR (unsigned int outchannel, unsigned int inchannel)
  {
    if (outchannel < DOFI_NUM_CHANNELS && inchannel < DOFI_NUM_CHANNELS)
    {
      unsigned long long mask = ((unsigned long long) 1 << inchannel);
      fOutputORBits[outchannel] &= ~mask;
    }
  }

  bool IsOutputOR (unsigned int outchannel, unsigned int inchannel)
   {
     bool rev = false;
     if (outchannel < DOFI_NUM_CHANNELS && inchannel < DOFI_NUM_CHANNELS)
     {
       unsigned long long mask = ((unsigned long long) 1 << inchannel);
       rev = ((fOutputORBits[outchannel] & mask) == mask);
     }
     return rev;
   }


  void Dump ()
  {
    // we speed this up by caching string before AppendTextWindow instead of separate printm JAM 7-03-23
    QString results = "==== Dofi device status dump: ====\n";
    for (int outc = 0; outc < DOFI_NUM_CHANNELS; ++outc)
    {
      results.append (QString ("Output %1 \n").arg (outc));
      results.append ("--Inputs ORed:\n\t");
      bool none = true;
      int form=0;
      for (int inc = 0; inc < DOFI_NUM_CHANNELS; ++inc)
      {
        if (IsOutputOR (outc, inc))
        {
          results.append (QString ("\t %1").arg (inc));
          none = false;
          if ((form++ % 8) == 7)
            results.append ("\n");
        }
      }
      if (none)
        results.append ("NONE");
      results.append(" \n");
      results.append ("--Inputs ANDed:\n\t");
      none = true;
      form=0;
      for (int inc = 0; inc < DOFI_NUM_CHANNELS; ++inc)
      {
        if (IsOutputAND (outc, inc))
        {
          none = false;
          results.append (QString (" %1").arg (inc));
          if ((form++ % 8) == 7)
            results.append ("\n");
        }
      }
      if (none)
        results.append (" NONE");
      results.append (" \n");
    }    // outc

    results.append ("\n---------------\nInput signals setup:\n");
    for (int inc = 0; inc < DOFI_NUM_CHANNELS; ++inc)
    {
      results.append (
          QString ("Input %1 length:%2 delay %3 inverted:%4\n").arg (inc).arg (GetInputLength (inc), 0, 16).arg (
              GetInputDelay (inc), 0, 16).arg (IsInputInvert (inc)));
    }

    results.append ("\n---------------\nInput scalers:\n");
    for (int inc = 0; inc < DOFI_NUM_CHANNELS; ++inc)
    {
      results.append(QString("\t%1:%2").arg(inc).arg(GetInputScaler(inc)));
          if (inc % 8 == 7)
            results.append ("\n");
    }
    results.append ("\n---------------\nOutput scalers:\n");
        for (int oc = 0; oc < DOFI_NUM_CHANNELS; ++oc)
        {
          results.append (QString ("\t%1:%2").arg (oc).arg (GetOutputScaler (oc)));
          if (oc % 8 == 7)
            results.append ("\n");
        }

        MuppetGui::fInstance->AppendTextWindow (results);
        MuppetGui::fInstance->FlushTextWindow();
      }

};

#endif
