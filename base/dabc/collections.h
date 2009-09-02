/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#ifndef DABC_collections
#define DABC_collections

#include <stdint.h>

#include <string.h>

#include <stdlib.h>


#ifndef DABC_logging
#include "dabc/logging.h"
#endif

#include <vector>

namespace dabc {

   class Mutex;
   class Command;
   class Basic;
   class Buffer;

   typedef uint64_t EventId;

   #define NullEventId 0

   static inline EventId CodeEvent(uint16_t code)
      { return (uint64_t) code; }

   static inline EventId CodeEvent(uint16_t code, uint16_t item)
      { return (uint64_t) item << 48 | code; }

   static inline EventId CodeEvent(uint16_t code, uint16_t item, uint32_t arg)
      { return (((uint64_t) item) << 48) | (((uint64_t) arg) << 16) | code; }

   static inline uint16_t GetEventCode(EventId evnt) { return evnt & 0xffffLU; }
   static inline uint16_t GetEventItem(EventId evnt) { return evnt >> 48; }
   static inline uint32_t GetEventArg(EventId evnt) { return (evnt >> 16) & 0xffffffffLU; }

   // ____________________________________________________________

   class PointersVector : public std::vector<void*> {
      public:
         PointersVector() : std::vector<void*>() {}

         void* pop()
         {
            if (size()==0) return 0;
            void* res = back();
            pop_back();
            return res;
         }

         bool has_ptr(void* item) const
         {
            for (unsigned n=0; n<size(); n++)
               if (at(n)==item) return true;
            return false;
         }

         bool remove_at(unsigned n)
         {
            if (n>=size()) return false;
            erase(begin()+n);
            return true;
         }

         bool remove(void* item)
         {
            for (iterator iter = begin(); iter!=end(); iter++)
               if (*iter == item) {
                  erase(iter);
                  return true;
               }
            return false;
         }
   };

   // ___________________________________________________________

   template<class T>
   class Queue {
      protected:
         T*        fQueue;
         T*        fBorder; // maximum pointer value
         unsigned  fCapacity;
         unsigned  fSize;
         T*        fHead;
         T*        fTail;
         bool      fCanExpand;

      public:
         Queue() :
            fQueue(0),
            fBorder(0),
            fCapacity(0),
            fSize(0),
            fHead(0),
            fTail(0),
            fCanExpand(false)
         {
         }

         Queue(unsigned capacity, bool canexpand = false) :
            fQueue(0),
            fBorder(0),
            fCapacity(0),
            fSize(0),
            fHead(0),
            fTail(0),
            fCanExpand(canexpand)
         {
            Allocate(capacity);
         }

         ~Queue()
         {
            delete[] fQueue;
            fQueue = 0;
         }

         void Init(unsigned capacity, bool canexpand)
         {
            Allocate(capacity);
            fCanExpand = canexpand;
         }

         void Allocate(unsigned capacity)
         {
            if (fCapacity>0) {
               delete[] fQueue;
               fCapacity = 0;
               fQueue = 0;
               fBorder = 0;
            }

            if (capacity>0) {
              fCapacity = capacity;
              fSize = 0;
              fQueue = new T [fCapacity];
              fBorder = fQueue + fCapacity;
              fHead = fQueue;
              fTail = fQueue;
            }
         }

         // increase capacity of queue without lost of content
         bool Expand(unsigned newcapacity = 0)
         {
            if (newcapacity <= fCapacity)
               newcapacity = fCapacity * 2;
            if (newcapacity < 16) newcapacity = 16;
            T* q = new T [newcapacity];
            if (q==0) return false;

            if (fSize>0){
               if (fHead>fTail) {
                  memcpy(q, fTail, (fHead - fTail) * sizeof(T));
               } else {
                  unsigned sz = fBorder - fTail;
                  memcpy(q, fTail, sz * sizeof(T));
                  memcpy(q + sz, fQueue, (fHead - fQueue) * sizeof(T));
               }
            }
            delete [] fQueue;

            fCapacity = newcapacity;
            fQueue = q;
            fBorder = fQueue + fCapacity;
            fHead = fQueue + fSize; // we are sure that size less than capacity
            fTail = fQueue;
            return true;
         }

         bool Remove(T value)
         {
            if (Size()==0) return false;

            T *i_tgt(fTail), *i_src(fTail);

            do {
               if (*i_src != value) {
                  if (i_tgt!=i_src) *i_tgt = *i_src;
                  if (++i_tgt == fBorder) i_tgt = fQueue;
               } else
                  fSize--;
               if (++i_src == fBorder) i_src = fQueue;
            } while (i_src != fHead);

            fHead = i_tgt;

            return i_tgt != i_src;
         }

         bool RemoveItem(unsigned indx)
         {
            if (indx>=fSize) return false;

            T* tgt = fTail + indx;
            if (tgt>=fBorder) tgt -= fCapacity;

            while (true) {
               T* src = tgt + 1;
               if (src==fBorder) src = fQueue;
               if (src==fHead) break;
               *tgt = *src;
               tgt = src;
            }

            fHead = tgt;
            fSize--;
            return true;
         }

         bool MakePlaceForNext()
         {
            return (fSize<fCapacity) || (fCanExpand && Expand());
         }

         void Push(T val)
         {
            if ((fSize<fCapacity) || (fCanExpand && Expand())) {
               *fHead++ = val;
               if (fHead==fBorder) fHead = fQueue;
               fSize++;
            } else {
               EOUT(("No more space in fixed queue size = %d", fSize));
            }
         }

         T* PushEmpty()
         {
            if ((fSize<fCapacity) || (fCanExpand && Expand())) {
               T* res = fHead;
               if (++fHead==fBorder) fHead = fQueue;
               fSize++;
               return res;
            }

            EOUT(("No more space in fixed queue size = %d", fSize));
            return 0;
         }

         void PopOnly()
         {
            if (fSize>0) {
               fSize--;
               if (++fTail==fBorder) fTail = fQueue;
            }
         }

         T Pop()
         {
            #ifdef DO_INDEX_CHECK
               if (fSize==0) EOUT(("Queue is empty"));
            #endif
            T* res = fTail++;
            if (fTail==fBorder) fTail = fQueue;
            fSize--;
            return *res;
         }

         T Front() const
         {
            #ifdef DO_INDEX_CHECK
               if (fSize==0) EOUT(("Queue is empty"));
            #endif
            return *fTail;
         }


         T Item(unsigned indx) const
         {
            #ifdef DO_INDEX_CHECK
            if (indx>=fSize)
               EOUT(("Wrong item index %u", indx));
            #endif
            T* item = fTail + indx;
            if (item>=fBorder) item -= fCapacity;
            return *item;
         }

         T* ItemPtr(unsigned indx) const
         {
            #ifdef DO_INDEX_CHECK
            if (indx>=fSize) {
               EOUT(("Wrong item index %u", indx));
               return 0;
            }
            #endif
            T* item = fTail + indx;
            if (item>=fBorder) item -= fCapacity;
            return item;
         }

         void Reset()
         {
            fHead = fQueue;
            fTail = fQueue;
            fSize = 0;
         }

         unsigned Capacity() const { return fCapacity; }

         unsigned Size() const { return fSize; }

         bool Full() const { return Capacity() == Size(); }

         bool Empty() const { return Size() == 0; }
   };

   // _______________________________________________________________

   class BuffersQueue : public Queue<Buffer*> {
      public:
         BuffersQueue(int capacity, bool expand = false) : Queue<Buffer*>(capacity, expand) {}
         ~BuffersQueue() { Cleanup(); }
         void Allocate(int capacity) { Cleanup(); Queue<Buffer*>::Allocate(capacity); }
         void Cleanup(Mutex* mutex = 0);
   };

   // _______________________________________________________________

   class CommandsQueue {
      protected:
         Queue<Command*>    fQueue;
         bool               fReplyQueue;
         Mutex              *fCmdsMutex;

      public:
         CommandsQueue(bool replyqueue = false, bool withmutex = true);
         virtual ~CommandsQueue();
         void Push(Command* cmd);
         Command* Pop();
         Command* Front();
         unsigned Size();

         void Cleanup();
   };



};

#endif
