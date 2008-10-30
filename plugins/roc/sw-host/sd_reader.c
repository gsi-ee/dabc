#include "sd_reader.h"

#include "Config.h"
#include "BootLoader.h"
#include "Util.h"
#include "Timer.h"
#include "SD_Card.h"
#include "Partition.h"
#include "FAT.h"
#include <stdlib.h>
#include <string.h>

#include "syscoreshell.h"

int load_file_from_sd_card(char* filename, Xuint8* target)
{
   if (SD_VERBOSE) sc_printf("load_file_from_sd_card()\r\n");

   if(timer_init(PIT_INTERVAL)) {
      sc_printf("main: timer_init() failed\r\n");
      return -1;
   }
   
   if(sd_init()) {
      sc_printf("bl_load: sd_init() failed\r\n");
      return -1;
   }
   
   if(partition_init()) {
      sc_printf("bl_load: partition_init() failed\r\n");
      return -1;
   }
   if (SD_VERBOSE) sc_printf("fat_init()\r\n");

   fat_volume_t *fat_volume;
   int i;

   // try partitions first
   for(i=0; i<4; i++) {
      if (SD_VERBOSE) sc_printf("trying to mount partition %d ...\r\n", i);
      if((fat_volume = fat_mount(i))) {
         if(fat_read_file_to_memory(fat_volume, filename, target) == 0) {
            if (SD_VERBOSE) sc_printf("loaded file '%s' from partition %d to memory address 0x%08X ...\r\n", filename, i, *target);
            if (SD_VERBOSE) sc_printf("fat_init() done\r\n");
            free(fat_volume);
            return 0;
         }
         free(fat_volume);
      }
   }

   // ...
   sc_printf("error: no FAT16/32 volumes found with file %s!\r\n", filename);
   
   return -1;
}

int save_file_to_sd_card(char* filename, Xuint8* target)
{
   if (SD_VERBOSE) sc_printf("save_file_to_sd_card()\r\n");

   if(timer_init(PIT_INTERVAL)) {
      sc_printf("main: timer_init() failed\r\n");
      return -1;
   }
   
   if(sd_init()) {
      sc_printf("bl_load: sd_init() failed\r\n");
      return -1;
   }
   
   if(partition_init()) {
      sc_printf("bl_load: partition_init() failed\r\n");
      return -1;
   }
   if (SD_VERBOSE) sc_printf("fat_init()\r\n");

   fat_volume_t *fat_volume;
   int i=0;

   // try partitions first
   if (SD_VERBOSE) sc_printf("trying to mount partition %d ...\r\n", i);
   if((fat_volume = fat_mount(i))) {
      if(fat_overwrite_file(fat_volume, filename, &target) == 0) {
         if (SD_VERBOSE) sc_printf("saved file '%s' from memory address 0x%08X to partition %d\r\n", filename, target, i);
         if (SD_VERBOSE) sc_printf("fat_init() done\r\n");
         free(fat_volume);
         return 0;
      }
      free(fat_volume);
      return -1;
   }

   sc_printf("error: no FAT16/32 volumes found!\r\n");
   
   return -1;
}

Xuint32 define_sd_file_size(char* filename)
{
   if(timer_init(PIT_INTERVAL)) {
      sc_printf("main: timer_init() failed\r\n");
      return 0;
   }
   
   if(sd_init()) {
      sc_printf("bl_load: sd_init() failed\r\n");
      return 0;
   }
   
   if(partition_init()) {
      sc_printf("bl_load: partition_init() failed\r\n");
      return 0;
   }
   if (SD_VERBOSE) sc_printf("fat_init()\r\n");

   fat_volume_t *fat_volume;
   int i;

   // try partitions first
   for(i=0; i<4; i++) {
      if (SD_VERBOSE) sc_printf("trying to mount partition %d ...\r\n", i);
      if((fat_volume = fat_mount(i))) {

         fat_node_t node;
         Xuint32 *p = NULL;
         Xuint32 p_len = 0;
          
         if(fat_find_path(fat_volume, filename, &node)) {
            sc_printf("unable to find node '%s'!\r\n", filename);
         } else 
         if(fat_collect_cluster_path(fat_volume, node.first_cluster, node.file_size, &p, &p_len)) {
            sc_printf("unable to collect cluster path for node '%s'!\r\n", filename);
            p_len = 0;
         } else
            p_len *= fat_volume->bytes_per_cluster;
         
         free(p);   
         free(fat_volume);
         
         if (p_len!=0) {
            if (SD_VERBOSE) sc_printf("File %s uses %u bytes on partition %d\r\n", filename, p_len, i); 
            return p_len;
         }
      }
   }
   
   return 0;
}

