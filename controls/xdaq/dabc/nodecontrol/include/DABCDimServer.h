#ifndef DABC_XD_DIMSERVER_H 
#define DABC_XD_DIMSERVER_H

#include "xdata/xdata.h"
#include "dis.hxx"
#include "dic.hxx"

  



#if __XDAQVERSION__  > 310    
#include "toolbox/BSem.h"
#else
#include "BSem.h"
#endif


#include "nameParser.h" 
#include "dabc/records.h"
#include "dabc/Parameter.h"



namespace dabc{
namespace xd{

class Registry;


/**
*  Subclass of dim server. Defines command and client exit handlers.
*  Also implements error handler and server exit handler.
*  Execution is handled by methods of dabc::xd::Registry via backpointers
*  We implement this as singleton, since more than one application
*  may use the dim server in one process (xdaq executive)
*  @author J. Adamczewski, GSI
*/
class DimServer: public ::DimServer
{
	
public:




     /** Get handle to unique dimserver object*/
    static DimServer* Instance();
    
    /** only delete instance if still exisiting*/   
    static void Delete();
    
     /** start the service of name for dnsnode and port.
      * need to wrap it here to launch the applicationList service
      * with correct server name prefix.*/
    void Start(std::string name, std::string dnsnode, unsigned int dnsport);
    
    /** implemented for symmetry*/
    void Stop();

    
    
    /** to use the callbacks for command and error handler functions*/
    void RegisterApplication(dabc::xd::Registry* app);
    
     /** remove from list of known applications*/
    void UnRegisterApplication(dabc::xd::Registry* app);
    

    
    void commandHandler(); 

    void errorHandler(int severity, int code, char *msg); 
    void exitHandler( int code );    
    void clientExitHandler();
    
    bool isStarted(){return isStarted_;}

private:

    /** do not use ctor, use Instance*/
    DimServer();
    /** do not use dtor, use Delete*/
    virtual ~DimServer();
    
    void StartApplicationListService(std::string& name);
        
    /** singleton pattern.*/
    static DimServer* theInstance_;
 
    /** registry of xdaq applications that use this dimserver.*/
    std::vector<dabc::xd::Registry*> theApplications_;   

    /** this will contain catenated list of application names */
    DimService* applicationList_;
    
    std::string applicationListString;
    /** buffer for catenated list of application names */
    char* applicationListBuffer_;
    
    bool isStarted_;

#if __XDAQVERSION__  > 310    
/** this mutex may protect instance pointer*/
    static toolbox::BSem* globalMutex_; 
    
    /** to protect application list*/
    toolbox::BSem* appMutex_;
#else
    /** this mutex may protect instance pointer*/
    static BSem* globalMutex_; 
    
    /** to protect application list*/
    BSem* appMutex_;
#endif    

};




/**
*  This class is used to keep track of dim services in vector.
*  Note that we need char array buffer to keep contents of std::string
*  process variables
*  @author J. Adamczewski, GSI
*/
class DimServiceEntry
{
    public:

#if __XDAQVERSION__  > 310    
    
          /** this ctor will create service of dimname from serializable properties and quality word*/
        DimServiceEntry(xdata::Serializable *variable, std::string dimname,  toolbox::BSem* mutex);
#else
                 /** this ctor will create service of dimname from serializable properties and quality word*/
        DimServiceEntry(xdata::Serializable *variable, std::string dimname,  BSem* mutex);
 
#endif    

        
        
        virtual ~DimServiceEntry();
        void UpdateDIM();
        const char* getDimName();
        /** change value of service variable to val
          * string expression is converted to type of serializable*/
        void SetValue(std::string val);
        
        /** set dabc type information to dim service quality word*/
        void SetType(dabc::xd::nameParser::recordtype t);
        
        /** dabc record type specifier of dim service*/
        dabc::xd::nameParser::recordtype GetType();
                
        /** set dabc visibility bit mask to dim service quality word*/
        void SetVisibility(dabc::xd::nameParser::visiblemask mask);
        
        /** returns dabc visibility property mask of service*/
        dabc::xd::nameParser::visiblemask GetVisibility();
        
        /** set dabc status information to dim service quality word*/
        void SetStatus(dabc::xd::nameParser::recordstat s);
         
        /** dabc status byte of dim service*/
        dabc::xd::nameParser::recordstat GetStatus();
        
        /** set dabc mode byte to dim service quality word*/
        void SetMode(dabc::xd::nameParser::recordmode m);
        
         /** dabc mode byte of dim service*/
        dabc::xd::nameParser::recordmode GetMode();
        
    private:
        
         /** create new dim service of name from properties of serializable
          * may init internal buffer structures depending on record type*/
        void InitService(xdata::Serializable *variable, std::string name);
       
        
        /** update dim buffers from serializable variable*/
        void UpdateBuffer(xdata::Serializable *variable, bool initialize=false);
        
        /** This is the dim service object*/
        ::DimService* dimservice_;
        
        /** buffer for dim structures copied from serializable contents*/
        char* stringbuffer_;
        
        /** backlink to xdaq process variable for updating*/
        xdata::Serializable* xdata_;

        /** parser utility for name components and quality bits.
          * also keeps copy of current dim quality flag. */
        dabc::xd::nameParser parser_;    

#if __XDAQVERSION__  > 310    
    
       /** protect service and buffer*/
        toolbox::BSem* servMutex_;
#else
    
        /** protect service and buffer*/
        BSem* servMutex_;
#endif    

        
        
};

/**Class to subscribe dabc parameter to remote dim service. Subclass of DimInfo.
  * When remote dim server changes the subscribed variable value, the local dabc Parameter
  * will be updated using the corresponding method
  *  @author J. Adamczewski, GSI*/   
class DimParameterInfo :  public ::DimStampedInfo
    {

        public:
        
            enum parameterType {
                     NONE,
                     INTEGER,
                     FLOAT, 
                     DOUBLE, 
                     SHORT, 
                     LONGLONG,  
                     STRING, 
                     STRUCTURE 
            };



        
            DimParameterInfo(dabc::Parameter* par, const char* fullname, int nolinkdefault=-1)
                : DimStampedInfo(fullname, nolinkdefault),fxPar(par),fiType(INTEGER){;}    

            DimParameterInfo(dabc::Parameter* par, const char* fullname, float nolinkdefault=-1.)
                : DimStampedInfo(fullname, nolinkdefault),fxPar(par),fiType(FLOAT){;}    
            
            DimParameterInfo(dabc::Parameter* par, const char* fullname, double nolinkdefault=-1.)
                : DimStampedInfo(fullname, nolinkdefault),fxPar(par),fiType(DOUBLE){;}    
            
            DimParameterInfo(dabc::Parameter* par, const char* fullname, short nolinkdefault=-1)
                : DimStampedInfo(fullname, nolinkdefault),fxPar(par),fiType(SHORT){;}    

            DimParameterInfo(dabc::Parameter* par, const char* fullname, longlong nolinkdefault=-1)
                : DimStampedInfo(fullname, nolinkdefault),fxPar(par),fiType(LONGLONG){;}    

            DimParameterInfo(dabc::Parameter* par, const char* fullname, char* nolinkdefault=0)
                : DimStampedInfo(fullname, nolinkdefault),fxPar(par),fiType(STRING){;}    

            DimParameterInfo(dabc::Parameter* par, const char* fullname, void * nolink, int nolinksize=0)
                : DimStampedInfo(fullname, nolink, nolinksize),fxPar(par),fiType(STRUCTURE){;}    
    
            virtual void infoHandler();

        bool HasParameter(dabc::Parameter* par) {return (par==fxPar);}

    
        private:
            /** backpointer to parameter that is to be updated*/
            dabc::Parameter* fxPar;
            /** type specifier for infoHandler*/
            parameterType fiType;


    };













/** This is xdaq bag for ratemeter record type:*/
class RateRecord
{
        public: 
        void registerFields(xdata::Bag<dabc::xd::RateRecord> * bag)
        {               
                bag->addField("rate", &rate_);
                bag->addField("displayMode", &displayMode_);                
                bag->addField("loLimit", &loLimit_);
                bag->addField("upLimit", &upLimit_);
                bag->addField("loAlarm", &loAlarm_);
                bag->addField("upAlarm", &upAlarm_);                
                bag->addField("color", &color_);
                bag->addField("colorAlarm", &colorAlarm_);
                bag->addField("units", &units_); 
        } 
        
         xdata::Float rate_;
         xdata::Integer displayMode_;                
         xdata::Float loLimit_;
         xdata::Float upLimit_;
         xdata::Float loAlarm_;
         xdata::Float upAlarm_;                
         xdata::String color_;
         xdata::String colorAlarm_;
         xdata::String units_; 
                             
};

/** This is xdaq bag for status record type:*/
class StatusRecord
{
        public: 
        void registerFields(xdata::Bag<dabc::xd::StatusRecord> * bag)
        {               
                bag->addField("severity", &severity_);
                bag->addField("color", &color_);
                bag->addField("status", &status_);
        } 
         xdata::Integer severity_;                
         xdata::String color_;
         xdata::String status_;
};



/** This is xdaq bag for info message record type:*/
class InfoRecord
{
        public: 
        void registerFields(xdata::Bag<dabc::xd::InfoRecord> * bag)
        {               
                bag->addField("verbosity", &verbosity_);
                bag->addField("color", &color_);
                bag->addField("info", &info_);
        } 
         xdata::Integer verbosity_;                
         xdata::String color_;
         xdata::String info_;
};


/** This is xdaq bag for histogram record type:*/
class HistogramRecord
{
        public: 
        void registerFields(xdata::Bag<dabc::xd::HistogramRecord> * bag)
        {               
                bag->addField("channels", &channels_);
                bag->addField("xLow", &xLow_);
                bag->addField("xHigh", &xHigh_);
                bag->addField("xLett", &xLett_);
                bag->addField("cont", &cont_);
                bag->addField("data", &data_);
                
        } 
         
         xdata::Integer channels_;   
         xdata::Float xLow_;
         xdata::Float xHigh_;
         xdata::String xLett_;
         xdata::String cont_;
         xdata::String color_;
         xdata::Vector<xdata::Integer> data_;
};









} // end namespace xd
} // end namespace dabc

#endif

