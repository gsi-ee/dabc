#include "bnet/common.h"

#include <string.h>

const char* bnet::NetDevice = "dabc::SocketDevice";
bool bnet::NetBidirectional = true;
bool bnet::UseFileSource = false;

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
