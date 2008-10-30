#ifndef _SC_CONSOLE_H
#define _SC_CONSOLE_H

void cmd_ls(int argc, char **argv);
void cmd_set(int argc, char **argv);
void cmd_pulse(int argc, char **argv);
void cmd_init(int argc, char **argv);
void cmd_reset(int argc_, char **argv_);
void cmd_setadc(int argc, char **argv);
void cmd_getadc(int argc, char **argv);
void cmd_SR_INIT(int argc, char **argv);
void cmd_BUFG_SELECT(int argc, char **argv);
void cmd_shift(int argc, char **argv);
void cmd_latency(int argc, char **argv);
void cmd_autodelay(int argc_, char **argv_);
void cmd_clear(int argc_, char **argv_);
void cmd_auto(int argc_, char **argv_);
void cmd_getmiss(int argc_, char **argv_);
void cmd_resetmiss(int argc_, char **argv_);
void cmd_setroc(int argc, char **argv);
void cmd_prefetch(int argc, char **argv);
void cmd_testsetup(int argc, char **argv);
void cmd_nx(int argc, char **argv);
void cmd_check_parity(int argc, char **argv);
void cmd_getaux(int argc, char **argv);
void cmd_data(int argc, char **argv);
void cmd_delay(int argc, char **argv);
void cmd_switchconsole(int argc_, char **argv_);
void cmd_help(int argc_, char **argv_);
void cmd_peek(int argc, char **argv);
void cmd_poke(int argc, char **argv);
void cmd_load(int argc, char **argv);
void cmd_save(int argc, char **argv);
void cmd_flash_access_uploadBitfile(int argc, char **argv);
void cmd_flash_access_showBitfileInfo(int argc, char **argv);
void cmd_flash_access_eraseActelFlash(int argc, char **argv);
void cmd_flash_access_readByte(int argc, char **argv);
void cmd_flash_access_readBytes(int argc, char **argv);
void cmd_flash_access_writeByte(int argc, char **argv);
void cmd_flash_access_copyChip(int argc, char **argv);
void cmd_flash_access_compareBitfiles(int argc, char **argv);
void cmd_flash_access_calcXOR(int argc, char **argv);

int executeConsoleCommand(char* cmd);

void consoleMainLoop();

#endif
