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
#ifndef F_FIND_PEAKS
#define F_FIND_PEAKS

#define TYPE__SHORT  0
#define TYPE__INT    1
#define TYPE__FLOAT  2
#define TYPE__DOUBLE 3
void f_find_peaks(
        void *,    // pointer to data
        int,       // data type: use definitions above
        int,       // first channel (0,...)
        int,       // number of channels
        int,       // channels to sum up (bin size in channels)
        double,    // noise factor to square root
        double,    // noise minimum (absolut)
        double *,  // array with noise (original channels)
        int,       // Maximum number of peaks. Arrays below must have this size.
        int *,     // returns number of peaks found
        int *,     // pointer to array of minima
        double *,  // pointer to array of minima values
        int *,     // pointer to array of maxima
                  int *,     // pointer to array of width
        double *); // pointer to array of integrals

#endif
