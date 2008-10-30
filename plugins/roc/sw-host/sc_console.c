#include "sc_console.h"

#include "syscoreshell.h"

#include "tty.h"

typedef struct {
  char   *name;
  char   *desc;
  int     hide;
  void   (*fp)(int, char **);
} cmds_t;


static cmds_t console_cmds[] = {
  {name: "ls",     desc: "List Registers", hide: 0, fp: cmd_ls},
  {name: "ll",     desc: "List Registers", hide: 0, fp: cmd_ls},
  {name: "help",   desc: "Print Help",     hide: 0, fp: cmd_help},
  {name: "auto",    desc: "Automatically detect the NX-ADC-Latency",   hide: 0, fp: cmd_auto},
  {name: "set",    desc: "Set Register",   hide: 0, fp: cmd_set},
  {name: "latency",    desc: "Set LATENCY between NX and ADC",   hide: 0, fp: cmd_latency},
  {name: "shift",    desc: "Set SR_INIT and BUFG_SELECT",   hide: 0, fp: cmd_shift},
  {name: "SR_INIT",    desc: "Set SR_INIT",   hide: 0, fp: cmd_SR_INIT},
  {name: "BUFG",    desc: "Set BUFG_SELECT",   hide: 0, fp: cmd_BUFG_SELECT},
  {name: "pulse",  desc: "TestPulseOff",   hide: 0, fp: cmd_pulse},
  {name: "data",   desc: "NX Data Out",    hide: 0, fp: cmd_data},
  {name: "init",   desc: "NX INIT",        hide: 0, fp: cmd_init},
  {name: "reset",  desc: "NX RESET",       hide: 0, fp: cmd_reset},
  {name: "clear",  desc: "FIFO RESET",     hide: 0, fp: cmd_clear},
  {name: "delay",  desc: "Set LT DELAY",     hide: 0, fp: cmd_delay},
  {name: "autodelay", desc: "Automatically detect the LT-Delay", hide: 0, fp: cmd_autodelay},
  {name: "cmd_setadc",  desc: "Set ADC-REG",       hide: 0, fp: cmd_setadc},
  {name: "cmd_getadc",  desc: "Get ADC-DATA",       hide: 0, fp: cmd_getadc},
  {name: "getmiss", desc: "Get the number of missed events", hide:0, fp: cmd_getmiss},
  {name: "resetmiss", desc: "Reset the missed events counter", hide:0, fp: cmd_resetmiss},
  {name: "roc", desc: "Set the ROC-Number", hide:0, fp: cmd_setroc},
  {name: "prefetch", desc: "Set the Prefetcher ON or OFF", hide:0, fp: cmd_prefetch},
  {name: "testsetup", desc: "Set the I2C-Registers for Testpulse", hide:0, fp: cmd_testsetup},
  {name: "nx", desc: "Choose the NX to control", hide:0, fp: cmd_nx},
  {name: "pc", desc: "Activate / Deactivate the HW parity checker", hide:0, fp: cmd_check_parity},
  {name: "switchconsole", desc: "Switch command input from/to ethernet/serial", hide:0, fp: cmd_switchconsole},
  {name: "uploadBitfile", desc: "Upload a bitfile from DDR into Actel Flash at numbered position", hide:0, fp: cmd_flash_access_uploadBitfile},
  {name: "showBitfileInfo", desc: "Show Information about Bitfiles stored in Actel Flash", hide:0, fp: cmd_flash_access_showBitfileInfo},
  {name: "readActelByte", desc: "Read a byte from Actel flash chip at specific address", hide:0, fp: cmd_flash_access_readByte},
  {name: "readActelBytes", desc: "Read some bytes from Actel flash chip starting at specific address", hide:0, fp: cmd_flash_access_readBytes},
  {name: "writeActelByte", desc: "Write byte to Actel flash chip at specific address", hide:0, fp: cmd_flash_access_writeByte},
  {name: "eraseActelFlash", desc: "Erase Actel flash chip number", hide:0, fp: cmd_flash_access_eraseActelFlash},
  {name: "copyChip", desc: "Copy chip 1 to chip 0 reverse header byteorder.", hide:0, fp: cmd_flash_access_copyChip},
  {name: "compareBitfiles", desc: "Compare Bitfiles in Chip 0 and 1.", hide:0, fp: cmd_flash_access_compareBitfiles},
  {name: "calcBinXOR", desc: "calculates the XOR checksum over the bin file", hide:0, fp: cmd_flash_access_calcXOR},
  {name: "loadCfg", desc: "Load configuration file from SD-Card", hide:0, fp: cmd_load},
  {name: "writeCfg", desc: "Write configuration file to SD-Card", hide:0, fp: cmd_save},
  {name: "getaux", desc:"Get Aux Data", hide:0, fp: cmd_getaux},
  {name: "exit", desc: "Exit from console main loop", hide:0, fp: 0},
  {name: "peek", desc: "", hide:1, fp: cmd_peek},
  {name: "poke", desc: "", hide:1, fp: cmd_poke},
  {name: 0, desc: 0,  hide: 1, fp: 0},
};

int str2int (char *str)
{
   char *p=NULL;
   int r = strtol(str, &p, 0);
   if (*p!='\0') sc_printf("Warning: Parameter %s contains non-numeric symbols.\r\n", str);
   return r;
}


void cmd_ls(int argc, char **argv) 
{
   int i,v=0;
   
   if (argc == 1){
      for (i=0; i<=45; i++) {
         v = read_I2C(NX_attribute[gC.NX_Number].Connector, i);
         sc_printf("Register %d: %X\t%3d\r\n", i, v, v);
      }      
      sc_printf("FIFO full: %X\r\n", XIo_In32(NX_FIFO_FULL));
      sc_printf("FIFO empty: %X\r\n", XIo_In32(NX_FIFO_EMPTY));
   } else {
      i = str2int(argv[1]);
      sc_printf("Register %d: %X\r\n", i, read_I2C(NX_attribute[gC.NX_Number].Connector, i));
   }   
}

void cmd_set(int argc, char **argv) 
{
   if (argc!=3) {
     sc_printf("Syntax: \r\nset NX REG VALUE\r\n");
     return;
   }  
   if (NX_attribute[gC.NX_Number].Connector==1) {
      XIo_Out32(I2C1_REGISTER, str2int(argv[1]));
      XIo_Out32(I2C1_DATA, str2int(argv[2]));
   } else if (NX_attribute[gC.NX_Number].Connector==2)  {
      XIo_Out32(I2C2_REGISTER, str2int(argv[1]));
      XIo_Out32(I2C2_DATA, str2int(argv[2]));
   }   
   cmd_ls(2, argv);
}


void cmd_clear(int argc_, char **argv_) 
{
   XIo_Out32(FIFO_RESET, 1);
   XIo_In32(OCM_REG1);
   XIo_In32(OCM_REG2);
   XIo_In32(OCM_REG3);
   XIo_Out32(FIFO_RESET, 0);
}

void cmd_pulse(int argc, char **argv) 
{
   if (argc==3) {
      XIo_Out32(FIFO_RESET, 1);
      XIo_Out32(FIFO_RESET, 0);
      
      XIo_Out32(TP_TP_DELAY, str2int(argv[1]) - 1);
      XIo_Out32(TP_COUNT, str2int(argv[2]) * 2);
      XIo_Out32(TP_START, 1);
   } else if (argc==4) {
      XIo_Out32(TP_RESET_DELAY, str2int(argv[1]) - 1);
      XIo_Out32(TP_TP_DELAY, str2int(argv[2]) - 1);
      XIo_Out32(TP_COUNT, str2int(argv[3]) * 2);

      XIo_Out32(NX_INIT, 1);
      usleep(200);
      XIo_Out32(NX_RESET, 0);
      usleep(200);
      XIo_Out32(FIFO_RESET, 1);
      usleep(200);
      XIo_Out32(FIFO_RESET, 0);
      usleep(200);
      XIo_Out32(NX_RESET, 1);
      usleep(2000);
      XIo_Out32(NX_INIT, 0);
   } else {
     sc_printf ("Syntax: pulse reset_delay pulse_pulse_delay number\r\n");
     sc_printf ("    or: pulse pulse_pulse_delay number\r\n");
     return;
   }  
}

void cmd_init(int argc, char **argv) 
{
   if (argc!=2) {
     sc_printf("Syntax: \r\ninit [0|1]\r\n");
     return;
   }  
   XIo_Out32(NX_INIT, str2int(argv[1]));
}


void cmd_reset(int argc_, char **argv_) 
{
   nx_reset();
}


void cmd_setadc(int argc, char **argv) 
{
   int nx=NX_attribute[gC.NX_Number].Connector;

   if (argc!=3) {
     sc_printf ("Syntax: \r\nsetadc ADDR VALUE\r\n");
     return;
   }  
   
   if (nx==1) {   
      XIo_Out32(ADC_ADDR, str2int(argv[1]));
      XIo_Out32(ADC_REG, str2int(argv[2]));
        usleep(20000);
       //Write data to used Registers
      XIo_Out32(ADC_ADDR, 255);
      XIo_Out32(ADC_REG, 1);
      usleep(20000);
      //Readback
      XIo_Out32(ADC_ADDR, str2int(argv[1]));
      XIo_Out32(ADC_READ_EN, 1);
      usleep(2000);
      sc_printf ("ADC-ANSWER: ");
      sc_printf ("%X", XIo_In32(ADC_ANSWER));
   } else if (nx==2) {
      XIo_Out32(ADC_ADDR2, str2int(argv[1]));
      XIo_Out32(ADC_REG2, str2int(argv[2]));
        usleep(20000);
       //Write data to used Registers
      XIo_Out32(ADC_ADDR2, 255);
      XIo_Out32(ADC_REG2, 1);
      usleep(20000);
      //Readback
      XIo_Out32(ADC_ADDR2, str2int(argv[1]));
      XIo_Out32(ADC_READ_EN2, 1);
      usleep(2000);
      sc_printf ("ADC-ANSWER: ");
      sc_printf ("%X", XIo_In32(ADC_ANSWER2));
   }
}


void cmd_getadc(int argc, char **argv) 
{
   int nx=NX_attribute[gC.NX_Number].Connector;

   if (argc!=2) {
     sc_printf ("Syntax: \r\ngetadc ADDR\r\n");
     return;
   }  
   sc_printf ("ADC-ANSWER: ");
   if (nx==1) {   
       XIo_Out32(ADC_ADDR, str2int(argv[1]));
      XIo_Out32(ADC_READ_EN, 1);
      usleep(2000);
      sc_printf ("%X", XIo_In32(ADC_ANSWER));
   } else if (nx==2) {
       XIo_Out32(ADC_ADDR2, str2int(argv[1]));
      XIo_Out32(ADC_READ_EN2, 1);
      usleep(2000);
      sc_printf ("%X", XIo_In32(ADC_ANSWER2));
   }
   sc_printf ("\r\n");
}


void cmd_SR_INIT(int argc, char **argv) 
{
   int con=NX_attribute[gC.NX_Number].Connector;
   
   if (argc!=2) {
     sc_printf("Syntax: \r\nSR_INIT VALUE\r\n");
     return;
   }  
   if (con==1)   { 
      XIo_Out32(SR_INIT, str2int(argv[1]));
      gC.rocSrInit1 = str2int(argv[1]);
   }   
   if (con==2)   {
      XIo_Out32(SR_INIT2, str2int(argv[1]));
      gC.rocSrInit2 = str2int(argv[1]);
   }   
}


void cmd_BUFG_SELECT(int argc, char **argv) 
{
   int con=NX_attribute[gC.NX_Number].Connector;
   
   if (argc!=2) {
     sc_printf("Syntax: \r\nBUFG_SELECT VALUE\r\n");
     return;
   }  
   if (con==1) {
      XIo_Out32(BUFG_SELECT, str2int(argv[1]));
      gC.rocBufgSelect1 = str2int(argv[1]);
   }   
   if (con==2) {
      XIo_Out32(BUFG_SELECT2, str2int(argv[1]));
      gC.rocBufgSelect2 = str2int(argv[1]);
   }   
}


void cmd_shift(int argc, char **argv) 
{
   if (argc!=2) {
     sc_printf("Syntax: \r\nshift VALUE\r\n");
     return;
   }
   shiftit(str2int(argv[1]));
}

void cmd_latency(int argc, char **argv) 
{
   int nx=gC.NX_Number;
   
   if (argc!=2) {
     sc_printf("Syntax: \r\nlatency VALUE\r\n");
     return;
   }  
   if (nx==0) {
      XIo_Out32(ADC_LATENCY1, str2int(argv[1]));
      gC.rocAdcLatency1 = str2int(argv[1]);
   }   
   if (nx==1) {
      XIo_Out32(ADC_LATENCY2, str2int(argv[1]));
      gC.rocAdcLatency2 = str2int(argv[1]);
   }   
   if (nx==2) {
      XIo_Out32(ADC_LATENCY3, str2int(argv[1]));
      gC.rocAdcLatency1 = str2int(argv[1]);
   }   
   if (nx==3) {
      XIo_Out32(ADC_LATENCY4, str2int(argv[1]));
      gC.rocAdcLatency2 = str2int(argv[1]);
   }   
}

void cmd_autodelay(int argc_, char **argv_) 
{
   nx_autodelay();
}


void cmd_auto(int argc_, char **argv_) 
{
   nx_autocalibration();
}

void cmd_getmiss(int argc_, char **argv_) 
{
   sc_printf("Missed events: %d\r\n", XIo_In32(NX_MISS));
}

void cmd_resetmiss(int argc_, char **argv_) 
{
   XIo_Out32(NX_MISS_RESET, 1);
   XIo_Out32(NX_MISS_RESET, 0);
}

void cmd_setroc(int argc, char **argv) 
{
   if (argc!=2) {
     sc_printf("Syntax: \r\nroc [number]\r\n");
     return;
   }  
   XIo_Out32(HW_ROC_NUMBER, str2int(argv[1]));
   gC.rocNumber = str2int(argv[1]);
}

void cmd_prefetch(int argc, char **argv) 
{
   if (argc!=2) {
     sc_printf("Syntax: \r\nprefetch [0|1]\r\n");
     return;
   }  
   prefetch(str2int(argv[1]));
}

void cmd_testsetup(int argc, char **argv) 
{
   testsetup();
}

void cmd_nx(int argc, char **argv) 
{
   if (argc==2) {
      set_nx(str2int(argv[1]), 1);
   } else if (argc==5) {
     activate_nx(str2int(argv[1]), str2int(argv[2]), str2int(argv[3]), str2int(argv[4]));
   } else {   
     sc_printf ("Syntax: nx number\r\n");
     sc_printf ("    or: nx number active Connector I2C_SlaveAddr\r\n");
   } 
   
}

void cmd_check_parity(int argc, char **argv) 
{
   if (argc==2) {
     XIo_Out32(CHECK_PARITY, str2int(argv[1]));
   } else {   
     sc_printf ("Syntax: pc [0|1]\r\n");
   } 
   
}

void cmd_getaux(int argc, char **argv)
{
   Xuint32 low, high, TS, Epoch, State, Channel, Type, ROC, lead;
  
   low = XIo_In32(AUX_DATA_LOW);
   high = XIo_In32(AUX_DATA_HIGH);
   sc_printf ("AUX: %X.%X\r\n", high, low);
      
   Type = high>>13;
   if (Type==3) {
      ROC = (high>>10)&0x7;
      Channel = (high>>8)&0x3;
      TS = ((high&0xFF)<<6)|((low>>26)&0x3f);
      Epoch = (low>>2)&0xffffff;
      State = (low&0x3);
      sc_printf("Synch -- ROC: %X, Channel: %X, TS: %X, Epoch: %X, State: %X\r\n", ROC, Channel, TS, Epoch, State);
   } else if (Type==4) {
      ROC = (high>>10)&0x7;
      Channel = (high>>3)&0x7F;
      TS = ((high&0x7)<<11)|((low>>20)&0x7ff);
      lead = (low>>19)&0x1;
      sc_printf("AUX -- ROC: %X, Channel: %X, TS: %X, Leading: %X\r\n", ROC, Channel, TS, lead);
   } else if (Type==7) {
      ROC = (high>>10)&0x7;
      Channel = (high>>8)&0x3;
      Epoch = (low>>2)&0xffffff;
      State = (low&0x3);
      sc_printf("ParityError -- ROC: %X, Channel: %X, Epoch: %X, State: %X\r\n", ROC, Channel, Epoch, State);
   } else {
     sc_printf("Type Error!");
   }
}

void cmd_data(int argc, char **argv) 
{
   int i;
   
   if (argc==1) {
      decode(0);      
   } else {
      for (i=1; i<=str2int(argv[1]); i++) {
        if (XIo_In32(NX_FIFO_EMPTY)==0) {
          if (argc==2) {
             decode(1); //decode(1) falls nur im Fehlerfall Daten angezeigt werden sollen, sonst decode (0)
           } else {
             decode(0); 
          }             
        } 
      }  
   }
}

void cmd_delay(int argc, char **argv) 
{
   if (argc!=2) {
     sc_printf("Syntax: \r\ndelay VALUE\r\n");
     return;
   }
   
   int delay = str2int(argv[1]);
   
   if (gC.NX_Number==0) {
      XIo_Out32(NX1_DELAY, delay);
      gC.rocNX1Delay = delay;
   }
   if (gC.NX_Number==1) {
      XIo_Out32(NX2_DELAY, delay);
      gC.rocNX2Delay = delay;
   }   
   if (gC.NX_Number==2) {
      XIo_Out32(NX3_DELAY, delay);
      gC.rocNX3Delay = delay;
   }   
   if (gC.NX_Number==3) {
      XIo_Out32(NX4_DELAY, delay);
      gC.rocNX4Delay = delay;
   }
}

void cmd_switchconsole(int argc_, char **argv_)
{
   sc_printf("Switching console ...\r\n");
   
   if(gC.global_consolemode == 1)//ttymode
      gC.global_consolemode = 0;
   else
      gC.global_consolemode = 1;
}

void cmd_help(int argc_, char **argv_) 
{
  sc_printf("Available commands: \r\n");
  cmds_t *c = console_cmds;
  while (c->name) {
    if (c->hide == 0) 
      sc_printf("  %14s  %s\r\n", c->name, c->desc);
    c++;
  }
}

void cmd_peek(int argc, char **argv) 
{
   if (argc!=2) {
     sc_printf ("Syntax: \r\npeek addr\r\n");
     return;
   }  
   Xuint32 val;
   Xuint8 rawdata[MAX_UDP_PAYLOAD];
   Xuint32 rawdatasize = 0;
   
   sc_printf("%d, %d", peek(str2int(argv[1]), &val, rawdata, &rawdatasize), val);
}

void cmd_poke(int argc, char **argv) 
{
   if (argc < 3) {
      sc_printf ("Syntax: \r\npoke addr val\r\n");
      return;
   }  
   sc_printf("%d", poke(str2int(argv[1]), str2int(argv[2]), (argc < 4 ? 0 : argv[3])));
}

void cmd_load(int argc, char ** argv) 
{
   if (argc!=2)
      load_config(defConfigFileName);
   else    
      load_config(argv[1]);
}

void cmd_save(int argc, char **argv) 
{
   if (argc!=2)
      save_config(defConfigFileName);
   else
      save_config(argv[1]);
}

void cmd_flash_access_uploadBitfile(int argc, char **argv)
{
   if (argc != 2)
   {
      sc_printf("Syntax: \r\n uploadBitfile [number] \r\n");
      return;
   }
   flash_access_flashBitfile(global_file_buffer, atoi(argv[1]));
}

void cmd_flash_access_showBitfileInfo(int argc, char **argv)
{
   if (argc != 1)
   {
      sc_printf("Syntax: \r\n showBitfileInfo \r\n");
      return;
   }
   flash_access_showBitfileInformation();
}

void cmd_flash_access_eraseActelFlash(int argc, char **argv)
{
   if (argc != 2)
   {
      sc_printf("Syntax: \r\n eraseActelFlash [chip number] \r\n");
      return;
   }
   flash_access_chipErase(atoi(argv[1]));
   sc_printf("Erase chip number: %d\r\n", atoi(argv[1]));
}

void cmd_flash_access_readByte(int argc, char **argv)
{
   if (argc != 2)
   {
      sc_printf("Syntax: \r\n readActelByte [address] \r\n");
      return;
   }
   sc_printf("Byte read: %X [hex]\r\n", flash_access_readByteFromFlashram(atoi(argv[1])));
}

void cmd_flash_access_readBytes(int argc, char **argv)
{
   if (argc != 3)
   {
      sc_printf("Syntax: \r\n readActelByte [address] [bytes]\r\n");
      return;
   }
   sc_printf("Printing %d [dez] Bytes at Actel address %X [hex]\r\n", atoi(argv[2]),atoi(argv[1]));
   
   
   signed int b = atoi(argv[2]);
   int i,j;
   j = 0;
   sc_printf("\r\n\r\n");
   for(;b > 0;b -= 8)
   {
      for (i=0;i <8;i++)
      {
         sc_printf("%X ",flash_access_readByteFromFlashram(atoi(argv[1])+i+j));
      }
      sc_printf("\r\n");
      j += 8;
   }
}

void cmd_flash_access_writeByte(int argc, char **argv)
{
   if (argc != 3)
   {
      sc_printf("Syntax: \r\n writeActelByte [address] [data]\r\n");
      return;
   }
   if(flash_access_writeByteToFlashram(atoi(argv[1]), atoi(argv[2])) == 1)
      sc_printf("Written byte at %X [hex] value %X [hex]\r\n", atoi(argv[1]), atoi(argv[2]));
}


void cmd_flash_access_copyChip(int argc, char **argv)
{
   if (argc != 1)
   {
      sc_printf("Syntax: \r\n copyChip\r\n");
      return;
   }
   flash_access_copyBitfile();
}



void cmd_flash_access_calcXOR(int argc, char **argv)
{
   if (argc != 1)
   {
      sc_printf("Syntax: \r\n calcBinXOR\r\n");
      return;
   }
   sc_printf("XOR over Bin file is %d [dev]\r\n", flash_access_calcXor());
}


void cmd_flash_access_compareBitfiles(int argc, char **argv)
{
   if (argc != 1)
   {
      sc_printf("Syntax: \r\n compareBitfiles\r\n");
      return;
   }
   flash_access_compareBitfiles();
}


int executeConsoleCommand(char* cmd)
{
   int cmdlen, i;

   while(*cmd == ' ') cmd++;
   
   if (*cmd == 0) return 0;
   for (cmdlen = 0; cmdlen < x_strlen(cmd); cmdlen++)
      if (cmd[cmdlen] == ' ') break;
   cmds_t* c = console_cmds;
   while (c->name) {
      int iscmd = 1;
      
      if (x_strlen(c->name) != cmdlen) 
         iscmd = 0;
      else
      for (i = 0; i < cmdlen; i++)
         if (c->name[i] != cmd[i]) iscmd = 0;
         
      if (iscmd) break;
      c++;
   }

   if (c->name == 0) {
      sc_printf("%s: command not found\r\n", cmd);
      return 0;
   }
   
   // this is exit function
   if (c->fp == 0) return 7;
   
   char *argv[40];
   int argc=0;

   char *cp = cmd;
   do {
      while (*cp == ' ') *cp++ = '\0';
      if (!*cp) break;
      argv[argc++] = cp;
      while (*cp != ' ' && *cp) cp++;
   } while (*cp);
   sc_printf("\r\n");
   c->fp(argc, argv);
   sc_printf("\r\n");
   
   return 1;
}

void consoleMainLoop()
{
   if(tty_init() != 0) return;
   
   while (gC.global_consolemode == 1) {
      
      ethernetPacketDispatcher();
      
      sc_printf("SysCore> ");

      char* cmd = tty_get_command();
      
      if (executeConsoleCommand(cmd) == 7) break;
   }
   
   tty_exit();
}
