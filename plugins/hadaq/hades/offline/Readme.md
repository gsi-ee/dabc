# Offline analysis of calibration run

Allows to process offline HLD data taken during calibration.
How to use:

1. Copy db file from TRB3 configuration to this directory
or provide correct file name in `read_db` function. Check that
condition to recognize begin/end of block is correct.

2. Check that number of channels depending on HUB id is correct.
   See `hld->SetCustomNumCh()` call which assign function

3. Check that custom ToT RMS is correct. See `getTotRms` function.

4. Check that any other custom configurations like paired channels
   in TOF are configured properly. All this settings can be found in
   BnrtInputHades.xml file

5. Try to run analysis.
