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
    *     Object* obj = new Object;
    *     dabc::Reference ref(obj);
    *     ref()->Print();
    *  }
    *
    *  When function is finishing its work, reference will be automatically released.
    *  If ownership is specified, object also will be destroyed
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
    *         bool Execute(const std::string& cmdname)
    *           { return GetObject() ? GetObject()->Execute(cmdname) : false; }
    *    };
    *
    *  Macro defines all nice-to-have methods which should be presented in the reference
    *  class - default constructor, copy constructor, assignment operator, some other.
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

      private:
         enum EFlags {
            flTransient = 0x01, ///< reference will be moved by any next copy operation
            flOwner     = 0x02  ///< indicates if reference also is object owner
         };

         Object*    fObj;       ///< pointer on the object
         unsigned   fFlags;     ///< flags, see EFlags

         /** \brief Return value of selected flag, non thread safe  */
         inline bool GetFlag(unsigned fl) const { return (fFlags & fl) != 0; }

         /** \brief Change value of selected flag, non thread safe  */
         inline void SetFlag(unsigned fl, bool on = true) { fFlags = on ? (fFlags | fl) : (fFlags & ~fl); }

      protected:

         Reference(bool withmutex, Object* obj) throw();

         Mutex* ObjectMutex() const;

         /** \brief Method used in copy constructor and assigned operations */
         void Assign(const Reference& src) throw();

         /** \brief Convert reference to string, one should be able to recover reference from the string */
         bool ConvertToString(char* buf, int buflen);

         /** \brief Reconstruct reference again from the string */
         Reference(const char* value, int valuelen) throw();

         /** \brief Method used in reference constructor/assignments to verify is object is suitable */
         template<class T>
         bool verify_object(Object* src, T* &tgt) { return (tgt=dynamic_cast<T*>(src))!=0; }

         /** Indicates default kind for created references */
         static bool transient_refs() { return true; }

      public:

         /** \brief Default constructor, creates empty reference */
         Reference();

         /** \brief Constructor, creates reference on the object. If not possible, exception is thrown */
         Reference(Object* obj, bool owner = false) throw();

         /** \brief Copy constructor, if source is transient than source reference will be emptied */
         Reference(const Reference& src) throw();

         /** \brief Copy constructor, if source is transient than source reference will be emptied
          * One also can specify if new reference will be transient or not - useful in constructors */
         Reference(const Reference& src, bool transient) throw();

         /** \brief Destructor, releases reference on the object */
         virtual ~Reference();

         /** \brief Returns true if reference is transient - any reference assign operation will remove object pointer */
         inline bool IsTransient() const { return GetFlag(flTransient); }

         /** \brief Set transient status of reference */
         inline void SetTransient(bool on = true) { SetFlag(flTransient,on); }

         /** \brief Returns true if reference is owner of the object */
         inline bool IsOwner() const { return GetFlag(flOwner); }

         /** \brief Set ownership flag for reference  */
         inline void SetOwner(bool on = true) { SetFlag(flOwner, on); }

         /** \brief Direct set of object to reference.
          * withmutex = false means that user already lock object mutex */
         void SetObject(Object* obj, bool owner = false, bool withmutex = true) throw();

         /** \brief Returns number of references on the object */
         unsigned NumReferences() const;

         /** \brief Releases reference on the object */
         void Release() throw();

         /** Copy reference to output object (disregard of transient flag).
          * Source reference will be empty after call */
         Reference Take();

         /** \brief Return pointer on the object */
         inline Object* GetObject() const { return fObj; }

         /** \brief Returns pointer on parent object */
         Object* GetParent() const;

         /** \brief Return name of referenced object, if object not assigned, returns "---" */
         const char* GetName() const;

         /** \brief Return class name of referenced object, if object not assigned, returns "---" */
         const char* ClassName() const;

         /** \brief Returns true if object name is the same as specified one.
          * If object not assigned, always return false. */
         bool IsName(const char* name) const;

         /** \brief Return pointer on the object */
         inline Object* operator()() const { return fObj; }

         inline bool null() const { return GetObject() == 0; }

         /** \brief Add child to list of object children */
         bool AddChild(Object* obj, bool setparent = true);

         /** Puts child into list of children.
          * See dabc::Object::PutChild for more details */
         Reference PutChild(Object* obj, bool delduplicate = true);

         /** \brief Return number of childs in referenced object.
          * If object null, always return 0 */
         unsigned NumChilds() const;

         /** \brief Return reference on child n. */
         Reference GetChild(unsigned n) const;

         /** \brief Return references for all childs */
         bool GetAllChildRef(ReferencesVector* vect);

         /** \brief Searches for child in referenced object.
          * If object null, always return null reference */
         Reference FindChild(const char* name) const;

         /** \brief Delete all childs in referenced object */
         void DeleteChilds();

         /** \brief Release reference and destroy referenced object
          * It also happens with Release call when reference is object owner */
         void Destroy() throw();

         /** \brief Assignment operator - copy reference */
         Reference& operator=(const Reference& src) throw();

         /** \brief Move operator - reference moved from source to target */
         Reference& operator<<(const Reference& src) throw();

         /** \brief Compare operator - return true if references refer to same object */
         inline bool operator==(const Reference& src) const { return GetObject() == src(); }

         /** \brief Compare operator - return true if references refer to different object */
         inline bool operator!=(const Reference& src) const { return GetObject() != src(); }

         /** \brief Compare operator - return true if reference refer to same object */
         inline bool operator==(Object* obj) const { return GetObject() == obj; }

         /** \brief Compare operator - return true if reference refer to different objects */
         inline bool operator!=(Object* obj) const { return GetObject() != obj; }

         /** \brief Return new reference on the object disregard to transient status */
         Reference Ref(bool withmutex = true) const { return Reference(withmutex, GetObject()); }

         /** \brief Show on debug output content of reference */
         void Print(int lvl=0, const char* from = 0) const;

         /** \brief Return folder of specified name, no special symbols are allowed.
          * \param[in] name     requested folder name
          * \param[in] force    if true, missing folder will be created
          * \returns            reference on the folder */
         Reference GetFolder(const char* name, bool force = false) throw();

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
      protected: \
         RefClass(bool withmutex, T* obj) : ParentClass(withmutex, obj) {} \
         inline void check_transient() { if (!null() && !transient_refs()) SetTransient(false); } \
      public: \
         /** \brief Default constructor, creates empty reference */ \
         RefClass() : ParentClass() {} \
         /** \brief Constructor, creates reference on the object. If not possible, exception is thrown */ \
         RefClass(T* obj, bool owner = false) throw() : ParentClass(obj, owner) { check_transient(); } \
         /** \brief Copy constructor, if source is transient than source reference will be emptied */ \
         RefClass(const RefClass& src) throw() : ParentClass(src) { check_transient(); } \
         /** \brief Copy constructor, if source is transient than source reference will be emptied */ \
         RefClass(const Reference& src) throw() : ParentClass() \
            { T* res(0); if (verify_object(src(),res)) { Assign(src); check_transient(); } } \
         /** \brief Return pointer on the object */ \
         inline T* GetObject() const { return (T*) ParentClass::GetObject(); } \
         /** \brief Return pointer on the object */ \
         inline T* operator()() const { return (T*) ParentClass::GetObject(); } \
         /** \brief Assignment operator - copy reference */ \
         RefClass& operator=(const RefClass& src) throw() { ParentClass::operator=(src); check_transient(); return *this; } \
         /** \brief Assignment operator - copy reference. Also check dynamic_cast that type is supported */ \
         RefClass& operator=(Reference& src) throw() \
         { \
            Release(); T* res(0); \
            if (verify_object(src(),res)) { Assign(src); check_transient(); } \
            return *this; \
         } \
         /** \brief Return new reference on the object - old reference will remain */ \
         /** \param[in] withmutex   if false, no mutex locking will be performed, it is supposed that mutex already locked */ \
         RefClass Ref(bool withmutex = true) const { return RefClass(withmutex, GetObject()); }


#endif

