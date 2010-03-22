// MBS DIM status.
// Connects to n MBS nodes and gets status
// for each node an MBS status object is created
// maximum 20 nodes
// Note that the MBS server socket is blocked for 60 sec
// when message logger is restarted.
// Therefore the message logger must retry creating the server

#include "MbsNodeDimStatus.h"
extern "C" {int f_mbs_status(char *, s_daqst *);}

/**************************************************************************/
int main(int argc,char **argv)
{
INTS4 i,l_status, l_sec=1, l_1st=1, l_delay=0, listc=0;
char c_dim[128], c_host[128],c_nodelist[512];
char c_times[128],c_line[64],c_setup[256],*pc_nodes=0,*pc;
s_daqst *ps_daqst[20];
MbsNodeDimStatus *node[20], *server;
char list[20][32];
strcpy(c_line,HISTOGRAMDESC);
strcpy(c_line,STATEDESC);
strcpy(c_line,RATEDESC);
strcpy(c_line,INFODESC);

if(argc == 1){
	system("cat $DABCSYS/applications/mbsmoni/readme.txt");
	cout << "mbsmoni [-t sec] [-s setupfile] [@]node [node2 [node3 ...]]" << endl;
	exit(0);
}
if(getenv("DIM_DNS_NODE")==0){
	cout << "Set DIM_DNS_NODE to node where DIM name server is running" << endl;
	exit(0);
}
if(getenv("LOGNAME")==0){
	cout << "Set LOGNAME to user name" << endl;
	exit(0);
}
strcpy(c_setup,"dimsetup.txt");
// first args can be time [sec] or setup
if(strstr(argv[l_1st],"-t") !=0) { //time
	if(argc > (l_1st+2)){
		l_sec=atoi(argv[l_1st+1]);
		l_1st += 2; // next
	} else {
		cout << "[-t sec] [-s setupfile] [@]node [node2 [node3 ...]]" << endl;
		exit(0);
	}}
if(strstr(argv[l_1st],"-s") !=0) { //time
	if(argc > (l_1st+2)){
		strcpy(c_setup,argv[l_1st+1]);
		l_1st += 2; // next
	} else {
		cout << "[-t sec] [-s setupfile] [@]node [node2 [node3 ...]]" << endl;
		exit(0);
	}}
if(strstr(argv[l_1st],"-t") !=0) { //time
	if(argc > (l_1st+2)){
		l_sec=atoi(argv[l_1st+1]);
		l_1st += 2; // next
	} else {
		cout << "[-t sec] [-s setupfile] [@]node [node2 [node3 ...]]" << endl;
		exit(0);
	}}
// node can be @filename
if(strchr(argv[l_1st],'@')!=0){
	pc_nodes=argv[l_1st]+1; // behind @
	ifstream fnodes(pc_nodes);
	if(!fnodes) { cout<<"File not found "<<pc_nodes<<endl; exit(0); }
	else while(fnodes.getline(c_line,sizeof(c_line)))
	if((c_line[0]!='#')&&(c_line[0]!='!')&&(c_line[0]!=0))strcpy(list[listc++],c_line);
} else for(i=l_1st;i<argc;i++)strcpy(list[listc++],argv[i]);

strcpy(c_nodelist,"Monitoring ");
for(i=0;i<listc;i++){strcat(c_nodelist,list[i]);strcat(c_nodelist," ");}
gethostname(c_host,128);
// some info from this server (no daqst):
server = new MbsNodeDimStatus(c_host, 0, c_nodelist, c_setup);
// loop over nodes, create objects
for(i=0;i<listc;i++){
	ps_daqst[i] = (s_daqst *) malloc (sizeof( s_daqst));
	memset(ps_daqst[i],0,sizeof(s_daqst));
	node[i] = new MbsNodeDimStatus(list[i],ps_daqst[i],"", c_setup);
}
// start serving
sprintf(c_dim,"MBS-%s-%s",c_host,getenv("LOGNAME"));
DimServer::start(c_dim);
cout<<"DIM server "<<c_dim<<" started"<<endl;
cout<<"DIM setup file: "<<c_setup<<endl;
if(pc_nodes)
cout<<"Node list file: "<<pc_nodes<<endl;
cout<<"Scan MBS nodes ["<<l_sec<<" sec]: "<<c_nodelist<<endl;
// loop over nodes, read status, update DIM services
while(1){
	strcpy(c_times,"");
	for(i=0;i<listc;i++){
		l_status = f_mbs_status (list[i],ps_daqst[i]);
		if(l_status != 1) { // STC__SUCCESS
			if(l_status == -2)
				cout<<setw(3)<<l_delay<<" Error connecting node "<<list[i]<<endl;
			else if(l_status == -1)
				cout<<setw(3)<<l_delay<<" Error version mismatch "<<list[i]<<endl;
			else
				cout<<setw(3)<<l_delay<<" Error connecting node "<<list[i]<<" "<<l_status<<endl;
			node[i]->Reset(); // clear daqst
			l_delay += l_sec; // for printout
		} else l_delay=0;
		node[i]->Update("");
		pc = ps_daqst[i]->c_date+13; // only min:sec
		strcat(c_times,pc);strcat(c_times," ");
	}
	server->Update(c_times);
	sleep(l_sec);
}
}
