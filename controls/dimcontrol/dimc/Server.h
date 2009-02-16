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
#ifndef DIMC_SERVER_H
#define DIMC_SERVER_H

#include "dis.hxx"
#include "dic.hxx"


#include "dimc/nameParser.h"
#include "dabc/records.h"
#include "dabc/Parameter.h"
#include "dabc/threads.h"

#define DIMC_STRINGBUFSIZE 512

namespace dimc{

class Registry;


/**
*  Subclass of dim server. Defines command and client exit handlers.
*  Also implements error handler and server exit handler.
*  Execution is handled by methods of dimc::Registry via backpointers
*  We implement this as singleton, since more than one application
*/
class Server: public ::DimServer
{

public:

     /** Get handle to unique dimserver object.*/
    static Server* Instance();

    /** only delete instance if still exisiting*/
    static void Delete();


     /** start the service of name for dnsnode and port.
      * need to wrap it here to launch the applicationList service
      * with correct server name prefix.*/
    void Start(std::string name, std::string dnsnode, unsigned int dnsport);

    /** implemented for symmetry*/
    void Stop();

    /** to use the callbacks for command and error handler functions*/
    void SetOwner(dimc::Registry* owner)
       {
          fOwner=owner;
       }


    void commandHandler();
    void errorHandler(int severity, int code, char *msg);
    void exitHandler( int code );
    void clientExitHandler();

    bool isStarted(){return fIsStarted;}

private:

    /** do not use ctor, use Instance*/
    Server();
    /** do not use dtor, use Delete*/
    virtual ~Server();

    /** singleton pattern.*/
    static Server* gInstance;

    /** the owning registry of this*/
    dimc::Registry* fOwner;

    bool fIsStarted;

    static dabc::Mutex gGlobalMutex;

};


class ServiceEntry
{
    public:

        /** ctor will create service from given dabc parameter*/
        ServiceEntry(dabc::Parameter *parameter, const std::string& dimname);

        /** ctor will create service from string expression*/
        ServiceEntry(const std::string& data, const std::string& dimname);

        virtual ~ServiceEntry();
        /** update DIM service; may update string buffer before*/
        void Update();

        /** Access to DimService name*/
        const std::string Name();

        /** Access to parameter reference*/
        dabc::Parameter* Parameter(){return fPar;}

        /** change value of registered parameter from DIM command*/
        void SetValue(const std::string val);

        void SetType(dimc::nameParser::recordtype t);
        dimc::nameParser::recordtype GetType();

        void SetVisibility(dimc::nameParser::visiblemask mask);
        dimc::nameParser::visiblemask GetVisibility();

        void SetStatus(dimc::nameParser::recordstat s);
        dimc::nameParser::recordstat GetStatus();

        dimc::nameParser::recordmode GetMode();
        void SetMode(dimc::nameParser::recordmode m);

    private:

       /* reference to exported parameter */
       dabc::Parameter* fPar;

       /* alternative reference to string for text export */
       std::string* fStringData;

       /** service associated with parameter/data reference*/
       ::DimService* fService;

       /** parser utility for name components and quality bits.
        * also keeps copy of current dim quality flag.
        * */
       dimc::nameParser fParser;

       char* fStringbuffer;



};



/**Class to subscribe dabc parameter to remote dim service. Subclass of DimInfo.
  * When remote dim server changes the subscribed variable value, the local dabc Parameter
  * will be updated using the corresponding method
  *  @author J. Adamczewski, GSI*/
class ParameterInfo :  public ::DimStampedInfo
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




            ParameterInfo(dabc::Parameter* par, const char* fullname, int nolinkdefault=-1)
                : DimStampedInfo(fullname, nolinkdefault),fxPar(par),fiType(INTEGER){;}

            ParameterInfo(dabc::Parameter* par, const char* fullname, float nolinkdefault=-1.)
                : DimStampedInfo(fullname, nolinkdefault),fxPar(par),fiType(FLOAT){;}

            ParameterInfo(dabc::Parameter* par, const char* fullname, double nolinkdefault=-1.)
                : DimStampedInfo(fullname, nolinkdefault),fxPar(par),fiType(DOUBLE){;}

            ParameterInfo(dabc::Parameter* par, const char* fullname, short nolinkdefault=-1)
                : DimStampedInfo(fullname, nolinkdefault),fxPar(par),fiType(SHORT){;}

            ParameterInfo(dabc::Parameter* par, const char* fullname, longlong nolinkdefault=-1)
                : DimStampedInfo(fullname, nolinkdefault),fxPar(par),fiType(LONGLONG){;}

            ParameterInfo(dabc::Parameter* par, const char* fullname, char* nolinkdefault=0)
                : DimStampedInfo(fullname, nolinkdefault),fxPar(par),fiType(STRING){;}

            ParameterInfo(dabc::Parameter* par, const char* fullname, void * nolink, int nolinksize=0)
                : DimStampedInfo(fullname, nolink, nolinksize),fxPar(par),fiType(STRUCTURE){;}

            virtual void infoHandler();

        bool HasParameter(dabc::Parameter* par) {return (par==fxPar);}


        private:
            /** backpointer to parameter that is to be updated*/
            dabc::Parameter* fxPar;
            /** type specifier for infoHandler*/
            parameterType fiType;


    };





} // end namespace dimc

#endif

