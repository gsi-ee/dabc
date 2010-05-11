#include <iostream>
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
        cout << name << " : " << sr->state << endl;
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
  };

int main(int argc, char **argv)
  {

    ErrorHandler errHandler;
    char *server, *ptr, *ptr1;
    char pref[128];
    char cmd[128];
    DimBrowser br;
    int type, n;

    if(argc < 2){
      cout << "ServerName" << endl;
      exit(0);
    }

    
    ServiceClient servcli;
    strcpy(pref,"DABC/");
    strcat(pref,argv[1]);
    strcat(pref,":0/Controller/Do");

while (1)
      {
        sleep(5);
        strcpy(cmd,pref);
        strcat(cmd,"Configure");
        cout << "> "<<cmd << " ....." << endl;
        DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
        sleep(5);
        strcpy(cmd,pref);
        strcat(cmd,"Enable");
        cout << "> "<<cmd << " ....." << endl;
        DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
        sleep(5);
        strcpy(cmd,pref);
        strcat(cmd,"Start");
        cout << "> "<<cmd << " ....." << endl;
        DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
        sleep(5);
        strcpy(cmd,pref);
        strcat(cmd,"Stop");
        cout << "> "<<cmd << " ....." << endl;
        DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
        sleep(5);
        strcpy(cmd,pref);
        strcat(cmd,"Halt");
        cout << "> "<<cmd << " ....." << endl;
        DimClient::sendCommand(cmd,"x1gSFfpv0JvDA");
      }
    return 0;
  }
