#include "bnet/common.h"

#include <string.h>

const char* bnet::NetDevice = "SocketDevice";
bool bnet::NetBidirectional = true;
bool bnet::UseFileSource = false;

void bnet::NodesVector::Reset(const char* mask, int numnodes)
{
   fUsedNodes.clear();
   fNumNodes = 0;
   if (mask==0) return;
   
   
   if (numnodes<=0) numnodes = strlen(mask);
   
   fNumNodes = numnodes;
   
   int node = 0;
   
   while (node<fNumNodes) {
      if (mask==0) return;
      if (*mask=='x') fUsedNodes.push_back(node);
      mask++;
      node++;
   }
}

bool bnet::NodesVector::HasNode(int node) const
{
   for (unsigned n=0;n<fUsedNodes.size();n++)
      if (fUsedNodes[n]==node) return true;
   return false;
}
