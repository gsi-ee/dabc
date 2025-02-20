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

#ifndef DABC_Reference
#define DABC_Reference

#include <string>

namespace dabc {

   class Command;
   class ReferencesVector;
   class Object;
   class Mutex;

   /** \brief %Reference on the arbitrary object
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    *  Ensure that object on which reference is pointing on is not disappear until reference
    *  is existing. In normal situation one should use reference in form:
    *
    *  void func(Object* obj)
    *  {
    *     dabc::Reference ref(new Object);
    *     ref.SetAutoDestroy(true);
    *     ref()->Print();
    *  }
    *
    *  When function is finishing its work, reference will be automatically released.
    *  If autodestroy flag was specified, object also will be destroyed.
    *
    *  Reference object cannot be used from several threads simultaneously,
    *  one should create new reference to work such way
    *
    *  When derived class should be created, DABC_REFERENCE macro could be
    *  used. For instance:
    *
    *   class WorkerRef : public Reference {
    *      DABC_REFERENCE(WorkerRef, Reference, Worker)
    *
    *      public:
    *         bool Execute(const std::string &cmdname)
    *           { return GetObject() ? GetObject()->Execute(cmdname) : false; }
    *    };
    *
    *  Macro defines all nice-to-have methods which should be presented in the reference
    *  class - default constructor, copy constructor, assignment operator, shift operator, some other.
    *
    *  Typically such reference class should be a friend for object class to have access to the
    *  object protected-private methods and members.
    *
    *  Main motivation for such class is to provide thread-safe methods via references.
    *  Once user acquire reference on the object, it can use any methods referenced object has.
    *  Methods, implemented in object class, generally should be protected and
    *  only can be used from class itself.
    *
    */

   class Reference {

      friend class Object;
      friend class Command;

      protected:

         Object*    fObj{nullptr};       ///< pointer on the object

         Mutex* ObjectMutex() const;

         /** \brief Method used in copy constructor and assigned operations */
         void Assign(const Reference &src);

         /** \brief Special method, which allows to generate new reference when object mutex is locked.
          *   It may be necessary when non-recursive mutexes are used. */
         bool AcquireRefWithoutMutex(Reference &ref);

         /** \brief Method used in reference constructor/assignments to verify is object is suitable */
         template<class T>
         bool verify_object(Object* src, T* &tgt) { return (tgt = dynamic_cast<T*>(src)) != nullptr; }

      public:

         /** \brief Constructor, creates reference on the object. If not possible, exception is thrown */
         Reference(Object* obj = nullptr);

         /** \brief Copy constructor, if source is transient than source reference will be emptied */
         Reference(const Reference& src) throw();

         /** \brief Destructor, releases reference on the object */
         virtual ~Reference();

         /** \brief Set autodestroy flag for the object
          * Once enabled, object will be destroyed when last reference will be cleared */
         void SetAutoDestroy(bool on = true);

         /** \brief Direct set of object to reference.
          * withmutex = false means that user already lock object mutex */
         void SetObject(Object* obj, bool withmutex = true);

         /** \brief Returns number of references on the object */
         unsigned NumReferences() const;

         /** \brief Releases reference on the object */
         void Release() throw();

         /** \brief Copy reference to output object.
          * Source reference will be empty after the call.
          * One probably could use left shift operator - this will save many intermediate operations */
         Reference Take();

         /** \brief Release reference and starts destroyment of  referenced object */
         void Destroy() throw();

         /** \brief Return pointer on the object */
         inline Object* GetObject() const { return fObj; }

         /** \brief Returns pointer on parent object */
         Object* GetParent() const;

         /** \brief Returns reference on parent object */
         Reference GetParentRef() const { return dabc::Reference(GetParent()); }

         /** \brief Return name of referenced object, if object not assigned, returns "---" */
         const char *GetName() const;

         /** \brief Return class name of referenced object, if object not assigned, returns "---" */
         const char *ClassName() const;

         /** \brief Returns true if object name is the same as specified one.
          * If object not assigned, always return false. */
         bool IsName(const char *name) const;

         /** \brief Return pointer on the object */
         inline Object* operator()() const { return fObj; }

         /** \brief Returns true if reference contains nullptr */
         inline bool null() const { return GetObject() == nullptr; }

         /** \brief Returns true if reference contains nullptr */
         inline bool operator!() const { return null(); }

         /** \brief Add child to list of object children */
         bool AddChild(Object* obj);

         /** \brief Return number of childs in referenced object.
          * If object null, always return 0 */
         unsigned NumChilds() const;

         /** \brief Return reference on child n. */
         Reference GetChild(unsigned n) const;

         /** \brief Return references for all childs */
         bool GetAllChildRef(ReferencesVector* vect) const;

         /** \brief Searches for child in referenced object.
          * If object null, always return null reference */
         Reference FindChild(const char *name) const;

         /** Remove child with given name and return reference on that child */
         bool RemoveChild(const char *name, bool cleanup = true);

         /** \brief Remove all childs in referenced object
          * If cleanup true (default) and object is owner, all objects will be destroyed */
         bool RemoveChilds(bool cleanup = true);

         /** \brief Assignment operator - copy reference */
         Reference& operator=(const Reference& src) throw();

         /** \brief Assignment operator - copy reference */
         Reference& operator=(Object* obj) throw();

         /** \brief Move operator - reference moved from source to target */
         Reference& operator<<(Reference& src) throw();

         /** \brief Compare operator - return true if references refer to same object */
         inline bool operator==(const Reference& src) const { return GetObject() == src(); }

         /** \brief Compare operator - return true if references refer to different object */
         inline bool operator!=(const Reference& src) const { return GetObject() != src(); }

         /** \brief Compare operator - return true if reference refer to same object */
         inline bool operator==(Object* obj) const { return GetObject() == obj; }

         /** \brief Compare operator - return true if reference refer to different objects */
         inline bool operator!=(Object* obj) const { return GetObject() != obj; }

         /** \brief Show on debug output content of reference */
         void Print(int lvl = 0, const char *from = nullptr) const;

         /** \brief Return folder of specified name, no special symbols are allowed.
          * \param[in] name     requested folder name
          * \param[in] force    if true, missing folder will be created
          * \returns            reference on the folder */
         Reference GetFolder(const std::string &name, bool force = false) throw();

         /** \brief Produce string, which can be used as name argument in
          * dabc::mgr.FindItem(name) call */
         std::string ItemName(bool compact = true) const;

         /** \brief Produce name, which can be used to find item, calling topitem.FindChild().
          * \details topitem should be one of the parent
          * TODO: one should be able to calculate relative name with any pairs of objects */
         std::string RelativeName(const dabc::Reference& topitem);
   };
}


#define DABC_REFERENCE(RefClass, ParentClass, T) \
      public: \
         /** \brief Constructor, creates reference on the object. If not possible, exception is thrown */ \
         RefClass(T *obj = nullptr) throw() : ParentClass(obj) {} \
         /** \brief Copy constructor */ \
         RefClass(const RefClass& src) throw() : ParentClass(src) {} \
         /** \brief Copy constructor */ \
         RefClass(const Reference& src) throw() : ParentClass() \
            { T *res = nullptr; if (verify_object(src(),res)) { Assign(src); } } \
         /** \brief Return pointer on the object */ \
         inline T* GetObject() const { return (T*) ParentClass::GetObject(); } \
         /** \brief Return pointer on the object */ \
         inline T* operator()() const { return (T*) ParentClass::GetObject(); } \
         /** \brief Assignment operator - copy reference */ \
         RefClass& operator=(const RefClass& src) throw() { ParentClass::operator=(src); return *this; } \
         /** \brief Assignment operator - copy reference. Also check dynamic_cast that type is supported */ \
         RefClass& operator=(const Reference& src) throw() \
         { \
            Release(); T *res = nullptr; \
            if (verify_object(src(),res)) Assign(src);  \
            return *this; \
         } \
         /** \brief Assignment operator - create reference for object */ \
         RefClass& operator=(dabc::Object* obj) throw() \
         { \
            Release(); T *res = nullptr; \
            if (verify_object(obj,res)) { RefClass ref((T*)obj); *this << ref; } \
            return *this; \
         } \
         /** \brief Move operator - reference moved from source to target */ \
         RefClass& operator<<(Reference& src) throw() \
         { \
            T* res = nullptr; \
            if (verify_object(src(),res)) dabc::Reference::operator<<(src); \
               else { Release(); src.Release(); } \
            return *this; \
         }


#endif

