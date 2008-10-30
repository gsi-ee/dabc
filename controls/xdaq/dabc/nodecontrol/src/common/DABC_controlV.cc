// $Id: DABC_controlV.cc,v 1.2 2008/06/13 08:13:15 adamczew Exp adamczew $






#if __XDAQVERSION__  > 310    
#include "toolbox/version.h"
#include "xdaq/version.h"
#else
#include "toolboxV.h"
#include "xdaqV.h"
#endif

#include "xoap/version.h"
#include "DABC_controlV.h"

#include <string>
#include <set>

GETPACKAGEINFO(DABC_Control)

#if __XDAQVERSION__  > 310  
void DABC_Control::checkPackageDependencies() throw (config::PackageInfo::VersionException)
#else 
void DABC_Control::checkPackageDependencies() throw (toolbox::PackageInfo::VersionException)
#endif
{
	CHECKDEPENDENCY(toolbox)
	CHECKDEPENDENCY(xdaq)
}

std::set<std::string, std::less<std::string> > DABC_Control::getPackageDependencies()
{
    std::set<std::string, std::less<std::string> > dependencies;
    ADDDEPENDENCY(dependencies,toolbox);
    ADDDEPENDENCY(dependencies,xoap);
    ADDDEPENDENCY(dependencies,xdaq);
    return dependencies;
}	
