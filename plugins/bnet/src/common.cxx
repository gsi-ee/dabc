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
#include "bnet/common.h"

#include <string.h>

const char* bnet::xmlClusterClass  = "bnet::Cluster";

const char* bnet::xmlIsRunning     = "IsRunning";
const char* bnet::xmlNetDevice     = "NetDevice";
const char* bnet::xmlWithController= "WithController";
const char* bnet::xmlNumEventsCombine = "NumEventsCombine";


const char* bnet::xmlIsGenerator   = "IsGenerator";
const char* bnet::xmlIsSender      = "IsSender";
const char* bnet::xmlIsReceiever   = "IsReceiver";
const char* bnet::xmlIsFilter      = "IsFilter";
const char* bnet::xmlCombinerModus = "CombinerModus";
const char* bnet::xmlNumReadouts   = "NumReadouts";
const char* bnet::xmlStoragePar    = "StoragePar";


const char* bnet::ReadoutPoolName      = "ReadoutPool";
const char* bnet::TransportPoolName    = "TransportPool";
const char* bnet::EventPoolName        = "EventPool";
const char* bnet::ControlPoolName      = "ControlPool";

const char* bnet::SenderThreadName     = "SenderThrd";
const char* bnet::ReceiverThreadName   = "ReceiverThrd";
const char* bnet::ControlThreadName    = "ControlThrd";


const char* bnet::xmlReadoutBuffer     = "ReadoutBuffer";
const char* bnet::xmlReadoutPoolSize   = "ReadoutPoolSize";
const char* bnet::xmlTransportBuffer   = "TransportBuffer";
const char* bnet::xmlTransportPoolSize = "TransportPoolSize";
const char* bnet::xmlEventBuffer       = "EventBuffer";
const char* bnet::xmlEventPoolSize     = "EventPoolSize";
const char* bnet::xmlCtrlBuffer        = "CtrlBuffer";
const char* bnet::xmlCtrlPoolSize      = "CtrlPoolSize";

const char* bnet::xmlFileBase          = "FileBase";


const char* bnet::CfgNodeId            = "CfgNodeId";
const char* bnet::CfgNumNodes          = "CfgNumNodes";
const char* bnet::CfgController        = "CfgController";
const char* bnet::CfgSendMask          = "CfgSendMask";
const char* bnet::CfgRecvMask          = "CfgRecvMask";
const char* bnet::CfgClusterMgr        = "CfgClusterMgr";
const char* bnet::CfgNetDevice         = "CfgNetDevice";
const char* bnet::CfgConnected         = "CfgConnected";
const char* bnet::CfgEventsCombine     = "CfgEventsCombine";
const char* bnet::CfgReadoutPool       = "CfgReadoutPool";

const char* bnet::parStatus            = "Status";
const char* bnet::parRecvStatus        = "RecvStatus";
const char* bnet::parSendStatus        = "SendStatus";

const char* bnet::parSendMask          = "SendMask";
const char* bnet::parRecvMask          = "RecvMask";


void bnet::NodesVector::Reset(std::string mask, int numnodes)
{
   fUsedNodes.clear();
   fNumNodes = 0;
   if (mask.length()==0) return;

   if (numnodes<=0) numnodes = mask.length();

   fNumNodes = numnodes;

   int node = 0;

   while (node<fNumNodes) {
      if (mask.length() < (unsigned) node) return;
      if (mask[node]=='x') fUsedNodes.push_back(node);
      node++;
   }
}

bool bnet::NodesVector::HasNode(int node) const
{
   for (unsigned n=0;n<fUsedNodes.size();n++)
      if (fUsedNodes[n]==node) return true;
   return false;
}
