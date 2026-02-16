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
#include "pex/Factory.h"
#include "pex/ReadoutModule.h"
#include "pex/Device.h"
#include "pex/Player.h"

#include "dabc/Command.h"
#include "dabc/logging.h"
#include "dabc/Manager.h"

dabc::FactoryPlugin pexfactory (new pex::Factory ("pex"));


// TODO: like example in http plugin, scan xml file for our frontend board object
// create them if existing and assign them to the module (must be existing before)
// internal parameters to be defined in the feb object ctor from xml

//void http::Factory::Initialize()
//{
//   if (dabc::mgr.null()) return;
//
//   // if dabc started without config file, do not automatically start http server
//   if (!dabc::mgr()->cfg() || (dabc::mgr()->cfg()->GetVersion()<=0)) return;
//
//   dabc::XMLNodePointer_t node = nullptr;
//
//   while (dabc::mgr()->cfg()->NextCreationNode(node, "HttpServer", true)) {
//
//      const char *name = dabc::Xml::GetAttr(node, dabc::xmlNameAttr);
//      const char *thrdname = dabc::Xml::GetAttr(node, dabc::xmlThreadAttr);
//
////      DOUT0("Found HttpServer node name = %s!!!", name ? name : "---");
//
//      std::string objname;
//      if (name) objname = name;
//      if (objname.empty()) objname = "/http";
//      if (objname[0]!='/') objname = std::string("/") + objname;
//      if (!thrdname || (*thrdname == 0)) thrdname = "HttpThread";
//
//      dabc::WorkerRef serv = new http::Civetweb(objname);
//      serv.MakeThreadForWorker(thrdname);
//      dabc::mgr.CreatePublisher();
//   }
//
//#ifndef DABC_WITHOUT_FASTCGI
//
//   while (dabc::mgr()->cfg()->NextCreationNode(node, "FastCgiServer", true)) {
//
//      const char *name = dabc::Xml::GetAttr(node, dabc::xmlNameAttr);
//      const char *thrdname = dabc::Xml::GetAttr(node, dabc::xmlThreadAttr);
//
////      DOUT0("Found FastCgiServer node name = %s!!!", name ? name : "---");
//
//      std::string objname;
//      if (name) objname = name;
//      if (objname.empty()) objname = "/fastcgi";
//      if (objname[0]!='/') objname = std::string("/") + objname;
//      if (!thrdname || (*thrdname == 0)) thrdname = "HttpThread";
//
//      dabc::WorkerRef serv = new http::FastCgi(objname);
//      serv.MakeThreadForWorker(thrdname);
//      dabc::mgr.CreatePublisher();
//   }
//
//#endif
//
//}

//dabc::Reference http::Factory::CreateObject(const std::string &classname, const std::string &objname, dabc::Command cmd)
//{
//   if (classname == "http::Server")
//      return new http::Server(objname, cmd);
//
//   if (classname == "http::Civetweb")
//      return new http::Civetweb(objname, cmd);
//
//   if (classname == "http::FastCgi")
//      return new http::FastCgi(objname, cmd);
//
//   return dabc::Factory::CreateObject(classname, objname, cmd);
//}






dabc::Module* pex::Factory::CreateModule (const std::string& classname, const std::string& modulename,
    dabc::Command cmd)
{
  DOUT0 ("pex::Factory::CreateModule called for class:%s, module:%s", classname.c_str (), modulename.c_str ());

//  if (strcmp (classname.c_str (), "pex::ReadoutModule") == 0)
    if (classname=="pex::ReadoutModule")
  {
    dabc::Module* mod = new pex::ReadoutModule (modulename, cmd);
    unsigned int boardnum = 0;    //cmd->GetInt(ABB_PAR_BOARDNUM, 0);
    DOUT1 ("pex::Factory::CreateModule - Created PEXOR Readout module %s for /dev/pexor-%d",
        modulename.c_str (), boardnum);
    return mod;
  }

  if (classname == "pex::Player")
        return new pex::Player(modulename, cmd);


  return dabc::Factory::CreateModule (classname, modulename, cmd);
}

dabc::Device* pex::Factory::CreateDevice (const std::string& classname, const std::string& devname,
    dabc::Command cmd)
{
  DOUT0 ("pex::Factory::CreateDevice called for class:%s, device:%s", classname.c_str (), devname.c_str ());

  if (strcmp (classname.c_str (), "pex::GenericDevice") != 0)
    return nullptr;

  dabc::Device* dev = new pex::GenericDevice (devname, cmd);

  return dev;
}


