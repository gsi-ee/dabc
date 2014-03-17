/*********************************************************************
 * Copyright:
 *   GSI, Gesellschaft fuer Schwerionenforschung mbH
 *   Planckstr. 1
 *   D-64291 Darmstadt
 *   Germany
 * created 15.2.1996, Horst Goeringer
 *********************************************************************
 * rawentn.h
 *    entry definitions for client part of adsmcli client/server package
 *********************************************************************
 * 13. 4.1999, H.G.: mod. declaration of rawGetFileAttr
 * 31. 5.2001, H.G.: replace urmtopn by rconnect
 *  9. 1.2002, H.G.: created from rawent.h
 *                   rawGetLLName: pass object and delimiter as argument
 * 13. 2.2003, H.G.: mod arg list rawDelList
 * 14. 2.2003, H.G.: mod arg list rawCheckFileList
 * 25.11.2004, H.G.: rawQueryFile: add new arg no. 2
 *  7.12.2006, H.G.: mod arg list rawDelList
 *  5.11.2008, H.G.: mod arg lists rawRecvError, rawRecvHead,
 *                   rawRecvStatus for (char ** -> char *)
 * 11.11.2008, H.G.: rawGetLLName: 'const char' for delimiter
 * 18.12.2008, H.G.: rawCheckObjList: additional argument (int)
 * 15. 6.2009, H.G.: rawSortValues: new entry
 * 22. 6.2008, H.G.: replace long->int if 64bit client (ifdef SYSTEM64)
 * 30.11.2009, H.G.: rawCheckFileList: add 'char *' (archive name)
 * 11. 2.2010, H.G.: rename rawTestFilePath -> rawCheckClientFile
 * 22. 2.2010, H.G.: add rawGetPathName
 * 26. 2.2010, H.G.: rawQueryString: add parameter (len output string)
 * 23. 8.2010, H.G.: remove SYSTEM64 flag: allow "long" in 64 bit OS
 * 11. 2.2011, H.G.: rename rawCheckObjlist -> rawCheckObjList,
 *                   rawCheckObjList: additional int argument 
 *                   rename rawCheckFilelist -> rawCheckFileList
 * 28. 7.2011, H.G.: remove rawCheckFileList, rawCheckObjList
 *                      (used only in rawCliCmd)
 *  4.12.2013, H.G.: rawRecvStatus: modify last arg to 'srawStatus *'
 * 29. 1.2014, H.G.: remove decl rawCheckClientFile, only once used
 *********************************************************************
 */

#ifdef _AIX
unsigned long ielpst(unsigned long, unsigned long *);

int rawCheckAuth( char *, char *,
                  char *, char *, int,
                  char *);
   /* check if client authorized for requested action */
#endif

int rawDelFile( int, srawComm *);
   /* delete object in archive */

int rawDelList( int, int, srawDataMoverAttr *, srawComm *,
                char **, char **);
   /* delete list of objects in archive */

int rawGetCmdParms(int , char **, char **);
   /* get all cmd parameters via array of char ptrs */

#ifdef VMS
unsigned long rawGetFileAttr(char *, int **);
   /* get file attributes (reclen, size in bytes) */

int rawGetFilelist( char *, char **);
   /* get list of full file names from generic input */
#else /* Unix */

int rawGetFileAttr(char *, unsigned long *);
   /* get file attributes (size in bytes), return rc */

int rawGetFilelist( char *, int, char **);
   /* get list of full file names from generic input */
#endif /* Unix */

int rawGetFileSize( char *, unsigned long *, unsigned int *);
   /* get list of full file names from generic input */

char *rawGetFSName( char *);
   /* get file space name from user identification */

char *rawGetFullFile(char *, char *);
   /* get full file name from generic input & ll name */

char *rawGetHLName( char *);
   /* get high level object name from path      */

int rawGetHostConn();
   /* get network connection type of client host */

int rawGetLLName( char *, const char *, char *);
   /* get low level object name from file name    */

char *rawGetPathName( char *);
   /* get path name from high level object name */

char *rawGetUserid();
   /* get user identification */

int rawQueryFile( int, int, srawComm *, void **);
   /* query for single object in archive */

void rawQueryPrint(srawObjAttr *, int);
   /* print query results for one object */

int rawQueryString(srawObjAttr *, int, int, char *);
   /* print query results for one object */

int rawRecvError( int, int, char *);
   /* receive error message */

int rawRecvHead( int, char *);
   /* receive common buffer header */

int rawRecvHeadC( int, char *, int, int, char *);
   /* receive common buffer header and check header values */

int rawRecvStatus( int, srawStatus *);
   /* receive status header and error msg, if applicable */

int rawSendRequest( int, int, int, int);
   /* send request buffer */

int rawSendStatus( int, int, char *);
   /* send status buffer */

int rawSortValues( int *, int, int, int, int *, int *);
   /* sort indexed list of numbers */

int rawTapeFile( char *, char *, int, int *, unsigned long *);
   /* send status buffer */

int rawTestFileName( char *);
   /* verify that specified name is valid */

#ifdef VMS
int rawCheckDevice (char *, char **, char **);
   /* check device type of specified file */

int rawGetTapeInfo( char *, char **, sTapeFileList **);
   /* get file names, sizes and buffer sizes from tape */

int rawSyncFilelist( char *, sTapeFileList **, int);
   /* synchronize both filelist buffers */

char *rawVMSName( char *, int);   
   /* check if valid VMS name; if not change it to valid VMS name */

#endif

int rconnect( char *, int, int *, int *);
   /* open connection to specified server */

