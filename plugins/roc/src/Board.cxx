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

#include "roc/Board.h"

roc::Board::Board() :
   fRole(roleNone)
{
}

roc::Board::~Board()
{
}

roc::Board* roc::Board::Connect(const char* name, BoardRole role)
{
   return 0;
}

int roc::Board::errno() const
{
   return 0;
}


bool roc::Board::poke(uint32_t addr, uint32_t value)
{
   return true;
}

uint32_t roc::Board::peek(uint32_t addr)
{
   return 0;
}

bool roc::Board::startDaq()
{
   return true;
}

bool roc::Board::suspendDaq()
{
   return true;
}

bool roc::Board::stopDaq()
{
   return true;
}

bool roc::Board::getNextData(nxyter::Data& data, double tmout)
{
   return false;
}

