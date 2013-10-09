// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#ifndef DABC_DataIO
#define DABC_DataIO

#ifndef DABC_string
#include "dabc/string.h"
#endif

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_Url
#include "dabc/Url.h"
#endif

namespace dabc {

   class Buffer;

   enum DataInputCodes {
      di_ValidSize     = 0xFFFFFFF0,   // last valid size for buffer
      di_None          = 0xFFFFFFF1,   // invalid return code
      di_Repeat        = 0xFFFFFFF2,   // no data in input, try as soon as possible
      di_RepeatTimeOut = 0xFFFFFFF3,   // no data in input, try with timeout
      di_EndOfStream   = 0xFFFFFFF4,   // no more data in input is expected, object can be destroyed
      di_Ok            = 0xFFFFFFF5,   // normal code
      di_CallBack      = 0xFFFFFFF6,   // data source want to work via callback
      di_Error         = 0xFFFFFFF7,   // error
      di_SkipBuffer    = 0xFFFFFFF8,   // when doing complete, buffer cannot be filled
      di_DfltBufSize   = 0xFFFFFFF9,   // default buffer size means that any size provided by memory pool will be accepted
      di_NeedMoreBuf   = 0xFFFFFFFA,   // when input needs more buffer for it work, DataInput can initiate callback at any time
      di_HasEnoughBuf  = 0xFFFFFFFB,   // input indicates that enough buffers are accumulated and transport should wait for callback
      di_MoreBufReady  = 0xFFFFFFFC,   // when input has more bufs ready for complete
      di_QueueBufReady = 0xFFFFFFFD    // when data input can provide next buffer
   };

   /** \brief Interface for implementing any kind of data input
    *
    * \ingroup dabc_user_classes
    * \ingroup dabc_all_classes
    *
    * dabc::DataInput object used by dabc::InputTransport to perform data reading.
    * Input consists from sequence of following calls:
    *   - \ref Read_Size -  defines required buffer size for next operation
    *   - \ref Read_Start - intermediate call with buffer of requested size
    *   - \ref Read_Complete - fill buffer with the data
    */

   class DataInput {
      public:

         virtual ~DataInput() {}

         /** \brief Initialize data input, using port and command
          *
          * This is generic virtual method to initialize input,
          * using configurations from Port and (or) from the Command
          *
          * \param[in] wrk  reference on input port
          * \param[in] cmd  reference on command object
          * \returns        false when method fails */
         virtual bool Read_Init(const WorkerRef& wrk, const Command& cmd) { return true; }


         /** \brief Defines required buffer size for next operation
          *
          * \returns
          *   - 0..di_ValidSize  - size of buffer for next read operation (di_ValidSize = 0xFFFFFFF0)
          *   - di_EndOfStream   - this is end of stream, normal close of the input
          *   - di_DfltBufSize   - any non-zero buffer can be provided
          *   - di_Repeat        - nothing to read now, try again as soon as possible
          *   - di_RepeatTimeOut - nothing to read now, try again after timeout
          *   - di_Error         - error, close input */
         virtual unsigned Read_Size() { return di_EndOfStream; }

         /** \brief Prepare buffer for reading (if required)
          *
          * \returns
          *   - di_Ok               - buffer must be filled in Read_Complete call
          *   - di_Error (or other) - error, skip buffer */
         virtual unsigned Read_Start(Buffer& buf) { return di_Ok; }

         /** \brief Complete reading of the buffer from source,
          *
          * \returns
          *   - di_Ok            - buffer filled and ready
          *   - di_EndOfStream   - this is end of stream, normal close of the input
          *   - di_SkipBuffer    - skip buffer
          *   - di_Error         - error, skip buffer and close input
          *   - di_Repeat        - not ready, call again as soon as possible
          *   - di_RepeatTimeOut - not ready, call again after timeout */
         virtual unsigned Read_Complete(Buffer& buf) { return di_EndOfStream; }

         /** \brief Provide timeout value
          *
          * \returns timeout in seconds
          *
          * When Read_Size or Read_Complete operations returns di_RepeatTimeout argument,
          * specified timeout will be used before next operation will be done */
         virtual double Read_Timeout() { return 0.1; }

         /** \brief Reads complete buffer

          * Perform consequent call of Read_Size(), Read_Start() and Read_Complete() methods
          *
          * \returns filled buffer */
         Buffer ReadBuffer();
   };

   // _________________________________________________________________

   enum DataOutputCodes {
       do_Ok,               // operation is ok
       do_Skip,             // if returned, buffer must be skipped
       do_Repeat,           // repeat operation as soon as possible
       do_RepeatTimeOut,    // repeat operation after timeout
       do_CallBack,         // operation will be called back
       do_Error,            // operation error, object must be cleaned
       do_Close             // output is closed, one should destroy it
   };


   /** \brief Interface for implementing any kind of data output
    *
    * \ingroup dabc_user_classes
    * \ingroup dabc_all_classes
    */

   class DataOutput {
      protected:
         std::string         fInfoName;     // parameter name for info settings

         DataOutput(const dabc::Url& url);

         void ShowInfo(int lvl, const std::string& info);

      public:

         virtual ~DataOutput() {}

         /** \brief Methods set parameter name, which could be used for debug output */
         void SetInfoParName(const std::string& name);

         /** \brief Method can be used to get debug info about output */
         virtual std::string ProvideInfo() { return std::string(); }

         /** This is generic virtual method to initialize output,
          * using configurations from Port or from the Command  */
         virtual bool Write_Init(const WorkerRef& wrk, const Command& cmd);

         /** Check if output can be done.
          * Return values:
          *     do_Ok       - object is ready to write next buffer
          *     do_Error    - error
          *     do_Skip     - skip current buffer
          *     do_Repeat   - repeat operation as soon as possible
          *     do_RepeatTimeOut - repeat operation after timeout
          *     do_Close    - output is closed and need to be destroyed */
         virtual unsigned Write_Check() { return do_Ok; }

         /** Start writing of buffer to output.
          * Return values:
          *    do_Ok     - operation is started, Write_Complete() must be called
          *    do_Error  - error
          *    do_Skip   - buffer must be skipped
          *    do_Close    - output is closed  */
         virtual unsigned Write_Buffer(Buffer& buf) { return do_Ok; }

         /** Complete writing of the buffer.
          * Return values:
          *    do_Ok     - writing is done
          *    do_Error  - error
          *    do_Close    - output is closed  */
         virtual unsigned Write_Complete() { return do_Ok; }

         /** Timeout in seconds for write operation.
          * Should be used when any of Write_ operation return do_RepeatTimeOut */
         virtual double Write_Timeout() { return 0.1; }

         /** Flush output object, called when buffer with EOL type is appeared */
         virtual void Write_Flush() {}

         /** Write buffer to the output. If callback is required, will fail */
         bool WriteBuffer(Buffer& buf);
   };

   // ===========================================================

   class FileInterface;

   /** \brief Interface for implementing file inputs
    *
    * \ingroup dabc_user_classes
    * \ingroup dabc_all_classes
    *
    * Provide convenient way for managing list of files as inputs.
    */

   class FileInput : public DataInput {
      protected:
         std::string          fFileName;
         dabc::Reference      fFilesList;
         dabc::FileInterface* fIO;
         std::string          fCurrentName;

         bool TakeNextFileName();
         const std::string& CurrentFileName() const { return fCurrentName; }
         void ClearCurrentFileName() { fCurrentName.clear(); }

         FileInput(const dabc::Url& url);

      public:

         virtual ~FileInput();

         void SetIO(dabc::FileInterface* io);

         virtual bool Read_Init(const WorkerRef& wrk, const Command& cmd);

   };

   // ============================================================================

   /** \brief Interface for implementing file outputs
    *
    * \ingroup dabc_user_classes
    * \ingroup dabc_all_classes
    *
    * Provide convenient way for managing automatic file numbering.
    */

   class FileOutput : public DataOutput {
      protected:

         std::string          fFileName;
         int                  fSizeLimitMB;
         std::string          fFileExtens;

         dabc::FileInterface* fIO;

         int                  fCurrentFileNumber;
         std::string          fCurrentFileName;
         long                 fCurrentFileSize;

         long                 fTotalFileSize;
         long                 fTotalNumBufs;
         long                 fTotalNumEvents;

         void ProduceNewFileName();
         const std::string& CurrentFileName() const { return fCurrentFileName; }

         /** Return true if new file should be started */
         bool CheckBufferForNextFile(unsigned sz);

         void AccountBuffer(unsigned sz, int numev = 0);

         FileOutput(const dabc::Url& url, const std::string& ext = "");

         std::string ProduceFileName(const std::string& suffix);

      public:

         virtual ~FileOutput();

         void SetIO(dabc::FileInterface* io);

         virtual std::string ProvideInfo();

         virtual bool Write_Init(const WorkerRef& wrk, const Command& cmd);
   };

}

#endif
