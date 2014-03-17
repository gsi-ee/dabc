/**********************************************************************
 * Copyright:
 *   GSI, Gesellschaft fuer Schwerionenforschung mbH
 *   Planckstr. 1
 *   D-64291 Darmstadt
 *   Germany
 * created 28. 8.1996 by Horst Goeringer
 **********************************************************************
 * rawCliProcn.c:
 *    utility programs for gStore package: clients only
 **********************************************************************
 * rawCheckFileList: remove objects already archived from file list
 * rawCheckObjList:  remove existing files from object and file list,
 *                   reorder according to restore parameter
 * rawDelFile:       delete single file in GSI mass storage
 * rawDelList:       delete list of files in GSI mass storage
 * rawGetFilelistEntries: get filelist entries from input file 
 * rawGetWSInfo:     get workspace and pool info from master
 * rawGetFullFile:   get full file name from generic input & ll name
 * rawQueryPrint:    print query results for one object to stdout
 *                   due to performance separate from rawQueryString
 * rawQueryString:   print query results for one object to string
 * rawScanObjbuf:    check if obj already available in obj buffer chain
 * rawSortValues:    sort indexed list of numbers
 * ielpst:           AIX only: elapsed (wall-clock) time measurement
 **********************************************************************
 * 26. 3.1996, H.G.: rawCheckObjList: work on ptr-linked object buffers
 * 21.11.1997, H.G.: rawDelFile: only warning (rc=1) if obj not found
 * 27. 7.1999, H.G.: rawGetFullFile: don't check matches, rely on ADSM
 * 19.12.2000, H.G.: rawCheckObjList: remove unused code, better doc
 * 18. 5.2001, H.G.: merge into new file rawcliproc.c:
 *                   rawDelFile.c
 *                   rawDelList (rawDelFilelist from rawproc.c)
 *                   rawTapeFile.c
 *                   ielpst.c
 * 20. 7.2001, H.G.: rawCheckObjList: mover node in srawRetrList
 * 24. 8.2001, H.G.: rawCheckObjList: stage status in srawRetrList
 * 24. 1.2002, H.G.: created from rawcliproc.c, ACTION DELETE -> REMOVE
 * 13. 2.2002, H.G.: rawDelFile: handle also stage file
 * 25. 4.2002, H.G.: move rawTapeFile to new file rawProcTape.c
 * 13. 1.2003, H.G.: rawCheckFileList: reset cFilell before calling
 *                   rawGetLLName
 * 31. 1.2003, H.G.: use rawdefn.h
 *  4. 2.2003, H.G.: rawDelList, obj list: srawArchList -> srawRetrList
 *                   rawCheckFileList, obj list: change type from
 *                   srawArchList -> srawRetrList
 * 14. 2.2003, H.G.: rawDelList: handles list of data movers
 *                   rawCheckFileList: new arg pcNodeMaster
 * 17. 2.2003, H.G.: rawCheckFileList, rawCheckObjList from rawprocn.c
 * 17. 2.2003, H.G.: rawCheckFileList: return in filelist ptr
 * 22. 5.2003, H.G.: renamed to rawCliProcn.c, new rawGetFilelistEntries
 * 27. 6.2003, H.G.: new function rawGetWSInfo
 * 18. 7.2003, H.G.: handle action QUERY_RETRIEVE_API
 * 23. 2.2004, H.G.: rawCheckObjList: StageFS, ArchiveFS in srawRetrList
 *  5. 3.2004, H.G.: rawCheckObjList: add cArchiveDate, cOwner to
 *                   srawRetrList
 *  6. 8.2004, H.G.: ported to Lynx
 * 26. 1.2005, H.G.: rawapitd-gsi.h -> rawapitd-gsin.h
 * 22. 4.2005, H.G.: rawScanObjbuf: handle filelist of any length
 *  3. 4.2006, H.G.: ported to sarge
 *  9. 5.2006, H.G.: rename QUERY_RETRIEVE_API -> QUERY_RETRIEVE_RECORD
 *  7.12.2006, H.G.: rawDelList: add argument iSocketMaster
 * 26.10.2007, H.G.: rawGetWSInfo: readable pool space information
 * 19. 3.2008, H.G.: rawCheckObjList: use extended stage/cache parameters
 * 14. 4.2008, H.G.: rawGetFilelistEntries: allow fully qualified filelist
 * 21. 4.2008, H.G.: rawGetWSInfo: show only free HW, not free pool space
 *  8. 5.2008, H.G.: rawGetFullFile: moved from rawProcn.c
 *                   rawQueryFile: moved -> rawProcn.c
 *                   rawQueryList: moved -> rawProcn.c
 *  9. 5.2008, H.G.: replace srawArchList (in rawclin.h, removed)
 *                   by srawFileList (added to rawcommn.h)
 * 11. 6.2008, H.G.: rawGetFilelistEntries: correct handling of case
 *                   only invalid entries in filelist
 * 16. 6.2008, H.G.: rawCheckObjList: take all 5 restore fields into
 *                   account for reordering
 * 17. 6.2008, H.G.: rawGetFilelistEntries: use cDataFSHigh
 * 16. 9.2008, H.G.: rawGetFilelistEntries: cDataFSHigh->cDataFSHigh1,2
 *  6.11.2008, H.G.: rawGetWSInfo: improved handling rc=0 from recv()
 * 11.11.2008, H.G.: add harmless suggestions of Hakan
 *  3.12.2008, H.G.: add suggestions of Hakan, part II
 *  9. 6.2009, H.G.: replace perror by strerror
 * 10. 6.2009, H.G.: rawCheckObjList: new sort criteria:
 *                   file in RC or WC: first and second in sort order
 *                   all 5 restore order parameters (3rd - 7th)
 *                   scan for all sort criteria over all query buffers
 * 15. 6.2009, H.G.: rawSortValues: sort indexed list of numbers
 * 14. 7.2009, H.G.: rawCheckObjList: when copying tape file entries:
 *                   check for new input buffer also if file in RC/WC
 * 27.11.2009, H.G.: rawCheckObjList: handle enhanced srawRetrList
 * 30.11.2009, H.G.: rawCheckFileList: new arg pcArchive
 * 10.12.2009, H.G.: rawDelFile: handle enhanced structure srawObjAttr
 *                   old version info also available in new structure
 * 29. 1.2010, H.G.: replace MAX_FILE -> MAX_FULL_FILE in:
 *                      static string cPath 
 *                      rawGetFilelistEntries:
 *                         cPath, cEntry, cTemp, cFileName
 *                      rawGetWSInfo: pcGenFile
 *                      rawGetFullFile: cname1, cPath
 *  3. 2.2010, H.G.: make 'archive -over' working again
 *                   rawDelFile: provide correct objId in command buffer
 *                   rawDelList: fix bug setting local ptr to ll name
 *  4. 2.2010, H.G.: rawDelList: provide all infos for rawDelFile
 *                   rawDelFile: no more query for obj needed  
 *  5. 2.2010, H.G.: move rawQueryPrint,rawQueryString from rawProcn.c
 * 10. 2.2010, H.G.: rawQueryPrint, rawQueryString: remove 
 *                      filetype, buffersize from output
 * 26. 2.2010, H.G.: rawQueryString: add parameter (len output string)
 *                      handle too short output string
 * 19. 3.2010, H.G.: rawGetFilelistEntries: add parameter (dir in list)
 * 12. 5.2010, H.G.: rawScanObjbuf: additional arg pathname,
 *                      for recursive file handling
 * 19. 5.2010, H.G.: rawCheckObjList: streamline filelist handling
 * 18. 8.2010, H.G.: rawGetFilelistEntries: stricter limit check,
 *                      remove trailing '/' in global path, if specified
 *  6. 9.2010, H.G.: replace print formats for 'int': %lu -> %u
 * 24. 9.2010, H.G.: rawQueryPrint, rawQueryString: 64 bit filesizes
 * 26.11.2010, H.G.: use string cTooBig for filesizes>=4GB
 * 11. 2.2011, H.G.: rename rawCheckObjlist -> rawCheckObjList,
 *                   rawCheckObjList: new argument iObjAll, optionally:
 *                      handle only objlists, no filelist (par. staging)
 *                   rename rawCheckFilelist -> rawCheckFileList
 * 17. 2.2011, H.G.: rawQueryPrint,rawQueryString:remove check iFileType
 * 28. 7.2011, H.G.: rawCheckFileList: enhanced for recursive actions
 *                      new args: iRecursive, pcTopDir, pcTophl 
 * 26. 8.2011, H.G.: rawCheckFileList: handle empty filelist positions
 *                      (from duplicates from diff filelist entries)
 * 15. 9.2011, H.G.: rawCheckFileList, non-recursive file loop:
 *                      suppress multiple messages
 * 20. 9.2011, H.G.: rawCheckFileList: add parameter iuTopDir
 *  1. 2.2012, H.G.: rawDelList: in RetrList:
 *                      iPoolId -> iPoolIdRC, use also iPoolIdWC
 *  9. 2.2012, H.G.: rawQueryString: use STATUS_LEN_LONG,
 *                      in cMsg1: 1024 -> STATUS_LEN
 * 18. 4.2012, H.G.: rawQueryPrint: status GSI_CACHE_COPY: no 'CACHE*'
 *                      in query output, is reserved for 'in creation'
 * 28. 6.2012, H.G.: rawGetWSInfo: improve messages
 * 13. 8.2012, H.G.: rawGetWSInfo: printing pool sizes: MB -> GB
 *                   rawCheckObjList: handle >MAX_FILE_NO objs
 * 14. 8.2012, H.G.: rawGetWSInfo: rewrite pool size parameter handling
 *                   elimate rounding errors
 * 15. 8.2012, H.G.: rawGetWSInfo: add GlobalPool
 * 17. 8.2012, H.G.: rawCheckObjList: consistent names
 * 23. 8.2012, H.G.: rawCheckFileList: correct compare if var path
 *  6.12.2012, H.G.: rawCheckFileList/fnmatch: not for Lynx
 *  7.10.2013, H.G.: rawGetWSInfo: handle up to 5 pools, more debug info
 *  4.12.2013, H.G.: modify last arg of rawRecvStatus to 'srawStatus *'
 * 24. 2.2014, H.G.: rawGetWSInfo: enable printing of file sizes <99 PB
 **********************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef Lynx
#include <socket.h>
#include <time.h>
#else
#include <fnmatch.h>
#include <sys/socket.h>
#include <sys/time.h>
#endif

#ifdef Linux
#include <netinet/in.h>
#endif

#include "rawapitd.h"
#include "rawapitd-gsin.h"
#include "rawcommn.h"
#include "rawdefn.h"
#include "rawclin.h"
#include "rawentn.h"
#include "rawapplcli.h"

extern FILE *fLogFile;
extern int *piEntryList;                /* for rawGetFilelistEntries */

static unsigned int iint = sizeof(int);
static unsigned int iRetrList = sizeof(srawRetrList);
static unsigned int iFileList = sizeof(srawFileList);

static char cPath[MAX_FULL_FILE] = "";
                  /* returned in rawGetFullFile, must not be locally */

#ifndef Lynx
/*********************************************************************
 * rawCheckFileList: remove objects already archived from file list
 *    returns number of removed objects
 *
 * created 15. 3.96, Horst Goeringer
 * 25. 3.1996, H.G.: work on ptr-linked object buffers
 *********************************************************************
 */

int rawCheckFileList(char **pcFileList, char **pcObjList,
                     char *pcArchive, char *pcNodeMaster,
                     int iRecursive, unsigned int iuTopDir,
                     char *pcTopDir, char *pcTophl)
{
   char cModule[32] = "rawCheckFileList";
   int iDebug = 0;

   char cDelimiter[2] = "";
   char *pcfl, *pcol;
   int *pifl, *piol;
   int ifile0, ifile;
   int iobj0, iobj;
   char *pcc, *pcc1;
   int inoMatch = 0;
   int iDirWC = 0;                 /* =1: wildcarded local directory */
   int iDirStar = 0;                /* =1: top dir with trailing '*' */

   srawFileList *psFile, *psFile0;           /* files to be archived */
   srawRetrList *psObj, *psObj0;         /* objects already archived */

   char cTopFile[MAX_FULL_FILE] = "";
                               /* (wildcarded) filename from top dir */
   char cCurFile[MAX_FULL_FILE] = "";    /* filename string for mods */
   char cFilell[MAX_FULL_FILE] = "", *pcFilell;
                                        /* obj ll name from filename */
   char cLastll[MAX_FULL_FILE] = "";  /* ll name from prev loop step */
   char cCurObj[MAX_FULL_FILE] = "";  /* obj name relative to top hl */
   unsigned int iuTophl = 0;

   bool_t bLastBuf;
   int iobjBuf;
   int **piptr;                  /* points to pointer to next buffer */
   int *pinext;                             /* points to next buffer */

   int ifcur, iocur;
   int ii, jj, iif;
   int iRC, idel, iign;

   pcfl = *pcFileList;
   pifl = (int *) pcfl;
   ifile0 = pifl[0];                      /* initial number of files */
   ifile = ifile0;
   psFile = (srawFileList *) ++pifl;     /* points now to first file */
   psFile0 = psFile;
   pifl--;                        /* points again to number of files */

   pcol = *pcObjList;
   piol = (int *) pcol;
   iobj0 = piol[0];                    /* number of archived objects */
   iobj = iobj0;
   psObj = (srawRetrList *) ++piol;    /* points now to first object */
   psObj0 = psObj;
   piol--;                      /* points again to number of objects */

   pcFilell = cFilell;
   if (iDebug)
   {
      fprintf(fLogFile,
         "\n-D- begin %s: initial %d files, %d objects (1st buffer)\n",
         cModule, ifile0, iobj0);
      fprintf(fLogFile, "    first file %s,\n    first obj %s%s%s\n",
         psFile->cFile, pcArchive, psObj->cNamehl, psObj->cNamell);
      if (iRecursive)
      {
         if (iRecursive == 9) fprintf(fLogFile,
            "    local path with wildcards\n");
         else
         {
            fprintf(fLogFile, "    recursive file handling");
            if (iRecursive == 12)
               fprintf(fLogFile, ", local path with wildcards\n");
            else /* (iRecursive == 2) */
               fprintf(fLogFile, "\n");
         }
      }
   }

   if (iRecursive)
   {
      if ( (strchr(pcTopDir , *pcStar) != NULL) ||
           (strchr(pcTopDir, *pcQM) != NULL) ||
           (strchr(pcTopDir, *pcPerc) != NULL) )
      {
         if ( (iRecursive == 9) || (iRecursive == 12) )
            iDirWC = 1;
         else
         {
            fprintf(fLogFile,
               "-E- %s: inconsistency: wildcarded local path %s, but iRecursive=%d\n",
               cModule, pcTophl, iRecursive);

            return -1;
         }

         if (strchr(pcTopDir , *pcStar) != NULL)
         {
            pcc = pcTopDir;
            pcc += strlen(pcTopDir) - 1;                /* last char */
            if (strncmp(pcc, pcStar, 1) == 0)
            {
               iDirStar = 1;
               if (iDebug) fprintf(fLogFile,
                  "    top dir with trailing '*'\n");
            }
         }
      }

      iuTophl = strlen(pcTophl);
      if (iuTophl < 2)
      {
         fprintf(fLogFile,
            "-E- %s: invalid top gStore directory %s\n",
            cModule, pcTophl);
         return -1;
      }

      if (iDebug)
      {
         if (iDirWC) fprintf(fLogFile,
            "    top directories %s\n    top gStore directory %s\n",
            pcTopDir, pcTophl);
         else fprintf(fLogFile,
            "    top directory %s\n    top gStore directory %s\n",
            pcTopDir, pcTophl);
      }
   }

   iRC = strncmp(psObj->cNamell, pcObjDelim, 1);
   if (iRC)
   {
      iRC = strncmp(psObj->cNamell, pcObjDelimAlt, 1);
      if (iRC)
      {
         fprintf(fLogFile,
            "-E- %s: invalid object delimiter %1s found\n",
            cModule, psObj->cNamell);
         return -1;
      }
      else
         strcpy(cDelimiter, pcObjDelimAlt);
   }
   else
      strcpy(cDelimiter, pcObjDelim);

   iign = 0;
   for (ifcur=1; ifcur<=ifile0; ifcur++)                /* file loop */
   {
      psObj = psObj0;                          /* first object again */
      iobj = iobj0;                     /* first object number again */

      if (strncmp(psFile->cFile, "\n", 1) == 0)
      {
         iign++;
         if (iDebug) fprintf(fLogFile,
            "    filelist position %d empty, to be ignored\n", ifcur);

         goto gNextFile;
      }

      if (iRecursive == 0)
      {
         strcpy(cFilell, "");
         iRC = rawGetLLName(psFile->cFile, cDelimiter, cFilell);

         if (iDebug) fprintf(fLogFile,
            "    %d: '%s'\n", ifcur, psFile->cFile);
      }
      else
      {
         strcpy(cTopFile, pcTopDir);

         /* get current dir */
         strcpy(cCurFile, psFile->cFile);
         pcc = strrchr(cCurFile, '/');
         strcpy(pcc, "\0");
         pcc++;                                   /* rel file name */

         if (iRecursive == 9)
         {
            /* wildcarded subdir, but gStore objects not recursive */

            strcat(cTopFile, "/");
            strcat(cTopFile, pcc);
            if (iDebug == 1) fprintf(fLogFile,
               "%d: strictly compare '%s' - '%s'\n", ifcur, cTopFile, psFile->cFile);

            /* compare path names, wildcards do not match '/' */
            iRC = fnmatch(cTopFile, psFile->cFile, FNM_PATHNAME);
            if (iRC != 0)
            {
               inoMatch = 1;
               if (iDebug == 1) fprintf(fLogFile,
                  "    '%s' not matching\n", psFile->cFile);
            }
            else
               inoMatch = 0;

         } /* (iRecursive == 9) */
         else if (iRecursive == 12)
         {
            /* recursive operation with wildcarded local subdir */

            if ( (strlen(cCurFile) > strlen(pcTopDir)) &&
                 (iDirStar == 0) )
               strcat(cTopFile, "/*");

            strcat(cTopFile, "/");
            strcat(cTopFile, pcc);
           
            if (iDebug == 1) fprintf(fLogFile,
               "%d: compare '%s' - '%s'\n",
               ifcur, cTopFile, psFile->cFile);

            iRC = fnmatch(cTopFile, psFile->cFile, 0);

            if (iRC != 0)
            {
               inoMatch = 1;
               if (iDebug == 1) fprintf(fLogFile,
                  "    '%s' not matching\n", psFile->cFile);
            }
            else
               inoMatch = 0;

         } /* (iRecursive == 12) */
         else
            inoMatch = 0;
            
         /* remove not matching file from list */
         if (inoMatch)
         {
            strncpy(psFile->cFile, "\n", 1);
            ifile--;
            if (ifile == 0) 
            {
               if (iDebug) fprintf(fLogFile,
                   "    last file, comparison finished\n");
               goto gEndCompare;
            }

            goto gNextFile;
         }

         /* get rel filename for gStore object ll */
         pcc = strrchr(psFile->cFile, '/');;
         strcpy(pcFilell, pcc);

         if (iDebug)
            fprintf(fLogFile, "    okay, compare with objs\n");

      } /* (iRecursive) */

      bLastBuf = bFalse;
      iobjBuf = 1;
      if (iDebug == 1)
         fprintf(fLogFile, "\n*** buffer %d: %d objs\n", iobjBuf, iobj);

      while (!bLastBuf)
      {
         for (iocur=1; iocur<=iobj; iocur++)  /* object loop */
         {
            /* compare current file with gStore objs in list */ 
            if (iRecursive == 0)
            {
               /* comparison of filename sufficient */
               iRC = strcmp(cFilell, psObj->cNamell);
            }
            else
            {
               if (iRecursive == 9)
               {
                  pcc = strrchr(cFilell, '/');
                  if (pcc == NULL)
                  {
                     fprintf(fLogFile,
                        "-E- %s: missing path in file name %s\n",
                        cModule, cFilell);

                     goto gNextFile;
                  }
               }

               /* compare filename and subdirs below top dir/hl */
               strcpy(cCurObj, psObj->cNamehl);
               strcat(cCurObj, psObj->cNamell);
               pcc = cCurObj;
               pcc += iuTophl;      /* gStore obj relative to top hl */

               strcpy(cCurFile, psFile->cFile);
               pcc1 = cCurFile;
               pcc1 += iuTopDir;
                  /* filename relative to top dir (above wildcards),
                     so iuTopDir may differ from strlen(cTopDir) */

               if (iDebug == 1) fprintf(fLogFile,
                  "       compare '%s' - '%s'\n", pcc1, pcc);

               iRC = strcmp(pcc1, pcc);
            }

            if (iRC == 0)                           /* file archived */
            {
               fprintf(fLogFile,
                  "-W- %s%s%s already archived in gStore\n", 
                  pcArchive, psObj->cNamehl, psObj->cNamell);

               /* remove file from list */
               strncpy(psFile->cFile, "\n", 1);
               ifile--;
               if (ifile == 0) 
               {
                  if (iDebug) fprintf(fLogFile,
                      "    last file, comparison finished\n");
                  goto gEndCompare;
               }

               strcpy(cLastll, psObj->cNamell);
               goto gNextFile;

            } /* iRC == 0: file already archived */

gNextObj:
            psObj++;
         } /* object loop */

         piptr = (int **) psObj;
         if (*piptr == NULL) 
         {
            bLastBuf = bTrue;
            if (iDebug > 1) 
               fprintf(fLogFile, "    %d: last obj buffer\n", iobjBuf);
            if (iDebug)
               fprintf(fLogFile, "*** %d: %s (%s) to be archived\n",
                  ifcur, psFile->cFile, cFilell);
         }
         else
         {
            iobjBuf++;
            pinext = *piptr;
            iobj = *pinext++;
            psObj = (srawRetrList *) pinext;
            if (iDebug > 1) fprintf(fLogFile,
               "\n*** new buffer %d: %d objs, first obj '%s'\n", 
               iobjBuf, iobj, psObj->cNamell);
         }
      } /* while (!bLastBuf) */

gNextFile:
      psFile++;

   } /* file loop */

gEndCompare:
   idel = ifile0-ifile;
   if (iDebug) fprintf(fLogFile,
      "    %d of %d files removed: %d files remaining\n",
      idel, ifile0, ifile);

   if (iign)
   {
      ifile -= iign;
      if (iDebug) fprintf(fLogFile,
         "    %d files ignored: %d files remaining\n",
         iign, ifile);
   }
   
   pifl[0] = ifile;
   *pcFileList = (char *) pifl;

   if ( (ifile > 0) && (idel+iign > 0) )   /* compress filelist */
   {
      iif = 1;
      psFile = psFile0;
      for (ifcur=1; ifcur<=ifile0; ifcur++)        /* file loop */
      {
         if (iDebug == 2)
            fprintf(fLogFile, "%d: %s\n", ifcur, psFile->cFile);

         iRC = strncmp(psFile->cFile, "\n", 1);
         if (iRC != 0)                            /* file found */
         { 
            if (iif >= ifile0)
               break;

            psFile0++;
            psFile++;
            iif++;
         }
         else
         {                                    /* compress action */
            for (iocur=ifcur+1; iocur<=ifile0; iocur++)
            { 
               psFile++;
               iif++;
               iRC = strncmp(psFile->cFile, "\n", 1);
               if (iRC == 0) 
               {
                  if (iif >= ifile0)
                     break;
               }
               else
               {
                  strcpy(psFile0->cFile, psFile->cFile);
                  if (iDebug > 1) fprintf(fLogFile,
                     "*** %d: %s\n", iif, psFile0->cFile); 
                  if (iif >= ifile0)
                     break;

                  psFile0++;
               }
            } /* for (iocur...) */

         } /* compress action */ 
      } /* for (ifcur...) */

      if (iDebug)
         fprintf(fLogFile, "    file list compressed\n");

   } /* ifile > 0 && idel+iign > 0 */

   if (iDebug)
   {
      fprintf(fLogFile, "-D- end %s\n\n", cModule);
      fflush(fLogFile);
   }

   return idel + iign;

} /* rawCheckFileList */
#endif

/********************************************************************
 * rawCheckObjList:
 *    iObjComp: no. of valid objs/files
 *    on input:
 *       pcObjList: chain of ptr-linked buffers with obj lists
 *       pcFileList: corresponding file list (one buffer)
 *       in both invalid elements (retrieve: already existing) with
 *       file names eliminated to "\0"
 *    on output (one buffer):
 *       pcObjComp:  compressed and sorted obj list
 *       pcFileList: compressed and sorted file list
 *    internal (one buffer):
 *       pcObjSort:  compressed intermediate results
 *       pcFileSort: compressed intermediate results
 *       pcFileComp: compressed and sorted file list, to be copied to
 *                   pcFileList
 *
 *    compress pcObjList -> pcObjComp
 *    (retrieve, stage, and unstage)
 *
 *    remove files/objs with 0 length ll names from obj + file lists
 *    (retrieve only)
 *
 *    reorder remaining files/objs according to 
 *       (1) read cache
 *       (2) write cache
 *       (3) 5 restore parameters
 *    (retrieve and stage only)
 *
 * created 18. 3.96, Horst Goeringer
 *********************************************************************
 */

int rawCheckObjList(
       int iObjAll,/*overall no. of objs incl those to be eliminated */
       int iObjComp,                    /* no. of compressed objects */
       int iSort,         /* =0: only compress of obj list (unstage) */
       char **pcObjList,       /* chained buffer list with obj names */
       char **pcFileList,                  /* buffer with file names */
       char **pcObjComp)  /* compressed output buffer with obj names */
{
   char cModule[32]="rawCheckObjList";
   int iDebug = 0;

   int iRC;
   int iDiff = 0;
   char cMsg[STATUS_LEN] = "";

   int iaValue[iObjAll+1][6];
      /* for sorting relevant values of all objs
       * 1st index: file/object no.
       * 2nd index 0: = 0: ignore (file 
       *              = 1: file in read cache 
       *              = 2: file in write cache 
       *              = 3: sort according to up to 5 TSM restore params
       * 2nd index 1 - 5: value of corresponing TSM restore parameters
       */
   int iaCompValue[iObjComp+1][6];
      /* compressed list of for sorting relevant values */
   int iaIndex[iObjComp];
                   /* sort index for each obj used after compression */

   int iaIndNew[iObjAll+1];
   int iaValTop[iObjAll+1];
   int iaValHiHi[iObjAll+1];
   int iaValHiLo[iObjAll+1];
   int iaValLoHi[iObjAll+1];
   int iaValLoLo[iObjAll+1];

   int iif = 0, ii1 = 0, ii2 = 0;
   int iisort1 = 0,                /* no. of 1st object to be sorted */
       iisort2 = 0;               /* no. of last object to be sorted */
   int iitop1 = 0,                  /* no. of 1st topId to be sorted */
       iitop2 = 0,                 /* no. of last topId to be sorted */
       iTopValcur = 0,                   /* no. of current top value */
       iObjTopValcur = 0;/* cur no. of object with current top value */
   int iihihi1 = 0, iihihi2 = 0, iHiHiValcur = 0, iihihicur = 0;
   int iihilo1 = 0, iihilo2 = 0, iHiLoValcur = 0, iihilocur = 0;
   int iilohi1 = 0, iilohi2 = 0, iLoHiValcur = 0, iilohicur = 0;
   int iiTSM = 0;                               /* no of TSM objects */

   int iFileSort = 0,              /* no. of TSM files: to be sorted */
       iReadCache = 0,                 /* no. of files in read cache */
       iWriteCache = 0,               /* no. of files in write cache */
       iIgnore = 0;                    /* no. of files to be ignored */

   /* no. of files using restore field */
   int iRestoTop = 0,
       iRestoHiHi = 0,
       iRestoHiLo = 0,
       iRestoLoHi = 0,
       iRestoLoLo = 0;

   /* no. of different restore fields */
   unsigned int iRestoTopValues = 0,
                iRestoHiHiValues = 0,
                iRestoHiLoValues = 0,
                iRestoLoHiValues = 0,
                iRestoLoLoValues = 0;

   unsigned int iRestoTopMin = 0,
                iRestoHiHiMin = 0,
                iRestoHiLoMin = 0,
                iRestoLoHiMin = 0,
                iRestoLoLoMin = 0;

   bool_t bRestoSort = bFalse;
                         /* if true: subset of restore fields sorted */
   bool_t bRestoTop = bFalse,     /* if true: top restore field used */
          bRestoHiHi = bFalse, /* if true: HighHigh resto field used */
          bRestoHiLo = bFalse,/* if true: HighLow restore field used */
          bRestoLoHi = bFalse,/* if true: LowHigh restore field used */
          bRestoLoLo = bFalse; /* if true: LowLow restore field used */

   char *pcfl, *pcol;
   int  *pifl, *piol;
   int iFileAll = 0;  /* overall number of files (buffer pcFileList) */
   int iobj0 = 0,   /* number of objects in current buffer pcObjList */
       iobj = 0;

   int iFileBufComp,      /* size of buffer with compressed filelist */ 
       iObjBufComp;        /* size of buffer with compressed objlist */
   /*    pcObjComp in arg list */
   char *pcFileComp;         /* local file buffer: filled in sorting */
   char *pcflcomp, *pcolcomp;
   int  *piflcomp, *piolcomp;

   char *pcFileSort,         /* local file buffer: filled in sorting */
        *pcObjSort;        /* local object buffer: filled in sorting */
   char *pcflsort, *pcolsort;
   int  *piflsort, *piolsort;

   int iiObj = 0;      /* no. of file/object found in current buffer */
   int iiBuf = 0;                    /* no. of current object buffer */
   int iiFile = 0;      /* no. of file/object used in current buffer */

   int **piptr;           /* points to pointer to next object buffer */
   int *pinext;                      /* points to next object buffer */

   srawFileList *psFile, *psFile0;             /* files in file list */
   srawFileList *psFileSort,             /* files in local file list */
                *psFileSort0,         /* 1st file in local file list */
                *psFileComp,             /* files in local file list */
                *psFileComp0;         /* 1st file in local file list */

   srawRetrList *psObj;          /* current object in current buffer */
   srawRetrList *psObj0;               /* 1st object (in 1st buffer) */
   srawRetrList *psObjSort,          /* objects in local object list */
                *psObjSort0,      /* 1st object in local object list */
                *psObjComp,         /* objects in output object list */
                *psObjComp0;     /* 1st object in output object list */

   int ii, jj, jjMin = 0;
   int icount = 0;

   bool_t bInit;

   /* linked object list (input) */
   pcol = *pcObjList;
   piol = (int *) pcol;
   iobj0 = piol[0];          /* number of archived objects in buffer */
   iobj = iobj0;
   psObj = (srawRetrList *) ++piol;    /* points now to first object */
   psObj0 = psObj;
   piol--;                      /* points again to number of objects */

   /* object list (output) */
   pcolcomp = *pcObjComp;
   piolcomp = (int *) pcolcomp;
   psObjComp = (srawRetrList *) ++piolcomp;          /* first object */
   psObjComp0 = psObjComp;
   piolcomp--;

   /* file list (input and output) */
   pcfl = *pcFileList;
   pifl = (int *) pcfl;
   iFileAll = pifl[0];                        /* number of all files */
   if (iFileAll > 0)
   {
      psFile = (srawFileList *) ++pifl;  /* points now to first file */
      psFile0 = psFile;
      pifl--;                     /* points again to number of files */
   }

   if (iDebug)
   {
      fprintf(fLogFile,
         "\n-D- begin %s: overall %d objs, in 1st buffer %d objs, compressed: %d objs\n",
         cModule, iObjAll, iobj0, iObjComp);
      if (iFileAll == 0)
         fprintf(fLogFile, "    no filelist handling\n");
      if (iDebug > 1)
      {
         if (iSort == 1)
         {
            fprintf(fLogFile, "    sort entries, first obj %s%s",
               psObj->cNamehl, psObj->cNamell);
            if (iFileAll > 0)
               fprintf(fLogFile, ", first file %s\n", psFile->cFile);
            else
               fprintf(fLogFile, "\n");
         }
         else fprintf(fLogFile,
            "    first obj %s%s\n", psObj->cNamehl, psObj->cNamell);
      }
      fflush(fLogFile);
   }

   if (iDebug == 2)
   {
      for (jj=1; jj<=iFileAll; jj++)
      {
         fprintf(fLogFile,
            "DDDD %d: file %s (%u)\n", jj, psFile->cFile, psFile);
         psFile++;
      }
      psFile = psFile0; /* reset */
   }

   /************ unstage: only compress object buffer ****************/

   if (iSort == 0)
   {
      iiBuf = 1;                               /* running buffer no. */
      ii = 0;

      /* loop over all objects */
      for (jj=1; jj<=iObjAll; jj++)
      {
         iiObj++;
         if (strlen(psObj->cNamell) < 2)
         {
            if (iDebug == 2) fprintf(fLogFile,
               "    object %d(%d): ignored\n", iiObj, jj);

            goto gNextObjUnstage;
         }

         ii++;
         memcpy((char *) psObjComp, (char *) psObj, iRetrList);
         if (iDebug) fprintf(fLogFile,
            "%d(%d): obj %s%s copied (Comp)\n", iiObj, jj,
            psObjComp->cNamehl, psObjComp->cNamell);
            
         psObjComp++;

gNextObjUnstage:
         psObj++;
         if (iiObj == iobj)
         {
            if (iDebug) fprintf(fLogFile,
               "      buffer %d: last obj (no. %d) handled\n",
               iiBuf, iiObj);

            piptr = (int **) psObj;
            if (*piptr == NULL)
            {
               if (iDebug) fprintf(fLogFile,
                  "      buffer %d: last obj buffer\n", iiBuf);
            }
            else
            {
               iiObj = 0;
               iiBuf++;
               pinext = *piptr;
               iobj = *pinext++;
               psObj = (srawRetrList *) pinext;
               if (iDebug) fprintf(fLogFile,
                  "      new buffer %d, %d objs, first: %s%s|\n",
                  iiBuf, iobj, psObj->cNamehl, psObj->cNamell);
            }
         } /* iiObj == iobj */
      } /* loop over all objects */

      *piolcomp = ii;
      goto gEndCheckObjlist;

   } /* (iSort == 0) */

   /************* allocate local file/object buffers *****************/

   if (iFileAll > 0)
   {
      iFileBufComp = sizeof(int) + iObjComp*sizeof(srawFileList);
      if ((pcFileComp = (char *) calloc(
              (unsigned) iFileBufComp, sizeof(char) ) ) == NULL)
      {
         fprintf(fLogFile,
            "-E- %s: allocating filelist buffer (Comp, %d byte)\n",
            cModule, iFileBufComp);
         if (errno)
            fprintf(fLogFile, "    %s\n", strerror(errno));
         perror("-E- allocating filelist buffer (Comp)");

         return -2;
      }
      if (iDebug) fprintf(fLogFile,
         "    filelist buffer (Comp) allocated (size %d)\n",
         iFileBufComp);

      piflcomp = (int *) pcFileComp;
      piflcomp[0] = iObjComp;          /* compressed number of files */
      psFileComp = (srawFileList *) ++piflcomp;        /* first file */
      psFileComp0 = psFileComp;

      if ( (pcFileSort = (char *) calloc(
               (unsigned) iFileBufComp, sizeof(char)) ) == NULL)
      {
         fprintf(fLogFile,
            "-E- %s: allocating filelist buffer (Sort, %d byte)\n",
            cModule, iFileBufComp);
         if (errno)
            fprintf(fLogFile, "    %s\n", strerror(errno));
         perror("-E- allocating filelist buffer (Sort)");

         return(-2);
      }

      if (iDebug) fprintf(fLogFile,
         "    filelist buffer (Sort) allocated for %d files (size %d)\n",
         iObjComp, iFileBufComp);

      piflsort = (int *) pcFileSort;
      piflsort[0] = iObjComp;          /* compressed number of files */
      psFileSort = (srawFileList *) ++piflsort;        /* first file */
      psFileSort0 = psFileSort;
   }

   iObjBufComp = sizeof(int) + iObjComp*sizeof(srawRetrList);
   if ( (pcObjSort = (char *) calloc(
            (unsigned) iObjBufComp, sizeof(char)) ) == NULL )
   {
      fprintf(fLogFile,
         "-E- %s: allocating objectlist buffer (%d byte)\n",
         cModule, iObjBufComp);
      if (errno)
         fprintf(fLogFile, "    %s\n", strerror(errno));
      perror("-E- allocating objlist buffer");

      return(-3);
   }
   if (iDebug) fprintf(fLogFile,
      "    objlist buffer (Sort) allocated for %d objs (size %d)\n",
      iObjComp, iObjBufComp);

   pcolsort = pcObjSort;
   piolsort = (int *) pcolsort;
   piolsort[0] = iObjComp;           /* compressed number of objects */
   psObjSort = (srawRetrList *) ++piolsort;          /* first object */
   psObjSort0 = psObjSort;
 
   /************** scan ALL objects to get sort criteria *************/

   for (jj=1; jj<=iObjComp; jj++)
   {
      iaIndex[jj] = -1;                                      /* init */
   }
   for (jj=1; jj<=iObjAll; jj++)
   {
      for (ii=0; ii<=5; ii++)
      {
         if (ii == 0)
            iaValue[jj][ii] = -1;                            /* init */
         else
            iaValue[jj][ii] = 0;                             /* init */
      }
   }

   psObj = psObj0;                       /* 1st object in 1st buffer */
   iobj = iobj0;                     /* no. of objects in 1st buffer */
   iiObj = 0;                        /* running object no. in buffer */
   iiBuf = 1;                                  /* running buffer no. */

   if (iDebug)
   {
      fprintf(fLogFile, "scan objects for sort criteria:\n");
      fflush(fLogFile);
   }

   /* loop over ALL objects to get sort criteria */
   for (jj=1; jj<=iObjAll; jj++)
   {
      iiObj++;                      /* counter objects in cur buffer */

      if (iDebug == 2)
      {
         fprintf(fLogFile, "DDD %d (all %d): obj %s (%d)",
            iiObj, jj, psObj->cNamell, psObj);
         if (iFileAll > 0) fprintf(fLogFile,
            ", file %s (%u)\n", psFile->cFile, psFile);
         else
            fprintf(fLogFile, "\n");
      }

      if (strlen(psObj->cNamell) < 2)
      {
         iaValue[jj][0] = 0;                               /* ignore */
         if (iDebug == 2) fprintf(fLogFile,
            "    object %d(%d): ignored\n", iiObj, jj);

         iIgnore++;
         goto gNextValue;
      }

      if (psObj->iStageFS)
      {
         if (iDebug == 2) fprintf(fLogFile,
            "    object %d(%d) %s%s: in read cache\n",
            iiObj, jj, psObj->cNamehl, psObj->cNamell);

         iReadCache++;
         iaValue[jj][0] = 1;

         /* copy to final destination */
         memcpy((char *) psObjComp, (char *) psObj, iRetrList);

         if (iFileAll > 0)
            memcpy(psFileComp->cFile, psFile->cFile, iFileList);

         if (iDebug)
         {
            fprintf(fLogFile,
               "%d: read cache obj %s%s (objId %u-%u) copied (Comp)\n",
               iReadCache, psObjComp->cNamehl, psObjComp->cNamell,
               psObjComp->iObjHigh, psObjComp->iObjLow);

            if (iFileAll > 0)
            {
               fprintf(fLogFile, "    orig file name %s (%u)\n",
                  psFile->cFile, psFile);
               fprintf(fLogFile, "    file name %s copied (Comp: %u)\n",
                  psFileComp->cFile, psFileComp);
            }

            if (jj == iObjAll) fprintf(fLogFile,
               "       in last buffer %d: last obj (no. %d) handled\n",
               iiBuf, iiObj);
            fflush(fLogFile);
         }

         /* mark as already handled
         strncpy(psObj->cNamell, "\0", 1); */

         psObjComp++;
         if (iFileAll > 0)
            psFileComp++;

         goto gNextValue;
      }

      if (psObj->iCacheFS)
      {
         if (iDebug == 2) fprintf(fLogFile,
            "    object %d(%d) %s%s: in write cache\n",
            iiObj, jj, psObj->cNamehl, psObj->cNamell);

         iWriteCache++;
         iaValue[jj][0] = 2;

         /* copy to temp. destination */
         memcpy((char *) psObjSort, (char *) psObj, iRetrList);
         if (iFileAll > 0)
            memcpy(psFileSort->cFile, psFile->cFile, iFileList);

         if (iDebug)
         {
            fprintf(fLogFile,
               "%d: write cache obj %s%s (%u-%u) copied (Sort)\n",
               iWriteCache, psObjSort->cNamehl, psObjSort->cNamell,
               psObjSort->iObjHigh, psObjSort->iObjLow);
            
            if (jj == iObjAll) fprintf(fLogFile,
               "       in last buffer %d: last obj (no. %d) handled\n",
               iiBuf, iiObj);
         }

         psObjSort++;
         if (iFileAll > 0)
            psFileSort++;

         goto gNextValue;
      }

      iaValue[jj][0] = 3;
      if (psObj->iRestoHigh)
      {
         iaValue[jj][1] = psObj->iRestoHigh;
         iRestoTop++;
         if (iDebug == 2) fprintf(fLogFile,
            "    object %d(%d) %s%s: top restore field %d\n",
            iiObj, jj, psObj->cNamehl, psObj->cNamell, iaValue[jj][1]);
      }
      if (psObj->iRestoHighHigh)
      {
         iaValue[jj][2] = psObj->iRestoHighHigh;
         iRestoHiHi++;
         fprintf(fLogFile,
            "-W- object %d(%d) %s%s: hihi restore field %d\n",
            iiObj, jj, psObj->cNamehl, psObj->cNamell, iaValue[jj][2]);
      }
      if (psObj->iRestoHighLow)
      {
         iaValue[jj][3] = psObj->iRestoHighLow;
         iRestoHiLo++;
         if (iDebug == 2) fprintf(fLogFile,
            "    object %d(%d) %s%s: hilo restore field %d\n",
            iiObj, jj, psObj->cNamehl, psObj->cNamell, iaValue[jj][3]);
      }
      if (psObj->iRestoLowHigh)
      {
         iaValue[jj][4] = psObj->iRestoLowHigh;
         iRestoLoHi++;
         fprintf(fLogFile,
            "-W- object %d(%d) %s%s: lohi restore field %d\n",
            iiObj, jj, psObj->cNamehl, psObj->cNamell, iaValue[jj][4]);
      }
      if (psObj->iRestoLow)
      {
         iaValue[jj][5] = psObj->iRestoLow;
         iRestoLoLo++;
         if (iDebug == 2) fprintf(fLogFile,
            "    object %d(%d) %s%s: lolo restore field %d\n",
            iiObj, jj, psObj->cNamehl, psObj->cNamell, iaValue[jj][5]);
      }

gNextValue:
      psObj++;
      if (iFileAll > 0)
         psFile++;

      if (iiObj == iobj)
      {
         if (iDebug) fprintf(fLogFile,
            "       buffer %d: last obj (no. %d) handled\n",
            iiBuf, iiObj);

         piptr = (int **) psObj;
         if (*piptr == NULL)
         {
            if (iDebug) fprintf(fLogFile,
               "       buffer %d: last obj buffer\n", iiBuf);
         }
         else
         {
            iiObj = 0;
            iiBuf++;
            pinext = *piptr;
            iobj = *pinext++;
            psObj = (srawRetrList *) pinext;
            if (iDebug) fprintf(fLogFile,
               "       new buffer %d, %d objs, first: %s%s|\n",
               iiBuf, iobj, psObj->cNamehl, psObj->cNamell);
         }
      } /* iiObj == iobj */
   } /* loop over all objects */

   if (iDebug) fprintf(fLogFile,
      "    usage of restore fields: %u-%u-%u-%u-%u\n",
      iRestoTop, iRestoHiHi, iRestoHiLo, iRestoLoHi, iRestoLoLo);

   /**** copy WC entries to final destination (behind RC entries) ****/

   if (iWriteCache)
   {
      psObjSort = psObjSort0;                        /* first object */
      if (iFileAll > 0)
         psFileSort = psFileSort0;                     /* first file */

      for (ii=1; ii<=iWriteCache; ii++)
      {
         memcpy((char *) psObjComp, (char *) psObjSort, iRetrList);
         psObjSort++;
         psObjComp++;

         if (iFileAll > 0)
         {
            memcpy(psFileComp->cFile, psFileSort->cFile, iFileList);
            psFileSort++;
            psFileComp++;
         }
      } /* loop write cache entries */
   } /* (iWriteCache) */

   if ( ((iReadCache) || (iWriteCache)) && (iDebug == 2) )
   {
      psObjComp = psObjComp0;                        /* first object */
      if (iFileAll > 0)
         psFileComp = psFileComp0;                     /* first file */

      if (iReadCache)
      {
         fprintf(fLogFile,
            "%d read cache entries (Comp)\n", iReadCache);
         for (ii=1; ii<=iReadCache; ii++)
         {
            fprintf(fLogFile, "   %d: obj %s%s",
               ii, psObjComp->cNamehl, psObjComp->cNamell);
            psObjComp++;

            if (iFileAll > 0)
            {
               fprintf(fLogFile, ", file %s\n",
                  psFileComp->cFile);
               psFileComp++;
            }
            else
               fprintf(fLogFile, "\n");
         }
      }

      if (iWriteCache)
      {
         fprintf(fLogFile,
            "%d write cache entries (Comp)\n", iWriteCache);
         for (ii=1; ii<=iWriteCache; ii++)
         {
            jj = iReadCache + ii;
            fprintf(fLogFile, "   %d: obj %s%s",
               jj, psObjComp->cNamehl, psObjComp->cNamell);
            psObjComp++;

            if (iFileAll > 0)
            {
               fprintf(fLogFile, ", file %s\n",
                  psFileComp->cFile);
               psFileComp++;
            }
            else
               fprintf(fLogFile, "\n");
         }
      }
   } /* (iReadCache || iWriteCache) && (iDebug == 2) */

   /* no more TSM files left */
   if (iReadCache + iWriteCache + iIgnore == iObjAll)
   {
      if (iDebug)
         fprintf(fLogFile, "no TSM objects\n");

      goto gEndCheckObjlist;
   }

   /*********** create compressed arrays and obj/file buffers ********/
   
   iFileSort = iObjAll - iReadCache - iWriteCache - iIgnore;
                                        /* should be no. of TSM objs */
   psObjSort = (srawRetrList *) piolsort;            /* first object */
   psObj = psObj0;                       /* 1st object in 1st buffer */

   if (iFileAll > 0)
   {
      psFileSort = (srawFileList *) piflsort;          /* first file */
      psFile = psFile0;
   }

   iobj = iobj0;                     /* no. of objects in 1st buffer */
   iiObj = 0;                        /* running object no. in buffer */
   iiBuf = 1;                                  /* running buffer no. */
   iiTSM = 0;               /* no. of objs with restore order fields */

   if (iDebug) fprintf(fLogFile,
      "    TSM restore values before sorting:\n");

   /* loop over all objects to select the TSM objects */
   for (jj=1; jj<=iObjAll; jj++)
   {
      iiObj++;
      if (iaValue[jj][0] < 3)
      {
         if (iDebug)
         {
            fprintf(fLogFile,
               "    %d: (%d) ignored", jj, iaValue[jj][0]);
            if (psObj->iStageFS)
               fprintf(fLogFile, " (RC)\n");
            else if (psObj->iCacheFS)
               fprintf(fLogFile, " (WC)\n");
            else
               fprintf(fLogFile, "\n");
         }
      }
      else
      {
         iiTSM++;
         iaCompValue[iiTSM][0] = iaValue[jj][0];
         iaCompValue[iiTSM][1] = iaValue[jj][1];
         iaCompValue[iiTSM][2] = iaValue[jj][2];
         iaCompValue[iiTSM][3] = iaValue[jj][3];
         iaCompValue[iiTSM][4] = iaValue[jj][4];
         iaCompValue[iiTSM][5] = iaValue[jj][5];

         if (iDebug) fprintf(fLogFile,
            "    %d(%d): %d %d-%d-%d-%d-%d\n", jj, iiTSM,
            iaValue[jj][0], iaValue[jj][1], iaValue[jj][2],
            iaValue[jj][3], iaValue[jj][4], iaValue[jj][5]);

         memcpy((char *) psObjSort, (char *) psObj, iRetrList);
         psObjSort++;

         if (iFileAll > 0)
         {
            memcpy(psFileSort->cFile, psFile->cFile, iFileList);
            psFileSort++;
         }
      }

      psObj++;
      if (iFileAll > 0)
         psFile++;

      if (iiObj == iobj)
      {
         if (iDebug == 2) fprintf(fLogFile,
            "       buffer %d: last obj (no. %d) handled\n",
            iiBuf, iiObj);

         piptr = (int **) psObj;
         if (*piptr == NULL)
         {
            if (iDebug == 2) fprintf(fLogFile,
               "       buffer %d: last obj buffer\n", iiBuf);
         }
         else
         {
            iiObj = 0;
            iiBuf++;
            pinext = *piptr;
            iobj = *pinext++;
            psObj = (srawRetrList *) pinext;
            if (iDebug == 2) fprintf(fLogFile,
               "       new buffer %d, %d objs, first: %s%s\n",
               iiBuf, iobj, psObj->cNamehl, psObj->cNamell);
         }
      } /* iiObj == iobj */
   } /* loop over all objects to select the TSM objects */

   if (iiTSM != iFileSort)
   {
      fprintf(fLogFile,
         "-E- %s: inconsistent no. of TSM files: %d - %d\n",
         cModule, iFileSort, iiTSM);
      return -2;
   }

   psObjSort = (srawRetrList *) piolsort;        /* first object */
   if (iFileAll > 0)
      psFileSort = (srawFileList *) piflsort;      /* first file */

   /*************** sort top restore values in array *****************/

   if (iDebug == 2) fprintf(fLogFile,
      "DDD list of %d compressed TSM objs/files (ps...Sort):\n",
      iFileSort);

   for (jj=1; jj<=iFileSort; jj++)
   {
      if (iRestoTop)
         iaValTop[jj] = iaCompValue[jj][1];
      else
         iaValTop[jj] = 0;

      if (iRestoHiHi)
         iaValHiHi[jj] = iaCompValue[jj][2];
      else
         iaValHiHi[jj] = 0;

      if (iRestoHiLo)
         iaValHiLo[jj] = iaCompValue[jj][3];
      else
         iaValHiLo[jj] = 0;

      if (iRestoLoHi)
         iaValLoHi[jj] = iaCompValue[jj][4];
      else
         iaValLoHi[jj] = 0;

      if (iRestoLoLo)
         iaValLoLo[jj] = iaCompValue[jj][5];
      else
         iaValLoLo[jj] = 0;

      iaIndex[jj] = jj;

      if (iDebug == 2)
      {
         fprintf(fLogFile, "    %d: obj %s%s",
            jj, psObjSort->cNamehl, psObjSort->cNamell);
         psObjSort++;

         if (iFileAll > 0)
         {
            fprintf(fLogFile, ", file %s\n", psFileSort->cFile);
            psFileSort++;
         }
         else
            fprintf(fLogFile, "\n");
      }
   }

   iisort1 = 1;
   iisort2 = iFileSort;

   if (iRestoTop)
   {
      iRC = rawSortValues(iaValTop, iFileSort, iisort1, iisort2,
                          iaIndex, iaIndNew); 

      /* rearrange other restore fields */
      iDiff = 0;
      for (jj=iisort1; jj<=iisort2; jj++)
      {
         if (iaIndNew[jj] != iaIndex[jj])
         {
            ii1 = iaIndNew[jj];
            if (iRestoHiHi)
               iaValHiHi[jj] = iaCompValue[ii1][2];
            if (iRestoHiLo)
               iaValHiLo[jj] = iaCompValue[ii1][3];
            if (iRestoLoHi)
               iaValLoHi[jj] = iaCompValue[ii1][4];
            if (iRestoLoLo)
               iaValLoLo[jj] = iaCompValue[ii1][5];
            iDiff++;
         }
      }

      /* update index field */
      memcpy(&iaIndex[iisort1], &iaIndNew[iisort1],
             (unsigned) iFileSort*iint);

      if (iDebug == 2)
      {
         fprintf(fLogFile,
            "DDD restore order after top sorting (%d changes):\n",
            iDiff);
         for (jj=iisort1; jj<=iisort2; jj++)
         {
            fprintf(fLogFile,
               "    %d: index %d, values: %u-%u-%u-%u-%u\n",
               jj, iaIndNew[jj], iaValTop[jj],
               iaValHiHi[jj], iaValHiLo[jj],
               iaValLoHi[jj], iaValLoLo[jj]);
         }
      }

      iRestoTopValues = 1;
      iRestoTopMin = iaValTop[iisort1];
      for (jj=iisort1+1; jj<=iisort2; jj++)
      {
         if (iaValTop[jj] > iRestoTopMin)
         {
            iRestoTopMin = iaValTop[jj];
            iRestoTopValues++;
         }
      }

      if (iDebug) fprintf(fLogFile,
         "%d different top restore values\n", iRestoTopValues);

      iitop1 = iisort1;
         /* index of 1st obj with cur top restore field */

      /* loop over different top restore fields */
      for (iTopValcur=1; iTopValcur<=iRestoTopValues; iTopValcur++)
      {
         if (iTopValcur == iRestoTopValues)
            iitop2 = iFileSort;
         else
         {
            iRestoTopMin = iaValTop[iitop1];
            for (jj=iitop1+1; jj<=iFileSort; jj++)
            {
               if (iaValTop[jj] > iRestoTopMin)
               {
                  iitop2 = jj-1;
                  break;
               }

               if (jj == iFileSort)
                  iitop2 = jj;
            }
         }

         if (iitop1 == iitop2)
         {
            if (iDebug) fprintf(fLogFile,
               "    %d. top restore value %d: only one object (%d)\n",
               iTopValcur, iaValTop[iitop1], iitop1);

            if (iTopValcur == iRestoTopValues)
               break;
            else
            {
               iitop1 = iitop2 + 1;
               continue;
            }
         }
         else if (iDebug) fprintf(fLogFile,
            "    %d. top restore value %d: objs %d - %d\n",
            iTopValcur, iaValTop[iitop1], iitop1, iitop2);

         /* sort entries according to hihi: DDD not yet used */
         if (iRestoHiHi)
         {
            ;
         } /* (iRestoHiHi) */

         /* sort entries according to hilo */
         if (iRestoHiLo)
         {
            if (iitop2 - iitop1)
            {
               iRC = rawSortValues(iaValHiLo, iFileSort, iitop1, iitop2,
                          iaIndex, iaIndNew); 

               /* rearrange other restore fields */
               iDiff = 0;
               for (jj=iitop1; jj<=iitop2; jj++)
               {
                  if (iaIndNew[jj] != iaIndex[jj])
                  {
                     ii1 = iaIndNew[jj];
                     if (iRestoTop)
                        iaValTop[jj] = iaCompValue[ii1][1];
                     if (iRestoHiHi)
                        iaValHiHi[jj] = iaCompValue[ii1][2];
                     if (iRestoLoHi)
                        iaValLoHi[jj] = iaCompValue[ii1][4];
                     if (iRestoLoLo)
                        iaValLoLo[jj] = iaCompValue[ii1][5];
                     iDiff++;
                  }
               }

               /* update index field */
               ii1 = iitop2 - iitop1 + 1;
               memcpy(&iaIndex[iitop1], &iaIndNew[iitop1],
                      (unsigned) ii1*iint);

               if (iDebug == 2)
               {
                  fprintf(fLogFile,
                     "       restore order after hilo sorting (%d - %d: %d changes):\n",
                     iitop1, iitop2, iDiff);
                  for (jj=iitop1; jj<=iitop2; jj++)
                  {
                     fprintf(fLogFile,
                        "       %d: index %d, values: %u-%u-%u-%u-%u\n",
                        jj, iaIndNew[jj], iaValTop[jj],
                        iaValHiHi[jj], iaValHiLo[jj],
                        iaValLoHi[jj], iaValLoLo[jj]);
                  }
               }

               iRestoHiLoValues = 1;
               iRestoHiLoMin = iaValHiLo[iitop1];

               for (jj=iitop1+1; jj<=iitop2; jj++)
               {
                  if (iaValHiLo[jj] > iRestoHiLoMin)
                  {
                     iRestoHiLoMin = iaValHiLo[jj];
                     iRestoHiLoValues++;
                  }
               }

               if (iDebug) fprintf(fLogFile,
                  "       %d different hilo restore values\n", iRestoHiLoValues);

            } /* (iitop2 - iitop1) */ 
            else
               iRestoHiLoValues = 1;

            iihilo1 = iitop1;
               /* index of 1st obj with cur hilo restore field */

            /* loop over different hilo fields for fixed top field */
            for (iHiLoValcur=1; iHiLoValcur<=iRestoHiLoValues; iHiLoValcur++)
            {
               if (iHiLoValcur == iRestoHiLoValues)
                  iihilo2 = iitop2;
               else
               {
                  iRestoHiLoMin = iaValHiLo[iihilo1];
                  for (jj=iitop1+1; jj<=iitop2; jj++)
                  {
                     if (iaValHiLo[jj] > iRestoHiLoMin)
                     {
                        iihilo2 = jj-1;
                        break;
                     }

                     if (jj == iitop2)
                        iihilo2 = jj;
                  }
               }

               if (iihilo1 == iihilo2)
               {
                  if (iDebug) fprintf(fLogFile,
                     "       %d. hilo restore value %d: only one object (%d)\n",
                     iHiLoValcur, iaValHiLo[iihilo1], iihilo1);

                  if (iHiLoValcur == iRestoHiLoValues)
                     break;
                  else
                  {
                     iihilo1 = iihilo2 + 1;
                     continue;
                  }
               }
               else if (iDebug) fprintf(fLogFile,
                  "       %d. hilo restore value %d: objs %d - %d\n",
                  iHiLoValcur, iaValHiLo[iihilo1], iihilo1, iihilo2);

               /* sort entries according to lohi: DDD not yet used */
               if (iRestoLoHi)
               {
                  ;
               } /* (iRestoLoHi) */

               if (iRestoLoLo)
               {
                  if (iihilo2 - iihilo1)
                  {
                     iRC = rawSortValues(iaValLoLo, iFileSort,
                               iihilo1, iihilo2, iaIndex, iaIndNew); 

                     /* rearrange other restore fields */
                     iDiff = 0;
                     for (jj=iihilo1; jj<=iihilo2; jj++)
                     {
                        if (iaIndNew[jj] != iaIndex[jj])
                        {
                           ii1 = iaIndNew[jj];
                           if (iRestoTop)
                              iaValTop[jj] = iaCompValue[ii1][1];
                           if (iRestoHiHi)
                              iaValHiHi[jj] = iaCompValue[ii1][2];
                           if (iRestoHiLo)
                              iaValHiLo[jj] = iaCompValue[ii1][3];
                           if (iRestoLoHi)
                              iaValLoHi[jj] = iaCompValue[ii1][4];
                           iDiff++;
                        }
                     }

                     /* update index field */
                     ii1 = iihilo2 - iihilo1 + 1;
                     memcpy(&iaIndex[iihilo1], &iaIndNew[iihilo1],
                            (unsigned) ii1*iint);

                     if (iDebug == 2)
                     {
                        fprintf(fLogFile,
                           "          restore order after lolo sorting (%d - %d: %d changes):\n",
                           iihilo1, iihilo2, iDiff);
                        for (jj=iihilo1; jj<=iihilo2; jj++)
                        {
                           fprintf(fLogFile,
                              "          %d: index %d, values: %u-%u-%u-%u-%u\n",
                              jj, iaIndNew[jj], iaValTop[jj],
                              iaValHiHi[jj], iaValHiLo[jj],
                              iaValLoHi[jj], iaValLoLo[jj]);
                        }
                     }
                  } /* (iihilo2 - iihilo1) */
                  else
                     iRestoLoLoValues = 1;

               } /* (iRestoLoLo) */
               else
                  iRestoLoLoValues = 0;

               iihilo1 = iihilo2 + 1;

            } /* iHiLoValcur: loop over different hilo restore fields */

         } /* (iRestoHiLo */
         else
            iRestoHiLoValues = 0;

         iitop1 = iitop2 + 1;

      } /* iTopValcur: loop over different top restore fields */
   } /* (iRestoTop) */
   else
      iRestoTopValues = 0;

   psObjSort = (srawRetrList *) piolsort;            /* first object */
   if (iFileAll > 0)
      psFileSort = (srawFileList *) piflsort;          /* first file */

   if (iDebug == 2)
   {
      fprintf(fLogFile,
         "DDD final restore order after sorting (%d-%d):\n",
         iisort1, iisort2);
      for (jj=iisort1; jj<=iisort2; jj++)
      {
         ii = iaIndex[jj];
         fprintf(fLogFile,
            "    %d: index %d, values: %u-%u-%u-%u-%u\n",
            jj, iaIndex[jj], iaCompValue[ii][1], iaCompValue[ii][2],
            iaCompValue[ii][3], iaCompValue[ii][4], iaCompValue[ii][5]);
      }
   }

   /******** copy + compress TSM entries to temp destination *********/

   psObjSort = (srawRetrList *) piolsort;            /* first object */
   psObj = psObj0;                       /* 1st object in 1st buffer */

   if (iFileAll > 0)
   {
      psFileSort = (srawFileList *) piflsort;          /* first file */
      psFile = psFile0;
   }

   iobj = iobj0;                     /* no. of objects in 1st buffer */
   iiObj = 0;                        /* running object no. in buffer */
   iiBuf = 1;                                  /* running buffer no. */
   icount = 0;

   if (iDebug == 2) fprintf(fLogFile,
      "DDD copy TSM objects to temp destination for compression\n");

   for (jj = 1; jj <= iObjAll; jj++)
   {
      /* skip empty, RC, WC entries */
      if (iaValue[jj][0] < 3)
      {
         if (iDebug == 2)
         {
            fprintf(fLogFile, "     (%d): ignored", jj);
            if (psObj->iStageFS)
               fprintf(fLogFile, " (RC)\n");
            else if (psObj->iCacheFS)
               fprintf(fLogFile, " (WC)\n");
            else
               fprintf(fLogFile, "\n");
         }

         iiObj++;
         psObj++;
         if (iFileAll > 0)
            psFile++;

         goto gNextCopy2Temp;
      }

      iiObj++; 
      icount++;

      memcpy((char *) psObjSort, (char *) psObj, iRetrList);
      if (iFileAll > 0)
         memcpy(psFileSort->cFile, psFile->cFile, iFileList);

      if (iDebug == 2)
      {
         fprintf(fLogFile,
            "    %d(%d): obj %s%s (objId %u-%u) copied (Comp), retrId %u-%u-%u-%u-%u\n",
            iiObj, jj, psObjSort->cNamehl, psObjSort->cNamell,
            psObjSort->iObjHigh, psObjSort->iObjLow,
            psObjSort->iRestoHigh, psObjSort->iRestoHighHigh,
            psObjSort->iRestoHighLow, psObjSort->iRestoLowHigh,
            psObjSort->iRestoLow);
      }

      psObj++;
      psObjSort++;

      if (iFileAll > 0)
      {
         psFile++;
         psFileSort++;
      }

      /* last object scanned */
gNextCopy2Temp:
      if (iiObj == iobj)
      {
         if (iDebug > 1) fprintf(fLogFile,
            "    buffer %d: last obj (no. %d) handled\n",
            iiBuf, iiObj);
         piptr = (int **) psObj;
         if (*piptr == NULL)
         {
            if (iDebug > 1) fprintf(fLogFile,
               "    %d: last obj buffer\n", iiBuf);
         }
         else
         {
            iiObj = 0;
            iiBuf++;
            pinext = *piptr;
            iobj = *pinext++;
            psObj = (srawRetrList *) pinext;
            if (iDebug > 1) fprintf(fLogFile,
               "    %d: new buffer, %d objs, first: |%s%s|\n",
               iiBuf, iobj, psObj->cNamehl, psObj->cNamell);
         }
      } /* iiObj == iobj */
   } /* loop copy TSM objects to temp destination for compression */

   if (iDebug == 2)
   {
      psObjSort = (srawRetrList *) piolsort;         /* first object */
      if (iFileAll > 0)
         psFileSort = (srawFileList *) piflsort;       /* first file */

      fprintf(fLogFile,
         "DDD compressed list of TSM objects (not yet sorted):\n");

      for (jj=1; jj<=iFileSort; jj++)
      {
         fprintf(fLogFile, "    %d: obj %s%s",
            jj, psObjSort->cNamehl, psObjSort->cNamell);
         psObjSort++;

         if (iFileAll > 0)
         {
            fprintf(fLogFile, ", file %s\n",
               psFileSort->cFile);
            psFileSort++;
         }
         else
            fprintf(fLogFile, "\n");
      }
   }

   if (icount != iFileSort)
   {
      fprintf(fLogFile,
         "-E- %s: unexpected no. of objects found: %d, expected %d\n",
         cModule, icount, iFileSort);
      return -5;
   }

   /********** copy sorted TSM entries to final destination **********
    **************** behind WC entries, using index ******************/

   psObjSort = (srawRetrList *) piolsort;            /* first object */
   if (iFileAll > 0)
      psFileSort = (srawFileList *) piflsort;          /* first file */

   if (iDebug == 2) fprintf(fLogFile,
      "DDD copy TSM objects in correct order to final destination\n");

   ii1 = 1;                              /* init position on 1st obj */
   for (jj = 1; jj <= iFileSort; jj++)
   {
      ii2 = iaIndex[jj];
      iDiff = ii2 - ii1;

      psObjSort += iDiff;
      if (iFileAll > 0)
         psFileSort += iDiff;

      icount++;

      memcpy((char *) psObjComp, (char *) psObjSort, iRetrList);
      if (iFileAll > 0)
         memcpy(psFileComp->cFile, psFileSort->cFile, iFileList);

      if (iDebug == 2)
      {
         fprintf(fLogFile,
            "    %d: TSM obj %s%s (index %d, objId %u-%u) copied (Comp), retrId %u-%u-%u-%u-%u\n",
            jj, psObjSort->cNamehl, psObjSort->cNamell, ii2,
            psObjSort->iObjHigh, psObjSort->iObjLow,
            psObjSort->iRestoHigh, psObjSort->iRestoHighHigh,
            psObjSort->iRestoHighLow, psObjSort->iRestoLowHigh, psObjSort->iRestoLow);
      }

      psObjComp++;
      if (iFileAll > 0)
         psFileComp++;
      ii1 = ii2;

   } /* for (jj...) */

   iObjAll = iReadCache + iWriteCache + iFileSort;
   if (iDebug)
   {
      psObjComp = psObjComp0;                        /* first object */
      if (iFileAll > 0)
         psFileComp = psFileComp0;                     /* first file */
      fprintf(fLogFile,
         "final list of all objects (compressed and sorted):\n");

      for (jj=1; jj<=iObjAll; jj++)
      {
         fprintf(fLogFile,
            "    %d: obj %s%s, objId %u-%u, retrId %u-%u-%u-%u-%u",
            jj, psObjComp->cNamehl, psObjComp->cNamell,
            psObjComp->iObjHigh, psObjComp->iObjLow,
            psObjComp->iRestoHigh, psObjComp->iRestoHighHigh,
            psObjComp->iRestoHighLow, psObjComp->iRestoLowHigh,
            psObjComp->iRestoLow);
         if (psObjComp->iStageFS)
            fprintf(fLogFile, " (RC)");
         else if (psObjComp->iCacheFS)
            fprintf(fLogFile, " (WC)");

         psObjComp++;

         if (iFileAll > 0)
         {
            fprintf(fLogFile, ", file %s\n", psFileComp->cFile);

            psFileComp++;
         }
         else
            fprintf(fLogFile, "\n");
      }
   }

gEndCheckObjlist:

   if ( (iSort) && (iFileAll > 0) )
   {
      memset(pcfl, 0x00, (unsigned) iFileBufComp);
      memcpy(pcfl, pcFileComp, (unsigned) iFileBufComp);
   }

   if (iDebug) fprintf(fLogFile,
      "-D- end %s\n\n", cModule);

   return 0;

} /* rawCheckObjList */

/*********************************************************************
 * rawDelFile: delete single file in GSI mass storage
 *
 * created  8.10.1996, Horst Goeringer
 *********************************************************************
 */

int rawDelFile( int iSocket, srawComm *psComm)
{
   char cModule[32] = "rawDelFile";
   int iDebug = 0;

   int iFSidRC = 0;
   int iFSidWC = 0;

   int iRC;
   int iBufComm;
   char *pcc;
   void *pBuf;
   char cMsg[STATUS_LEN] = "";

   srawStatus sStatus;

   iBufComm = ntohl(psComm->iCommLen) + HEAD_LEN;
   if (iDebug) printf(
      "\n-D- begin %s: delete file %s%s%s\n",
      cModule, psComm->cNamefs, psComm->cNamehl, psComm->cNamell);

   if (psComm->iStageFSid)
      iFSidRC = ntohl(psComm->iStageFSid);
   if (psComm->iFSidWC)
      iFSidWC = ntohl(psComm->iFSidWC);

   if (iDebug)
   {
      printf("    object %s%s%s found (objId %u-%u)", 
         psComm->cNamefs, psComm->cNamehl, psComm->cNamell, 
         ntohl(psComm->iObjHigh), ntohl(psComm->iObjLow));
      if (iFSidRC) printf(
         ", on %s in read cache FS %d\n", psComm->cNodeRC, iFSidRC);
      else
         printf( "\n"); 
      if (iFSidWC) printf(
         "     on %s in write cache FS %d\n", psComm->cNodeWC, iFSidWC);
   }

   psComm->iAction = htonl(REMOVE); 

   pcc = (char *) psComm;
   if ( (iRC = send( iSocket, pcc, (unsigned) iBufComm, 0 )) < iBufComm)
   {
      if (iRC < 0) printf(
         "-E- %s: sending delete request for file %s (%d byte)\n",
         cModule, psComm->cNamell, iBufComm);
      else printf(
         "-E- %s: delete request for file %s (%d byte) incompletely sent (%d byte)\n",
         cModule, psComm->cNamell, iBufComm, iRC);
      if (errno)
         printf("    %s\n", strerror(errno));
      perror("-E- sending delete request");

      return -1;
   }

   if (iDebug) printf(
      "    delete command sent to server (%d bytes), look for reply\n",
      iBufComm);

   /******************* look for reply from server *******************/

   iRC = rawRecvStatus(iSocket, &sStatus);
   if (iRC != HEAD_LEN)
   {
      if (iRC < HEAD_LEN) printf(
         "-E- %s: receiving status buffer\n", cModule);
      else
      {
         printf("-E- %s: message received from server:\n", cModule);
         printf("%s", sStatus.cStatus);
      }

      if (iDebug)
         printf("\n-D- end %s\n\n", cModule);

      return(-1);
   }

   if (iDebug) printf(
      "    status (%d) received from server (%d bytes)\n",
      sStatus.iStatus, iRC);

   /* delete in this proc only if WC and RC, 2nd step WC */
   printf("-I- gStore object %s%s%s successfully deleted",
      psComm->cNamefs, psComm->cNamehl, psComm->cNamell);
   if (iFSidWC)
      printf( " from write cache\n"); 
   else
      printf( "!\n"); 

   if (iDebug)
      printf("-D- end %s\n\n", cModule);

   return 0;

} /* end rawDelFile */

/*********************************************************************
 * rawDelList:  delete list of files in GSI mass storage
 *
 * created 22.10.1997, Horst Goeringer
 *********************************************************************
 */

int rawDelList( int iSocketMaster,
                            /* socket for connection to entry server */
                int iDataMover,                /* no. of data movers */
                srawDataMoverAttr *pDataMover0,
                srawComm *psComm,
                char **pcFileList,
                char **pcObjList) 
{
   char cModule[32] = "rawDelList";
   int iDebug = 0;

   int iSocket;
   char *pcfl, *pcol;
   int *pifl, *piol;
   int ifile;
   int iobj0, iobj;
   int iobjBuf;

   srawFileList *psFile, *psFile0;           /* files to be archived */
   srawRetrList *psObj;                  /* objects already archived */
   srawDataMoverAttr *pDataMover;              /* current data mover */

   char *pcc, *pdelim;

   bool_t bDelete, bDelDone;
   int **piptr;                  /* points to pointer to next buffer */

   int ii, jj, kk;
   int iRC, idel;

   pcfl = *pcFileList;
   pifl = (int *) pcfl;
   ifile = pifl[0];                            /* number of files */
   psFile = (srawFileList *) ++pifl;  /* points now to first file */
   psFile0 = psFile;
   pifl--;                     /* points again to number of files */

   if (iDebug)
   {
      printf("\n-D- begin %s\n", cModule);
      printf("    initial %d files, first file %s\n",
             ifile, psFile0->cFile);
   } 

   pDataMover = pDataMover0;
   pcol = *pcObjList;
   piol = (int *) pcol;
   iobj0 = 0;                     /* total no. of archived objects */

   iobjBuf = 0;                             /* count query buffers */
   idel = 0;                                /* count deleted files */
   bDelDone = bFalse;
   while (!bDelDone)
   {
      iobjBuf++;
      iobj = piol[0];            /* no. of objects in query buffer */
      psObj = (srawRetrList *) ++piol;
                                     /* points now to first object */
      if (iDebug)
         printf("    buffer %d: %d objects, first obj %s%s (server %d)\n",
                iobjBuf, iobj, psObj->cNamehl, psObj->cNamell, psObj->iATLServer);

      psComm->iAction = htonl(REMOVE);
      for (ii=1; ii<=iobj; ii++)    /* loop over objects in buffer */
      {
         iobj0++;
         pcc = (char *) psObj->cNamell;
         pcc++;                          /* skip object delimiter  */

         if (iDebug) printf(
            "    obj %d: %s%s, objId %d-%d\n",
            ii, psObj->cNamehl, psObj->cNamell,
            psObj->iObjHigh, psObj->iObjLow);

         bDelete = bFalse;
         psFile = psFile0;
         for (jj=1; jj<=ifile; jj++)                  /* file loop */
         {
            if (iDebug) printf(
               "    file %d: %s\n", jj, psFile->cFile);

            pdelim = strrchr(psFile->cFile, *pcFileDelim);
            if (pdelim == NULL)
            {
#ifdef VMS
               pdelim = strrchr(psFile->cFile, *pcFileDelim2);
               if (pdelim != NULL) pdelim++;
               else
#endif
               pdelim = psFile->cFile;
            }
            else pdelim++;                 /* skip file delimiter  */

            iRC = strcmp(pdelim, pcc);
            if ( iRC == 0 )
            {
               bDelete = bTrue;
               break;
            }
            psFile++;
         } /* file loop (jj) */

         if (bDelete)
         {
            if (iDebug)
            {
               printf("    matching file %d: %s, obj %d: %s%s",
                      jj, psFile->cFile, ii, psObj->cNamehl, psObj->cNamell);
               if (psObj->iStageFS)
                  printf(", on DM %s in StageFS %d\n",
                         psObj->cMoverStage, psObj->iStageFS);
               else if (psObj->iCacheFS)
               {
                  printf(", on DM %s in ArchiveFS %d\n",
                         psObj->cMoverStage, psObj->iCacheFS);
                  printf("    archived at %s by %s\n",
                         psObj->cArchiveDate, psObj->cOwner);
               }
               else
                  printf(" (not in disk pool)\n");
            }

            psComm->iObjHigh = htonl(psObj->iObjHigh);
            psComm->iObjLow = htonl(psObj->iObjLow);
            psComm->iATLServer = htonl(psObj->iATLServer);

            if (psObj->iStageFS)
            {
               psComm->iPoolIdRC = htonl(psObj->iPoolIdRC);
               psComm->iStageFSid = htonl(psObj->iStageFS);
               strcpy(psComm->cNodeRC, psObj->cMoverStage);
            }
            else
            {
               psComm->iPoolIdRC = htonl(0);
               psComm->iStageFSid = htonl(0);
               strcpy(psComm->cNodeRC, "");
            }

            if (psObj->iCacheFS)
            {
               if (psObj->iStageFS)
                  psComm->iPoolIdWC = htonl(0); /* WC poolId unavail */
               else
                  psComm->iPoolIdWC = htonl(psObj->iPoolIdRC);
               psComm->iFSidWC = htonl(psObj->iCacheFS);
               strcpy(psComm->cNodeWC, psObj->cMoverCache);
            }
            else
            {
               psComm->iPoolIdWC = htonl(0);
               psComm->iFSidWC = htonl(0);
               strcpy(psComm->cNodeWC, "");
            }

            iRC = rawGetLLName(psFile->cFile,
                               pcObjDelim, psComm->cNamell);

            if (iDataMover > 1)
            {
               if ((strcmp(pDataMover->cNode, psObj->cMoverStage) == 0) ||
                   (strlen(psObj->cMoverStage) == 0))  /* not staged */
               {
                  iSocket = pDataMover->iSocket;
                  if (iDebug) printf(
                     "    current data mover %s, socket %d\n",
                     pDataMover->cNode, iSocket);
               }
               else
               {
                  pDataMover = pDataMover0;
                  for (kk=1; kk<=iDataMover; kk++)
                  {
                     if (strcmp(pDataMover->cNode,
                                psObj->cMoverStage) == 0)
                        break;
                     pDataMover++;
                  }

                  if (kk > iDataMover)
                  {
                     printf("-E- %s: data mover %s not found in list\n",
                            cModule, psObj->cMoverStage);
                     return -1;
                  }

                  iSocket = pDataMover->iSocket;
                  if (iDebug) printf(
                     "    new data mover %s, socket %d\n",
                     pDataMover->cNode, iSocket);
               }
            } /* (iDataMover > 1) */

            iRC = rawDelFile(iSocketMaster, psComm);
            if (iRC)
            {
               if (iDebug)
                  printf("    rawDelFile: rc = %d\n", iRC);
               if (iRC < 0)
               {
                  printf("-E- %s: file %s could not be deleted\n",
                         cModule, psFile->cFile);
                  if (iDebug)
                     printf("-D- end %s\n\n", cModule);

                  return -1;
               }
               /* else: object not found, ignore */

            } /* (iRC) */

            idel++;

         } /* if (bDelete) */
         else if (iDebug) printf(
            "    file %s: obj %s%s not found in gStore\n",
            psFile0->cFile, psObj->cNamehl, psObj->cNamell);

         psObj++;

      } /* loop over objects in query buffer (ii) */

      piptr = (int **) psObj;
      if (*piptr == NULL) bDelDone = bTrue;
      else piol = *piptr;

   } /* while (!bDelDone) */

   if (iDebug)
      printf("-D- end %s\n\n", cModule);

   return(idel);

} /* rawDelList */

/**********************************************************************
 * rawGetFilelistEntries:
 *    get filelist entries from input file
 *    if global path specified, add to file name
 *    uses external buffer ptr piEntryList provided by caller
 *    replaces '%' (historical) by '?' (supported by TSM)
 *    removes tape file specifications
 *    removes duplicate entries
 *    orders: generic file names first
 *
 * created 22. 5.2003 by Horst Goeringer
 **********************************************************************
 */

int rawGetFilelistEntries( char *pcFileName,
                           int *piDataFS,
                           char *pcDataFS,
                           int *piEntries,
                           int *piGlobalDir)
{
   char cModule[32] = "rawGetFilelistEntries";
   int iDebug = 0;

   char cMsg[STATUS_LEN] = "";
   int ii, jj;
   int iDelCount = 0;
   int iFilesIgnored = 0;   /* =1: no. of files with specific subdir */
   int iGlobalDir = 0;       /* =1: global dir in 1st line specified */
   int iDataFS = 0;                   /* =1: path in central data FS */
   int imax = MAX_FULL_FILE - 1;
   char cPath[MAX_FULL_FILE] = ""; 
   char cEntry[MAX_FULL_FILE] = "";
   char cTemp[MAX_FULL_FILE] = "";
   char cFileName[MAX_FULL_FILE] = "";
   char cQualifier[16] = ".filelist";
   const char *pcLeading = "@";
   char *pcc;
   char *ploc;
   char *pccT = NULL; 
   char *pccE; 

   int iRemove = 0;
   int iGeneric = 0;                 /* = 1: generic file name found */
   int iSingle = 0;   /* = 1: file name without wildcard chars found */
   int iRC;
   int iError;

   int iGenericEntries = 0;
   int iSingleEntries = 0;
   int iEntries = -1;
   FILE *fiFile = NULL;

   int iMaxEntries;
   int iSizeBuffer;
   int *piFilelist, *piFilelisto;
   srawFileList *pFilelist,                  /* first filelist entry */
                *pFilelistc,               /* current filelist entry */
                *pFilelistco,   /* current filelist entry old buffer */
                *pFilelistc0;   /* current filelist entry for remove */

   if (iDebug)
      printf("\n-D- begin %s\n", cModule);

   iDataFS = *piDataFS;
   piFilelist = piEntryList;             /* copy of external pointer */
   iPathPrefix = strlen(cPathPrefix);              /* from rawclin.h */

   if ((int) strlen(pcFileName) >= imax)
   {
      fprintf(fLogFile,
         "-E- %s: file name too long (max %d)\n",
         cModule, --imax);
      iError = -1;
      goto gErrorFilelist;
   }
   strcpy(cFileName, pcFileName);                      /* local copy */

   pccT = (char *) strrchr(pcFileName, *pcLeading);
   if (pccT)
   {
      pccE = (char *) strrchr(pccT, *pcObjDelim);
      if (pccE)                             /* '@' only in path name */
         pccT = NULL;
   }

   if (pccT == NULL)
   {
      if (iDebug) fprintf(fLogFile,
         "    no leading '%s' in file name %s\n", pcLeading, pcFileName);
      goto gEndFilelist;
   }

   pcc = cQualifier;
   ploc = strrchr(cFileName, pcc[0]);
   if (ploc != NULL)
   {
      ploc++;
      pcc++;
      if (strcmp(ploc, pcc) != 0)
      {
         if (iDebug) fprintf(fLogFile,
            "    no trailing %s\n", cQualifier);
         goto gEndFilelist;
      }
   }
   else
   {
      if (iDebug) fprintf(fLogFile,
         "    no trailing %s in file name %s\n", cQualifier, pcFileName);
      goto gEndFilelist;
   }

   if (iDebug) fprintf(fLogFile,
      "    %s is a filelist\n", cFileName);

   fiFile = fopen(pcFileName, "r");
   if (fiFile == NULL)
   {
      fprintf(fLogFile, "-E- %s: opening filelist %s\n",
         cModule, pcFileName);
      if (errno)
         fprintf(fLogFile, "    %s\n", strerror(errno));
      perror("-E- opening filelist");

      iError = -1;
      goto gErrorFilelist;
   }
   if (iDebug) fprintf(fLogFile,
      "    filelist %s opened\n", pcFileName);

   piFilelisto = piFilelist;
   iMaxEntries = *piFilelist;
   if (iDebug) fprintf(fLogFile,
      "    max no. of entries in filelist buffer: %d\n", iMaxEntries);
   if (iDebug == 2) fprintf(fLogFile,
      "DDD piFilelist %p, *piFilelist %d\n",
      piFilelist, *piFilelist);

   pFilelist = (srawFileList *) &(piFilelist[1]);
                                          /* skip max no. of entries */
   pFilelistc = pFilelist;                          /* current entry */
   pccE = cEntry;
   iEntries = 0;
   for(;;)
   {
      pccE = fgets(cEntry, imax, fiFile);

      if ( (pccE != NULL) && (strlen(pccE) > 1) )
      {
         /* remove blanks, line feeds, and so on */
         ii = strlen(pccE);
         memset(cTemp, '\0', strlen(cTemp)); 
         pccT = cTemp;
         iDelCount = 0;
         while (ii-- > 0)
         {
            if ( (*pccE != '\0') && (*pccE != '\n') && (*pccE != '\r') &&
                 (*pccE != ' ') )
            {
               strncpy(pccT, pccE, 1);
               pccT++;
            }
            else
               iDelCount++;

            pccE++;

         }
         strncpy(pccT, "\0", 1);           /* terminates copied name */

         if (iDebug == 2) fprintf(fLogFile,
            "'%s' -> '%s': %d chars removed\n", cEntry, cTemp, iDelCount);

         /* check for path prefix */
         if (iEntries == 0)
         {
            if (strncmp(cTemp, cPathPrefix, (unsigned) iPathPrefix) == 0)
            {
               iGlobalDir = 1;

               if (iDebug) fprintf(fLogFile,
                  "    path prefix '%s' in first filelist entry found\n",
                  cPathPrefix);

               pccT = cTemp;
               for (ii=1; ii<=iPathPrefix; ii++)
               { pccT++; }

               if (strlen(pccT) == 0)
               {
                  fprintf(fLogFile,
                     "-W- no path name found after path prefix: %s\n",
                     cEntry);
                  continue;
               }

               ii = strlen(pccT) - 1;
               pcc = pccT;
               pcc += ii;
               if (strncmp(pcc, "/", 1) == 0)        /* trailing '/' */
                  strncpy(cPath, pccT, (unsigned) ii);
               else
                  strcpy(cPath, pccT);

               if (iDebug) fprintf(fLogFile,
                  "    path '%s' for files in filelist\n", cPath);

               if ( (strncmp(cPath, cDataFSHigh1,
                             strlen(cDataFSHigh1)) == 0) ||
                    (strncmp(cPath, cDataFSHigh2,
                             strlen(cDataFSHigh2)) == 0) )
               {
                  iDataFS = 1;
                  strcpy(pcDataFS, cPath);

                  if (iDebug) fprintf(fLogFile,
                     "    central data FS path %s\n", cPath);
               }

               continue;

            } /* path prefix found */
         } /* (iEntries == 0) */

         if (strncmp(cTemp, cPathPrefix, (unsigned) iPathPrefix) == 0)
         {
            fprintf(fLogFile,
               "-E- only one path specification in file list allowed, %s and following files ignored\n",
               cTemp);
            break;
         }

         /* replace '%' (historical) by '?' (supported by TSM) */
         while ( (ploc = strchr(cTemp, *pcPerc)) != NULL)
         {
            if (iDebug)
               fprintf(fLogFile, "-W- replace %s", cTemp);
            *ploc = *pcQM;
            if (iDebug)
               fprintf(fLogFile, " by %s\n", cTemp);
         }

         if ( (iEntries == iMaxEntries) &&             /* reallocate */
              (iEntries > 0) )
         {
            iMaxEntries += iMaxEntries;
            if (iDebug) fprintf(fLogFile,
               "    entry buffer full, reallocate: max %d entries\n",
               iMaxEntries);

            iSizeBuffer = iMaxEntries*MAX_FULL_FILE + sizeof(int);
            piFilelisto = piFilelist;
            piFilelist = (int *) calloc(
               (unsigned) iSizeBuffer, sizeof(char) );
            if (piFilelist == NULL)
            {
               fprintf(fLogFile,
                  "-E- %s: reallocating filelist buffer (size %d)\n",
                  cModule, iSizeBuffer);
               if (errno)
                  fprintf(fLogFile, "    %s\n", strerror(errno));
               perror("-E- reallocating filelist buffer");

               iError = -1;
               goto gErrorFilelist;
            }

            if (iDebug) fprintf(fLogFile,
               "    filelist entry buffer reallocated (size %d)\n",
               iSizeBuffer);

            *piFilelist = iMaxEntries;
            if (iDebug == 2) fprintf(fLogFile,
               "DDD piFilelist %p, *piFilelist %d\n",
                piFilelist, *piFilelist);

            pFilelistc = (srawFileList *) &(piFilelist[1]);   /* new */
            pFilelistco = pFilelist;                   /* old buffer */
            pFilelist = pFilelistc;              /* first new buffer */

            for (ii=1; ii<=iEntries; ii++)
            {
               if (iDebug == 2) fprintf(fLogFile,
                  "DDD pFilelistc %p\n", pFilelistc);
               strcpy(pFilelistc->cFile, pFilelistco->cFile);
               pFilelistc++;
               pFilelistco++;
            }

            if (iDebug) fprintf(fLogFile,
               "    %d old entries copied to new buffer, next:\n",
               iEntries);

            if (iDebug == 2) fprintf(fLogFile,
               "DDD free piFilelisto %p\n", piFilelisto);
            free(piFilelisto);
            piFilelisto = piFilelist;

         } /* (iEntries == iMaxEntries) */

         if (iDebug == 2) fprintf(fLogFile,
            "DDD pFilelistc %p\n", pFilelistc);

         if ( (ploc = strchr(cTemp, *pcObjDelim)) != NULL)
         {
            iFilesIgnored++;
            if (iFilesIgnored == 1) fprintf(fLogFile,
               "-W- file-specific path not yet implemented in filelist\n"); 
            fprintf(fLogFile, "-W- %s ignored\n", cTemp);
            continue;
         }

         if (iGlobalDir)
         {
            strcpy(pFilelistc->cFile, cPath);
            strncat(pFilelistc->cFile, "/", 1);
         }
         strcat(pFilelistc->cFile, cTemp);
         iEntries++;

         if (iDebug) fprintf(fLogFile,
            "    %3d: %s \n", iEntries, pFilelistc->cFile);

         pFilelistc++;
      }
      else
         break;

   } /* read loop */

   *piGlobalDir = iGlobalDir;                          /* for caller */

   pFilelistc = pFilelist;
   if (iDebug)
   {
      fprintf(fLogFile, "    after allocation:\n");
      for (ii=1; ii<=iEntries; ii++)
      {
         fprintf(fLogFile,
            "    %3da: %s \n", ii, pFilelistc->cFile);
         pFilelistc++;
      }
   }

   /* check if duplicate entries */
   if (iEntries)
   {
      pFilelistc = pFilelist;
      iRemove = 0;
      for (ii=1; ii<iEntries; ii++)
      {
         if (strlen(pFilelistc->cFile) == 0)      /* already removed */
         {
            pFilelistc++;
            continue;
         }

         pFilelistco = pFilelistc;
         pFilelistco++;
         for (jj=ii+1; jj<=iEntries; jj++)
         {
            if (strcmp(pFilelistc->cFile, pFilelistco->cFile) == 0)
            {
               if (iDebug)
                  fprintf(fLogFile, "-W- duplicate entry %s removed\n",
                          pFilelistco->cFile); 
               strcpy(pFilelistco->cFile, "");
               iRemove++;
            }
            pFilelistco++;
         }

         pFilelistc++;
      }

      if (iDebug)
      {
         if (iRemove) fprintf(fLogFile,
            "    %d duplicate entries found in list\n", iRemove);
         else fprintf(fLogFile,
            "    no duplicate entries found in list\n");
      }
   } /* (iEntries) */
   else
   {
      if (iDebug)
         fprintf(fLogFile, "    no valid entries found in list\n");
      goto gEndFilelist;
   }
   
   pFilelistc = pFilelist;
   if (iDebug)
   {
      fprintf(fLogFile, "    after removal of duplicates:\n");
      for (ii=1; ii<=iEntries; ii++)
      {
         fprintf(fLogFile,
            "    %3db: %s \n", ii, pFilelistc->cFile);
         pFilelistc++;
      }
   }

   /* reorder remaining entries */
   if (iRemove)
   {
      pFilelistc = pFilelist;
      for (ii=1; ii<iEntries; ii++)
      {
         if (strlen(pFilelistc->cFile) == 0)              /* removed */
         {
            pFilelistco = pFilelistc;
            pFilelistco++;
            pFilelistc0 = pFilelistc;
            for (jj=ii+1; jj<=iEntries; jj++)
            {
               strcpy(pFilelistc0->cFile, pFilelistco->cFile);
               pFilelistc0++;
               pFilelistco++;
            }
            continue;
                /* no ptr incr, as same location to be checked again */
         }
         pFilelistc++;
      }
      iEntries -= iRemove;
   }

   *piFilelist = iEntries;                           /* store result */

   /* check if sorting necessary */
   pFilelistc = pFilelist;
   for (ii=1; ii<=iEntries; ii++)
   {
      ploc = strchr(pFilelistc->cFile, *pcStar);
      if (ploc != NULL)
         iGeneric = 1;
      else
      {
         ploc = strchr(pFilelistc->cFile, *pcQM);
         if (ploc != NULL)
            iGeneric = 1;
         else
         {
            ploc = strchr(pFilelistc->cFile, *pcPerc);
            if (ploc != NULL)
               iGeneric = 1;
            else
               iSingle = 1;
         }
      }
      if ( (iGeneric) && (iSingle) )
         break;

      pFilelistc++;
   }

   /* reorder: generic names first */
   if ( (iGeneric) && (iSingle) )
   {
      piFilelisto = piFilelist;

      /* create buffer for sorted entries */
      iSizeBuffer = iEntries*MAX_FULL_FILE + sizeof(int);
      piFilelist = (int *) calloc((unsigned) iSizeBuffer, sizeof(char));
      if (piFilelist == NULL)
      {
         fprintf(fLogFile,
            "-E- %s: allocating ordered filelist buffer (size %d)\n",
            cModule, iSizeBuffer);
         if (errno)
            fprintf(fLogFile, "    %s\n", strerror(errno));
         perror("-E- allocating ordered filelist buffer");

         iError = -1;
         goto gErrorFilelist;
      }

      if (iDebug) fprintf(fLogFile,
         "    ordered filelist buffer allocated (size %d)\n",
         iSizeBuffer);

      if (iDebug == 2) fprintf(fLogFile,
         "DDD piFilelist %p, *piFilelist %d\n",
         piFilelist, *piFilelist);

      pFilelistc = (srawFileList *) &(piFilelist[1]);
      pFilelist = pFilelistc;
      pFilelistco = (srawFileList *) &(piFilelisto[1]);
      iGenericEntries = 0;

      /* copy generic entries */
      for (ii=1; ii<=iEntries; ii++)
      {
         iGeneric = 0;
         iSingle = 0;
         ploc = strchr(pFilelistco->cFile, *pcStar);
         if (ploc != NULL)
            iGeneric = 1;
         else
         {
            ploc = strchr(pFilelistco->cFile, *pcQM);
            if (ploc != NULL)
               iGeneric = 1;
            else
            {
               ploc = strchr(pFilelistco->cFile, *pcPerc);
               if (ploc != NULL)
                  iGeneric = 1;
               else
                  iSingle = 1;
            }
         }

         if (iGeneric)
         {
            strcpy(pFilelistc->cFile, pFilelistco->cFile);
            strcpy(pFilelistco->cFile, "");
            pFilelistc++;
            iGenericEntries++;
         }
         pFilelistco++;

      } /* loop pFilelistco */

      pFilelistco = (srawFileList *) &(piFilelisto[1]);
      iSingleEntries = 0;

      /* copy non-generic entries */
      for (ii=1; ii<=iEntries; ii++)
      {
         if (strlen(pFilelistco->cFile) != 0)
         {
            strcpy(pFilelistc->cFile, pFilelistco->cFile);
            pFilelistc++;
            iSingleEntries++;
         }
         pFilelistco++;
      }

      if (iDebug) fprintf(fLogFile,
         "    %d generic file names, followed by %d non-generic file names\n",
         iGenericEntries, iSingleEntries);

   } /* (iGeneric) && (iSingle) */

   pFilelistc = pFilelist;
   if (iDebug)
   {
      fprintf(fLogFile, "    after reordering:\n");
      for (ii=1; ii<=iEntries; ii++)
      {
         fprintf(fLogFile,
            "    %3dc: %s \n", ii, pFilelistc->cFile);
         pFilelistc++;
      }
   }

gEndFilelist:
   *piEntries = iEntries;
   iError = 0;

   if ( (iDebug == 2) && (pccT) ) fprintf(fLogFile,
      "DDD piFilelist %p, *piFilelist %d\n",
      piFilelist, *piFilelist);

gErrorFilelist:
   if (fiFile)
   {
      iRC = fclose(fiFile);
      if (iRC)
      {
         fprintf(fLogFile, "-E- %s: closing filelist %s\n",
            cModule, pcFileName);
         if (errno)
            fprintf(fLogFile, "    %s\n", strerror(errno));
         perror("-E- closing filelist");
      }
   }

   *piDataFS = iDataFS;
   piEntryList = piFilelist;             /* pass to external pointer */

   if (iDebug)
      printf("-D- end %s\n\n", cModule);

   return iError;

} /* rawGetFilelistEntries */

/********************************************************************
 * rawGetWSInfo: get workspace and pool info from master server
 *
 * created 30. 6.2003 by Horst Goeringer
 ********************************************************************
 */

int rawGetWSInfo( srawCliActionComm *pCliActionComm,
                  srawPoolStatus *pPoolInfo,
                  srawWorkSpace **ppWorkSpace) 
{
   char cModule[32] = "rawGetWSInfo";
   int iDebug = 0;

   int iRC = 0;
   int iSocket = 0;
   int iPoolInfo = 1;           /* =1: provide info on stage pool(s) */
   int iPrintPoolInfo = 0;        /* =1: print info on stage pool(s) */
   int iWorkSpaceInfo = 0;          /* =1: provide info on workspace */
   char cMsg[STATUS_LEN] = "";

   int iAction = 0;
   int iBuf;
   int ii, ii1, ii2;
   int iIdent = 0;
   int iStatus = 0;
   int iAttrLen = 0;

   int iPoolId = 0;        /* identifier of requested pool, = 0: all */
   int iSleepClean = 0;          /* sleep if clean job needs started */

   int iPoolmax = 0;
   int iPoolcur = 0;
   int iBufPool = 0;

   char cPoolNameRetr[32] = "RetrievePool";
   char cPoolNameStage[32] = "StagePool";
   int iHardwareMax = 0;            /* max space in hardware (MByte) */
   int iHardwareFree = 0;          /* free space in hardware (MByte) */
   int iPoolRetrMax = 0;     /* overall size of RetrievePool (MByte) */
   int iPoolRetrFree = 0;      /* free space of RetrievePool (MByte) */
   int iPoolRetrFiles = 0;    /* no. of files stored in RetrievePool */
   int iPoolStageMax = 0;       /* overall size of StagePool (MByte) */
   int iPoolStageFree = 0;        /* free space of StagePool (MByte) */
   int iPoolStageFiles = 0;      /* no. of files stored in StagePool */

   int iRandomExcess = 0;
         /* unused space of other pools used in RetrievePool (MByte) */
   int iPoolStageAvail = 0;
   int iPoolStageMaxWS = 0;      /* max WS size in StagePool (MByte) */
   int iPoolStageCheck = 0;
                  /* min work space check size for StagePool (MByte) */
   int iStageSizeUnavail = 0;
               /* StagePool space currently allocated by other pools */

   int iWorkSizeNew = 0;
              /* still to be staged for requested work space (MByte) */
   int iWorkSizeAll = 0;     /* size of requested work space (MByte) */
   int iWorkFilesAll = 0;        /* total no. of files in work space */
   int iWorkSizeSta = 0;  /* part of requ. work space already staged */
   int iWorkFilesSta = 0;
                        /* no. of files in work space already staged */
   int iWorkSizeStaStage = 0;
                           /* part of staged work space in StagePool */
   int iWorkFilesStaStage = 0;   /* no. of staged files in StagePool */
   int iWorkSizeStaRetr = 0;
                        /* part of staged work space in RetrievePool */
   int iWorkFilesStaRetr = 0; /* no. of staged files in RetrievePool */
   int iWorkSizeEst = 0;  /* part of requ. work space size estimated */
   int iWorkFilesEst = 0;
                   /* no. of files in work space with size estimated */
   int iWorkStatus = 0;       /* status of work space sent by server */


   int *piBuffer;
   char *pcc;
   char cMsgPref[8] = "";
   char cMisc[128] = "";
   char pcGenFile[MAX_FULL_FILE] = "";   /* full (generic) file name */

   srawWorkSpace *pWorkSpace; 
   srawPoolStatusData *pPoolInfoData, *pPoolInfoData0;

   if (iDebug)
      printf("\n-D- begin %s\n", cModule);

   piBuffer = pCliActionComm->piBuffer;      /* area to receive data */
   iSocket = pCliActionComm->iSocket;
   iAction = pCliActionComm->iAction;
  
   iPoolInfo = 1;
   if (iAction == QUERY_POOL)
   {
      iPoolId = pCliActionComm->iStatus;
      if (iPoolId == 0)
         iPrintPoolInfo = 3;
      else
         iPrintPoolInfo = iPoolId;

      iWorkSpaceInfo = 0;
   }
   else
   {
      strcpy(pcGenFile, pCliActionComm->pcFile);
      iPoolId = 0;     /* show info about StagePool and RetrievePool */

      if (pCliActionComm->iStatus > 0)
         iPrintPoolInfo = pCliActionComm->iStatus;
      else
         iPrintPoolInfo = 0;

      iWorkSpaceInfo = 1;
   }

   if (iDebug)
   {
      printf("    action %d", iAction);
      if (iPrintPoolInfo)
      {
         printf(", print pool info");

         if (iAction == QUERY_POOL)
            printf(", poolId  %d\n", iPoolId);
         else
         {
            if (iPrintPoolInfo == 1)
               printf(" for RetrievePool\n");
            else if (iPrintPoolInfo == 2)
               printf(" for StagePool\n");
            else
               printf(" for RetrievePool and StagePool\n");
         }
      }
      else
         printf("\n");
   }

   if (iPoolInfo)
   {
      pcc = (char *) piBuffer;               /* header for pool info */
      pPoolInfo = (srawPoolStatus *) piBuffer;         /* for caller */
      iRC = rawRecvHead(iSocket, pcc);
      if (iRC <= 0)
      {
        printf("-E- receiving header pool status from master server\n");
        return -1;
      }

      iIdent = ntohl(pPoolInfo->iIdent);
      iStatus = ntohl(pPoolInfo->iPoolNo);
      iAttrLen = ntohl(pPoolInfo->iStatusLen);

      if (iDebug) printf(
         "    header pool status received (%d bytes): ident %d, status %d, attrlen %d\n",
         iRC, iIdent, iStatus, iAttrLen);

      /* expect IDENT_POOL */
      if (iIdent != IDENT_POOL)
      {
         if (iIdent == IDENT_STATUS)
         {
            if ( (iStatus == STA_ERROR) || (iStatus == STA_ERROR_EOF) )
            {
               printf("-E- received error status from server");
               if (iDebug)
                  printf(" instead of pool info:\n");
               else printf(":\n");

               if (iAttrLen > 0)
               {
                  pcc = cMsg;
                  iRC = rawRecvError(iSocket, iAttrLen, pcc);
                  if (iRC < 0) printf(
                     "-E- receiving error msg from server, rc = %d\n",
                     iRC);
                  else printf("    %s\n", pcc);
               }
               else printf("    no error message available\n");

            } /* (iStatus == STA_ERROR) || (iStatus == STA_ERROR_EOF) */
            else
            { 
               printf(
                  "-E- unexpected status (type %d) received from server\n",
                  iStatus);
            }
         } /* iIdent == IDENT_STATUS */
         else
         { 
            printf(
               "-E- unexpected header (%d) received from server\n",
               iIdent);
         }

         return -1;

      } /* (iIdent != IDENT_POOL) ) */

      iPoolmax = iStatus;
      iBufPool = iAttrLen;        /* size pool buffer without header */

      pcc += HEAD_LEN;
      pPoolInfoData = (srawPoolStatusData *) pcc;
      pPoolInfoData0 = pPoolInfoData;             /* first pool info */

      /* receive staging pool info */
      iBuf = iBufPool;
      while(iBuf > 0)
      {
         if ( (iRC = recv( iSocket, pcc, (unsigned) iBuf, 0 )) <= 0)
         {
            if (iRC < 0) printf(
               "-E- %s: receiving pool info\n", cModule);
            else
            {
               ii = iBufPool - iBuf;
               printf(
                  "-E- %s: connection to entry server broken, %d byte of pool info (%d byte) received\n",
                  cModule, ii, iBufPool);
            }
            if (errno)
               printf("    %s\n", strerror(errno));
            perror("-E- receiving pool info");

            return -1;
         }

         iBuf -= iRC;
         pcc += iRC;

      } /* while(iBuf > 0) */

      if (iDebug)
      {
         printf("    status of %d pools received (%d bytes)\n",
            iPoolmax, iBufPool);
         if ( (iPoolmax == 3) || (iPoolmax == 5) ) printf(
            "    data of pools 2 (StagePool) and %d (GlobalPool) are merged\n",
            iPoolmax);
      }

      /* get pool names first for info messages */
      if ( (iAction == QUERY_POOL) || (iAction == QUERY_WORKSPACE) ||
           (iAction == STAGE) )
         for (iPoolcur=1; iPoolcur<=iPoolmax; iPoolcur++)
      {
         if (iPoolcur == 1)                          /* RetrievePool */
         {
            /* convert to GByte */
            iPoolRetrMax = ntohl(pPoolInfoData->iMaxSize)/1000;
            iPoolRetrFree = ntohl(pPoolInfoData->iFreeSize)/1000;

            if (iPoolRetrFree < 0)
            {
               iRandomExcess = -iPoolRetrFree;
               iPoolRetrFree = 0;
            }
            iPoolRetrFiles = ntohl(pPoolInfoData->iFiles);
            if ( (iPoolRetrFree == iPoolRetrMax) && (iPoolRetrFiles > 0) )
               iPoolRetrFree--;         /* eliminate rounding errors */

            if ( (iPrintPoolInfo == 1) || (iPrintPoolInfo == 3) )
            {
               printf("    %s: used for 'gstore retrieve'\n",
                  pPoolInfoData->cPoolName);
               printf("       free space  %8d GByte\n", iPoolRetrFree);
               printf("       no min file lifetime of files\n");
            }
         }

         if (iPoolcur == 2)                             /* StagePool */
         {
            /* corrected numbers, if deviating from 1st pool */
            iHardwareMax = ntohl(pPoolInfoData->iMaxSizeHW)/1000;
            iHardwareFree = ntohl(pPoolInfoData->iFreeSizeHW)/1000;

            iPoolStageMax = ntohl(pPoolInfoData->iMaxSize)/1000;
            iPoolStageFree = ntohl(pPoolInfoData->iFreeSize)/1000;

            if (iPoolStageFree < 0)
               iPoolStageFree = 0;

            iPoolStageFiles = ntohl(pPoolInfoData->iFiles);
            if ( (iPoolStageFree == iPoolStageMax) &&
                 (iPoolStageFiles > 0) )
               iPoolStageFree--;        /* eliminate rounding errors */

            iPoolStageAvail = ntohl(pPoolInfoData->iFileAvail);

            if ( (iPrintPoolInfo == 2) || (iPrintPoolInfo == 3) ) printf(
               "    %s: used for 'gstore stage'\n",
               pPoolInfoData->cPoolName);

            /* no global pool */
            if ( (iPoolmax == 2) || (iPoolmax == 4) )
            {
               ii = iPoolStageFree + iPoolRetrFree;
               if (iHardwareFree != ii)
               iHardwareFree = ii;      /* eliminate rounding errors */

               if ( (iPrintPoolInfo == 2) || (iPrintPoolInfo == 3) ||
                    (iDebug) )
               {
                  printf("       free space  %8d GByte\n", iPoolStageFree);
                  printf(
                     "       min file availability of %d days guaranteed\n",
                     ntohl(pPoolInfoData->iFileAvail));

                  /* at end of last pool iteration, NOT after loop */
                  printf(
                     "    free HW space  %8d GByte, max available %8d GByte\n",
                     iHardwareFree, iHardwareMax);
                  printf(
                     "    RetrievePool and StagePool share available disk space\n");
               }
            }
            else if (iDebug)
            {
               printf("       poolNo %d: free space  %8d GByte\n",
                  iPoolcur, iPoolStageFree);
               printf("          free HW space  %8d GByte, max available %8d GByte\n",
                  iHardwareFree, iHardwareMax);
            }

            if ( (iAction == STAGE) || (iAction == QUERY_WORKSPACE) )
            {
               /* in GB, for later use */
               iPoolStageMaxWS = ntohl(pPoolInfoData->iMaxWorkSize)/1000;
               iPoolStageCheck = ntohl(pPoolInfoData->iCheckSize)/1000;
            }
         } /* (iPoolcur == 2) */

         if ( (iPoolcur < iPoolmax) &&
              ((iPoolcur == 3) || (iPoolcur == 4)) )
         {
            if (iDebug) printf(
               "       poolNo %d: (%s) ignored\n",
               iPoolcur, pPoolInfoData->cPoolName);
         }

         if (iPoolcur == iPoolmax)                     /* GlobalPool */
         {
            ii1 = ntohl(pPoolInfoData->iMaxSize)/1000;
            iPoolStageMax += ii1;
            ii2 = ntohl(pPoolInfoData->iFreeSize)/1000;
            if (ii2 < 0)
               ii2 = 0;
            iPoolStageFree += ii2;

            if (iDebug) printf(
               "       poolNo %d: free space  %8d GByte\n",
               iPoolcur, ii2);

            /* corrected numbers, if deviating from 1st pool */
            ii1 = ntohl(pPoolInfoData->iMaxSizeHW)/1000;
            iHardwareMax += ii1;
            ii2 = ntohl(pPoolInfoData->iFreeSizeHW)/1000;
            iHardwareFree += ii2;

            if (iDebug) printf(
               "          free HW space  %8d GByte, max available %8d GByte\n",
               ii2, ii1);

            iPoolStageFiles += ntohl(pPoolInfoData->iFiles);
            if ( (iPoolStageFree == iPoolStageMax) &&
                 (iPoolStageFiles > 0) )
               iPoolStageFree--;        /* eliminate rounding errors */

            ii = iPoolStageFree + iPoolRetrFree;
            if (iHardwareFree != ii)
               iHardwareFree = ii;      /* eliminate rounding errors */

            if ( (iPrintPoolInfo == 2) || (iPrintPoolInfo == 3) ||
                 (iDebug) )
            {
               printf("       free space  %8d GByte\n", iPoolStageFree);
               printf("       min file availability of %d days guaranteed\n",
                  iPoolStageAvail);    /* known from prev. iteration */

               /* at end of last pool iteration, NOT after loop */
               if (iDebug) printf(
                  "    HW: free space  %8d GByte, max available %8d GByte\n",
                  iHardwareFree, iHardwareMax);
            }
         } /* (iPoolcur == iPoolmax) */

         pPoolInfoData++;

      } /* loop pools */

      pPoolInfoData = pPoolInfoData0;              /* 1st pool again */

   } /* (iPoolInfo) */

   /********************* get work space info ************************/

   if (iWorkSpaceInfo)
   {
      pWorkSpace = (srawWorkSpace *) pcc;
      *ppWorkSpace = pWorkSpace;                       /* for caller */

      iRC = rawRecvHead(iSocket, pcc);
      if (iRC <= 0)
      {
         printf("-E- receiving work space buffer header\n");
         return -1;
      }
      if (iDebug)
         printf("-D- header work space buffer received (%d bytes)\n",
                iRC);

      iIdent = ntohl(pWorkSpace->iIdent);
      iStatus = ntohl(pWorkSpace->iWorkId);
      iAttrLen = ntohl(pWorkSpace->iStatusLen);

      /* expect IDENT_WORKSPACE */
      if (iIdent != IDENT_WORKSPACE)
      {
         if (iIdent == IDENT_STATUS)
         {
            if ( (iStatus == STA_ERROR) || (iStatus == STA_ERROR_EOF) )
            {
               if (iDebug) printf("\n");
               printf("-E- received error status from server");
               if (iDebug)
                  printf(" instead of work space info:\n");
               else printf(":\n");

               if (iAttrLen > 0)
               {
                  pcc = (char *) piBuffer;
                  iRC = rawRecvError(iSocket, iAttrLen, pcc);
                  if (iRC < 0) printf(
                     "-E- receiving error msg from server, rc = %d\n",
                     iRC);
                  else printf("    %s\n", pcc);
               }
               else printf("    no error message available\n");

            } /* (iStatus == STA_ERROR) || (iStatus == STA_ERROR_EOF) */
            else
            { 
               printf(
                  "-E- unexpected status (%d) received from server\n",
                  iStatus);
            }
         } /* iIdent == IDENT_WORKSPACE */
         else
         { 
            printf(
               "-E- unexpected header (%d) received from server\n",
               iIdent);
         }

         return -1;

      } /* (iIdent != IDENT_WORKSPACE) ) */

      /* receive work space info */
      iBuf = iAttrLen;
      pcc += HEAD_LEN;
      while(iBuf > 0)
      {
         if ( (iRC = recv( iSocket, pcc, (unsigned) iBuf, 0 )) <= 0)
         {
            if (iRC < 0) printf(
               "-E- %s: receiving work space info (%d byte)\n",
               cModule, iAttrLen);
            else
            {
               ii = iAttrLen - iBuf;
               printf("-E- %s: connection to sender broken, %d byte of work space info (%d byte) received\n",
                  cModule, ii, iAttrLen);
            }
            if (errno)
               printf("    %s\n", strerror(errno));
            perror("-E- receiving work space info");

            return -1;
         } 

         iBuf -= iRC;
         pcc += iRC;

      } /* while(iBuf > 0) */

      if (iDebug)
         printf("    remainder work space buffer received (%d byte)\n",
                iAttrLen);

      iWorkSizeAll = ntohl(pWorkSpace->iWorkSizeAll);
      iWorkFilesAll = ntohl(pWorkSpace->iWorkFilesAll);
      iWorkStatus = ntohl(pWorkSpace->iStatus);
      iWorkSizeSta = ntohl(pWorkSpace->iWorkSizeSta);
      iWorkFilesSta = ntohl(pWorkSpace->iWorkFilesSta);
      iWorkSizeStaStage = ntohl(pWorkSpace->iWorkSizeStaTemp);
      iWorkFilesStaStage = ntohl(pWorkSpace->iWorkFilesStaTemp);
      iWorkSizeEst = ntohl(pWorkSpace->iWorkSizeEst);
      iWorkFilesEst = ntohl(pWorkSpace->iWorkFilesEst);

      iWorkSizeStaRetr = iWorkSizeSta - iWorkSizeStaStage;
      iWorkFilesStaRetr = iWorkFilesSta - iWorkFilesStaStage;

      /* provide infos if stage of large workspace or if ws_query */
      if (iPoolInfo || (iAction == QUERY_WORKSPACE) || (iDebug) )
      {
         printf("    %d matching files, overall size %d MByte\n",
            iWorkFilesAll, iWorkSizeAll);
         if (iWorkFilesEst) printf(
            "    size estimated for %d files: %d MByte\n",
            iWorkFilesEst, iWorkSizeEst);

         if (iWorkSizeSta == iWorkSizeAll)
         {
            if (iDebug)
            {
               printf(
                  "\n    all files already available on central disk\n");
               if ( (iPoolId == 2) &&
                    iWorkSizeStaRetr &&
                    (iAction == QUERY_WORKSPACE) )
                  printf(
                     "    to get a guaranteed availability of %d days on disk pool use 'gstore stage'\n",
                     iPoolStageAvail);
            }
         }
         else if (iWorkSizeSta)
         {
            if (iDebug)
            {
               printf(
                  "\n    %d files already available on central disk (%d MByte)\n",
                  iWorkFilesSta, iWorkSizeSta);
               if (iWorkFilesStaStage) printf(
                  "    %d files already in %s (%d MByte)\n",
                  iWorkFilesStaStage, cPoolNameStage, iWorkSizeStaStage);
            }
            printf("    %d files still to be staged (%d MByte)\n",
               iWorkFilesAll-iWorkFilesSta,
               iWorkSizeAll - iWorkSizeSta);
         }
         else
            printf("    all files to be staged\n");

         /* handle problems or limitations indicated by iWorkStatus:
            = 0: needed size ws available
            = 1: overall size ws > free size StagePool
            = 2: overall size ws > allowed size of ws
            = 3: needed size ws > size StagePool
            = 9: needed size ws > clean limit, but available: no sleep 
            =10: needed size ws partially available,
                 sleep 10 min before start staging
            =20: needed size ws partially available,
                 sleep 20 min before start staging
         */
         if ( (iWorkStatus < 9) && (iWorkStatus != 0) )
         {
            if ( (iAction == QUERY_WORKSPACE) ||
                 (iWorkStatus == 2) )
            {
               strcpy(cMsgPref, "-W-");
               printf("%s requested workspace cannot be staged completely\n",
                  cMsgPref);
            }
            else
            {
               strcpy(cMsgPref, "-E-");
               printf("%s requested workspace cannot be staged:\n",
                  cMsgPref);
            }

            if (iWorkStatus < 0)
               printf("%s staging disk pool currently unavailable\n",
                      cMsgPref);
            else if (iWorkStatus == 1)
            {
               printf("    currently free in %s:   %d MByte\n",
                      cPoolNameStage, iPoolStageFree);
               printf(
                  "    still needed:                  %d MByte (%d files)\n",
                  iWorkSizeAll-iWorkSizeSta,
                  iWorkFilesAll-iWorkFilesSta);
               if (iAction == QUERY_WORKSPACE)
                  printf( "-I- Check later again!\n");
               else printf(
                  "-I- Query work space status before retrying later!\n");
            }
            else if ( (iWorkStatus == 2) || (iWorkStatus == 3) )
            {
               if (iWorkStatus == 2) printf(
                  "%s max work space allowed in '%s' is currently limited to %d MByte\n",
                  cMsgPref, cPoolNameStage, iPoolStageMaxWS);
               else printf(
                  "%s overall size of %s is limited to %d MByte\n",
                  cMsgPref, cPoolNameStage, iPoolStageMax);

               if (iWorkStatus == 3) printf(
                  "-I- Please reduce your work space requirements\n");
            }
            else if (iWorkStatus < 9)
               printf(
                  "-E- unexpected workspace status received from server (%d)\n",
                  iWorkStatus);

            iWorkStatus = 0;

         } /* (iWorkStatus < 9) && (iWorkStatus != 0) */

         if ( (iDebug) && (iWorkSizeSta != iWorkSizeAll) )
         {
            if (iAction == QUERY_WORKSPACE)
            {
               iWorkSizeNew = iWorkSizeAll - iWorkSizeSta;
               if (iRandomExcess)
               {
                  if (iWorkSizeNew > iHardwareFree)
                  {
                     printf("    currently unused in %s: %d MByte\n",
                        cPoolNameStage, iPoolStageFree);
                     printf(
                        "    currently free in %s:   %d MByte, remainder temporarily used by other pools\n",
                        cPoolNameStage, iHardwareFree);
                  }
                  else
                     printf("    currently free in %s:   %d MByte\n",
                         cPoolNameStage, iHardwareFree);
               }
               else printf(
                  "    currently free in %s:   %d MByte\n",
                  cPoolNameStage, iPoolStageFree);

            } /* (iAction == QUERY_WORKSPACE) */
         } /* (iWorkSizeSta != iWorkSizeAll) */

         /* server set clean request */
         if (iWorkStatus >= 9)
         {
            if (iDebug)
            {
               /* if (ntohl(pPoolInfoData->iFreeSize) >= 0)
                  does not work in Linux:
                  negative values are not recognized!            */
               ii = ntohl(pPoolInfoData0->iFreeSize);

               printf("-D- currently free (HW): %d MByte\n",
                      iHardwareFree);
               printf(
                  "    currently %d MByte unused in %s are allocated by other pools\n",
                  ii*(-1), cPoolNameStage);
            }

            if (iAction == QUERY_WORKSPACE)
               strcpy(cMisc, "must be initiated (gstore stage)");
            else
               strcpy(cMisc, "is initiated");
            printf(
               "-I- a clean job %s to provide the requested space\n",
               cMisc);

            if (iWorkStatus > 9)
            {
               /* iSleepClean = iWorkStatus*60;     */     /* in sec */
               iSleepClean = iWorkStatus;
           /*  if (iDebug)
                  printf("-D- sleep for %ds\n", iSleepClean);
               sleep(iSleepClean);
            */
            }

         } /* (iWorkStatus >= 9) */

      } /* (iPoolInfo) || (iAction == QUERY_WORKSPACE) */
   } /* (iWorkSpaceInfo) */

   if (iDebug)
      printf("-D- end %s\n\n", cModule);

   return 0;

} /* rawGetWSInfo */

/********************************************************************
 * rawGetFullFile: get full file name from generic input and ll name
 *    returns full file name
 *
 * created 18. 3.1996, Horst Goeringer
 ********************************************************************
 */

char *rawGetFullFile(char *pcFile, char *pcNamell)
{

   char cModule[32] = "rawGetFullFile";
   int iDebug = 0;

   char cname1[MAX_FULL_FILE] = "";    /* name structure assumed:    */ 
   int ilen;
   int iRC;

   char *pdelim=NULL, *pdelim2=NULL;
   char *pc, *pcll;

   if (iDebug)
      fprintf(fLogFile, "\n-D- begin %s\n", cModule);

   strcpy(cPath, pcFile);
   pdelim = strrchr(cPath, *pcFileDelim);
   if (pdelim != NULL)                           /* path specified */
   {
      strncpy(pdelim, "\0", 1);
      strcpy(cname1, ++pdelim);
   }
   else 
   {
#ifdef VMS
      pdelim2 = strrchr(cPath, *pcFileDelim2);
      if (pdelim2 != NULL)      /* logical device specified */
      {
         strncpy(pdelim2, "\0", 1);
         strcpy(cname1, ++pdelim2);
      }
      else
      {
#endif
         strncpy(cPath, "\0", 1);
         strcpy(cname1, pcFile);
#ifdef VMS
      }
#endif
   } /* (pdelim == NULL) */

   ilen = strlen(cname1);
   if (iDebug)
   {
#ifdef VMS
      if (pdelim != NULL)
         fprintf(fLogFile, "-D- %s: path %s], name1: %s (len %d)\n",
                 cModule, cPath, cname1, ilen);
            /* !!! bracket in fprintf needed, as not yet in cPath */
      else
      {
         if (pdelim2 != NULL)
            fprintf(fLogFile, "-D- %s: device %s, name1: %s (len %d)\n",
                    cModule, cPath, cname1, ilen);
         else
            fprintf(fLogFile, "-D- %s: no prefix, name1: %s (len %d)\n",
                    cModule, cname1, ilen);
      }
#else
      fprintf(fLogFile,
         "    path: %s, name1: %s (len %d)\n", cPath, cname1, ilen);
#endif
   }
  
   pc = &cname1[0];
   pcll = pcNamell;
   iRC = strncmp(pcll, pcObjDelim, 1);
   if (iRC == 0) pcll++;                  /* skip object delimiter */
	else
	{
      iRC = strncmp(pcll, pcObjDelimAlt, 1);
      if (iRC == 0) pcll++;               /* skip object delimiter */      
	}
   
   if (strlen(cPath) > 0)
   {
#ifdef VMS
      if (pdelim2 != NULL)
         strcat(cPath, pcFileDelim2);
      else
#endif
         strcat(cPath, pcFileDelim);
   }
   strcat(cPath, pcll);
   if (iDebug) fprintf(fLogFile,
      "-D- end %s: full file name found: %s\n\n", cModule, cPath);

   return( (char *) cPath);

} /* rawGetFullFile */

/*********************************************************************
 * rawQueryPrint: print query results for one object 
 * created 19. 2.1996, Horst Goeringer 
 *********************************************************************
 */

void rawQueryPrint(
        srawObjAttr *pQAttr,
        int ipMode) /* =  0: default print mode,
                       =  1: debug print mode,
                       = 10: default print mode, stage status unknown
                       = 11: debug print mode, stage status unknown
                     */
{
   char cModule[32]="rawQueryPrint";
   char ctype[8] = "";
   char cMClass[12] = "";
   char cPath[MAX_OBJ_HL];
   char cStatus[16] = "";

   int ii;
   int iStage = 0;                         /* StagePool (read cache) */
   int iCache = 0;                      /* ArchivePool (write cache) */ 
   int iATLServer = 0;     /* =1: aixtsm1(AIX), =2: gsitsma(Windows) */
   int iFileType = -1;
   int iMediaClass = 0;

   unsigned long *plFileSizeC;                    /* 8 byte filesize */
   unsigned long lFileSize;                       /* 8 byte filesize */
   int iVersionObjAttr = 0;
          /* version no. of srawObjAttr:
             =3: 288 byte, 2 restore fields
             =4: 304 byte, 5 restore fields, ATL server no.
             =5: 384 byte, maior enhancement */

   iVersionObjAttr = ntohl(pQAttr->iVersion);
   if ( (ipMode == 1) || (ipMode == 11) ) fprintf(fLogFile,
      "\n-D- begin %s: objAttr V%d\n", cModule, iVersionObjAttr);

   if ( (iVersionObjAttr != VERSION_SRAWOBJATTR) &&
        (iVersionObjAttr != VERSION_SRAWOBJATTR-1) )
   {
      fprintf(fLogFile,
         "-E- %s: invalid cacheDB entry version %d\n",
         cModule, iVersionObjAttr);

      return;
   }

   iFileType = ntohl(pQAttr->iFileType);
   iMediaClass = ntohl(pQAttr->iMediaClass);
   iATLServer = ntohl(pQAttr->iATLServer);

   switch(iMediaClass)
   {
      case MEDIA_FIXED:
         strcpy(cMClass, "DISK ");
         break;
      case MEDIA_LIBRARY:
         strcpy(cMClass, "TAPE ");
         break;
      case MEDIA_NETWORK:
         strcpy(cMClass, "NETWORK");
         break;
      case MEDIA_SHELF:
         strcpy(cMClass, "SHELF");
         break;
      case MEDIA_OFFSITE:
         strcpy(cMClass, "OFFSITE");
         break;
      case MEDIA_UNAVAILABLE:
         strcpy(cMClass, "UNAVAILABLE");
         break;
      case GSI_MEDIA_STAGE:
      case GSI_MEDIA_LOCKED:
         strcpy(cMClass, "STAGE");
         strcpy(cStatus, "staged");
         iStage = 1;
         break;
      case GSI_MEDIA_INCOMPLETE:
         strcpy(cMClass, "STAGE*");
         strcpy(cStatus, "still staging");
         iStage = 1;
         break;
      case GSI_STAGE_INVALID:
         strcpy(cMClass, "STAGE*");
         strcpy(cStatus, "staging failed");
         iStage = 1;
         break;
      case GSI_MEDIA_CACHE:                       /* GSI write cache */
      case GSI_CACHE_LOCKED:
         strcpy(cMClass, "CACHE");
         strcpy(cStatus, "cached");
         iCache = 1;
         break;
      case GSI_CACHE_INCOMPLETE:
         strcpy(cMClass, "CACHE*");
         strcpy(cStatus, "still caching");
         iCache = 1;
         break;
      case GSI_CACHE_COPY:
         strcpy(cMClass, "CACHE");
         strcpy(cStatus, "still copying");
         iCache = 1;
         break;
      default:
         fprintf(fLogFile, "-E- Invalid media class %d found\n",
            iMediaClass);
         break;
   }

   if (iATLServer)
   {
      if (iATLServer < 0)
         iATLServer = -iATLServer;

      ii = MAX_ATLSERVER;
      if (ii > 1)
         sprintf(cMClass, "%s%d", cMClass, iATLServer);
      else
         sprintf(cMClass, "%s", cMClass);
   }

   if ( (ipMode == 10) || (ipMode == 11) )
      strcat(cMClass, "??");             /* stage status not checked */

   if (strlen(pQAttr->cDateCreate) == 16)       /* no trailing blank */
      strcat(pQAttr->cDateCreate, " ");
   strcpy(cPath, (char *) rawGetPathName(pQAttr->cNamehl));
   fprintf(fLogFile, "%s%s%s  %s  %s  %s", 
      pQAttr->cNamefs, cPath, pQAttr->cNamell, 
      pQAttr->cOwner, pQAttr->cDateCreate, cMClass);

   pQAttr->iFileSize = ntohl(pQAttr->iFileSize);
   pQAttr->iFileSize2 = ntohl(pQAttr->iFileSize2);
   plFileSizeC = (unsigned long *) &(pQAttr->iFileSize);
   lFileSize = *plFileSizeC;

   if (lFileSize)
   {
      if ( (pQAttr->iFileSize2 == 0) ||
           (sizeof(long) == 8) )
         fprintf(fLogFile, "  %12lu", lFileSize); 
      else
         fprintf(fLogFile, "    %s   ", cTooBig);
   }

   /* reconvert to net format */
   pQAttr->iFileSize = htonl(pQAttr->iFileSize);
   pQAttr->iFileSize2 = htonl(pQAttr->iFileSize2);

   if (iStage)
      fprintf(fLogFile, "  %s  %s", pQAttr->cNode, pQAttr->cStageUser);
   if (iCache)
      fprintf(fLogFile, "  %s", pQAttr->cNode);
   fprintf(fLogFile, "\n");

   if ( (ipMode == 1) || (ipMode == 11) )
   {
      if (ntohl(pQAttr->iFS))
         fprintf(fLogFile, "    %s on data mover %s, FS %d (poolId %d)\n",
            cStatus, pQAttr->cNode, ntohl(pQAttr->iFS),
            ntohl(pQAttr->iPoolId));
      fprintf(fLogFile,
         "    obj-Id: %u-%u, restore order: %u-%u-%u-%u-%u\n",
         ntohl(pQAttr->iObjHigh),
         ntohl(pQAttr->iObjLow),
         ntohl(pQAttr->iRestoHigh),
         ntohl(pQAttr->iRestoHighHigh),
         ntohl(pQAttr->iRestoHighLow),
         ntohl(pQAttr->iRestoLowHigh),
         ntohl(pQAttr->iRestoLow) );
      fprintf(fLogFile,
         "    owner: %s, OS: %.8s, mgmt-class: %s, file set %d\n",
         pQAttr->cOwner,
         pQAttr->cOS, pQAttr->cMgmtClass, ntohl(pQAttr->iFileSet));
   }

   if ( (ipMode == 1) || (ipMode == 11) )
      fprintf(fLogFile, "-D- end %s\n\n", cModule);

} /* rawQueryPrint */

/*********************************************************************/
/* rawQueryString: print query results for one object to string */      
/* created 19. 2.96, Horst Goeringer */
/*********************************************************************/

int rawQueryString(
        srawObjAttr *pQAttr,
        int ipMode, /* =  0: default print mode,
                       =  1: debug print mode,
                       = 10: default print mode, stage status unknown
                       = 11: debug print mode, stage status unknown  */
        int iOut,                            /* length output string */
        char *pcOut) 
{
   char cModule[32] = "rawQueryString";
   int iDebug = 0;

   int iMsg = 0;
   char ctype[8] = "";
   char cMClass[12] = "";
   char cPath[MAX_OBJ_HL];
   char cMsg[STATUS_LEN_LONG] = "";
   char cMsg1[STATUS_LEN] = "";

   int ii;
   int iStage = 0;                         /* StagePool (read cache) */
   int iCache = 0;                      /* ArchivePool (write cache) */
   int iATLServer;         /* =1: aixtsm1(AIX), =2: gsitsma(Windows) */
   int iFileType = -1;
   int iMediaClass = 0;

   unsigned long *plFileSizeC;                    /* 8 byte filesize */
   unsigned long lFileSize;                       /* 8 byte filesize */
   int iVersionObjAttr = 0;
          /* version no. of srawObjAttr:
             =3: 288 byte, 2 restore fields
             =4: 304 byte, 5 restore fields, ATL server no.
             =5: 384 byte, maior enhancement */

   iVersionObjAttr = ntohl(pQAttr->iVersion);
   if (iDebug) fprintf(fLogFile,
      "\n-D- begin %s: version ObjAttr %d\n", cModule, iVersionObjAttr);

   if ( (iVersionObjAttr != VERSION_SRAWOBJATTR) &&
        (iVersionObjAttr != VERSION_SRAWOBJATTR-1) )
   {
      fprintf(fLogFile,
         "-E- %s: invalid cacheDB entry version %d\n",
         cModule, iVersionObjAttr);

      return 0;
   }

   iFileType = ntohl(pQAttr->iFileType);
   iMediaClass = ntohl(pQAttr->iMediaClass);
   iATLServer = ntohl(pQAttr->iATLServer);

   switch(iMediaClass)
   {
      case MEDIA_FIXED:
         strcpy(cMClass, "DISK ");
         break;
      case MEDIA_LIBRARY:
         strcpy(cMClass, "TAPE ");
         break;
      case MEDIA_NETWORK:
         strcpy(cMClass, "NETWORK");
         break;
      case MEDIA_SHELF:
         strcpy(cMClass, "SHELF");
         break;
      case MEDIA_OFFSITE:
         strcpy(cMClass, "OFFSITE");
         break;
      case MEDIA_UNAVAILABLE:
         strcpy(cMClass, "UNAVAILABLE");
         break;
      case GSI_MEDIA_STAGE:
      case GSI_MEDIA_LOCKED:
         strcpy(cMClass, "STAGE");
         iStage = 1;
         break;
      case GSI_MEDIA_INCOMPLETE:
      case GSI_STAGE_INVALID:
         strcpy(cMClass, "STAGE*");
         break;
      case GSI_MEDIA_CACHE:                       /* GSI write cache */
      case GSI_CACHE_LOCKED:
         strcpy(cMClass, "CACHE");
         iCache = 1;
         break;
      case GSI_CACHE_INCOMPLETE:
      case GSI_CACHE_COPY:
         strcpy(cMClass, "CACHE*");
         iCache = 1;
         break;
      default:
         fprintf(fLogFile, "-E- Invalid media class %d found\n",
            iMediaClass);
         break;
   }

   if (iATLServer)
   {
      if (iATLServer < 0)
         iATLServer = -iATLServer;

      ii = MAX_ATLSERVER;
      if (ii > 1)
         sprintf(cMClass, "%s%d", cMClass, iATLServer);
      else
         sprintf(cMClass, "%s", cMClass);
   }
   if ( (ipMode == 10) || (ipMode == 11) )
      strcat(cMClass, "??");             /* stage status not checked */

   if (strlen(pQAttr->cDateCreate) == 16)       /* no trailing blank */
      strcat(pQAttr->cDateCreate, " ");
   strcpy(cPath, (char *) rawGetPathName(pQAttr->cNamehl));

   sprintf(cMsg, "%s%s%s  %s  %s  %s", 
      pQAttr->cNamefs, cPath, pQAttr->cNamell, 
      pQAttr->cOwner, pQAttr->cDateCreate, cMClass);

   pQAttr->iFileSize = ntohl(pQAttr->iFileSize);
   pQAttr->iFileSize2 = ntohl(pQAttr->iFileSize2);
   plFileSizeC = (unsigned long *) &(pQAttr->iFileSize);
   lFileSize = *plFileSizeC;

   /* reconvert to net format */
   pQAttr->iFileSize = htonl(pQAttr->iFileSize);
   pQAttr->iFileSize2 = htonl(pQAttr->iFileSize2);

   if (lFileSize) 
   {
      if ( (pQAttr->iFileSize2 == 0) ||
           (sizeof(long) == 8) )
         sprintf(cMsg1, "  %12lu", lFileSize);
      else
         sprintf(cMsg1, "    %s   ", cTooBig); 
      strcat(cMsg, cMsg1);
   }
   if (iStage)
   {
      sprintf(cMsg1, "  %s  %s", pQAttr->cNode, pQAttr->cStageUser);
      strcat(cMsg, cMsg1);
   }
   if (iCache)
   {
      sprintf(cMsg1, "  %s", pQAttr->cNode);
      strcat(cMsg, cMsg1);
   }
   strcat(cMsg, "\n");

   if ( (ipMode == 1) || (ipMode == 11) )
   {
      if (ntohl(pQAttr->iFS))
      {
         sprintf(cMsg1,
            "    staged on data mover %s, FS %d (poolId %d)\n",
            pQAttr->cNode, ntohl(pQAttr->iFS),
            ntohl(pQAttr->iPoolId));
         strcat(cMsg, cMsg1);
      }

      sprintf(cMsg1,
         "    obj-Id: %u-%u, restore order: %u-%u-%u-%u-%u\n",
         ntohl(pQAttr->iObjHigh),
         ntohl(pQAttr->iObjLow),
         ntohl(pQAttr->iRestoHigh),
         ntohl(pQAttr->iRestoHighHigh),
         ntohl(pQAttr->iRestoHighLow),
         ntohl(pQAttr->iRestoLowHigh),
         ntohl(pQAttr->iRestoLow) );
      strcat(cMsg, cMsg1);

      sprintf(cMsg1,
         "    version %d, owner: %s, OS: %s, mgmt-class: %s, file set %d\n",
         ntohl(pQAttr->iVersion), pQAttr->cOwner,
         pQAttr->cOS, pQAttr->cMgmtClass, ntohl(pQAttr->iFileSet));
      strcat(cMsg, cMsg1);
   }

   iMsg = strlen(cMsg);

   if (iOut < iMsg)
   {
      fprintf(fLogFile,
         "-W- %s: output string provided too short (%d byte), %d byte needed\n",
        cModule, iOut, iMsg);
      strncpy(pcOut, cMsg, (unsigned) iOut-1);
      strcat(pcOut, "\0");
      
      fprintf(fLogFile,
         "-W- %s: query information incomplete:\n%s\n", cModule, pcOut);

      iMsg = iOut;
   }
   else
   {
      if (iDebug) fprintf(fLogFile,
         "    %s: length query info %d byte, length of string provided %d byte, msg:\n",
         cModule, iMsg, iOut);
      strcpy(pcOut, cMsg);
   }

   if (iDebug)
      fprintf(fLogFile, "-D- end %s\n\n", cModule);

   return iMsg;

} /* rawQueryString */

/**********************************************************************
 * rawScanObjbuf:
 *   check if obj already available in obj buffer chain
 * created 10. 7.2003 by Horst Goeringer
 **********************************************************************
 */

int rawScanObjbuf(char *pcPath, char *pcFile,
                  int iObjnoAll, int *piObjBuf0)
{
   char cModule[32] = "rawScanObjbuf";
   int iDebug = 0;

   int ii;
   int iFound = 0;   /* =1: specified file already available in list */
   int iObjnoBuf;                /* no. of objects in current buffer */
   int iObjnoCur = 0;         /* current object no. (in all buffers) */
   int iBufno = 0;                          /* no. of object buffers */
   int *piObjBuf;                    /* ptr to current object buffer */
   int **ppiNextBuf;

   srawRetrList *psRetrList;     /* points to object in buffer chain */

   if (iDebug)
      printf("\n-D- begin %s: check %d objs for %s%s\n",
         cModule, iObjnoAll, pcPath, pcFile);

   piObjBuf = piObjBuf0;

   /* loop over object buffers */
   for (;;)
   {
      iObjnoBuf = *piObjBuf;
      psRetrList = (srawRetrList *) &(piObjBuf[1]);
      iBufno++;
      if (iDebug == 2)
      {
         printf("DDD buffer %d: piObjBuf %p, value %d, psRetrList %p\n",
            iBufno, piObjBuf, *piObjBuf, psRetrList);
         fflush(stdout);
      }

      /* loop over objects in current buffer */
      for (ii=1; ii<=iObjnoBuf; ii++)
      {
         iObjnoCur++;
         if (iDebug == 2)
         {
            printf("    %d: %s%s, addr %p\n",
               iObjnoCur, psRetrList->cNamehl, psRetrList->cNamell,
               psRetrList);
	    fflush(stdout);
         }

         if ( (strcmp(pcFile, psRetrList->cNamell) == 0) &&
              (strcmp(pcPath, psRetrList->cNamehl) == 0) )
         {
            if (iDebug) printf(
               "    file %s%s already available in list\n",
               pcPath, pcFile);
            iFound = 1;
            goto gEndScan;
         }

         psRetrList++;

      } /* loop over objects in current buffer */

      if (iObjnoCur == iObjnoAll)
      {
         if (iDebug) printf(
            "    all %d files scanned\n", iObjnoAll);
         break;
      }

      if (psRetrList)
      {
         ppiNextBuf = (int **) psRetrList;
         piObjBuf = *ppiNextBuf;
         psRetrList = (srawRetrList *) &(piObjBuf[1]);
         if (iDebug == 2)
	 {
	    printf("DDD next piObjBuf %p, value %d, psRetrList %p\n",
               piObjBuf, *piObjBuf, psRetrList);
	    fflush(stdout);
	 }
      }
      else                                            /* last buffer */
         break;

   } /* loop over buffers */

gEndScan:
   if (iDebug)
      printf("-D- end %s: %d buffers scanned\n\n", cModule, iBufno);

   if (iFound)
      return 1;
   else
      return 0;

} /* end rawScanObjbuf */

/**********************************************************************
 * rawSortValues:
 *   sort indexed list of numbers
 * created 15. 6.2000 by Horst Goeringer
 **********************************************************************
 */

int rawSortValues(
      int *iaValue,
      int iAll,
      int iFirst,
      int iLast,
      int *iaIndex,
      int *iaIndNew)
{
   char cModule[32] = "rawSortValues";
   int iDebug = 0;

   int ii, ii1, iif, jj;
   int iLoop;
   int iInd;
   int iFound = 0;
   int iaValNew[iAll+1];

   iLoop = iLast - iFirst + 1;

   if (iDebug)
      printf("\n-D- begin %s\n", cModule);

   if (iDebug)
   {
      printf("    numbers on input:\n                    value  index\n");
      for (ii=iFirst; ii<=iLast; ii++)
      {
         printf("       %6d: %10d %6d\n", ii, iaValue[ii], iaIndex[ii]);
      }
   }

   iFound = iFirst - 1;                /* 1st possible index: iFirst */
   for (jj=iFirst; jj<=iLast; jj++)
   {
      if (iFound >= iFirst)
      {
         for (iif=iFirst; iif<=iFound; iif++)
         {
            if ( (iaValue[jj] < iaValNew[iif]) && (iaValNew[iif]) )
            {
               for (ii1=iFound; ii1>=iif; ii1--)
               {
                  iaValNew[ii1+1] = iaValNew[ii1];
                  iaIndNew[ii1+1] = iaIndNew[ii1];
               }

               iaValNew[iif] = iaValue[jj];
               iaIndNew[iif] = iaIndex[jj];
               iFound++;
               if (iDebug == 2) fprintf(fLogFile,
                  "    at pos %d inserted: val %d, ind %d\n",
                  iif, iaValNew[iif], iaIndNew[iif]);

               break;
            }
            else
            {
               if (iif == iFound)
               {
                  iFound++;
                  iaValNew[iFound] = iaValue[jj];
                  iaIndNew[iFound] = iaIndex[jj];
                  if (iDebug == 2) fprintf(fLogFile,
                     "    at pos %d appended: val %d, ind %d\n",
                     iFound, iaValNew[iFound], iaIndNew[iFound]);

                  break;                                 /* loop iif */
               }

               continue;                                 /* loop iif */
            }
         } /* loop iif */
      } /* (iFound) */
      else
      {
         iaValNew[iFirst] = iaValue[jj];
         iaIndNew[iFirst] = iaIndex[jj];
         iFound = iFirst;

         if (iDebug == 2) fprintf(fLogFile,
            "    start value: val %d, ind %d\n",
            iaValNew[iFound], iaIndNew[iFound]);
      }
   } /* loop objects */

   memcpy(&iaValue[iFirst], &iaValNew[iFirst], (unsigned) iLoop*iint);

   if (iDebug)
   {
      printf("    numbers on output:\n                    value  index\n");
      for (ii=iFirst; ii<=iLast; ii++)
      {
         printf("       %6d: %10d %6d\n", ii, iaValue[ii], iaIndNew[ii]);
      }
   }

   if (iDebug)
      printf("-D- end %s\n\n", cModule);

   return 0;

} /* end rawSortValues*/

#ifdef _AIX
/*********************************************************************
 * ielpst: Elapsed (wall-clock) time measurement
 *
 *    Usage examples:
 *
 *  ielpst( 1, iBuf[0..1] );
 *  ... code ...
 *  ElapsedTime(in microsecs) = ielpst( 1, iBuf[0..1] );
 *
 *  ielpst( 1000, iBuf[0..1] );
 *  ... code ...
 *  ElapsedTime(in millisecs) = ielpst( 1000, iBuf[0..1] );
 *
 *********************************************************************
 * created 17.11.1992, Michael Kraemer
 *********************************************************************
 */

unsigned long ielpst( unsigned long iScale,
                      unsigned long *iBuf )
{
   char cModule[32] = "ielpst";
   int iDebug = 0;

   struct timestruc_t stv;
   unsigned long iTemp[2];

   gettimer( TIMEOFDAY, &stv );

   if ( iScale <= 0 ) iScale = 1;
   iTemp[1] = stv.tv_sec  - iBuf[1];
   if ( ( iTemp[0] = stv.tv_nsec - iBuf[0] ) & 0x80000000 ) {
      iTemp[0] += 1000000000L; iTemp[1] -= 1;
   }
   iBuf[1] = stv.tv_sec; iBuf[0] = stv.tv_nsec;
   return( iTemp[1] * ( 1000000L / iScale ) + ( iTemp[0] / 1000 ) / iScale );

} /* ielpst */

/* FORTRAN entry */
unsigned long ielpst_( unsigned long *iScale,
                       unsigned long *iBuf ) 
{
   return( ielpst( *iScale, iBuf ) );
} /* ielpst_ */
#endif
