#include "IbTestSchedule.h"

#include <stdio.h>
#include <cstdlib>
#include <cmath>

#include "dabc/Manager.h"
#include "dabc/Configuration.h"
#include "dabc/statistic.h"

typedef std::vector<std::string> StringsVector;



void Tokanize(std::string str, StringsVector& arr)
{
   while (str.length()>0) {

      while ((str.length()>0) && (str.find_first_of(" \n\t")==0)) str.erase(0, 1);
      if (str.length()==0) return;
      size_t pos = str.find_first_of(" \n\t");
      if (pos==str.npos) {
         arr.push_back(str);
         return;
      } else {
         arr.push_back(str.substr(0, pos));
         str.erase(0, pos);
      }
   }
}



IbTestClusterRouting::IbTestClusterRouting()
{
   fNumSpines = 12;

   for (int n1=0;n1<MaxNumNodes;n1++) {
      fMatrix[n1] = new IbTestRoute* [MaxNumNodes];
      for (int n2=0;n2<MaxNumNodes;n2++)
         fMatrix[n1][n2] = new IbTestRoute[MaxNumLids];
   }

   Reset();
}

IbTestClusterRouting::~IbTestClusterRouting()
{
   for (int n1=0;n1<MaxNumNodes;n1++) {
      for (int n2=0;n2<MaxNumNodes;n2++) delete[] fMatrix[n1][n2];
      delete[] fMatrix[n1];
      fMatrix[n1] = 0;
   }
}

void IbTestClusterRouting::Reset()
{
   fNumNodes = 0;
   fNumLids = 1;
   for (int n=0;n<MaxNumNodes;n++)
      for (int m=0;m<MaxNumNodes;m++)
         for (int l=0;l<MaxNumLids;l++)
            fMatrix[n][m][l].SetNull();
   fNodes.clear();
   fSwitchNames.clear();
}

int IbTestClusterRouting::FindNode(const std::string &name)
{
   NodesMap::iterator iter = fNodes.find(name);

   if (iter == fNodes.end()) return -1;

   return iter->second.id;
}


int IbTestClusterRouting::AddNode(const std::string &name, int lid, int* liddiff)
{
   NodesMap::iterator iter = fNodes.find(name);

   int id(-1), diff(0);

   if (iter == fNodes.end()) {
      id = fNumNodes;
      fNodes[name].id = id;
      fNodes[name].lid = lid;
      diff = 0;
      fNumNodes++;
   } else {
      id = iter->second.id;
      diff = lid - iter->second.lid;
      if ((diff < 0) || (diff >= MaxNumLids)) {
         EOUT("Something completely wrong with lid calculations lid = %d", diff);
         exit(5);
      }
   }

   if (diff>=fNumLids) fNumLids = diff+1;

   if (liddiff) *liddiff = diff;

   return id;
}

int IbTestClusterRouting::AddSwitch(const std::string &name)
{
   NamesMap::iterator iter = fSwitchNames.find(name);

   int id = -1;

   if (iter == fSwitchNames.end()) {
      id = fSwitchNames.size();
      fSwitchNames[name] = id;
   } else
      id = iter->second;

   return id;
}

int IbTestClusterRouting::FindSwitch(const std::string &name)
{
   NamesMap::iterator iter = fSwitchNames.find(name);

   if (iter == fSwitchNames.end()) return -1;

   return iter->second;
}

void IbTestClusterRouting::AddCSCSwitches(int nspines, int nleafs)
{
   for (int n=1;n<=nspines;n++)
      AddSwitch(dabc::format("ibspine%02d",n).c_str());
   for (int n=1;n<=nleafs;n++)
      AddSwitch(dabc::format("ibswitch%02d",n).c_str());

   fNumSpines = nspines;
}


bool IbTestClusterRouting::ReadFile(std::string filename)
{
   Reset();

   if (filename == "max") {
      GenerateFullTopology(36, false);
      return true;
   }

   if (filename == "maxlids") {
      GenerateFullTopology(36, true);
      return true;
   }

   if (filename == "min") {
      GenerateFullTopology(6, true);
      return true;
   }

   AddCSCSwitches(12, 35);

   FILE *f;
   char sbuf[100];

   f = fopen (filename.c_str() , "r");
   if (f == NULL) { EOUT("Error opening file %s", filename.c_str()); return false; }

   int cnt = 0;

   int numnull = 0;


   char name1[100], name2[100], sw1name[100], sw2name[100], sw3name[100];
   int lid1, lid2;

   while (fgets (sbuf, sizeof(sbuf), f)) {
      cnt++;

      int res = sscanf(sbuf, "%s -> %s %d -> %d %s %s %s", name1, name2, &lid1, &lid2, sw1name, sw2name, sw3name);

      if ((res<4) || (res==6)) {
         EOUT("Problem to decode string %s ", sbuf);
         continue;
      }

      int lid2diff(0);

      int node1 = AddNode(name1, lid1);
      int node2 = AddNode(name2, lid2, &lid2diff);

      switch (res) {
         case 4: // route is not defined
            fMatrix[node1][node2][lid2diff].SetNull();
            numnull++;
            break;

         case 5: // route is single switch
            fMatrix[node1][node2][lid2diff].Set1Hop(AddSwitch(sw1name));
            break;

         case 7: // route via three switches
            fMatrix[node1][node2][lid2diff].Set3Hop(AddSwitch(sw1name), AddSwitch(sw2name), AddSwitch(sw3name));
            break;

         default:
            EOUT("What???");
            break;
      }

   }

   fclose(f);

   DOUT0("read %d lines numnodes %d numswitches %d numnulls %d numlids %d", cnt, NumNodes(), NumSwitches(), numnull, NumLids());

   for (int n=0;n<NumNodes();n++)
      fMatrix[n][n][0].SetLocal();

   ReinjectOptimizedRoutes();

   RemoveUndefinedRoutes();

   return true;
}

void IbTestClusterRouting::ReinjectOptimizedRoutes()
{
   for (int n1=0; n1<NumNodes(); n1++) {
      // first reintroduce routes inside single switch

      for (int n2=0; n2<NumNodes(); n2++) {
         if (n2==n1) continue;

         IbTestRoute r12 = GetRoute(n1, n2);
         if (r12.GetNumHops()!=1) continue;

         IbTestRoute r21 = GetRoute(n2, n1);

         if (r21.IsNull())
           fMatrix[n2][n1][0] = r12;
         else
         if (r21 != r12) {
            EOUT("Missmatch of short routes %d <-> %d", n1, n2);
         }

         for (int n3=0; n3<NumNodes(); n3++) {
            if ((n3==n1) || (n3==n2)) continue;

            IbTestRoute r13 = GetRoute(n1, n3);

            if (r13.IsNull()) continue;

            IbTestRoute r23 = GetRoute(n2, n3);

            for (int lid=0;lid<NumLids();lid++) {
               if (r23.IsNull())
                  fMatrix[n2][n3][lid] = fMatrix[n1][n3][lid];
               else
                  if (fMatrix[n2][n3][lid] != fMatrix[n1][n3][lid])
                     EOUT("Missmatch of routes from %d, %d to %d with lid %d", n1, n2, n3, lid);
            }

         } // for n3

      } // for n2

   } // for n1
}

bool IbTestClusterRouting::CheckTargetRoutes()
{
   // top-level loop over leaf switches

   for (int sw=NumSpines(); sw<NumSwitches(); sw++) {
      // select all nodes which are connected to that switch

      typedef std::map<int, bool> IntMap;

      IntMap nodes;

      for (int n=0;n<NumNodes();n++) {
         int other = (n + NumNodes()/2) % NumNodes();
         IbTestRoute r = GetRoute(n, other);
         if (r.IsNull()) continue;
         if (r.GetHop1()==sw) nodes[n] = true;
      }

      for (IntMap::iterator iter1 = nodes.begin(); iter1!=nodes.end(); iter1++) {
         for (IntMap::iterator iter2 = iter1; iter2!=nodes.end(); iter2++) {
             int node1 = iter1->first;
             int node2 = iter2->first;
             if (node1==node2) continue;

             for (int node=0;node<NumNodes();node++) {
                if ((node==node1) || (node==node2)) continue;

                // exclude nodes from same switch
                if (nodes.find(node) != nodes.end()) continue;

                for (int lid=0; lid<NumLids(); lid++) {

                   IbTestRoute r1 = GetRoute(node1, node, lid);
                   IbTestRoute r2 = GetRoute(node2, node, lid);

                   if (!r1.IsNull() && !r2.IsNull() && (r1!=r2)) {
                      EOUT("Route %s differs from %s", GetRouteAsString(node1, node, lid).c_str(), GetRouteAsString(node2, node, lid).c_str());
                      return false;
                   }
                }
             }

         }
      }
   }

   return true;
}

int IbTestClusterRouting::CheckUsefulLIDs()
{
   int maxlid(1), minlid(NumLids());

   int hist[20];
   for (int n=0;n<20;n++) hist[n] = 0;

   int missing[NumSpines()];
   for (int n=0;n<NumSpines();n++) missing[n] = 0;

   for (int n1 = 0; n1 < NumNodes(); n1++)
      for (int n2 = 0; n2 < NumNodes(); n2++) {
         if (n1==n2) continue;

         if (GetRoute(n1,n2).GetNumHops() != 3) continue;

         int mask(0), lid(0);

         for (lid=0; lid <NumLids(); lid++) {
            IbTestRoute r = GetRoute(n1, n2, lid);
            if (r.GetNumHops() != 3) continue;

            mask |= (1 << r.GetHop2());

            if (mask == ((1 << NumSpines()) - 1)) break;
         }

         if (lid > maxlid) maxlid = lid;
         if (lid < minlid) minlid = lid;

         if (lid<20) hist[lid]++;

         for (int spine=0;spine<NumSpines();spine++)
            if ((mask & (1 << spine)) == 0)
               missing[spine]++;

//         if ((lid==16) && (hist[lid]%100 == 0))
//            DOUT0("Maximum LID for connection %s -> %s", NodeName(n1), NodeName(n2));
      }

   DOUT0("Maximum number of LIDs = %d minimum = %d", maxlid, minlid);

   for (int n=0;n<20;n++)
      if (hist[n]!=0)
         DOUT0("   Hist[%d] = %d", n, hist[n]);

   for (int spine=0;spine<NumSpines();spine++)
      if (missing[spine] > 0)
         DOUT0(" %s missing %d times", SwitchName(spine), missing[spine]);


   return maxlid;
}


void IbTestClusterRouting::GenerateFullTopology(int switchsize, bool manylids)
{
   if (switchsize==0) switchsize=36;

   if ((switchsize / 3) * 3 != switchsize) {
      EOUT("Switch size %d cannot be divided by three therefore half-fat tree cannot be build, use 6 as default");
      switchsize = 6;
   }

   AddCSCSwitches(switchsize/3, switchsize);

   int nodesperswitch = switchsize/3 * 2;

   for (int n=0;n<NumLeafs();n++)
      for (int m=0;m<nodesperswitch;m++)
        AddNode(dabc::format("node%02d-%02d", n + 1, m + 1).c_str());

   if (manylids) {
      fNumLids = NumSpines();
      if (fNumLids > MaxNumLids) fNumLids = MaxNumLids;
   } else
      fNumLids = 1;

   for (int n1 = 0; n1 < NumNodes(); n1++)
      for (int n2 = 0; n2 < NumNodes(); n2++)
         for (int lid = 0; lid < NumLids(); lid++) {
            if (n1==n2) { fMatrix[n1][n2][lid].SetLocal(); continue; }

            int sw1 = NumSpines() + n1 / nodesperswitch;
            int sw2 = NumSpines() + n2 / nodesperswitch;

            if (sw1==sw2) fMatrix[n1][n2][lid].Set1Hop(sw1);
                     else fMatrix[n1][n2][lid].Set3Hop(sw1, 0, sw2);
      }

   // here optimal distribution over spines will be done
   BuildOptimalRoutes();
}


const char* IbTestClusterRouting::NodeName(int nodeid) const
{
   NodesMap::const_iterator iter = fNodes.begin();
   while (iter != fNodes.end()) {
      if (iter->second.id == nodeid) return iter->first.c_str();
      iter++;
   }
   return "---";
}

const char* IbTestClusterRouting::SwitchName(int id) const
{
   NamesMap::const_iterator iter = fSwitchNames.begin();
   while (iter != fSwitchNames.end()) {
      if (iter->second == id) return iter->first.c_str();
      iter++;
   }
   return "---";
}


IbTestRoute IbTestClusterRouting::GetRoute(int node1, int node2, int lid) const
{
   IbTestRoute res;

   if ((node1<0) || (node1>=NumNodes()) || (node2<0) || (node2>=NumNodes()) ||
        (lid<0) || (lid>MaxNumLids))
      res.SetNull();
   else
      res = fMatrix[node1][node2][lid];

   return res;
}

std::string IbTestClusterRouting::GetRouteAsString(int node1, int node2, int lid) const
{
   IbTestRoute res = GetRoute(node1, node2, lid);

   switch (res.GetNumHops()) {
      case 0:
         return dabc::format("%3d -> %3d local", node1, node2);
      case 1:
         return dabc::format("%3d -> %3d  %s", node1, node2, SwitchName(res.GetHop1()));
      case 3:
         return dabc::format("%3d -> %3d  %s %s %s", node1, node2, SwitchName(res.GetHop1()), SwitchName(res.GetHop2()), SwitchName(res.GetHop3()));
   }

   return dabc::format("%3d -> %3d not exists", node1, node2);
}

void IbTestClusterRouting::ExcludeNode(int nodeid)
{
   if ((nodeid<0) || (nodeid>=NumNodes())) return;

   for (int nn = nodeid; nn < NumNodes()-1; nn++)
      for (int i=0;i<NumNodes();i++)
         for (int lid=0;lid<NumLids();lid++)
            fMatrix[nn][i][lid] = fMatrix[nn+1][i][lid];

   for (int nn = nodeid; nn < NumNodes()-1; nn++)
      for (int i=0;i<NumNodes()-1;i++)
         for (int lid=0;lid<NumLids();lid++)
            fMatrix[i][nn][lid] = fMatrix[i][nn+1][lid];

   NodesMap::iterator iter = fNodes.begin();
   while (iter != fNodes.end()) {
      if (iter->second.id == nodeid)
         fNodes.erase(iter++);
      else {
         if (iter->second.id > nodeid) iter->second.id--;
         iter++;
      }
   }
   fNumNodes--;
}


void IbTestClusterRouting::RemoveUndefinedRoutes()
{
   int max(0), pmax(0), nexcluded(0);

   std::vector<bool> usage;
   usage.resize(NumNodes(), true);

   do {

      max = 0; pmax = 0;
      for (int node = 0; node<NumNodes(); node++) {
         if (!usage[node]) continue;
         int cnt = 0;
         for (int nn = 0; nn < NumNodes(); nn++) {
            if ((nn==node) || !usage[nn]) continue;
            if (fMatrix[node][nn][0].IsNull()) cnt++;
            if (fMatrix[nn][node][0].IsNull()) cnt++;
         }
         if (cnt>max) { max = cnt; pmax = node; }
      }

      if (max>0) {
         DOUT0("Exclude node %d %s while it has %d nulls", pmax, NodeName(pmax), max);
         usage[pmax] = false;
         nexcluded++;
      }
   } while (max > 0);

   for (int nn=NumNodes()-1; nn>=0; nn--)
      if (!usage[nn]) ExcludeNode(nn);

   DOUT0("Excluded %d nodes Remained %d %u", nexcluded, NumNodes(), fNodes.size());
}

void IbTestClusterRouting::BuildOptimalRoutes()
{
   // it is supposed that first switches are spines

   IbTestIntMatrix paths(NumSwitches(), NumSwitches());

   paths.Fill(0);

   int spine = 0;

   // distribute initial pathes over all spines that not always first spine is used for start routing
   for (int sw1 = NumSpines(); sw1<NumSwitches(); sw1++)
      for (int sw2 = NumSpines(); sw2<NumSwitches(); sw2++) {
         if (sw1==sw2) continue;

         // for very first switch distribute spines sequentially
         if (sw1 == NumSpines()) {
            paths(sw1,sw2) = spine;
            spine = (spine+1) % NumSpines();
         } else
         if ((sw1-1 == sw2) && (sw1 == NumSpines() + 1)) {
            paths(sw1,sw2) = spine;
            spine = (spine+1) % NumSpines();
         } else
         if (sw1-1 != sw2) {
            paths(sw1,sw2) = (paths(sw1-1,sw2) + 1) % NumSpines();
         } else {
            paths(sw1,sw2) = (paths(sw1-2,sw2) + 1) % NumSpines();
         }
      }

   for (int n1 = 0; n1 < NumNodes(); n1++)
      for (int n2 = 0; n2 < NumNodes(); n2++) {
         if (n1==n2) continue;
         if (fMatrix[n1][n2][0].GetNumHops()!=3) continue;
         int sw1 = fMatrix[n1][n2][0].GetHop1();
         int sw2 = fMatrix[n1][n2][0].GetHop3();

         for (int lid = 0; lid < NumLids(); lid++)
            if ((fMatrix[n1][n2][lid].GetHop1() == sw1) && (fMatrix[n1][n2][lid].GetHop3() == sw2))
               fMatrix[n1][n2][lid].Set3Hop(sw1, (paths(sw1,sw2) + lid) % NumSpines(), sw2);

         paths(sw1,sw2) = (paths(sw1,sw2) + 1) % NumSpines();
      }
}


void IbTestClusterRouting::PrintSpineStatistic()
{
   IbTestIntColumn spines(NumSwitches());

   spines.Fill(0);

   IbTestIntMatrix switches[NumSpines()];

   for (int n=0;n<NumSpines();n++) {
      switches[n].SetSize(NumSwitches(), NumSwitches());
      switches[n].Fill(0);
   }

   int total = 0;

   for (int n1=0;n1<NumNodes();n1++)
      for (int n2=0;n2<NumNodes();n2++) {
         if (n1==n2) continue;
         IbTestRoute route = GetRoute(n1, n2);
         if (route.GetNumHops()!=3) continue;

         int spine = route.GetHop2();

         int sw1 = route.GetHop1();
         int sw2 = route.GetHop3();

         spines(spine)++;
         total++;

         if (spine<NumSpines()) switches[spine](sw1,sw2)++;
                  else { EOUT("WRONG SPINE %d", spine); return; }
      }


   for (int n=0;n<NumSwitches();n++)
      if ((spines(n)>0) && (total > 0))
         DOUT0("Name: %s cnt: %d rel : %5.3f", SwitchName(n), spines(n), 1.*spines(n)/total);

   DOUT0("Printout of complete matrix between switches");
   DOUT0("  maximum number of routes between two switches is 24x24 = 576");
   DOUT0("  maximum number of routes via single spine is 24x24/NumSpines() = 48");

   for (int loop=0;loop<2;loop++) {
      if (loop==1) DOUT0(" ============ only strange routes ============= ");

      for (int sw1=0;sw1<NumSwitches();sw1++)
         for (int sw2=0;sw2<NumSwitches();sw2++) {
            if (sw1==sw2) continue;

            int sum = 0;
            int max = 0;

            for (int n=0;n<NumSpines();n++) {
               sum+=switches[n](sw1,sw2);
               if (switches[n](sw1,sw2) > max) max = switches[n](sw1,sw2);
            }
            if ((sum==0) || ((loop==1) && (max<=48))) continue;

            std::string sbuf;

            // for (int n=0;n<NumSpines();n++) sbuf += dabc::format(" %5.2f", 100.*switches[n](sw1,sw2)/sum);

            for (int n=0;n<NumSpines();n++) {
               sbuf += dabc::format(" %2d", switches[n](sw1,sw2));
               if (switches[n](sw1,sw2)>48) sbuf+="*";
                                       else sbuf+=" ";
            }

            DOUT0("%s -> %s  %4d %s", SwitchName(sw1), SwitchName(sw2), sum, sbuf.c_str());
         }
   }



   DOUT0("");
   DOUT0("============================================================================");
   DOUT0("Printout of routes summary from single switch");
   DOUT0("  maximum number of routes from single switch is 24x%d x 24 = %d", NumLeafs()-1, 24*(NumLeafs()-1)*24);
   DOUT0("  maximum number of routes via single spine is 24x%d x 24/NumSpines() = %d", (NumLeafs()-1), 24*(NumLeafs())-1*24/NumSpines());


   for (int loop=0;loop<2;loop++) {
      if (loop==1) {
         DOUT0("=====================================================");
         DOUT0("Now with relative values");
      }

      for (int sw1=0;sw1<NumSwitches();sw1++) {

         int sum[NumSpines()];
         for (int n=0;n<NumSpines();n++) sum[n]=0;

         for (int sw2=0;sw2<NumSwitches();sw2++) {
            if (sw1==sw2) continue;
            for (int n=0;n<NumSpines();n++) sum[n]+=switches[n](sw1,sw2);
         }

         int total = 0;
         for (int n=0;n<NumSpines();n++) total+=sum[n];

         if (total==0) continue;

         std::string sbuf;

         for (int n=0;n<NumSpines();n++) {
            if (loop==0)
               sbuf += dabc::format(" %5d", sum[n]);
            else
               sbuf += dabc::format(" %5.3f", sum[n]/(24.*(NumLeafs()-1)*2));

            if (sum[n]>24*(NumLeafs()-1)*2) sbuf+="*"; else sbuf+=" ";
         }

         DOUT0(" %s ->  %5d %s", SwitchName(sw1), total, sbuf.c_str());
      }
   }

   DOUT0("");
   DOUT0("============================================================================");
   DOUT0("Printout of routes summary to single switch");
   DOUT0("  maximum number of routes to single switch is 24x%d x 24 = %d", (NumLeafs()-1), 24*(NumLeafs()-1)*24);
   DOUT0("  maximum number of routes via single spine is 24x%d x 24/NumSpines() = %d", (NumLeafs()-1), 24*(NumLeafs()-1)*24/NumSpines());

   for (int loop=0; loop<2; loop++) {
      if (loop==1) {
         DOUT0("=====================================================");
         DOUT0("Now with relative values");
      }

      for (int sw2=0;sw2<NumSwitches();sw2++) {

         int sum[NumSpines()];
         for (int n=0;n<NumSpines();n++) sum[n]=0;

         for (int sw1=0;sw1<NumSwitches();sw1++) {
            if (sw1==sw2) continue;
            for (int n=0;n<NumSpines();n++) sum[n]+=switches[n](sw1,sw2);
         }

         int total = 0;
         for (int n=0;n<NumSpines();n++) total+=sum[n];

         if (total==0) continue;

         std::string sbuf;

         for (int n=0;n<NumSpines();n++) {
            if (loop==0)
               sbuf += dabc::format(" %5d", sum[n]);
            else
               sbuf += dabc::format(" %5.3f", sum[n]/(24.*(NumLeafs()-1)*2));
            if (sum[n]>24*(NumLeafs()-1)*2) sbuf+="*"; else sbuf+=" ";
         }

         DOUT0(" -> %s  %5d %s", SwitchName(sw2), total, sbuf.c_str());
      }
   }
}

bool IbTestClusterRouting::MatchNodes(IbTestClusterRouting& other)
{
   int n1 = 0;
   int n2 = 0;

   while ((n1<NumNodes()) && (n2<other.NumNodes())) {
      const char* n1name = NodeName(n1);
      const char* n2name = other.NodeName(n2);
      if ((n1name==0) || (n2name==0)) { EOUT("Cannot define node name - FAILURE"); return false; }

      if (strcmp(n1name, n2name)==0) {
         n1++; n2++; continue;
      }

      int id1 = FindNode(n2name);
      int id2 = other.FindNode(n1name);

      if ((id1<0) && (id2<0)) {
         ExcludeNode(n1);
         other.ExcludeNode(n2);
         DOUT0("Rare case - nodes n1:%s n2:%s unique in their routes tables", n1name, n2name);
         continue;
      }

      if ((id1>=0) && (id2>=0)) {
         EOUT("Complete disorder of nodes n1:%s n2:%s - should resort them before matching roting tables", n1name, n2name);
         return false;
      }

      if (id1>=0) {
         if (id1<n1) EOUT("Disordering of nodes in first routing table %d %d %s %s", id1, n1, n1name, n2name);
         while (id1-->n1)
            ExcludeNode(n1);
      } else {
         if (id2<n2) EOUT("Disordering of nodes in second routing table %d %d %s %s", id2, n2, n1name, n2name);
         while (id2-->n2)
            other.ExcludeNode(n2);
      }
   }

   return true;
}

void IbTestClusterRouting::PrintDifferecne(IbTestClusterRouting& other)
{
   DOUT0("======================");

   DOUT0("Comparation of two routing tables with %d %d nodes", NumNodes(), other.NumNodes());

   if (NumNodes() != other.NumNodes()) {
      EOUT("Number of nodes mismatch");
      return;
   }

   if (NumSwitches() != other.NumSwitches()) {
      EOUT("Number of switches mismatch");
      return;
   }

   int numlids = NumLids();

   if (numlids != other.NumLids()) {
      EOUT("Number of lids mismatch %d %d", NumLids(), other.NumLids());
      if (numlids > other.NumLids()) numlids = other.NumLids();
   }

   for (int node = 0; node < NumNodes(); node++) {
      if (strcmp(NodeName(node), other.NodeName(node))!=0) {
         EOUT("Node %d names mismatch %s %s", node, NodeName(node), other.NodeName(node));
         return;
      }
   }

   for (int sw = 0; sw < NumSwitches(); sw++) {
      if (strcmp(SwitchName(sw), other.SwitchName(sw))!=0) {
         EOUT("Switch %d names mismatch %s %s", sw, SwitchName(sw), other.SwitchName(sw));
         return;
      }
   }

   int numdiff(0), numcnt(0);

   for (int n1=0;n1<NumNodes();n1++)
      for (int n2=0;n2<NumNodes();n2++) {
         if (n1==n2) continue;

         for (int lid=0;lid<numlids;lid++) {
            if (GetRoute(n1,n2,lid) != other.GetRoute(n1,n2,lid)) numdiff++;
            numcnt++;
         }
      }

   DOUT0("Overall number of differences are %d or %5.3f percent", numdiff, 100.*numdiff/numcnt);
   DOUT0("======================");
}


void IbTestClusterRouting::FindSameRouteTwice(bool bothlanes, const IbTestRoute& route0, bool rnd)
{
   std::map<IbTestRoute, int> routes;

   if (rnd) srand48(345);

   int random_shift = rnd ? lrint(rand_0_1() * (NumNodes()-1)) : 0;

   DOUT0("random_shift = %d rnd = %s", random_shift, DBOOL(rnd));

   for (int in1=0;in1<NumNodes();in1++)
      for (int in2=0;in2<NumNodes();in2++) {

         int n1 = (in1 + random_shift) % NumNodes();
         int n2 = (in2 + random_shift) % NumNodes();

         if (n1==n2) continue;

         IbTestRoute route1 = GetRoute(n1, n2);

         IbTestRoute route2, rev_route1, rev_route2;

         std::map<int, bool> nodes; // map of used nodes
         std::map<int, bool> switches; // map of used switches

         if (!route0.IsNull() && (route0!=route1)) continue;

         if (routes.find(route1) != routes.end()) continue;

         // we are interested only in long routes
         if (route1.GetNumHops()!=3) continue;

         bool find = false;

         nodes[n1] = true;
         nodes[n2] = true;
         switches[route1.GetHop3()] = true;

         int dirk1(-1), dirk2(-1);

         for (int k1=0;(k1<NumNodes()) && !find;k1++) if (nodes.find(k1)==nodes.end())
            for (int k2=0;(k2<NumNodes()) && !find;k2++) if (nodes.find(k2)==nodes.end()) {
               if (k1==k2) continue;

               route2 = GetRoute(k1, k2);

               if (bothlanes)
                  find = (route1==route2);
               else
                  find = (route2.GetHop1() == route1.GetHop1()) &&
                         (route2.GetHop2() == route1.GetHop2()) &&
                          switches.find(route2.GetHop3())==switches.end();


               if (find) {
                  routes[route1] = 1;
                  nodes[k1] = true;
                  nodes[k2] = true;
                  dirk1 = k1;
                  dirk2 = k2;
                  switches[route2.GetHop3()] = true;
               }
            }

         // now let try to find route in reverse direction

         IbTestRoute rev_route;
         rev_route.Set3Hop(route1.GetHop3(), route1.GetHop2(), route1.GetHop1());

         int revn1(-1), revn2(-1);

         find = false;
         for (int k1=0;(k1<NumNodes()) && !find;k1++) if (nodes.find(k1)==nodes.end())
            for (int k2=0;(k2<NumNodes()) && !find;k2++) if (nodes.find(k2)==nodes.end()) {
               if (k1==k2) continue;

               rev_route1 = GetRoute(k1, k2);

               if (bothlanes)
                  find = (rev_route1==rev_route);
               else
                  find = (rev_route1.GetHop3() == route1.GetHop1()) &&
                         (rev_route1.GetHop2() == route2.GetHop2()) &&
                          switches.find(rev_route1.GetHop1())==switches.end();

               if (find)  {
                  revn1 = k1;
                  revn2 = k2;
                  nodes[k1] = true;
                  nodes[k2] = true;
                  switches[rev_route1.GetHop1()]=true;
               }
            }

         int revk1(-1), revk2(-1);

         find = false;
         for (int k1=0;(k1<NumNodes()) && !find;k1++) if (nodes.find(k1)==nodes.end())
            for (int k2=0;(k2<NumNodes()) && !find;k2++) if (nodes.find(k2)==nodes.end()) {
               if (k1==k2) continue;

               rev_route2 = GetRoute(k1, k2);

               if (bothlanes)
                  find = (rev_route2==rev_route);
               else
                  find = (rev_route2.GetHop3() == route1.GetHop1()) &&
                         (rev_route2.GetHop2() == route2.GetHop2()) &&
                          switches.find(rev_route2.GetHop1())==switches.end();

               if (find) {
                  revk1 = k1;
                  revk2 = k2;
                  nodes[k1] = true;
                  nodes[k2] = true;
                  switches[rev_route2.GetHop1()]=true;
                  break;
               }
            }

         if ((dirk1>=0) && (dirk2>=0) && (revn1>=0) && (revn2>=0) && (revk1>=0) && (revk2>=0)) {
            DOUT0("%s -> %s : %s -> %s -> %s",
                  NodeName(n1), NodeName(n2),
                  SwitchName(route1.GetHop1()), SwitchName(route1.GetHop2()), SwitchName(route1.GetHop3()));
            DOUT0("%s -> %s : %s -> %s -> %s",
                  NodeName(dirk1), NodeName(dirk2),
                  SwitchName(route2.GetHop1()), SwitchName(route2.GetHop2()), SwitchName(route2.GetHop3()));
            DOUT0("%s -> %s : %s -> %s -> %s",
                  NodeName(revn1), NodeName(revn2),
                  SwitchName(rev_route1.GetHop1()), SwitchName(rev_route1.GetHop2()), SwitchName(rev_route1.GetHop3()));
            DOUT0("%s -> %s : %s -> %s -> %s",
                  NodeName(revk1), NodeName(revk2),
                  SwitchName(rev_route2.GetHop1()), SwitchName(rev_route2.GetHop2()), SwitchName(rev_route2.GetHop3()));

            DOUT0("List for test where bothlanes = %s:", DBOOL(bothlanes));
            DOUT0("  %s", NodeName(n1));
            DOUT0("  %s", NodeName(n2));
            DOUT0("  %s", NodeName(dirk1));
            DOUT0("  %s", NodeName(dirk2));
            DOUT0("  %s", NodeName(revn1));
            DOUT0("  %s", NodeName(revn2));
            DOUT0("  %s", NodeName(revk1));
            DOUT0("  %s", NodeName(revk2));

            return;
         }

      }
}

void IbTestClusterRouting::FillBadIdsFor2Switches(IbTestIntColumn& column) const
{
   column.Fill(-1);
   if ((column.size()!=NumNodes()) || (NumNodes() % 2 != 0)) {
      EOUT("One should have exact number of nodes in ID array");
      FillRandomIds(column);
      return;
   }

   for (int n=0;n<NumNodes()/2;n++) {
      column(n*2) = n;
      column(n*2+1) = n + NumNodes()/2;
   }

}

void IbTestClusterRouting::FillInteractiveNodes(IbTestIntColumn& column) const
{
   column.Fill(-1);
   int cnt = 0;

   for (int n=0; n<NumNodes(); n++) {
      const char* name = NodeName(n);
      if (name==0) continue;

      if (strstr(name,"node")==name)
         column(cnt++) = n;

      if (cnt==column.size()) break;
   }

   column.SetSize(cnt);

   DOUT0("Selected are %d interactive nodes", column.size());
}



void IbTestClusterRouting::FillRandomIds(IbTestIntColumn& column) const
{
   column.Fill(-1);

   int cnt = 0;

   int ntry = 0;

   while ((cnt<column.size()) && (ntry++ < column.size()*1000)) {

      int rand_num = lrint(rand_0_1()*NumNodes());

      if ((rand_num<0) || (rand_num>=NumNodes())) continue;

      if (!column.Find(rand_num))
         column(cnt++) = rand_num;
   }

   if (cnt<column.size()) {
      EOUT("Was not able to fill random number in so many iterations - help it");

      for (int n=0; n<NumNodes(); n++)
         if ((cnt<column.size()) && !column.Find(n)) {
            EOUT("Add node %d", n);
            column(cnt++) = n;
         }
   }
}

int IbTestClusterRouting::GetNodeSwitch(int node)
{
   if ((node<0) || (node>NumNodes()) || (NumNodes()<2)) return -1;

   IbTestRoute route = GetRoute(node, node==0 ? 1 : 0);

   if (route.GetNumHops()<1) return -1;

   return route.GetHop1();
}


bool IbTestClusterRouting::SelectNodes(const std::string &all_args, IbTestIntColumn& ids)
{
   ids.SetSize(NumNodes());
   ids.Fill(-1);

   std::vector<std::string> args;
   Tokanize(all_args, args);

   if (args.size()==0) { ids.SetSize(0); return false; }

   int cnt(0), find(-1);

   for (unsigned n=0;n<args.size();n++) {
      if (args[n] == "cpu") {
         EOUT("cpu argument appears here - failure");
         continue;
      } else

      if (args[n] == "badids") {
         FillBadIdsFor2Switches(ids);
         break;
      } else
      if (args[n] == "nodes") {
         FillInteractiveNodes(ids);
         cnt = ids.size();
      } else
      if (args[n] == "selnodes")  {
         int newcnt = 0;
         for (int n=0;n<cnt;n++)
            if (strncmp(NodeName(ids(n)),"node", 4)==0)
               ids(newcnt++) = ids(n);
         cnt = newcnt;
      } else
      if (args[n] == "addnodes")  {
         for (int id=0;id<NumNodes();id++) {
            if (ids.Find(id)) continue;
            if (strncmp(NodeName(id),"node", 4)==0)
               ids(cnt++) = id;
         }
      } else
      if (args[n] == "addquads")  {
         for (int id=0;id<NumNodes();id++) {
            if (ids.Find(id)) continue;
            if (strncmp(NodeName(id),"quad", 4)==0)
               ids(cnt++) = id;
         }
      } else
      if (args[n].compare(0, 8, "ibswitch")==0) {
         int swid = FindSwitch(args[n]);
         if (swid<0) { EOUT("Cannot find switch of name %s", args[n].c_str()); continue; }

         for (int node=0;node<NumNodes();node++)
            if ((GetNodeSwitch(node)==swid) && !ids.Find(node))
               ids(cnt++) = node;

      } else
      if (args[n].compare(0, 10, "noibswitch")==0) {
         // exclude all nodes, connected to specified switch
         int swid = FindSwitch(args[n].c_str()+2);
         if (swid<0) { EOUT("Cannot find switch of name %s", args[n].c_str()+2); continue; }

         int indx = -1;

         for (int node=0;node<NumNodes();node++)
            if ((GetNodeSwitch(node)==swid) && ((indx = ids.FindIndex(node))>=0)) {
               ids.Remove(indx);
               cnt--;
            }
      } else

      if ((find = FindNode(args[n])) >= 0) {
         if (!ids.Find(find))
            ids(cnt++) = find;

      } else {
         EOUT("Cannot interpret argument %s", args[n].c_str());
      }
   }

   if (cnt>0) ids.SetSize(cnt);

   return ids.size()>0;
}


bool IbTestClusterRouting::SaveNodesList(const std::string &fname, const IbTestIntColumn& ids)
{
   FILE *f = fopen (fname.c_str(), "w");

   if (f==0) {
      EOUT("Cannot open file %s for writing", fname.c_str());
      return false;
   }

   for (int n=0;n<ids.size();n++)
      fprintf(f,"%s\n", NodeName(ids(n)));

   fclose(f);

   return true;
}

bool IbTestClusterRouting::LoadNodesList(const std::string &fname, IbTestIntColumn& ids)
{

   FILE *f = fopen (fname.c_str(), "r");

   if (f==0) {
      EOUT("Cannot open file %s for reading", fname.c_str());
      return false;
   }

   ids.SetSize(NumNodes());
   int cnt(0);

   char sbuf[100];

   while (fgets (sbuf, sizeof(sbuf), f)) {

      char* pos = strchr(sbuf, '\n');
      if (pos!=0) *pos = 0;

      int id = FindNode(sbuf);

      if (id>=0)
         ids(cnt++) = id;
      else
         EOUT("Node %s not found", sbuf);
   }

   ids.SetSize(cnt);

   fclose(f);

   return true;
}




// _________________________________________________________________________________

IbTestSchedule::IbTestSchedule()
{
   fSchedule = 0;
   fTimeSlots = 0;

   fNumSenders = 0;
   fMaxNumSlots = 0;
   fNumSlots = 0;

}


IbTestSchedule::IbTestSchedule(int MaxNumSlots, int NumSenders)
{
  fSchedule = 0;
  fTimeSlots = 0;

  fNumSenders = 0;
  fMaxNumSlots = 0;
  fNumSlots = 0;

  Allocate(MaxNumSlots, NumSenders);
}

IbTestSchedule::~IbTestSchedule()
{
   Allocate(0, 0);
}

void IbTestSchedule::Allocate(int maxnumslots, int numsend)
{
   if (fSchedule!=0) {
     for(int n=0;n<fMaxNumSlots;n++)
       delete[] fSchedule[n];
     delete[] fSchedule;
     fSchedule = 0;
   }

   if (fTimeSlots!=0) {
      delete[] fTimeSlots;
      fTimeSlots = 0;
   }

   fNumSenders = numsend;
   fMaxNumSlots = maxnumslots;
   fNumSlots = maxnumslots;

   if ((fNumSenders>0) && (fMaxNumSlots>0)) {
      fSchedule = new IbTestScheduleItem* [fMaxNumSlots];
      for(int n=0;n<fMaxNumSlots;n++)
         fSchedule[n] = new IbTestScheduleItem[fNumSenders];

      // allocate one element more to set end time, when schedule need to be repeated
      fTimeSlots = new double[fMaxNumSlots+1];
   }

   Clear();
}


void IbTestSchedule::Clear()
{
   if (numSlots()==0) return;

   for (int nslot=0; nslot<numSlots(); nslot++) {
      IbTestScheduleItem* slot = getScheduleSlot(nslot);
      for (int nsend=0;nsend<numSenders();nsend++) slot[nsend].Reset();
      SetTimeSlot(nslot, 0);
   }
   SetTimeSlot(numSlots(), 0);
}

void IbTestSchedule::FillReceiveSchedule(IbTestSchedule& recv)
{
   recv.Allocate(numSlots(), numSenders());

   for (int nslot=0; nslot<numSlots(); nslot++) {
      IbTestScheduleItem* slot = getScheduleSlot(nslot);
      IbTestScheduleItem* slot2 = recv.getScheduleSlot(nslot);

      recv.SetTimeSlot(nslot, timeSlot(nslot));

      for (int nsend=0;nsend<numSenders();nsend++)
         if (!slot[nsend].Empty()) {
            int recv = slot[nsend].node;

            if (slot2[recv].Empty()) {
               slot2[recv].node = nsend;
               slot2[recv].lid = slot[nsend].lid;
            } else {
               EOUT("In slot %d more than 1 sender to node ", nslot, recv);
            }
         }
   }

   recv.SetEndTime(endTime());
}

bool IbTestSchedule::IsNodeActive(int node)
{
   if ((node<0) || (node>=numSenders())) return false;
   for (int nslot=0; nslot<numSlots(); nslot++)
      if (!IsEmptyItem(nslot, node)) return true;
   return false;
}

void IbTestSchedule::ExcludeInactiveNode(int node)
{
   if ((node<0) || (node>=numSenders())) return;
   for (int nslot=0; nslot<numSlots(); nslot++) {
      IbTestScheduleItem* slot = getScheduleSlot(nslot);

      slot[node].Reset();

      for (int nsend=0;nsend<numSenders();nsend++)
         if (slot[nsend].node == node) slot[nsend].Reset();
   }
}

bool IbTestSchedule::ShiftToNextOperation(int node, double& basetm, int& nslot)
{
   int cnt(0);

   do {
      nslot++;

      if (nslot==numSlots()) {
         basetm += endTime();
         nslot = 0;
      }

      // no one operation found
      if (cnt++ > 3*numSlots()) return false;

   } while (IsEmptyItem(nslot, node));

   return true;
}


IbTestScheduleItem* IbTestSchedule::getScheduleSlot(int nslot)
{
   if ((nslot<0) || (nslot>=fMaxNumSlots)) return 0;
   return fSchedule[nslot];
}

double IbTestSchedule::timeSlot(int nslot)
{
   if ((nslot<0) || (nslot>fMaxNumSlots) || (fTimeSlots==0)) return 0.;
   return fTimeSlots[nslot];
}

void IbTestSchedule::SetTimeSlot(int nslot, double tm)
{
   if ((nslot>=0) && (nslot<=fMaxNumSlots) && fTimeSlots) fTimeSlots[nslot] = tm;
}


double IbTestSchedule::totalTime()
{
  double res = 0;
  for(int nslot=0;nslot<numSlots();nslot++)
     res += timeSlot(nslot);
  return res;
}

double IbTestSchedule::calcOccupation()
{
   return (1.* totalTime() / numSenders() / IbTestMatrixMean - 1.)*100.;
}

double IbTestSchedule::calcBandwidth(IbTestIntMatrix* matr)
{
   double time = totalTime();
   if ((time<=0) || (matr==0)) return 0;
   double sum = 0;
   for(int nslot=0;nslot<numSlots();nslot++) {
     IbTestScheduleItem* slot_sch = getScheduleSlot(nslot);

     for(int nsend=0;nsend<numSenders();nsend++) {
        int nrecv = slot_sch[nsend].node;
        if (nrecv>=0) sum+=(*matr)(nsend,nrecv);
     }
   }

   return sum/numSenders()/time * 100.;
}

void IbTestSchedule::Print(IbTestIntMatrix* matr)
{
   DOUT0("Print out of schedule:");

   for(int nslot=0;nslot<numSlots();nslot++) {
      DOUT0("Slot %d time %6.5f s", nslot, timeSlot(nslot));

      IbTestScheduleItem* slot_sch = getScheduleSlot(nslot);

      for(int nsend=0;nsend<numSenders();nsend++) {
         int nrecv = slot_sch[nsend].node;
         if (nrecv>=0)
            DOUT0("  Sender %d sends to %d element = %d", nsend, nrecv, matr ? (*matr)(nsend,nrecv) : 1);
         else
            DOUT0("  Sender %d sends nothing", nsend);
     }
   }

   DOUT0("Schedule end time %6.5f  number of slots %d occupation = + %4.1f %% bandwidth = %5.1f %%",
            endTime(), numSlots(), calcOccupation(), calcBandwidth(matr));
}


void IbTestSchedule::FillRoundRoubin(IbTestIntColumn* ids, double schstep)
{

   int numsenders = ids ? ids->size() : numSenders();

   SetNumSlots(numsenders-1);

   for(int nslot=0;nslot<numSlots();nslot++) {

      SetTimeSlot(nslot, schstep*nslot);

      IbTestScheduleItem* slot_sch = getScheduleSlot(nslot);

      for(int nsend=0;nsend<numSenders();nsend++)
         slot_sch[nsend].Reset();

      for(int nsend=0;nsend<numsenders;nsend++) {
         int nrecv = (nsend + nslot + 1) % numsenders;

         if (ids==0)
            slot_sch[nsend].node = nrecv;
         else
            slot_sch[(*ids)(nsend)].node = (*ids)(nrecv);
      }
   }

   SetEndTime(numSlots()*schstep);
}

bool IbTestSchedule::BuildOptimized(IbTestClusterRouting& routing, IbTestIntColumn* ids, bool show)
{
   int numids = ids ? ids->size() : numSenders();

   /** matrix identified if transfer from one node to another should be done */
   IbTestBoolMatrix transfer(numSenders(), numSenders());
   transfer.Fill(false);

   IbTestBoolColumn receivers(numSenders());

   IbTestIntMatrix occup(routing.NumSwitches(), routing.NumSwitches());

   fNumSlots = 0;

   while (numSlots() < maxNumSlots()) {
      /* first check if we should transfer something */

      bool isanytransfer = false;

      for (int n=0;n<numids;n++) {
         int sender = ids ? (*ids)(n) : n;
         for (int m=0;m<numids;m++) {
            int receiver = ids ? (*ids)(m) : m;
            if ((sender!=receiver) && !transfer(sender,receiver)) isanytransfer = true;
         }
      }

      if (!isanytransfer) {
         if (show)
            DOUT0("Build optimized schedule for %d nodes with %d slots", numids, numSlots());

         return true;
      }

      // from here we start to fill new time slot into the schedule

      IbTestScheduleItem* slot = getScheduleSlot(fNumSlots++);
      if (slot==0) {
         EOUT("No more slots");
         return false;
      }

      // first specify that none of sender do something
      for (int n=0;n<numSenders();n++) slot[n].Reset();
      receivers.Fill(false);
      occup.Fill(0);

      for (int loop=0; loop < 2; loop++)
         // when loop == 0 we try to find connection with longest routes
         // when loop == 1 we just try to find any connection

      for (int nsend = 0; nsend<numids; nsend++) {

         int sender = (nsend + numSlots()) % numids;

         if (ids) sender = (*ids)(sender);

         if (!slot[sender].Empty()) continue;

         for (int nrecv = 0; nrecv < numids; nrecv++) {

            int receiver = (nrecv + numSlots()) % numids;
            if (ids) receiver = (*ids)(receiver);

            // check if receiver already getting data in this slot or we already transfer packet to that receiver
            if ((sender == receiver) || receivers(receiver) || transfer(sender, receiver)) continue;

            for (int lid=0;lid<routing.NumLids();lid++) {

               IbTestRoute route = routing.GetRoute(sender, receiver, lid);

               if (route.IsNull()) break;

               if (loop==0) {
                   // on the first loop consider only route over three hopes
                   if (route.GetNumHops() != 3) break;

                   // if route causes congestion, ignore
                   if ((occup(route.GetHop1(), route.GetHop2()) > 1) ||
                       (occup(route.GetHop2(), route.GetHop3()) > 1)) continue;

                   occup(route.GetHop1(), route.GetHop2())++;
                   occup(route.GetHop2(), route.GetHop3())++;
               } else {
                   // at the second loop only short routes will be considered
                   if (route.GetNumHops() != 1) break;
               }

               // mark that we send packet to that node and this node is already occupied
               receivers(receiver) = true;
               transfer(sender, receiver) = true;

               // schedule itself should be mark with correct ids
               slot[sender].node = receiver;
               slot[sender].lid = lid;

               break;
            } // loop over lids

            // if slot entry is filled, no need to check other senders
            if (!slot[sender].Empty()) break;

         } // loop over nrecv

      } // loop over nsend
   } // loop over slots

   EOUT("It was not possible to produce schedule inside reserved number of slots %d", maxNumSlots());
   return false;
}

bool IbTestSchedule::BuildRegularSchedule(IbTestClusterRouting& routing, IbTestIntColumn* ids, bool show)
{
   // all parameters of regular half-fat tree network are define by number of spines

   int regular_nodesperswitch = 2 * routing.NumSpines();

   int regular_numleafs = 3 * routing.NumSpines();

   int regular_numnodes = regular_nodesperswitch * regular_numleafs;

   int regular_num_slots = regular_numnodes;

   IbTestIntMatrix coding(regular_numleafs, regular_nodesperswitch);
   coding.Fill(-1);

   int numids = ids ? ids->size() : routing.NumNodes();

   for (int id=0; id<numids; id++) {

      int node = ids ? (*ids)(id) : id;

      // no need to analyze negative node ids
      if (node<0) continue;

      int othernode = (id + numids/2) % numids;
      if (ids) othernode = (*ids)(othernode);

      IbTestRoute route = routing.GetRoute(node, othernode);

      if (route.IsNull()) {
         EOUT("No route between %d %d ", node, othernode);
         return false;
      }

      int leaf = route.GetHop1() - routing.NumSpines();
      if ((leaf<0) || (leaf>=regular_numleafs)) {
         EOUT("Leaf %d number for node %d out of regular range 0..%d", leaf, node, regular_numleafs);
         return false;
      }

      bool placed = false;

      for (int n=0;n<regular_nodesperswitch;n++) {
         if (coding(leaf, n) < 0) {
            coding(leaf, n) = node;
            placed = true;
            break;
         }
      }

      if (!placed) {
         EOUT("Cannot placed node %d in regular scheme - too many nodes on switch %d", node, leaf);
         return false;
      }
   }

   fNumSlots = regular_num_slots;

   for (int nslot=0; nslot < regular_num_slots; nslot++) {
      IbTestScheduleItem* slot = getScheduleSlot(nslot);
      if (slot==0) {
         EOUT("No more slots nslot = %d NumSlots = %d Max = %d", nslot, numSlots(), maxNumSlots());
         return false;
      }

      // reset slot at the beginning with normal indexes
      for (int indx=0;indx<numSenders(); indx++) slot[indx].Reset();

      int super_slot = nslot / regular_nodesperswitch;
      int super_subslot = nslot % regular_nodesperswitch;

      for (int nsend = 0; nsend < regular_numnodes; nsend++) {

         int sender_switch = nsend / regular_nodesperswitch;

         int sender_sub_id = nsend % regular_nodesperswitch;

         int spine = (sender_sub_id + super_slot) % routing.NumSpines();

         int receiver_switch = (sender_switch + sender_sub_id + super_slot) % regular_numleafs;

         int receiver_subid = (sender_sub_id + super_subslot) % regular_nodesperswitch;

         int nrecv = receiver_switch * regular_nodesperswitch + receiver_subid;

         if (nrecv!=nsend) {

            int code_nsend = coding(sender_switch, sender_sub_id);
            int code_nrecv = coding(receiver_switch, receiver_subid);

            if ((code_nsend<0) || (code_nrecv<0)) continue;

            slot[code_nsend].node = code_nrecv;
            slot[code_nsend].lid = -1;

            for (int lid=0;lid<routing.NumLids();lid++) {
               IbTestRoute route = routing.GetRoute(code_nsend, code_nrecv, lid);
               if (route.GetNumHops()==1) {
                  slot[code_nsend].lid = lid;
                  break;
               }

               if ((route.GetNumHops()==3) && (route.GetHop2()==spine)) {
                  slot[code_nsend].lid = lid; break;
               }
            }
//            if (slot[code_nsend].lid<0)
//               EOUT("Did not found LID for spine %d ", spine);
         }
      }
   }

   // from here normal indexes are used, one could forget about recoding

   IbTestIntMatrix occup(routing.NumSwitches(), routing.NumSwitches());

   // loop over all entries to check for bad slots - when lid<0
   for (int nslot=0; nslot < regular_num_slots; nslot++) {
      IbTestScheduleItem* slot = getScheduleSlot(nslot);
      for (int nsend = 0; nsend < numSenders(); nsend++) {
         if (slot[nsend].Empty() || (slot[nsend].lid>=0)) continue;

         // this is bad slot, we should try to put it inside new slots

         bool need_new_slot = true;

         for (int nslotnew = regular_num_slots; nslotnew<numSlots(); nslotnew++) {
            IbTestScheduleItem* newslot = getScheduleSlot(nslotnew);
            if (newslot==0) return false;
            if (!newslot[nsend].Empty()) continue;

            bool to_next_slot = false;

            occup.Fill(0);

            // check one of new slots if it has empty place for that receiever and count occupation
            for (int nsend1 = 0; nsend1 < numSenders(); nsend1++) {
               if (newslot[nsend1].Empty()) continue;
               if (newslot[nsend1].node == slot[nsend].node) {
                  to_next_slot = true;
                  break;
               }

               IbTestRoute route = routing.GetRoute(nsend1, newslot[nsend1].node, newslot[nsend1].lid);

               if (route.GetNumHops()==3) {
                  occup(route.GetHop1(), route.GetHop2())++;
                  occup(route.GetHop2(), route.GetHop3())++;
               }
            }

            if (to_next_slot) continue;

            for (int lid=0;lid<routing.NumLids();lid++) {
               IbTestRoute route = routing.GetRoute(nsend, slot[nsend].node, lid);
               if (route.IsNull()) continue;

               if (route.GetNumHops()==1) {
                  newslot[nsend].node = slot[nsend].node;
                  newslot[nsend].lid = lid;
                  need_new_slot = false;
                  break;
               }

               if ((occup(route.GetHop1(), route.GetHop2())<2) && (occup(route.GetHop2(), route.GetHop3())<2)) {
                  newslot[nsend].node = slot[nsend].node;
                  newslot[nsend].lid = lid;
                  need_new_slot = false;
                  break;
               }
            }

            if (!need_new_slot) break;

         }

         if (need_new_slot) {
            // add new slot for our sender

            IbTestScheduleItem* newslot = getScheduleSlot(fNumSlots++);
            if (newslot==0) return false;
            for (int nsend1 = 0; nsend1 < numSenders(); nsend1++) newslot[nsend1].Reset();

            newslot[nsend].node = slot[nsend].node;
            newslot[nsend].lid = 0;
         }

         // remove bad entry
         slot[nsend].Reset();

      } // loop over senders
   } // loop over nslot

   return true;
}


void IbTestSchedule::PrintSlotWithRouting(const IbTestClusterRouting& routing, int nslot)
{
   if ((nslot<0) || (nslot >= numSlots())) return;

   IbTestScheduleItem* slot = getScheduleSlot(nslot);
   if (slot==0) return;

   int cnt = 0;
   for (int nsend=0;nsend<numSenders();nsend++)
      if (!slot[nsend].Empty()) cnt++;

   DOUT0("  Slot %d has %d items", nslot, cnt);

   cnt = 0;
   for (int nsend=0;nsend<numSenders();nsend++)
      if (!slot[nsend].Empty()) {
         std::string str = routing.GetRouteAsString(nsend, slot[nsend].node, slot[nsend].lid);
         DOUT0("    Item: %d  %s", cnt++, str.c_str());
      }
}


void IbTestSchedule::PrintWithRouting(const IbTestClusterRouting& routing, int nlast)
{
   DOUT0("Schedule for %d senders has %d slots", numSenders(), numSlots());

   for (int nslot = 0; nslot < numSlots(); nslot++) {
      if ((nlast>0) && (nslot < numSlots() - nlast)) continue;

      PrintSlotWithRouting(routing, nslot);
   }

}

bool IbTestSchedule::CanMoveItemTo(const IbTestClusterRouting& routing, int nsender, int nslot1, int nslot2, int& newlid)
{
//   bool dodebug = ((nslot1==616) && (nsender==638)) ||
//         ((nslot1==837) && (nsender==635)) ||
//         ((nslot1==838) && (nsender==626)) ||
//         ((nslot1==839) && (nsender==633)) ||
//         ((nslot1==840) && (nsender==628)) ||
//         ((nslot1==841) && (nsender==642));

   if (nslot1==nslot2) return false;

   if (Item(nslot1, nsender).Empty() || !Item(nslot2, nsender).Empty()) return false;

   int nreceiver = Item(nslot1, nsender).node;

   for (int lid=0; lid<routing.NumLids(); lid++) {

      IbTestRoute route = routing.GetRoute(nsender, nreceiver, lid);
      if (route.IsNull()) return false;

      int npath1(0), npath2(0);

      int nhops = route.GetNumHops();

      IbTestScheduleItem* slot2 = getScheduleSlot(nslot2);

      for (int nsend=0;nsend<numSenders();nsend++) {
         if (slot2[nsend].Empty()) continue;

         if (slot2[nsend].node == nreceiver) {
            //         if (dodebug) DOUT0("There is already entry for receiver %d", nreceiver);
            return false;
         }

         if (nhops!=3) continue;

         IbTestRoute rrr = routing.GetRoute(nsend, slot2[nsend].node, slot2[nsend].lid);

         if (rrr.GetNumHops()!=3) continue;

         if (route.GetPath1() == rrr.GetPath1()) npath1++;
         if (route.GetPath2() == rrr.GetPath2()) npath2++;
      }

      //   if (dodebug) DOUT0("Accumulated paths %d %d", npath1, npath2);

      if ((npath1<2) && (npath2<2)) {
         if (lid != Item(nslot1, nsender).lid) newlid = lid;
                                          else newlid = -1;
         return true;
      }
   }

   return false;
}


bool IbTestSchedule::RollBack(IbTestScheduleMoveList& lst)
{
   while (lst.size()>0) {
      IbTestScheduleMove oper = lst.back();
      lst.pop_back();

      if (oper.Empty()) { EOUT("Empty operation in rallback list"); return false; }

      if (!Item(oper.slot1, oper.nsend).Empty()) {
         EOUT("Target item is not empty!!!");
         return false;
      }

      Item(oper.slot1, oper.nsend) = Item(oper.slot2, oper.nsend);

      if (oper.lid>=0) Item(oper.slot1, oper.nsend).lid = oper.lid;

      Item(oper.slot2, oper.nsend).Reset();
   }

   return true;
}

void IbTestSchedule::MoveSender(IbTestScheduleMoveList& lst, int nsender, int src_slot, int tgt_slot, int lid)
{
   lst.push_back(IbTestScheduleMove(nsender, src_slot, tgt_slot, lid>=0 ? Item(src_slot, nsender).lid : -1));

   Item(tgt_slot, nsender) = Item(src_slot, nsender);
   if (lid>=0) Item(tgt_slot, nsender).lid = lid;
   Item(src_slot, nsender).Reset();
}

bool IbTestSchedule::TryMoveSender(const IbTestClusterRouting& routing, IbTestScheduleMoveList& global_lst, int nsender0, int nslot0, int nslot1, int routekind, int nslot2, bool show)
{
   // Try to move sender from nslot0 to nslot1.
   //   routekind == 0: try to detect if no one sending to the nreceiver0 is done
   //   routekind == 1: only local sending to the nreceiver0 should be searched and moved
   //   routekind == 2: global sending to the nreceiver0 will be searched and tried to moved
   // global_lst should accumulate operations done to move items in the schedule

   if ((nslot0==nslot1) || !Item(nslot1, nsender0).Empty()) return false;

   IbTestScheduleItem* slot0 = getScheduleSlot(nslot0);
   IbTestScheduleItem* slot1 = getScheduleSlot(nslot1);

   int nreceiver0 = slot0[nsender0].node;

   int nsender1 = -1;

   for (int nsend=0;nsend<numSenders();nsend++)
      if (slot1[nsend].node == nreceiver0) {
        nsender1 = nsend;
        break;
     }

   if (nsender1>=0) {

      if (routekind==0) return false;

      // here lid is not matter, one want to see if route is short or long
      IbTestRoute route1 = routing.GetRoute(nsender1, nreceiver0, 0);

      if ((route1.GetNumHops()==1) && (routekind!=1)) return false;

      if ((route1.GetNumHops()==3) && (routekind!=2)) return false;
   }

//   if (nslot0==842) {
//      DOUT0("Sender %d wants to transmit to %d in slot %d sender %d do it with route %s, try to move it", nsender0, nreceiver0, nslot1, nsender1, routing.GetRouteAsString(nsender1, nreceiver0).c_str());
//   }

   IbTestScheduleMoveList lst1;

   // try to move nsender1 to some other slot
   if (nsender1>=0)
      for (int nslot=0; nslot<numSlots(); nslot++) {
         if ((nslot==nslot0) || (nslot==nslot1) || (nslot==nslot2) || !IsEmptyItem(nslot, nsender1)) continue;

         int newlid;
         if (CanMoveItemTo(routing, nsender1, nslot1, nslot, newlid)) {

            MoveSender(lst1, nsender1, nslot1, nslot, newlid);

         } else {

            // for a moment disable this branch - it is experimental
            // continue;

            // could we try to move it more globally if route we select is simple -
            // means does not require hard resorting

            if ((routekind!=1) || /*(nslot0!=842) || */ (nslot2>=0)) continue;

            // first try for our "bad" slot
            bool didmove = TryMoveSender(routing, lst1, nsender1, nslot1, nslot, 0, nslot0, show);
            if (!didmove) didmove = TryMoveSender(routing, lst1, nsender1, nslot1, nslot, 1, nslot0, show);

            if (!didmove) continue;

            if (show) DOUT0("!!!! We did move our bad sender !!!");
         }

         nsender1 = -1;
         break;
      }

   // if we cannot move sender which are sending to receiver we need to address,
   // than we have no other chance to do our job
   if (nsender1>=0) {
      RollBack(lst1);
      return false;
   }

   // we should try all lids
   for (int lid0=0; lid0<routing.NumLids(); lid0++) {

      IbTestRoute route0 = routing.GetRoute(nsender0, nreceiver0, lid0);

      if (route0.IsNull()) return false;

      // if route0 is local, one can simply move it to the new slot

      if (route0.GetNumHops() == 1) {

         global_lst.insert(global_lst.end(), lst1.begin(), lst1.end());

         MoveSender(global_lst, nsender0, nslot0, nslot1, lid0);

         //      DOUT0("After successful moving of sender %d from slot %d to slot %d global rall back size = %u", nsender0, nslot0, nslot1, global_lst.size());

         //               DOUT0("We moved 1-hop operation %d -> %d from slot %d to slot %d", nsender0, nreceiver0, nslot0, nslot1);
         return true;
      }

      // if route0 is not local, we should exclude route paths from the slot (if necessary)
      int npath1(0), npath2(0);

      for (int nsend=0; nsend<numSenders(); nsend++) {
         if (slot1[nsend].Empty()) continue;

         IbTestRoute rrr = routing.GetRoute(nsend, slot1[nsend].node, slot1[nsend].lid);

         if (rrr.GetNumHops()==3) {
            if (rrr.GetPath1() == route0.GetPath1()) npath1++;
            if (rrr.GetPath2() == route0.GetPath2()) npath2++;
         }
      }

      //   if (nslot0>835) {
      //      DOUT0("Sender %d wants to transmit to %d in slot %d over such paths goes %d %d buffers", nsender0, nreceiver0, nslot1, npath1, npath2);
      //   }

      // now we should exclude one or both paths from the schedule slot

      IbTestScheduleMoveList lst2;


      // if first path used two times, try to remove it from the current slot
      if (npath1>=2)
         for (int nsend=0;nsend<numSenders();nsend++) {
            if ((nsend==nsender0) || (nsend==nsender1) || slot1[nsend].Empty()) continue;
            IbTestRoute rrr = routing.GetRoute(nsend, slot1[nsend].node, slot1[nsend].lid);

            if (rrr.GetNumHops()!=3) continue;

            // select only route where first path is the same
            if ( (rrr.GetPath1() != route0.GetPath1()) ||
                  (rrr.GetPath2() == route0.GetPath2()) ) continue;

            // now check is this route can be moved somewhere in other place

            for (int nslot=0; nslot<numSlots(); nslot++) {
               if ((nslot==nslot0) || (nslot==nslot1) || (nslot==nslot2)) continue;

               int newlid;
               if (!CanMoveItemTo(routing, nsend, nslot1, nslot, newlid)) continue;

               MoveSender(lst2, nsend, nslot1, nslot, newlid);

               //           DOUT0("Move sender %d from slot %d to slot %d", nsend, nslot1, nslot);

               npath1--;
               break;
            }

            if (npath1<2) break;
         }

      // if second path used two times, try to remove it from the current slot
      if ((npath2>=2) && (npath1<2))
         for (int nsend=0;nsend<numSenders();nsend++) {
            if ((nsend==nsender0) || (nsend==nsender1) || slot1[nsend].Empty()) continue;
            IbTestRoute rrr = routing.GetRoute(nsend, slot1[nsend].node, slot1[nsend].lid);

            if (rrr.GetNumHops()!=3) continue;

            // select route where only second path is the same
            if ( (rrr.GetPath1() == route0.GetPath1()) ||
                  (rrr.GetPath2() != route0.GetPath2()) ) continue;

            // now check is this route can be moved somewhere in other place

            for (int nslot=0; nslot<numSlots(); nslot++) {
               if ((nslot==nslot0) || (nslot==nslot1) || (nslot==nslot2)) continue;

               int newlid;
               if (!CanMoveItemTo(routing, nsend, nslot1, nslot, newlid)) continue;

               MoveSender(lst2, nsend, nslot1, nslot, newlid);

               //                   DOUT0("Move sender %d from slot %d to slot %d", nsend, nslot1, nslot);
               npath2--;
               break;
            }

            if (npath2<2) break;
         }

      if ((npath1<2) && (npath2<2)) {

         if (lst1.size()>0)
            global_lst.insert(global_lst.end(), lst1.begin(), lst1.end());
         if (lst2.size()>0)
            global_lst.insert(global_lst.end(), lst2.begin(), lst2.end());

         MoveSender(global_lst, nsender0, nslot0, nslot1, lid0);

         //      if (nslot0>835)
         //         DOUT0("After successful moving of sender %d from slot %d to slot %d global rall back size = %u", nsender0, nslot0, nslot1, global_lst.size());

         //slot1[nsender0].node = nreceiver0;
         //slot0[nsender0].Reset();

         return true;
      }

      RollBack(lst2); // rall back operation2 to decrease path1, path2 usage

   } // loop over all lids

   RollBack(lst1); // rall back operation2 to move sender1

   return false;
}


bool IbTestSchedule::TryToCompress(IbTestClusterRouting& routing, bool show, double runtime)
{
   // in this method we try to compress schedule by swapping operation and trying to remove last slots,
   // which are probably most empty

   int numBefore = numSlots();

   dabc::TimeStamp start = dabc::Now();

   int nslot0 = numSlots() - 1;

   // here we accumulating movement, done for single slot
   // if slot cannot be eliminated, all operations will be ralled-back
   IbTestScheduleMoveList global_lst;

   while ((numSlots() > 0) && (nslot0>=0)) {

      if ((runtime>0) && (dabc::Now() - start > runtime)) {
         RollBack(global_lst);
         if (show) DOUT0("break sorting due to long time");
         break;
      }

      IbTestScheduleItem* slot0 = getScheduleSlot(nslot0);

      // first find non-empty entry in the selected slot0
      int nsender0 = -1;
      int num_senders = 0;

      for (int nsend=0;nsend<numSenders();nsend++)
         if (!slot0[nsend].Empty()) {
            if (nsender0<0) nsender0 = nsend;
            num_senders++;
         }

      if (nsender0 < 0) {
         if (show) DOUT0("Remove slot %d - no entries remain", nslot0);
         for (int n1 = nslot0; n1 < fNumSlots - 1; n1++) {
            IbTestScheduleItem* sl0 = getScheduleSlot(n1);
            IbTestScheduleItem* sl1 = getScheduleSlot(n1+1);
            for (int k=0; k<numSenders();k++)
               sl0[k] = sl1[k];
         }

         fNumSlots--;
         nslot0--;
         global_lst.clear();
//         if (nslot0>835) PrintSlotWithRouting(routing, nslot0);
         continue;
      }

      // Try to move sender from nslot0 to nslot1. Do it in three loops:
      //   in the first loop try to detect if no one sending to the nreceiver0
      //   in the second loop only local sending to the nreceiver0 will be searched and moved
      //   in the third loop global sending to the nreceiver0 will be searched

      bool can_move(false);

      for (int nloop1 = 0; (nloop1<3) && !can_move; nloop1++)
         for (int nslot1 = 0; (nslot1 < numSlots()) && !can_move; nslot1++)
            can_move = TryMoveSender(routing, global_lst, nsender0, nslot0, nslot1, nloop1, show);


      if (!can_move) {

         if (global_lst.size()>0) {
            if (show) DOUT0("Roll back %u operations for slot %d num senders remains %d", global_lst.size(), nslot0, num_senders);
//            if (nslot0>835) PrintSlotWithRouting(routing, nslot0);
         }

         RollBack(global_lst);

//         if (nslot0>835) PrintSlotWithRouting(routing, nslot0);

         nslot0--;

//         if (nslot0>835) PrintSlotWithRouting(routing, nslot0);

         if ((nslot0 % 50 == 0) && show) DOUT0("Processing slot %d", nslot0);
         continue;
      }

   // if we found slot where no senders or only local senders for nreceiver0, try following

   }

   if (show && (numBefore>0))
      DOUT0("Try to compress before:%d after %d ratio %4.2f", numBefore, numSlots(), 1. * numSlots() / numBefore);

   return numSlots() < numBefore;

}



double IbTestSchedule::CheckConjunction(IbTestClusterRouting& routing, bool show)
{
   IbTestIntMatrix occup(routing.NumSwitches(), routing.NumSwitches());

   dabc::Average switch_occup, line_occup, num_conjunc;

   for(int nslot=0;nslot<numSlots();nslot++) {

      occup.Fill(0);

      IbTestScheduleItem* slot_sch = getScheduleSlot(nslot);

      for(int nsend=0;nsend<numSenders();nsend++) {
         int nrecv = slot_sch[nsend].node;
         int lid = slot_sch[nsend].lid;

         if (nrecv<0) continue;

         IbTestRoute route = routing.GetRoute(nsend, nrecv, lid);

         if (route.IsNull()) {
            EOUT("NULL route between nodes %d -> %d", nsend, nrecv);
            continue;
         }

         if (route.IsLocal()) {
            EOUT("Schedule include sends to itself??? %d -> %d", nsend, nrecv);
            continue;
         }

         switch (route.GetNumHops()) {
            case 1:
               // mark number of packets going via switch
               occup(route.GetHop1(), route.GetHop1())++;
               break;

            case 3:

               // mark number of packets going via switches
               occup(route.GetHop1(), route.GetHop1())++;
               occup(route.GetHop2(), route.GetHop2())++;
               occup(route.GetHop3(), route.GetHop3())++;

               // mark number of packets going via cable
               occup(route.GetHop1(), route.GetHop2())++;
               occup(route.GetHop2(), route.GetHop3())++;

               break;
         }
      } // loop over sender in current schedule

      int numconj = 0;

      for (int nsw1=0; nsw1<routing.NumSwitches(); nsw1++) {
         if (occup(nsw1, nsw1)>0) switch_occup.Fill(occup(nsw1, nsw1));

         for (int nsw2=0; nsw2<routing.NumSwitches(); nsw2++) {
            if (nsw1==nsw2) continue;
            if (occup(nsw1, nsw2)>0) line_occup.Fill(occup(nsw1, nsw2));

            if (occup(nsw1, nsw2)>2) numconj+=(occup(nsw1, nsw2)-2);
         }
      }

      num_conjunc.Fill(numconj);

   } // loop over all slots

   if (show) {
      switch_occup.Show("Switches", true);
      line_occup.Show("Interswitch", true);
      num_conjunc.Show("Num conjunctions", true);
   }

   return num_conjunc.Mean() / numSenders();
}


bool IbTestSchedule::ProveSchedule(IbTestIntColumn* ids)
{
   IbTestIntMatrix matrix(numSenders(), numSenders());
   matrix.Fill(0);

   for (int nslot=0; nslot<numSlots(); nslot++) {
      IbTestScheduleItem* slot = getScheduleSlot(nslot);

      for (int nsend=0;nsend<numSenders();nsend++) {
         int nrecv = slot[nsend].node;

         if (nrecv<0) continue;

         matrix(nsend, nrecv)++;

         if (matrix(nsend, nrecv)>1) {
            EOUT("Matrix element %d %d more than once", nsend, nrecv);
            return false;
         }
      }
   }

   if (ids==0) {
      for (int n1=0;n1<numSenders();n1++)
         for (int n2=0;n2<numSenders();n2++) {
            int expected = n1==n2 ? 0 : 1;
            if (matrix(n1,n2)!=expected) {
               EOUT("Matrix %d %d != %d", n1, n2, expected);
               return false;
            }
         }
   } else {
      for (int i1=0;i1<ids->size();i1++)
         for (int i2=0;i2<ids->size();i2++) {
            int n1 = ids->at(i1);
            int n2 = ids->at(i2);
            int expected = n1==n2 ? 0 : 1;
            if (matrix(n1,n2)!=expected) {
               EOUT("Matrix %d %d != %d", n1, n2, expected);
               return false;
            }
         }
      }

   DOUT0("Schedule with %d slots is OK", numSlots());

   return true;
}


bool IbTestSchedule::RecodeIds(const IbTestIntColumn& ids, IbTestSchedule& new_sch)
{
   new_sch.Allocate(numSlots(), ids.size());
   new_sch.SetNumSlots(numSlots());

   for (int nslot=0;nslot<numSlots();nslot++) {
      new_sch.SetTimeSlot(nslot, timeSlot(nslot));

      IbTestScheduleItem* slot = getScheduleSlot(nslot);
      IbTestScheduleItem* new_slot = new_sch.getScheduleSlot(nslot);

      for (int id=0;id<ids.size();id++) {
         if (ids(id) >= numSenders()) return false;
         new_slot[id] = slot[ids(id)];

         int newrecv = ids.FindIndex(new_slot[id].node);

         if (newrecv<0) new_slot[id].Reset();
                  else new_slot[id].node = newrecv;
      }
   }

   new_sch.SetEndTime(endTime());

   return true;
}

bool IbTestSchedule::CopyTo(IbTestSchedule& new_sch)
{
   new_sch.Allocate(numSlots(), numSenders());
   new_sch.SetNumSlots(numSlots());

   for (int nslot=0;nslot<numSlots();nslot++) {
      new_sch.SetTimeSlot(nslot, timeSlot(nslot));

      IbTestScheduleItem* slot = getScheduleSlot(nslot);
      IbTestScheduleItem* new_slot = new_sch.getScheduleSlot(nslot);

      for (int id=0;id<numSenders();id++)
         new_slot[id] = slot[id];
   }

   new_sch.SetEndTime(endTime());

   return true;
}

int IbTestSchedule::numLids()
{
   int max = 0;
   for (int nslot=0;nslot<numSlots();nslot++) {
      IbTestScheduleItem* slot = getScheduleSlot(nslot);
      for (int id=0;id<numSenders();id++)
         if (slot[id].lid > max) max = slot[id].lid;
   }

   return max+1;
}

void IbTestSchedule::FillRegularTime(double schedule_step)
{
   for (int nslot=0;nslot<numSlots();nslot++)
      SetTimeSlot(nslot, nslot*schedule_step);
   SetEndTime(numSlots()*schedule_step);
}


bool IbTestSchedule::SaveToFile(const std::string &fname)
{
   FILE *f = fopen (fname.c_str(), "w");

   if (f==0) {
      EOUT("Cannot open file %s for writing", fname.c_str());
      return false;
   }

   fprintf(f,"Num slots: %d\n", numSlots());
   fprintf(f,"Num senders: %d\n", numSenders());

   for (int nslot=0; nslot<numSlots(); nslot++) {
      fprintf(f, "Slot: %d\n", nslot);
      IbTestScheduleItem* slot = getScheduleSlot(nslot);
      for (int nsend=0;nsend<numSenders();nsend++)
         fprintf(f,"%3d -> %3d %2d\n", nsend, slot[nsend].node, slot[nsend].lid);
   }

   fclose(f);

   DOUT0("Schedule with %d slots for %d senders saved to file %s", numSlots(), numSenders(), fname.c_str());

   return true;
}

bool IbTestSchedule::ReadFromFile(const std::string &fname)
{
   char sbuf[100];

   FILE* f = fopen (fname.c_str(), "r");

   if (f==0) return false;

   int numslots(0), numsenders(0);

   if (!fgets (sbuf, sizeof(sbuf), f) ||
         sscanf(sbuf,"Num slots: %d", &numslots)!=1)
         { fclose(f); EOUT("reading num slots"); return false; }

   if (!fgets (sbuf, sizeof(sbuf), f) ||
         sscanf(sbuf,"Num senders: %d", &numsenders)!=1)
         { fclose(f); EOUT("reading num senders"); return false; }


   Allocate(numslots, numsenders);

   fNumSlots = numslots;

   for (int nslot=0; nslot<numSlots(); nslot++) {

      IbTestScheduleItem* slot = getScheduleSlot(nslot);

      int filenslot(-1), filensend(-1);

      if (!fgets (sbuf, sizeof(sbuf), f) ||
            (sscanf(sbuf,"Slot: %d", &filenslot)!=1) || (filenslot!=nslot))
            { fclose(f); EOUT("reading slot %d", nslot); return false; }

      for (int nsend=0;nsend<numSenders();nsend++)
         if (!fgets (sbuf, sizeof(sbuf), f) ||
               (sscanf(sbuf,"%d -> %d %d", &filensend, &slot[nsend].node, &slot[nsend].lid)!=3) ||
               (filensend!=nsend))
         { fclose(f); EOUT("reading sender %d in slot %d", nsend, nslot); return false; }
   }
   fclose(f);

   DOUT0("Schedule nslot %d nsenders %d read from file %s", numSlots(), numSenders(), fname.c_str());

   return true;
}

// __________________________________________________________________________________


void BuildConjunctionCurve(IbTestClusterRouting& routing)
{
   int maxnum = (routing.NumNodes() / 100) * 100;

   for (int num = 50; num <= maxnum; num+=50) {

      IbTestIntColumn ids(num);

      dabc::Average rel_conj;

      for (int ntest=0;ntest<10;ntest++) {
         routing.FillRandomIds(ids);

         IbTestSchedule sch1(routing.NumNodes()-1, routing.NumNodes());
         sch1.FillRoundRoubin(&ids);
         double rel = sch1.CheckConjunction(routing);

         rel_conj.Fill(rel);
      }

      DOUT0("NumNodes:%3d Rel.conj: %5.3f", num, rel_conj.Mean());
   }
}

extern "C" void PrintRouting()
{
   std::string filename = dabc::mgr()->cfg()->GetUserPar("FileName");

   bool optimize = dabc::mgr()->cfg()->GetUserPar("Optimize") == "true";

   if (filename.length()==0) {
      EOUT("File name not specified - exit");
      return;
   }

   DOUT0("Reading file");

   IbTestClusterRouting routing;


   if (!routing.ReadFile(filename)) return;

   DOUT0("Reading file done");

   if (optimize) routing.BuildOptimalRoutes();

   routing.PrintSpineStatistic();

   if (routing.CheckTargetRoutes())
      DOUT0("!!! All routes defined by target LID !!!");

   if (routing.NumLids()>0)
      routing.CheckUsefulLIDs();
}

extern "C" void CompareRouting()
{
   StringsVector arr;

   std::string files = dabc::mgr()->cfg()->GetUserPar("FilesList");

   if (files.length()>0) {
      Tokanize(files, arr);
   } else {
      arr.push_back(dabc::mgr()->cfg()->GetUserPar("FileName1"));
      arr.push_back(dabc::mgr()->cfg()->GetUserPar("FileName2"));
   }

   if (arr.size()<2) {
      EOUT("Only %u files in the list", arr.size());
      return;
   }

   for (unsigned n=0; n<arr.size()-1;n++) {

      if ((arr[n].length()==0) || (arr[n+1].length()==0)) {
         EOUT("File names are not specified - exit");
         return;
      }

      DOUT0("Comparing files %s %s", arr[n].c_str(), arr[n+1].c_str());

      IbTestClusterRouting routing1, routing2;

      if (!routing1.ReadFile(arr[n])) return;
      if (!routing2.ReadFile(arr[n+1])) return;

      routing1.MatchNodes(routing2);

      // DOUT0("Routing1 %d nodes Routing2 %d nodes", routing1.NumNodes(), routing2.NumNodes());

      routing1.PrintDifferecne(routing2);
   }

}



extern "C" void TestSorting()
{
   std::string filename = dabc::mgr()->cfg()->GetUserPar("FileName");
   std::string outfilename = dabc::mgr()->cfg()->GetUserPar("OutputFileName");

   bool optimize = dabc::mgr()->cfg()->GetUserPar("Optimize") == "true";

   int seed = dabc::mgr()->cfg()->GetUserParInt("Seed", 765);
   int sorttime = dabc::mgr()->cfg()->GetUserParInt("SortTime", 60);

   // seed = lrint(dabc::Now().AsDouble()*1000000) % 10000;

   if (filename.empty()) {
      EOUT("File name not specified - exit");
      return;
   }

   DOUT0("Reading file %s", filename.c_str());

   IbTestClusterRouting routing;

   if (!routing.ReadFile(filename)) return;

   DOUT0("Reading file done");

   if (optimize) routing.BuildOptimalRoutes();

   DOUT0("SEED = %d", seed);

   if (seed>0) srand48(seed);

   IbTestIntColumn ids(routing.NumNodes());

   switch (seed) {
      case 0:
         for (int k=0;k<ids.size();k++) ids(k) = k;
         break;
      case -1:
         routing.FillBadIdsFor2Switches(ids);
         break;
      case 1:
         routing.FillInteractiveNodes(ids);
         break;

      default:
         routing.FillRandomIds(ids);
         break;
   }

   DOUT0("Produce schedule");

   IbTestSchedule sch2(routing.NumNodes()*3, routing.NumNodes());
   //sch2.BuildOptimized(routing, &ids, true);
   sch2.BuildRegularSchedule(routing, &ids, true);
   sch2.ProveSchedule(&ids);
   sch2.CheckConjunction(routing, true);

   sch2.PrintWithRouting(routing, 5);
   sch2.TryToCompress(routing, true, sorttime);
   sch2.ProveSchedule(&ids);
   sch2.CheckConjunction(routing, true);

   if (outfilename.length()>0) {
      sch2.SaveToFile(outfilename);
      if (!sch2.ReadFromFile(outfilename)) return;
      sch2.CheckConjunction(routing, true);
   }

   IbTestSchedule sch1(routing.NumNodes()-1, routing.NumNodes());
   sch1.FillRoundRoubin(&ids);
   sch1.ProveSchedule(&ids);
   sch1.CheckConjunction(routing, true);
}



extern "C" void TestSchedule()
{
   std::string filename = dabc::mgr()->cfg()->GetUserPar("FileName");

   bool optimize = dabc::mgr()->cfg()->GetUserPar("Optimize") == "true";

   if (filename.length()==0) {
      EOUT("File name not specified - exit");
      return;
   }

   DOUT0("Reading file");

   IbTestClusterRouting routing;

   if (!routing.ReadFile(filename)) return;

//   routing.FindSameRouteTwice(routing.GetRoute(100,200));

   routing.FindSameRouteTwice(true);
   routing.FindSameRouteTwice(false, IbTestRoute(), true);

   return;


   DOUT0("Reading file done");

   if (optimize) routing.BuildOptimalRoutes();

   IbTestIntColumn ids(routing.NumNodes());

   int seed = lrint(dabc::Now().AsDouble()*1000000) % 10000;

   DOUT0("SEED = %d", seed);

   srand48(seed);

   for (int n=0;n<4;n++) {

      DOUT0("=========================");
      DOUT0("Loop cnt %d", n);

      if (n==0)
         for (int k=0;k<ids.size();k++) ids(k) = k;
      else
         routing.FillRandomIds(ids);

      IbTestSchedule sch1(routing.NumNodes()-1, routing.NumNodes());

      sch1.FillRoundRoubin(&ids);

      sch1.CheckConjunction(routing, true);

      IbTestSchedule sch2(ids.size()*3, ids.size());
      sch2.BuildOptimized(routing, &ids, true);
      sch2.CheckConjunction(routing, true);

      if (optimize) {
         sch2.PrintWithRouting(routing, 10);
         sch2.TryToCompress(routing, true);
         sch2.CheckConjunction(routing, true);
      }
   }

   BuildConjunctionCurve(routing);

}


extern "C" void SelectCSCNodes()
{
   std::string all_args = dabc::mgr()->cfg()->GetUserPar("SelectArgs");

   std::string nodeslistfile = dabc::mgr()->cfg()->GetUserPar("NodesList");

   std::string schedulefile = dabc::mgr()->cfg()->GetUserPar("ScheduleFile");

   if (nodeslistfile.empty()) nodeslistfile = "nodes.txt";
   if (schedulefile.empty()) schedulefile = "schedule.txt";

   StringsVector files;

   Tokanize(dabc::mgr()->cfg()->GetUserPar("FilesList"), files);

   if (files.size()==0) {
      EOUT("No any routing files in the list");
      return;
   }

   IbTestClusterRouting routing;

   DOUT0("Reading file %s args:%s", files.back().c_str(), all_args.c_str());

   if (!routing.ReadFile(files.back())) return;

   IbTestIntColumn ids;

   if (all_args=="schedule") {
      if (!routing.LoadNodesList(nodeslistfile, ids)) {
         EOUT("Cannot read nodes list from the file");
         return;

      }
      DOUT0("Nodes list has %d nodes", ids.size());

      // for (int n=0;n<ids.size();n++)
      //    DOUT0("node %d = %d", n, ids(n));

      DOUT0("============ First test round-robin schedule =============== ");
      IbTestSchedule sch1(routing.NumNodes()-1, routing.NumNodes());
      sch1.FillRoundRoubin(&ids);
      sch1.CheckConjunction(routing, true);

      DOUT0("============ Now check sorted schedule =============== ");

      IbTestSchedule sch2(routing.NumNodes()*3, routing.NumNodes());
      sch2.BuildOptimized(routing, &ids, true);
      sch2.TryToCompress(routing, false, 30);
      sch2.TryToCompress(routing, false, 30);
      sch2.ProveSchedule(&ids);
      sch2.CheckConjunction(routing, true);
      DOUT0("OPTIMIZED SCHEDULE SIZE = %d", sch2.numSlots());

      DOUT0("============ last is regular schedule =============== ");

      IbTestSchedule sch3(routing.NumNodes()*3, routing.NumNodes());
      sch3.BuildRegularSchedule(routing, &ids, false);
      sch3.TryToCompress(routing, false, 30);
      sch3.TryToCompress(routing, false, 30);
      sch3.ProveSchedule(&ids);
      sch3.CheckConjunction(routing, true);

      DOUT0("REGULAR SCHEDULE SIZE = %d", sch3.numSlots());

      IbTestSchedule sch4;

      if (sch2.numSlots() < sch3.numSlots())
         sch2.RecodeIds(ids, sch4);
      else
         sch3.RecodeIds(ids, sch4);

      sch4.SaveToFile(schedulefile);

   } else {

      if (!routing.SelectNodes(all_args, ids)) {
         EOUT("Cannot select nodes according to specified arguments");
         return;
      }

      DOUT0("Select nodes done size = %d, store to file %s",ids.size(), nodeslistfile.c_str());

      routing.SaveNodesList(nodeslistfile, ids);
   }

}


