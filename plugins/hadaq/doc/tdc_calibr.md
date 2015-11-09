# FPGA TDC calibration {#hadaq_tdc_calibr}

  
## Basics 


### That is measured by FPGA TDC?
 
FPGA TDC for each hit provides values of two counters.
First is **epoch** counter, incremented with 200 MHz frequency.
Second is **fine** counter, which starts when hit is detected and ends when new epoch is registered.
To produce time stamp for registered hit we should use following equation:

    STAMP = epoch*5ns - Calibr(fine)
    
One should understand - larger value of **fine** counter means longer distance to the next epoch, but smaller value of time stamp.

Form of **Calibr** function individual for each channel and can vary with temperature, core voltage or other factors.
          

### Can one use liner approximation?

In general yes - one could use liner approximation for **Calibr(fine)** function. Typically such function characterized by min and max value of **fine** counter bin. Minimal fine counter bin value (typically ~30) correspond to 0 shift of time stamp, maximal fine counter (around 450-500) to 5 ns shift.   

Main (and probably only) advantage of linear approximation - it is simple. 

But liner approximation has several drawbacks. First of al, it introduces error into stamp value in the order of ~200 ps. It could be acceptable for signals with worse resultion. But another typical effect of linear approximation - it introduces double-peak structure on the signals which does not have double peaks at all. Especially such effect can be very well seen with test pulses.   



### Statistical approach for fine-counter calibration

Approach is very simple - for _every_ channel one measures many (~1e5 - 1e6) hits from _random_ signal and build distribution for fine-counter values. Higher value in such distribution - larger width of correspondent bin. While sum of all bins widths is 5 ns, one can very easily calculate time shift corresponding to each fine counter.

At the moment it is best-known method for calibration of fine counter. 

But it has some disadvantages. First - it typically requires special measurements to perform calibration. Potentially one could use _normal_ hits, but not allways normal measurements provide enough statistic for all channels. With less statistic precision will be much worse. 

Another problem of such calibration approach - significant size of produced calibration curves. For every channel one creates lookup table with approx 500 bins. If one uses setup with 1000 channels (not very big), every calibration set consume 1000 channels _X_ 500bins _X_ 4bytes _=_ 2 MB of data.             


### Effect of temparature on the measurements

Effect is clear - calibration curves changing with the temperature. Such changes can be compensated if perfrom calibration frequently. In ideal case calibration should be done constantly - every time when temperature changes more than 0.5 grad. 

If calibration procedure is not performed, temperature changes will affect mean and rms values. Very rough estimation gives 10 ps change for temperature change on 1 grad. Potentially one could compensate tempreture changes without constantly calibrating TDCs channels , but at the moment it is not fully clear how. 


### Time stamp of falling edges - time-over-threshold measurement

FPGA TDC introducing time shifts for the falling edge of measured signals. Such time shift is individual for every channel and vary between 28 and 42 ns. This value should be subtracted from every fallind edge time stamp. How to measure this values?

Most simple approach - use internal pulse generator, which actiavted with trigger type 0xD. This signal has pulse width 30 ns. If one measures distance between rising and falling edges in this test pulse, one could easily estimate time shift, which could be later applyed for every falling-edge stamp.  
   
As result, to obtain time-over-threshole (TOT) value, one should do:

    TOT = Tfalling - time_shift - Trisisng


## TDC calibration with `stream` frameowrk






## TDC calibration in `DABC`

