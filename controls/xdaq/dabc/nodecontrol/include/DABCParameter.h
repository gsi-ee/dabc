#ifndef DABC_XD_PARAMETER_H
#define DABC_XD_PARAMETER_H

#include "xdata/xdata.h"
#include "dabc/Parameter.h"


namespace dabc{
namespace xd{
    
class Registry;




/** Container for serializable and external reference. may also have callback
  * function that is invoked every time the serializable is changed, as triggerd by
  * listener function actionPerformed. Vice versa,
  * if reference variable is changed from outside, method ChangedReference will
  * update the serializable. Subclasses may add further references to objects
  * depending on serializable.
  *  @author J. Adamczewski, GSI*/   
class ControlParameter:  public xdata::ActionListener
        {
           public:
           ControlParameter(dabc::xd::Registry* owner, const std::string& name, 
                                xdata::Serializable* ser, 
                                char* reference,
                                void (*callback)()=0) : 
            owner_(owner),name_(name),ser_(ser),reference_(reference),callback_(callback){;}                      
           ControlParameter() :name_(""),ser_(0),reference_(0),callback_(0){;}                         

           virtual ~ControlParameter()
                {
                    delete ser_;   
                } 
           
           std::string& Name(){return name_;}
           
           xdata::Serializable* Serializable(){return ser_;}
           
           char* Reference(){return reference_;}
           
           dabc::xd::Registry* Owner(){return owner_;}
           
           /** callback when infospace (dim) parameter changes.
            * will update reference*/ 
           virtual void actionPerformed (xdata::Event& e);
           
           /** This method is called when variable at reference
             * was changed. Will update infospace and dim*/     
           void ChangedReference();
           
           
           private: 

           dabc::xd::Registry* owner_;
           std::string name_;
           xdata::Serializable* ser_;
           char* reference_;
           void (*callback_)();
          
        };    

/** special parameter reference container that knows module
  * and module application*/
class ModuleParameter:  public dabc::xd::ControlParameter
        {
           public:
           ModuleParameter(dabc::xd::Registry*  owner, const std::string& name, 
                                xdata::Serializable* ser, 
                                dabc::Parameter* par=0) : 
                                ControlParameter(owner, name, ser, 0, 0),
                                par_(par){;}                      
           ModuleParameter() : ControlParameter(),par_(0){;}                         

           virtual ~ModuleParameter() {;} 
           
          
           
           dabc::Parameter* CoreParameter(){return par_;}
            
           /** callback when infospace (dim) parameter changes.
            * will update reference*/ 
           virtual void actionPerformed (xdata::Event& e);
           
           private: 
            dabc::Parameter* par_;
          
        };    
    







}
} // end namespaces


#endif
