#ifndef BNET_defines
#define BNET_defines

#include "dabc/bnetdefs.h"

#include <stdint.h>
#include <stdio.h>

#include <vector>

namespace bnet {

   struct names {
      static const char* WorkerModule() { return "BnetWorker"; }

      static const char* EventLifeTime() { return "EventLifeTime"; }
      static const char* ControlKind() { return "TestControlKind"; }
      static const char* SubmitPreTime() { return "SubmitPreTime"; }
   };

   enum BnetCommandsIds {
      BNET_CMD_MAGIC      = 0x1ff1,

      // these commands are send over network
      BNET_CMD_EXIT      = 76543201,
      BNET_CMD_TIMESYNC  = 76543202,
      BNET_CMD_EXECSYNC  = 76543203,
      BNET_CMD_GETSYNC   = 76543204,
      BNET_CMD_COLLECT   = 76543205,
      BNET_CMD_TEST      = 76543206,
      BNET_CMD_CREATEQP  = 76543207,
      BNET_CMD_CONNECTQP = 76543208,
      BNET_CMD_CLOSEQP   = 76543209,
      BNET_CMD_ALLTOALL  = 76543210,
      BNET_CMD_GETRUNRES = 76543211,
      BNET_CMD_SHOWRUNRES= 76543212,
      BNET_CMD_CLEANUP   = 76543213,
      BNET_CMD_ASKQUEUE  = 76543214,
      BNET_CMD_COLLRATE  = 76543215,
      BNET_CMD_ACTIVENODES=76543216,

      // these commands are local

      BNET_CMD_WAIT       = 80001,  // wait for 1 seconds
      BNET_CMD_MEASURE    = 80002,  // measure command delay
      BNET_CMD_CONNECTDONE= 80003  // finilize connections
   };

   class TestEventHandling : public EventHandling {
      public:
         TestEventHandling(const char* name) : EventHandling(name) {}

         virtual bool GenerateSubEvent(const bnet::EventId&, int subid, int numsubids, dabc::Buffer& buf);

         virtual bool ExtractEventId(const dabc::Buffer& buf, EventId& evid);

         virtual dabc::Buffer BuildFullEvent(const bnet::EventId& evid, dabc::Buffer* bufs, int numbufs);
   };


   #pragma pack(1)

   struct CommandMessage
   {
      int32_t magic;        // just BNET_CMD_MAGIC value to ensure that this is command
      int32_t cmdid;        // command id
      int32_t node;         // node, for which command is submitted
      double delay;         // how long should node waits before exit from command wait
      int32_t getresults;   // how many double values return from results array
      int32_t cmddatasize;
   };

   struct TransportHeader {
      uint32_t   srcnode;   // source node id
      uint32_t   tgtnode;   // target node id
      uint64_t   evid;      // event id
      double     send_tm;   // time when operation was send (for debugging)
      uint32_t   seqid;     // sequence number of transfer - to detect skipped operations
      uint32_t   sendkind;  // indicate that is send 0 - dummy (empty buffer), 1 - normal data
      uint32_t   sendlen;   // length of send data
   };

   enum ControlPacketKind {
      cpk_Null        = 0,
      cpk_SubevSizes  = 123,    // data with subevents sizes
      cpk_Turns       = 234,    // data with event association and in which time slot these events should be send
      cpk_BuilderInfo = 345,    // information from event builder
      cpk_SkipMarkers = 456,    // boundaries to skip events, turns, ...
      cpk_SchedSlot   = 567     // definition of base time for next schedules
   };

   struct ControlSubheader {
      uint32_t kind;  // kind of data - see ControlPacketKind
      uint32_t len;   // length of the data - including header
   };


#pragma pack()

}

namespace dabc {


   extern double rand_0_1();

   extern double GaussRand(double mean, double sigma);

   template<class T>
   class Matrix {
      public:
         Matrix() : fNRow(0), fNCol(0), fMatrix(0) {}

         Matrix(int _nrow, int _ncol) : fNRow(_nrow), fNCol(_ncol), fMatrix(0) {
            Allocate();
         }

         virtual ~Matrix() {
            Clean();
         }

         void SetSize(int _nrow, int _ncol) {
            if ((nrow()!=_nrow) || (ncol()!=_ncol)) {
               Clean();
               fNRow = _nrow;
               fNCol = _ncol;
               Allocate();
            }
         }

         int nrow() const { return fNRow; }
         int ncol() const { return fNCol; }

         T** getmatrix() const { return fMatrix; }

         T* getrow(int row) const { return fMatrix[row]; }

         T operator()(int row, int col) const { return fMatrix[row][col]; }

         T& operator()(int row, int col) { return fMatrix[row][col]; }

         void Fill(T value)
         {
            for (int row=0;row<nrow();row++)
               for (int col=0;col<ncol();col++) {
                  fMatrix[row][col] = value;
               }
         }

         void FillRandom(double mean, double sigma, double min = 0., double max = 0.)
         {
            for (int row=0;row<nrow();row++)
               for (int col=0;col<ncol();col++) {
                  double value = GaussRand(mean,sigma);
                  if (value<min) value = min;
                  if ((max>0) && (value>max)) value = max;
                  fMatrix[row][col] = T(value);

               }
         }

         int CountEqual(T value) {
            int cnt = 0;
            for (int row=0;row<nrow();row++)
               for (int col=0;col<ncol();col++)
                  if (fMatrix[row][col] == value) cnt++;
            return cnt;
         }

         void PrintMatrix(const char* format = "%5d") {
            for (int row=0;row<nrow();row++) {
               for (int col=0;col<ncol();col++)
                  printf(format,fMatrix[row][col]);
               printf("\n");
            }
         }

      protected:
         void Allocate()
         {
            if ((nrow()==0) || (ncol()==0)) return;
            fMatrix = new T* [nrow()];
            for (int n=0;n<nrow();n++)
               fMatrix[n] = new T[ncol()];
         }

         void Clean()
         {
            if (fMatrix) {
               for(int n=0;n<nrow();n++)
                  delete[] fMatrix[n];
               delete[] fMatrix;
               fMatrix = 0;
            }
         }

         int   fNRow;
         int   fNCol;

         T** fMatrix;
   };

  template<class T>
  class Column {
     public:
        Column() : fSize(0), fVector(0) {}

        Column(int _size) : fSize(_size), fVector(0)
        {
           Allocate();
        }

        virtual ~Column()
        {
           Clean();
        }

        void SetSize(int _size)
        {
           if (_size == size()) return;

           if (_size < size()) {
              fSize = _size;
           } else {
              Clean();
              fSize = _size;
              Allocate();
           }
        }

        int size() const { return fSize; }

        T* getvector() const { return fVector; }

        T at(int n) const { return fVector[n]; }
        T& at(int n) { return fVector[n]; }

        T operator()(int n) const { return fVector[n]; }
        T& operator()(int n) { return fVector[n]; }

        bool Remove(int indx)
        {
           if ((indx<0) || (indx>=size())) return false;
           for (int n=indx;n<size()-1;n++)
              fVector[n] = fVector[n+1];
           SetSize(size()-1);
           return true;
        }

        void Fill(T value)
        {
           for (int n=0;n<size();n++)
              fVector[n] = value;
        }

        int FindIndex(T value) const
        {
           for (int n=0;n<size();n++)
              if (fVector[n] == value) return n;
           return -1;
        }

        bool Find(T value) const { return FindIndex(value) >= 0; }

        void Sort()
        {
           for (int n=0;n<size()-1;n++) {
              int pmin = n;
              for (int k=n+1; k<size();k++)
                 if (fVector[k] < fVector[pmin]) pmin = k;
              if (pmin!=n) {
                 T value = fVector[n];
                 fVector[n] = fVector[pmin];
                 fVector[pmin] = value;
              }

           }

        }

     protected:
        void Allocate() {
           fVector = new T[size()];
        }

        void Clean() {
           if (fVector!=0) {
              delete[] fVector;
              fVector = 0;
              fSize = 0;
           }
        }

        int  fSize;
        T*   fVector;
  };

   typedef Matrix<int> IntMatrix;
   typedef Matrix<bool> BoolMatrix;

   typedef Column<int> IntColumn;
   typedef Column<bool> BoolColumn;
}

#endif
