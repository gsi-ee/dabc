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

#ifndef DABC_ReferencesVector
#define DABC_ReferencesVector

#ifndef DABC_Reference
#include "dabc/Reference.h"
#endif

#include <vector>

namespace dabc {

   /** \brief Vector of \ref dabc::Reference objects
    *
    * \ingroup dabc_all_classes
    */

   class ReferencesVector {
      private:

         typedef std::vector<Reference> refs_vector;

         refs_vector* fVector{nullptr};  ///< vector with references

         void ExpandVector();

      public:
         /** \brief Constructor */
         ReferencesVector() throw();

         /** \brief Destructor */
         virtual ~ReferencesVector() throw();

         /** \brief Add reference to the vector */
         bool Add(Reference& ref) throw();

         /** \brief Add reference to the vector at specified position */
         bool AddAt(Reference& ref, unsigned pos) throw();

         /** \brief Remove reference on specified object  */
         bool Remove(Object *obj) throw();

         /** \brief Remove reference on specified object  */
         void RemoveAt(unsigned n) throw();

         /** \brief Clear all references, if owner specified objects will be destroyed */
         bool Clear(bool asowner = false) throw();

         /** \brief Returns number of items in vector */
         unsigned GetSize() const { return fVector->size(); }

         /** \brief Returns pointer on the object */
         inline Object *GetObject(unsigned n) const
            { return (n < GetSize()) ? fVector->at(n).GetObject() : nullptr; }

         /** \brief Returns new reference on object with index n */
         inline Reference Get(unsigned n) const { return Reference(n < GetSize() ? fVector->at(n).GetObject() : nullptr); }

         /** \brief Returns new reference on object with index n */
         inline Reference operator[](unsigned n) const { return Get(n); }

         /** \brief Remove reference from vector and return it to the user */
         Reference TakeRef(unsigned n);

         /** \brief Remove last reference from vector */
         Reference TakeLast();

         /** \brief Remove reference at specified position. operator << is used to avoid child locking */
         bool ExtractRef(unsigned n, Reference& ref);

         /** \brief Remove reference at specified position. operator << is used to avoid child locking */
         bool ExtractRef(Object *obj, Reference& ref);


         /** \brief Simple search of object by name, no any subfolder structures */
         Object *FindObject(const char *name, int len = -1) const;

         /** \brief Return true if vector has pointer on the object */
         bool HasObject(Object *ptr);
   };

}

#endif
