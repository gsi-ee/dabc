#ifndef SD_READER_H
#define SD_READER_H

#include "xbasic_types.h"

int load_file_from_sd_card(char* filename, Xuint8* target);
int save_file_to_sd_card(char* filename, Xuint8* target);
Xuint32 define_sd_file_size(char* filename);

#endif
