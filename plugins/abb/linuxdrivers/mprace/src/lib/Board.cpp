/*******************************************************************
 * Change History:
 * 
 * $Log: Board.cpp,v $
 * Revision 1.5  2008/04/07 16:09:03  adamczew
 * *** empty log message ***
 *
 * Revision 1.2  2007-03-02 14:58:23  marcus
 * DMAEngineWG basic functionality working.
 *
 * Revision 1.1  2007/02/12 18:09:13  marcus
 * Initial commit.
 *
 *******************************************************************/

#include "Board.h"
#include "Logger.h"
#include "Exception.h"

using namespace mprace;

void Board::enableLog() {
	log = new Logger();
}

void Board::disableLog() {
	delete log;
	log=0;
}

Board::~Board() {
	if (log != 0)
		delete log;
}

void Board::writeBlock(const unsigned int address, const unsigned int *data, const unsigned int count, const bool inc) {
	unsigned int i;
	
	for( i=0 ; i<count ; i++ )
		this->write(address+i,*(data+i));
}

void Board::readBlock(const unsigned int address, unsigned int *data, const unsigned int count, const bool inc) {
	unsigned int i,j;
	
	for( i=0 ; i<count ; i++ )
		*(data+i) = this->read(address+i);
}

DMAEngine& Board::getDMAEngine() {
	throw Exception( Exception::DMA_NOT_SUPPORTED );
}

void Board::writeDMA(const unsigned int address, const DMABuffer& buf, const unsigned int count, const unsigned int offset, const bool inc, const bool lock) {
	throw Exception( Exception::DMA_NOT_SUPPORTED );
}

void Board::readDMA(const unsigned int address, DMABuffer& buf, const unsigned int count, const unsigned int offset, const bool inc, const bool lock) {
	throw Exception( Exception::DMA_NOT_SUPPORTED );
}
