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

#ifndef DABC_Object
#define DABC_Object

#ifndef DABC_defines
#include "dabc/defines.h"
#endif

#ifndef DABC_Reference
#include "dabc/Reference.h"
#endif

#ifndef DABC_string
#include "dabc/string.h"
#endif

#ifndef DABC_ConfigIO
#include "dabc/ConfigIO.h"
#endif

namespace dabc {

   class Manager;
   class Thread;
   class Mutex;
   class Configuration;
   class ReferencesVector;
   class HierarchyContainer;

   extern const char* xmlDeviceNode;
   extern const char* xmlThreadNode;
   extern const char* xmlMemoryPoolNode;
   extern const char* xmlModuleNode;
   extern const char* xmlConnectionNode;

   extern const char* xmlQueueAttr;
   extern const char* xmlBindAttr;
   extern const char* xmlSignalAttr;
   extern const char* xmlRateAttr;
   extern const char* xmlLoopAttr;
   extern const char* xmlInputQueueSize;
   extern const char* xmlOutputQueueSize;
   extern const char* xmlInlineDataSize;

   extern const char* xmlPoolName;
   extern const char* xmlWorkPool;
   extern const char* xmlFixedLayout;
   extern const char* xmlCleanupTimeout;
   extern const char* xmlBufferSize;
   extern const char* xmlNumBuffers;
   extern const char* xmlNumSegments;
   extern const char* xmlAlignment;
   extern const char* xmlShowInfo;

   extern const char* xmlNumInputs;
   extern const char* xmlNumOutputs;
   extern const char* xmlInputPrefix;
   extern const char* xmlInputMask;
   extern const char* xmlOutputPrefix;
   extern const char* xmlOutputMask;

   extern const char* xmlUseAcknowledge;
   extern const char* xmlFlushTimeout;
   extern const char* xmlConnTimeout;

   extern const char* xmlTrueValue;
   extern const char* xmlFalseValue;

   // list of parameters, taken from URL
   // this parameters can be (should be) used in many places, which
   // allows to specify them through standardized URL syntax
   extern const char* xmlProtocol;
   extern const char* xmlHostName;
   extern const char* xmlFileName;     // used as file name option in separate xml node
   extern const char* xmlFileNumber;   // used as file number option in separate xml node
   extern const char* xmlFileSizeLimit; // used as separate xml node
   extern const char* xml_maxsize;      // used as option in file url
   extern const char* xml_number;       // used as option in file url


   extern const char* xmlMcastAddr;
   extern const char* xmlMcastPort;
   extern const char* xmlMcastRecv;

   extern const char* typeThread;
   extern const char* typeDevice;
   extern const char* typeSocketDevice;
   extern const char* typeSocketThread;
   extern const char* typeApplication;

   /** \brief Base class for most of the DABC classes.
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    * Provides possibility to build __thread safe__ hierarchy of the objects.
    * Has references counter to be sure how many references are existing on this object.
    *
    * Thread-safety for the Object only has means together with Reference class.
    * Only existence of reference can ensure that object will not be destroyed or completely changed
    * by some other threads. General concept is that object can only be changed if reference counter
    * is equal to 0 (in fact, only in constructor). There is main exception - list child objects can be
    * changed in any moment, therefore to ensure at each moment that child or children objects will
    * not change in meanwhile, one should use references on child. In all other special cases
    * such possibility will be extra marked in documentation.
    * In normal case attempt to change the referenced object will rise an exception.
    */

   class Object {
      friend class Manager;
      friend class Reference;

      private:

         /** These are object live stages. Normally they should go one after another */
         enum EState {
            stConstructor       = 0,      ///<  state during constructor
            stNormal            = 1,      ///<  state during object normal functioning
            stWaitForThread     = 2,      ///<  object must be cleaned by the thread
            stDoingDestroy      = 3,      ///<  we are inside destroy method
            stWaitForDestructor = 4,      ///<  one waits unit refcounter decreases
            stDestructor        = 5       ///<  state during destructor
         };

         static unsigned  gNumInstances;  ///< actual number of existing instances
         static unsigned  gNumCreated;    ///< number of created instances, will used for object id

         /** \brief Initializes all variables of the object */
         void Constructor();

         /** \brief Destroys all internal data, reentrant */
         void Destructor();

         /** \brief Increments reference counter, return false if it cannot be done
          * \param[in] withmutex can indicate that object mutex is already locked and we do not need repeat it again */
         bool IncReference(bool withmutex = true);

         /** \brief Decrements reference counter, return true if object must be destroyed */
         bool DecReference(bool ask_to_destroy, bool do_decrement = true, bool from_thread = false);

         /** \brief Returns object state value */
         inline EState GetState() const { return (EState) (fObjectFlags & flStateMask); }

         /** \brief Set object state value */
         inline void SetState(EState st) { fObjectFlags = (fObjectFlags & ~flStateMask) | (unsigned) st ; }

         static Reference SearchForChild(Reference& ref, const char* name, bool firsttime, bool force) throw();

         /** Method set cleanup bit that object will be cleaned up in all registered objects
          * Used only by manager therefore private */
         void SetCleanupBit();

      protected:

         enum EFlags {
            flStateMask      = 0x00f,  ///< use 4 bits for state
            flIsOwner        = 0x010,  ///< flag indicates default ownership for child objects
            flCleanup        = 0x020,  ///< flag indicates that one should cleanup pointer from depended objects
            flHasThread      = 0x040,  ///< flag indicates that object has thread and should be cleaned up via thread
            flAutoDestroy    = 0x080,  ///< object will be automatically destroyed when no references exists, normally set in constructor, example Command
            flLogging        = 0x100,  ///< object is marked to provide logging information, for debug purposes only
            flNoMutex        = 0x200,  ///< object will be created without mutex, only can be used in constructor
            flHidden         = 0x400   ///< hide object from hierarchy scan
         };

         unsigned           fObjectFlags;    ///< flag, protected by the mutex
         Reference          fObjectParent;   ///< reference on the parent object
         std::string        fObjectName;     ///< object name
         Mutex*             fObjectMutex;    ///< mutex protects all private property of the object
         int                fObjectRefCnt;   ///< accounts how many references existing on the object, __thread safe__
         ReferencesVector*  fObjectChilds;   ///< list of the child objects
         int                fObjectBlock;    ///< counter for blocking calls, as long as non-zero, non of child can be removed

         /** \brief Return value of selected flag, __not thread safe__  */
         inline bool GetFlag(unsigned fl) const { return (fObjectFlags & fl) != 0; }

         /** \brief Change value of selected flag, __not thread safe__  */
         inline void SetFlag(unsigned fl, bool on = true) { fObjectFlags = (on ? (fObjectFlags | fl) : (fObjectFlags & ~fl)); }

         /** \brief Returns mutex, used for protection of Object data members */
         inline Mutex* ObjectMutex() const { return fObjectMutex; }

         /** \brief Return number of references on the object */
         unsigned NumReferences();

         /** \brief Internal DABC method, used to activate object cleanup via object thread
          * Returns: false - object cannot be cleanup by the thread,
          *          true  - thread guarantees that CallDestroyFromThread() will be called from thread context */
         virtual bool AskToDestroyByThread() { return false; }

         /** \brief Internal DABC method, should be called by thread which was requested to destroy object */
         bool CallDestroyFromThread();

         /** \brief User method to cleanup object content before it will be destroyed
          * Main motivation is to release any references on other objects to avoid any
          * cross-references and as result deadlocks in objects cleanup.
          * Method is called in context of thread, to which object belongs to.
          * If user redefines this method, he is responsible to call method of parent class */
         virtual void ObjectCleanup();

         /** This method is called at the moment when DecReference think that object can
          * be destroyed and wants to return true. Object can decide to find another way to
          * the destructor and reject this. After _DoDeleteItself() returns true, it is fully
          * object responsibility to delete itself. */
         virtual bool _DoDeleteItself() { return false; }

         /** Specifies if object will be owner of its new childs */
         void SetOwner(bool on = true);

         /** \brief Set autodestroy flag for the object
          * Once enabled, object will be destroyed when last reference will be cleared */
         void SetAutoDestroy(bool on = true);

         /** \brief Method called by the manager when registered dependent object is destroyed
          *  Should be used in user class to clear all references on the object to let destroy it
          */
         virtual void ObjectDestroyed(Object*) {}

         /** \brief Return true if object is in normal state.
          * Typically should used from inside object itself (therefore protected) to reject from the
          * beginning actions which are too complex for object which is started to be destroyed */
         bool IsNormalState();

         /** \brief Method used to produce full item name,
          * \details Produced name can be used to find such item in the objects hierarchy */
         void FillFullName(std::string &fullname, Object* upto, bool exclude_top_parent = false) const;

         /** \brief Same as IsNormalState() but without mutex lock - user should lock mutex himself */
         bool _IsNormalState();

         /** \brief Method used to create new item to be placed as child of the object */
         virtual Object* CreateObject(const std::string& name) { return new Object(0, name); }

         /** \brief Method called when new childs are add or old are removed */
         virtual void _ChildsChanged() {}

         /** Structure used to specify arguments for object constructor */
         struct ConstructorPair {
            friend class Object;

            private:
               Reference parent;
               std::string name;

               ConstructorPair() : parent(), name() {}
               ConstructorPair(const ConstructorPair& src) : parent(src.parent), name(src.name) {}
         };

         /** \brief Internal DABC method, used to produce pair - object parent and object name,
          * which is typically should be used as argument in class constructor
          * \param[in] prnt        reference on parent object
          * \param[in] fullname    relative to parent path and name
          * \param[in] withmanager identify if object should be inserted into manager hierarchy (default) or not
          * \returns               instance of ConstructorPair object*/
         static ConstructorPair MakePair(Reference prnt, const std::string& fullname, bool withmanager = true);

         /** \brief Internal DABC method, used to produce pair - object parent and object name,
          * which is typically should be used as argument in class constructor */
         static ConstructorPair MakePair(Object* prnt, const std::string& fullname, bool withmanager = true);

         /** \brief Internal DABC method, used to produce pair - object parent and object name,
          * which is typically should be used as argument in class constructor */
         static ConstructorPair MakePair(const std::string& fullname, bool withmanager = true);

         Object(const ConstructorPair& pair, unsigned flags = flIsOwner);

      public:

         Object(const std::string& name, unsigned flags = flIsOwner);

         Object(Reference parent, const std::string& name, unsigned flags = flIsOwner);

         // FIXME: one should find a way to catch a call to the destructor
         virtual ~Object();

         /** \brief Returns pointer on parent object, __thread safe__ */
         Object* GetParent() const { return fObjectParent(); }

         /** \brief \returns reference on parent object, __thread safe__ */
         Reference GetParentRef() const { return fObjectParent; }

         /** \brief Checks if specified argument is in the list of object parents */
         bool IsParent(Object* obj) const;

         /** \brief Returns name of the object, __thread safe__ */
         const char* GetName() const { return fObjectName.c_str(); }

         /** \brief Checks if object name is same as provided string, __thread safe__ */
         bool IsName(const char* str) const { return fObjectName.compare(str)==0; }

         /** \brief Checks if object name is same as provided string, __thread safe__ */
         bool IsName(const std::string& str) const { return fObjectName.compare(str)==0; }

         /** \brief Checks if object name is same as provided, __thread safe__
          *
          * \param[in] str   string with name to be compared
          * \param[in] len   specifies how many characters should be checked
          *                  if len == 0, method will return false.
          *                  if  len < 0, str will be supposed normal null-terminated string
          * \returns   true if object name equal to provided string */
         bool IsName(const char* str, int len) const;

         /** Check if object name match to the mask */
         bool IsNameMatch(const std::string& mask) const;

         /** \brief Returns true if object is owner of its children, __thread safe__ */
         bool IsOwner() const;

         /** \brief Return true if object selected for logging, __thread safe__ */
         bool IsLogging() const;

         /** \brief Sets logging flag, __thread safe__ */
         void SetLogging(bool on = true);

         /** \brief Return true if object wants to be hidden from hierarchy scan, __thread safe__ */
         bool IsHidden() const;

         // List of children is __thread safe__ BUT may change in any time in between two calls
         // To perform some complex actions, references on child objects should be used


         // TODO: should it be public?
         // TODO: do we need another method

         /** \brief Add object to list of child objects, __thread safe__
          *
          * \param[in] child        object to add
          * \param[in] withmutex    true if object mutex should be locked
          * \param[in] setparent    true if parent field of child should be set
          * \returns                true if successful */
         bool AddChild(Object* child, bool withmutex = true) throw();

         /** \brief Add object to list of child objects at specified position
          *
          * \param[in] child       object to add
          * \param[in] pos         position at which add object, if grater than number of childs, will be add at the end
          * \param[in] withmutex   true if object mutex should be locked
          * \param[in] setparent   true if parent field of child should be set
          * \returns               true if successful */
         bool AddChildAt(Object* child, unsigned pos, bool withmutex = true);

         /** \brief Dettach child from parent object
          *  If cleanup==true and parent is owner of child, child will be destroyed*/
         bool RemoveChild(Object* child, bool cleanup = true) throw();

         /** \brief Dettach child object from paraent at specified position
          *  If cleanup==true and object is owner of child, child will be destroyed*/
         bool RemoveChildAt(unsigned n, bool cleanup = true) throw();

         /** \brief Remove all childs.
          * If object is owner and cleanup flag (default) is specified, when childs will be destroyed.  */
         bool RemoveChilds(bool cleanup = true);

         /** \brief returns number of child objects */
         unsigned NumChilds() const;

         /** \brief returns pointer on child object */
         Object* GetChild(unsigned n) const;

         /** \brief returns reference on child object */
         Reference GetChildRef(unsigned n) const;

         bool GetAllChildRef(ReferencesVector* vect) const;

         /** \brief returns pointer on child object with given name */
         Object* FindChild(const char* name) const;

         //TODO: one need method reverse to FindChild - MakePath()
         // one should be able to produce string which can be used to find specified object

         /** \brief returns reference on child object with given name */
         Reference FindChildRef(const char* name, bool force = false) const throw();

         /** \brief Return folder of specified name, no special symbols allowed.
          *
          * \param[in] name    folder name
          * \param[in] force   if true, missing folder will be created
          * \returns           reference on the folder */
         Reference GetFolder(const std::string& name, bool force = false) throw();

         /** \brief Print object content on debug output */
         virtual void Print(int lvl = 0);

         /** \brief Produce string, which can be used as name argument in
          * dabc::mgr.FindItem(name) call.
          *
          * \param[in] compact   if true [default], objects belonging to the application
          * will not include application name in their path
          * \returns  string with item name */
         std::string ItemName(bool compact = true) const;

         // ALL following methods about object destroyment and cleanup

         /** \brief User method for object destroyment.
          * In many cases object may not be destroyed immediately while references can exists.
          * Nevertheless, pointer on the object is no longer valid - object can be destroyed
          * in other thread in any time.
          * In some special cases objects cannot be destroyed at all - they will be cleaned by
          * other means (like thread - it is only destroyed by manager when no longer used
          * TODO: probably, one should remove it and always use reference */
         static void Destroy(Object* obj) throw();

         /**  \brief Returns class name of the object instance.
          *
          * In some cases class name used to correctly locate object in xml file */
         virtual const char* ClassName() const { return "Object"; }

         /** \brief Method to locate object in xml file
          *
          * Can be reimplemented in derived classes to check more attributes like class name */
         virtual bool Find(ConfigIO &cfg);

         /** \brief Method could be used to save any attributes of the object
          *  Implementation should look like:
          *
          *      dabc::Record rec(cont);
          *      rec.Field("number").SetInt(12);
          *
          *  If overwritten in derived class,
          *  ParentClass::BuildHierarchy(cont) must be called to store
          *  hierarchy of child objects */
         virtual void BuildHierarchy(HierarchyContainer* cont);

         /** \brief Fill fields map, which is relevant for the object */
         virtual void BuildFieldsMap(RecordFieldsMap* cont) {}


         // operations with object name (and info) are __not thread safe__
         // therefore, in the case when object name must be changed,
         // locking should be applied by any other means

         /** \brief Static variable counts total number of objects instances */
         static unsigned NumInstances() { return gNumInstances; }

         /** \ brief Methods to inspect how many objects pointers are remained
          *
          * Garbage collector will not remove lost objects itself - one can only watch
          * how many objects and which kind are remain in memory.
          * Method only work when DABC compiled with option "make extrachecks=true"
          */
         static void InspectGarbageCollector();

         /** \brief Check if name matches to specified mask
          *
          * Mask can include special symbols `*` and `?` */
         static bool NameMatch(const std::string& name, const std::string& mask);

         /** \brief Check if name matches to specified mask.
          *
          * Mask can be a list of masks separated by semicolon like `name1*:name2*:??name??` */
         static bool NameMatchM(const std::string& name, const std::string& mask);

#ifdef DABC_EXTRA_CHECKS
         static void DebugObject(const char* classname = 0, Object* instance = 0, int kind = 0);
#endif

      protected:
         /** \brief Changes object name. Should not be used if any references exists on the object */
         void SetName(const char* name);

         /** \brief Method should be used by the object to delete itself */
         void DeleteThis();
   };
};

#endif
