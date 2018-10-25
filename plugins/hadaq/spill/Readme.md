# Spill analyzer

Implemented in stream framework

Requires DABC build with stream

     . path/streamlogin
     make -j8

Code initialized with attached first.C

Combiner module is standard DAQ task, hadaq::SpillProcessor gets its all data.
Concrete implementation should be done in stream framework
