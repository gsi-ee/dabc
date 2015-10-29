# FPGA TDC calibration {#hadaq_tdc_calibr}

  
## Basics 


### That is measured by FPGA TDC?
 
FPGA TDC for each hit provides values of two counter.
First is **epoch** counter, incremented with 200 MHz frequency.
Second is **fine** counter, which starts when hit is registered and ends when new epoch is registered.
To produce time stamp for registered hit we should use following equation:

    STAMP = epoch*5ns - Calibr(fine)
    
One should understand - larger value of **fine** counter means longer distance to the next epoch, but smaller value of time stamp.

Form of **Calibr** function individual for each channel and can vary with temperature, core voltage or other factors.
          

### Can one use liner approximation?




### Statistical approach


### Effect of temparature on the measurement



## TDC calibration with `stream` frameowrk




## TDC calibration in `DABC`

