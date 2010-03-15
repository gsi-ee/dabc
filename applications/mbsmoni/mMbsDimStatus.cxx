// MBS DIM status.
// Connects to n MBS nodes and gets status
// for each node a mbs status object is created
// maximum 20 nodes

#include "MbsNodeDimStatus.h"
extern "C" {int f_mbs_status(char *, s_daqst *);}

/**************************************************************************/
int main(int argc,char **argv)
{
  INTS4 i,l_status, l_sec=1, l_1st=1;
  char c_dim[128], c_host[128],c_nodelist[512],c_times[128],*pc;
  s_daqst *ps_daqst[20];
  MbsNodeDimStatus *node[20];

  if(argc == 1){
    printf("[-t sec] node1 [node2 [node3]]\n");
    exit(0);
  }
  if(argv[1][0]=='-'){ //time
    if(argc > 3){
      l_sec=atoi(argv[2]);
      l_1st=3; // first node
    } else {
    printf("[-t sec] node1 [node2 [node3 ...]]\n");
    exit(0);
  }}
  strcpy(c_nodelist,"Monitoring ");
  for(i=l_1st;i<argc;i++){strcat(c_nodelist,argv[i]);strcat(c_nodelist," ");}
  gethostname(c_host,128);
  // some info from this server (no daqst):
  node[0] = new MbsNodeDimStatus(c_host, 0, c_nodelist);
  // loop over nodes, create objects
  for(i=l_1st;i<argc;i++){
    ps_daqst[i] = (s_daqst *) malloc (sizeof( s_daqst));
    node[i] = new MbsNodeDimStatus(argv[i],ps_daqst[i],"");
  }
  // start serving
  sprintf(c_dim,"MBS-%s",c_host);
  DimServer::start(c_dim);
  printf("DIM server %s started\n",c_dim);
  printf("Scan MBS nodes [%d sec]: %s\n",l_sec,c_nodelist);
  // loop over nodes, read status, update DIM services
  while(1)
    {
      strcpy(c_times,"");
      for(i=l_1st;i<argc;i++){
        l_status = f_mbs_status (argv[i],ps_daqst[i]);
        if(l_status > 0) node[i]->MD_update("");
        else printf("Error getting MBS status from %s\n",argv[i]);
        pc = ps_daqst[i]->c_date+13; // only min:sec
        strcat(c_times,pc);strcat(c_times," ");
      }
      node[0]->MD_update(c_times);
      sleep(l_sec);
    }
}
