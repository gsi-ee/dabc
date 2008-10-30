#ifndef DABC_XD_LOCKGUARD_H
#define DABC_XD_LOCKGUARD_H


#if __XDAQVERSION__  > 310
    #include "toolbox/BSem.h"             
#else
    #include "BSem.h"
#endif



namespace dabc{
namespace xd{

/** Auxiliary class to un-/lock sections safely with BSem mutex*/
class LockGuard
    {
       public:
#if __XDAQVERSION__  > 310             
        LockGuard(toolbox::BSem* m=0): mutex_(m)
#else
        LockGuard(BSem* m=0): mutex_(m)
#endif
            {
                if(mutex_) mutex_->take();
            }
        ~LockGuard()
            {
                if(mutex_) mutex_->give();
            } 
       private: 
       
#if __XDAQVERSION__  > 310           
        toolbox::BSem* mutex_;
#else       
        BSem* mutex_;   
#endif

    };
} // end namespace xd
} // end namespace dabc
 
 
#endif
