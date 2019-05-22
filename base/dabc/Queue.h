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

#ifndef DABC_Queue
#define DABC_Queue

#include <string.h>

#include <vector>

#ifndef DABC_defines
#include "dabc/defines.h"
#endif

#ifndef DABC_logging
#include "dabc/logging.h"
#endif

#ifndef DABC_string
#include "dabc/string.h"
#endif

namespace dabc {

   /** \brief Template of circular queue
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    */

   template<class T, bool canexpand = false>
   class Queue {

      public:
         class Iterator {

            friend class Queue<T,canexpand>;

            Queue<T,canexpand>* prnt{nullptr};
            T* item{nullptr};
            unsigned loop{0};

            Iterator(Queue<T,canexpand>* _prnt, T* _item, unsigned _loop) : prnt(_prnt), item(_item), loop(_loop) {}

            public:

            Iterator() = default;
            Iterator(const Iterator& src) : prnt(src.prnt), item(src.item), loop(src.loop) {}

            T* operator()() const { return item; }

            bool operator==(const Iterator& src) const { return (item == src.item) && (loop == src.loop); }
            bool operator!=(const Iterator& src) const { return (item != src.item) || (loop != src.loop); }

            Iterator& operator=(const Iterator& src) { prnt = src.prnt; item = src.item; return *this; }

            // prefix ++iter
            Iterator& operator++()
            {
               if (++item == prnt->fBorder) { item = prnt->fQueue; loop++; }
               return *this;
            }

            // postfix  iter++
            void operator++(int)
            {
               if (++item == prnt->fBorder) { item = prnt->fQueue; loop++; }
            }
         };


      protected:

         friend class Queue<T,canexpand>::Iterator;

         T*         fQueue{nullptr};
         T*         fBorder{nullptr}; // maximum pointer value
         unsigned   fCapacity{0};
         unsigned   fSize{0};
         T*         fHead{nullptr};
         T*         fTail{nullptr};
         unsigned   fInitSize{0}; ///< original size of the queue, restored then Reset() method is called

         T*         QueueItem(unsigned n) { return fQueue + n; }

      public:
         Queue() = default;

         Queue(unsigned capacity)
         {
            Init(capacity);
         }

         virtual ~Queue()
         {
            delete[] fQueue;
            fQueue = nullptr;
         }

         void Init(unsigned capacity)
         {
            fInitSize = capacity;
            Allocate(capacity);
         }

         void Allocate(unsigned capacity)
         {
            if (fCapacity>0) {
               delete[] fQueue;
               fCapacity = 0;
               fQueue = nullptr;
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

         /** \brief Method can be used to copy content of the
          * queue into externally allocated array */
         virtual void CopyTo(T* tgt)
         {
            if (fSize>0) {
               if (fHead>fTail) {
                  memcpy((void *)tgt, (const void *)fTail, (fHead - fTail) * sizeof(T));
               } else {
                  unsigned sz = fBorder - fTail;
                  memcpy((void *)tgt, (const void *) fTail, sz * sizeof(T));
                  memcpy((void *) (tgt + sz), (const void *) fQueue, (fHead - fQueue) * sizeof(T));
               }
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

            CopyTo(q);

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
            return (fSize<fCapacity) || (canexpand && Expand());
         }

         void Push(T val)
         {
            if (MakePlaceForNext()) {
               *fHead++ = val;
               if (fHead==fBorder) fHead = fQueue;
               fSize++;
            } else {
               EOUT("No more space in fixed queue size = %d", fSize);
            }
         }

         T* PushEmpty()
         {
            if (MakePlaceForNext()) {
               T* res = fHead;
               if (++fHead==fBorder) fHead = fQueue;
               fSize++;
               return res;
            }

            EOUT("No more space in fixed queue size = %d", fSize);
            return 0;
         }


         void PushRef(const T& val)
         {
            if (MakePlaceForNext()) {
               *fHead++ = val;
               if (fHead==fBorder) fHead = fQueue;
               fSize++;
            } else {
               EOUT("No more space in fixed queue size = %d", fSize);
            }
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
            #ifdef DABC_EXTRA_CHECKS
               if (fSize==0) EOUT("Queue is empty");
            #endif
            T* res = fTail++;
            if (fTail==fBorder) fTail = fQueue;
            fSize--;
            return *res;
         }

         T& Front() const
         {
            #ifdef DABC_EXTRA_CHECKS
               if (fSize==0) EOUT("Queue is empty");
            #endif
            return *fTail;
         }

         // TODO: implement iterator to access all items in the queue

         T& Item(unsigned indx) const
         {
            #ifdef DABC_EXTRA_CHECKS
            if (indx>=fSize)
               EOUT("Wrong item index %u", indx);
            #endif
            T* item = fTail + indx;
            if (item>=fBorder) item -= fCapacity;
            return *item;
         }

         T* ItemPtr(unsigned indx) const
         {
            #ifdef DABC_EXTRA_CHECKS
            if (indx>=fSize) {
               EOUT("Wrong item index %u", indx);
               return 0;
            }
            #endif
            T* item = fTail + indx;
            if (item>=fBorder) item -= fCapacity;
            return item;
         }

         T& Back() const
         {
            #ifdef DABC_EXTRA_CHECKS
               if (fSize==0) EOUT("Queue is empty");
            #endif
            return Item(fSize-1);
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

         Iterator begin() { return Iterator(this, fTail, 0); }
         Iterator end() { return Iterator(this, fHead, fHead > fTail ? 0 : 1); }
   };

   // ___________________________________________________________

   /** \brief Specialized vector with pointers
    *
    * \ingroup dabc_all_classes
    *
    * Allows to check pointer duplications
    */

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

   /** \brief Template of queue with complex objects
    *
    * \ingroup dabc_all_classes
    *
    * Special case of the queue when structure or class is used as entry of the queue.
    * Main difference from normal queue - one somehow should cleanup item when it is not longer used
    * without item destructor. The only way is to introduce reset() method in contained class which
    * do the job similar to destructor. It is also recommended that contained class has
    * copy constructor and assign operator defined. For example, one should have class:
    *   struct Rec {
    *      int i;
    *      bool b;
    *      Rec() : i(0), b(false) {}
    *      Rec(const Rec& src) : i(src.i), b(src.b) {}
    *      Rec& operator=(const Rec& src) { i = src.y; b = src.b; return *this; }
    *      void reset() { i = 0; b = false; }
    *      ~Rec() { reset(); }
    *   };
    *
    */

   template<class T, bool canexpand = true>
   class RecordsQueue : public Queue<T, canexpand> {
      typedef Queue<T, canexpand> Parent;
      public:
         RecordsQueue() : Parent() {}

         virtual ~RecordsQueue() {}

         RecordsQueue(unsigned capacity) : Parent(capacity) {}

         void Allocate(unsigned capacity) { Parent::Allocate(capacity); }

         bool Empty() const { return Parent::Empty(); }

         bool Full() const { return Parent::Full(); }

         unsigned Size() const { return Parent::Size(); }

         unsigned Capacity() const { return Parent::Capacity(); }

         T& Item(unsigned n) const { return Parent::Item(n); }

         void Push(const T& val) { Parent::PushRef(val); }

         T& Front() const { return Parent::Front(); }

         T& Back() const { return Parent::Back(); }

         void PopOnly() { Front().reset(); Parent::PopOnly(); }

         virtual void CopyTo(T* tgt)
         {
            for (unsigned n=0; n<Size(); n++)
               *tgt++ = Item(n);
         }

         T* PushEmpty() { return Parent::PushEmpty(); }

         void Reset()
         {
            if (Parent::fCapacity != Parent::fInitSize)
               Parent::Allocate(Parent::fInitSize);
            else {
               for (unsigned n=0;n<Size();n++)
                  Item(n).reset();
               Parent::Reset();
            }
         }

         template<class DDD>
         T* FindItemWithId(DDD id)
         {
            for (unsigned n=0;n<Size();n++) {
               T* item = &Item(n);
               if (item->id()==id) return item;
            }
            return 0;
         }

         /** Helper methods to preallocate memory in each record in the queue */
         void AllocateRecs(unsigned rec_capacity)
         {
            for (unsigned n=0;n<Capacity();n++)
               Parent::QueueItem(n)->allocate(rec_capacity);
         }

   };

}

#endif

