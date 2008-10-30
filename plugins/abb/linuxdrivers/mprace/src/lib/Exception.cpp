#include "Exception.h"

using namespace mprace;

const char* Exception::descriptions[] = {
	"Unknown exception",
	"Kernel Memory allocation failed",
	"User Memory mmap failed",
	"Interrupt failed",
	"Device not open",
	"File Not Found",
	"Unknown file format",
	"DMA not supported",
	"Invalid FPGA number",
	"Address out of range",
	"Huge Pages open failed",
	"Huge Pages mmap failed"
};
