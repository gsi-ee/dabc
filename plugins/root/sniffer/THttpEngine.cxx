// $Id$
// Author: Sergey Linev   21/12/2013

#include "THttpEngine.h"

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// THttpEngine                                                          //
//                                                                      //
// Abstract class for implementing http protocol for THttpServer        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


THttpEngine::THttpEngine(const char* name, const char* title) :
   TNamed(name, title),
   fServer(0)
{
   // normal constructor
}

THttpEngine::~THttpEngine()
{
   // destructor

   fServer = 0;
}
