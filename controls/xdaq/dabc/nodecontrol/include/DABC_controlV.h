// $Id: DABC_controlV.h,v 1.5 2008/06/30 12:09:12 adamczew Exp adamczew $


//
// Version definition for DABCbase library
//
#ifndef _DABC_controlV_h_
#define _DABC_controlV_h_

#if __XDAQVERSION__  > 310
#include "config/PackageInfo.h"
#else
#include "PackageInfo.h"
#endif



// dummy include to force rebuild of version whenever something was changed
//#include "DABCApplication.h"
//#include "DABCDimServer.h"
//#include "DABCManager.h"
//#include "DABCRegistry.h"


namespace DabcXDAQControl
{
	const std::string package  =  "DABC Control";
	//const std::string versions =  "v0.2";
    const std::string versions = PACKAGE_VERSION_STRING(0,9,9);

	const std::string description = "Data Acquisition Backbone Core";
	const std::string link = "http://dabc.gsi.de";
    const std::string authors = "Joern Adamczewski-Musch, Experiment Electronics, GSI Darmstadt";
    const std::string summary ="Package providing runtime Application and controls framework for Data Acquisition Backbone Core system";
#if __XDAQVERSION__  > 310
    config::PackageInfo getPackageInfo();
	void checkPackageDependencies() throw (config::PackageInfo::VersionException);
#else
    toolbox::PackageInfo getPackageInfo();
	void checkPackageDependencies() throw (toolbox::PackageInfo::VersionException);
#endif
	std::set<std::string, std::less<std::string> > getPackageDependencies();
}

#endif
