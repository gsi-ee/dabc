#include "DABCParameter.h"

#include "DABCRegistry.h"
//#include "dabc/Parameter.h"
//#include "xdata/xdata.h"



void dabc::xd::ControlParameter::actionPerformed (xdata::Event& e)
{
  	//std::cout <<"dabc::xd::ControlParameter::actionPerformed got event type:"<<e.type() << std::endl;

   if ((e.type() == "ItemChangedEvent"))
   {
   //std::cout <<"DABCControlParameter::actionPerformed for "<<name_<<"..." << std::endl;   
   // change reference pointer if defined:
   if(reference_!=0)
      {
       // this is stupid thing, but stems from xdaq non-consequent usage of templates:
      // would need the data types as non-templated subtypes of xdata::Serializable here!
      xdata::String* serString=dynamic_cast<xdata::String*>(ser_);
      xdata::Float* serFloat=dynamic_cast<xdata::Float*>(ser_);
      xdata::Double* serDouble=dynamic_cast<xdata::Double*>(ser_);
      xdata::Integer* serInteger=dynamic_cast<xdata::Integer*>(ser_);
      xdata::UnsignedInteger* serUInt=dynamic_cast<xdata::UnsignedInteger*>(ser_);
      xdata::UnsignedLong* serULong=dynamic_cast<xdata::UnsignedLong*>(ser_);
      xdata::UnsignedShort* serUShort=dynamic_cast<xdata::UnsignedShort*>(ser_);
      xdata::Boolean* serBool=dynamic_cast<xdata::Boolean*>(ser_);
      if(serString)
         {
            std::string text=serString->toString(); 
            *((std::string*) (reference_)) =text;            
         }
      else if(serFloat)
         {
            *((float*)(reference_))=(float) *serFloat;
         }
      else if(serDouble)
       {
            *((double*)(reference_))=(double) *serDouble;
       }
      else if(serInteger)
          {
               *((int*)(reference_))=(int) *serInteger;
          }
      else if(serUInt)
          {
              *((unsigned int*)(reference_))=(unsigned int) *serUInt;
          }    
      else if(serULong)
          {
             *((unsigned long*)(reference_))=(unsigned long) *serULong;
          } 
      else if(serUShort)
          {
             *((unsigned short*)(reference_))=(unsigned short) *serUShort;
          }         
       else if(serBool)
          {
             *((bool*)(reference_))=(bool) *serBool;
          }
          
         }  // if(reference_)       
   // optionally invoke user change event callback:
   if(callback_) (*callback_)();
   
   

 }
} 


void dabc::xd::ControlParameter::ChangedReference()
{
 xdata::String* serString=dynamic_cast<xdata::String*>(ser_);
   xdata::Float* serFloat=dynamic_cast<xdata::Float*>(ser_);
   xdata::Double* serDouble=dynamic_cast<xdata::Double*>(ser_);
   xdata::Integer* serInteger=dynamic_cast<xdata::Integer*>(ser_);
   xdata::UnsignedInteger* serUInt=dynamic_cast<xdata::UnsignedInteger*>(ser_);
   xdata::UnsignedLong* serULong=dynamic_cast<xdata::UnsignedLong*>(ser_);
   xdata::UnsignedShort* serUShort=dynamic_cast<xdata::UnsignedShort*>(ser_);
   xdata::Boolean* serBool=dynamic_cast<xdata::Boolean*>(ser_);
   if(serString)
      {
         *serString= *((std::string*) (reference_));            
      }
   else if(serFloat)
      {
         *serFloat= *((float*)(reference_));
      }
   else if(serDouble)
    {
         *serDouble= *((double*)(reference_));
    }
   else if(serInteger)
       {
            *serInteger=*((int*)(reference_));
       }
   else if(serUInt)
       {
           *serUInt=*((unsigned int*)(reference_));
       }    
   else if(serULong)
       {
          *serULong=*((unsigned long*)(reference_));
       } 
   else if(serUShort)
       {
          *serUShort=*((unsigned short*)(reference_));
       }         
    else if(serBool)
       {
          *serBool=*((bool*)(reference_));
       }        
    owner_->UpdateDIMService(name_);   

}


///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//////////////// Module Parameter container:

void dabc::xd::ModuleParameter::actionPerformed (xdata::Event& e)
{
//std::cout <<"dabc::xd::ModuleParameter::actionPerformed got event type:"<<e.type() << std::endl;

if ((e.type() == "ItemChangedEvent"))
   {
   //std::cout <<"ModuleParameter::actionPerformed for "<<Name()<<"..." << std::endl;   
   if(par_)
      {
         std::string val=Serializable()->toString();
         par_->InvokeChange(val.c_str());
  
      }
 }
} 



