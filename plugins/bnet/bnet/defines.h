#ifndef BNET_defines
#define BNET_defines

#include <stdint.h>
#include <stdio.h>

#include <vector>

// maximum command buffer required to provide QPs connection over cluster
// for 1000 QPs with 16 Lids and 12 byte per QP one require at least  192000 bytes

#define BNET_MAXLID  16


#define BNET_QP_QUEUE_SIZE 128

#define BNET_CMD_MAGIC 0x1ff1

// these commands are send over network
#define BNET_CMD_EXIT        76543201
#define BNET_CMD_TIMESYNC    76543202
#define BNET_CMD_EXECSYNC    76543203
#define BNET_CMD_GETSYNC     76543204
#define BNET_CMD_COLLECT     76543205
#define BNET_CMD_TEST        76543206
#define BNET_CMD_CREATEQP    76543207
#define BNET_CMD_CONNECTQP   76543208
#define BNET_CMD_CLOSEQP     76543209
#define BNET_CMD_ALLTOALL    76543210
#define BNET_CMD_GETRUNRES   76543211
#define BNET_CMD_SHOWRUNRES  76543212
#define BNET_CMD_CLEANUP     76543213
#define BNET_CMD_ASKQUEUE    76543214
#define BNET_CMD_COLLRATE    76543217
#define BNET_CMD_ACTIVENODES 76543218

// these commands are local

#define BNET_CMD_WAIT        80001  // wait for 1 seconds
#define BNET_CMD_MEASURE     80002  // measure command delay
#define BNET_CMD_CONNECTDONE 80003  // finilize connections

#define BNET_WORKERNAME     "IbTest"

#define CMDCH_RECV_ARG        123456
#define CMDCH_SEND_ARG        654321

// this is command sent via common channel for different kind of action
// command specific settings should be sent after all or selected nodes
// responded

namespace bnet {

   #pragma pack(1)


   struct CommandMessage
   {
      int32_t magic;        // just BNET_CMD_MAGIC value to ensure that this is command
      int32_t cmdid;        // command id
      int32_t node;         // node, for which command is submitted
      double delay;         // how long should node waits before exit from command wait
      int32_t getresults;   // how many double values return from results array
      int32_t cmddatasize;

      void* cmddata() const { return (int8_t*) this + sizeof(CommandMessage); }
   };

   struct TimeSyncMessage
   {
      int32_t msgid;
      double  master_time;
      double  slave_shift;
      double  slave_time;
      double  slave_scale;
   };

   struct TransportHeader {
      uint32_t   srcid;   // source node id
      uint32_t   tgtid;   // target node id
      uint64_t eventid;   // event id
   };

#pragma pack()

   class DoublesVector : public std::vector<double> {
      public:
      DoublesVector() : std::vector<double>() {}

      void Sort();

      double Mean(double max_cut = 1.);
      double Dev(double max_cut = 1.);
      double Max();
      double Min();
   };

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
