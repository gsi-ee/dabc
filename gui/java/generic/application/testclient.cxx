#include <iostream>
#include <iomanip>
#include <dic.hxx>
using namespace std;
#include <string.h>
#include <unistd.h>

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

class ServiceClient : public DimClient
  {

    void infoHandler()
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
        //cout << "c> "<<name << " : " << sr->state << endl;
    	strcpy(last,sr->state);
       }
public:
    ServiceClient()
      {
        int type;
        char *name, *format, fullname[132];
        DimInfo *dimi;
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
    char last[16];
    int count;
  };

int main(int argc, char **argv)
  {

    ErrorHandler errHandler;
    char *server, *ptr, *ptr1;
    char pref[128];
    char cmd[128];
    DimBrowser br;
    int type, n, num;

    ServiceClient servcli;

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
			DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
			sleep(5);
			strcpy(cmd,pref);
			strcat(cmd,"Enable");
			cout << "c> "<<cmd << " ....." << endl;
			DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
			sleep(5);
			strcpy(cmd,pref);
			strcat(cmd,"Start");
			cout << "c> "<<cmd << " ....." << endl;
			DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
			sleep(5);
			strcpy(cmd,pref);
			strcat(cmd,"Stop");
			cout << "c> "<<cmd << " ....." << endl;
			DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
			sleep(5);
			strcpy(cmd,pref);
			strcat(cmd,"Halt");
			cout << "c> "<<cmd << " ....." << endl;
			DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
		  }
    } else while (1) sleep(5);

    return 0;
  }
