#include <iostream>
#include <iomanip>
#include <dic.hxx>
using namespace std;
#include <string.h>
#include <unistd.h>

//-----------------------------------------------
class ErrorHandler : public DimErrorHandler
  {
    void errorHandler(int severity, int code, char *msg)
      {
        int index = 0;
        char **services;
        cout << "CLI: Error "<< severity << " " << msg << endl;
        services = DimClient::getServerServices();
        cout << "CLI: "<< "from "<< DimClient::getServerName()<< " services:" << endl;
        while (services[index])
          {
            cout << "CLI: "<< services[index] << endl;
            index++;
          }
      }
public:
    ErrorHandler()
      {
        DimClient::addErrorHandler(this);
      }
  };

//-----------------------------------------------
class ServiceClient : public DimClient
  {

    void infoHandler() // for all registered services
      {
        DimInfo *dimi;
        char *name,*format,*color,*state;
        int i=1,sev,qual;

        dimi   =getInfo();
        name   =dimi->getName();
        format =dimi->getFormat();
        qual   =dimi->getQuality();
        sr     =(staterec *)dimi->getData();
    	if(strcmp(sr->state,last)==0) count++;
    	else {
    		if(count < 4)cout<< "ERROR missing states! "<<count+1<<" of 5" << endl;
    		count=0;
    	}
    	cout<<count<<"."<<flush;
    	if((control<2)&&(lines++ > 40)){cout<<endl;lines=0;}
        //cout << "c> "<<name << " : " << sr->state << endl;
    	strcpy(last,sr->state);
    	if(strcmp(sr->state,"Halted")  ==0)halt++;
    	if(strcmp(sr->state,"Ready") ==0)ready++;
    	if(strcmp(sr->state,"Configured")==0)config++;
    	if(strcmp(sr->state,"Running")  ==0)run++;
       }
public:
    ServiceClient(int ctrl)
      {
        int type;
        char *name, *format, fullname[132];
        DimInfo *dimi;
        count=0;
        lines=0;
        control=ctrl;
        halt=5;
        ready=5;
        config=5;
        run=5;
        br = new DimBrowser();
        br->getServices("*Status");
        while ((type = br->getNextService(name, format))!= 0)
          {
            if(type == DimSERVICE) // no command or rpc
              {
                cout <<"Register " << name << endl;
                dimi = new DimInfo(name,-1,this);
               }
          }
        sleep(3);
      }
private:
    DimBrowser *br;
        typedef struct {
          int sev;
          char color[16];
          char state[16];
        } staterec;
    staterec *sr;
public:
    char last[16];
    int count, lines, control;
    int config, ready, run, halt;
  };

//-----------------------------------------------
int main(int argc, char **argv)
  {

    ErrorHandler errHandler;
    char *server, *ptr, *ptr1;
    char pref[128];
    char cmd[128];
    DimBrowser br;
    int type, n, num;

    ServiceClient servcli(argc);

    if(argc > 1){ // send commands
		strcpy(pref,"DABC/");
		strcat(pref,argv[1]);
		strcat(pref,":0/Controller/Do");
		num=0;

		while (1)
		  {
			num++;
			sleep(5);
			cout<<setw(6)<<num<<endl;
			strcpy(cmd,pref);
			strcat(cmd,"Configure");
			cout << "c> "<<cmd << " ....." << endl;
			servcli.config=0;
			DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
			// wait for config=5
			while(servcli.config != 5) sleep(1);
			sleep(1);
			strcpy(cmd,pref);
			strcat(cmd,"Enable");
			cout << "c> "<<cmd << " ....." << endl;
			servcli.ready=0;
			DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
			// wait for ready
			while(servcli.ready != 5) sleep(1);
			sleep(1);
			strcpy(cmd,pref);
			strcat(cmd,"Start");
			cout << "c> "<<cmd << " ....." << endl;
			servcli.run=0;
			DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
			// wait for run
			while(servcli.run != 5) sleep(1);
			sleep(5);
			strcpy(cmd,pref);
			strcat(cmd,"Stop");
			cout << "c> "<<cmd << " ....." << endl;
			servcli.ready=0;
			DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
			while(servcli.ready != 5) sleep(1);
			sleep(1);
			strcpy(cmd,pref);
			strcat(cmd,"Halt");
			cout << "c> "<<cmd << " ....." << endl;
			servcli.halt=0;
			DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
			// wait for halt
			while(servcli.halt != 5) sleep(1);
		  }
    } else while (1) sleep(5);

    return 0;
  }
