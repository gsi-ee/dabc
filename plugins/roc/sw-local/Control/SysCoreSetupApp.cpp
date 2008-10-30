//============================================================================
// Name        : SysCoreSetup.cpp
// Author      : Stefan Müller-Klieser
// Version     :
// Copyright   : Kirchhoff-Institut für Physik
// Description : this app does the initial setup of the SysCore Board
//============================================================================

#include <iostream>
#include <fstream>
#include <string>

#include "../CppSockets/Utility.h"

#include "SysCoreControl.h"

using namespace std;


int main(int argc, char* argv[])
{   
   string boardIp;
   uint16_t portnum = ROC_DEFAULT_CTRLPORT;

   cout << "Usage: setup boardIP [portnum = " << portnum << " ]" << endl;
   
   if (argc < 2) return -1;
   
   boardIp = argv[1];
   if (argc>2) portnum = atoi(argv[2]);
   
   cout << endl << "-----   SysCoreControl setup started!   -----" << endl<<endl;
   SysCoreControl control;
   
   int boardnum = control.addBoard(boardIp.c_str(), true, portnum);
   
   if(control.getNumberOfBoards() != 1) {
      cout << "-----   Setup could not connect to SysCoreBoard. Exit!   -----" << endl;
      return -1;
   }
   
   string input;
   
   cout <<endl<< "-----   Setup connected successfully to SysCoreBoard.   -----" << endl<<endl;
   
   uint32_t a = control[boardnum].getIpAddress();
   string ip;
   Utility::l2ip(a,ip);
   
   while(1)
   {
      cout << "This SysCoreBoard has the Ip address " << ip << ". Do you want to change it? [y/n] ";
      cin >> input;
      cout << endl;
   
      if(input == "y")
      {
         cout << "Please enter the new SysCoreBoard Ip address: ";
         cin >> boardIp;
            ipaddr_t newaddr;
         Utility::u2ip(boardIp, newaddr);
            a = newaddr;
         
         control[boardnum].poke(ROC_IP_ADDRESS, a);
         cout << endl << "IP address changed .. connect to SysCoreBoard using its new IP..." << endl;
            
         boardnum = control.addBoard(boardIp.c_str());
            
         break;
      }
      else if(input == "n")
      {
         break;
      }
      else
      {
         cout << "Wrong input!" << endl;
      }
   }
   
   if(control.getNumberOfBoards() != 2)
   {
      cout <<endl<< "-----   Setup could not change the IP! Exit!   -----" << endl<<endl;
      return 0;
   }
      
   cout <<endl<< "-----   Setup successfully changed the IP.   -----" << endl<<endl;
   
/*
   //MAC Address
   uint32_t upper, lower;
   ret = control[boardnum].peek(ROC_MAC_ADDRESS_UPPER, upper);
   ret = control[boardnum].peek(ROC_MAC_ADDRESS_LOWER, lower);
   
   while(1)
   {
      cout << "This SysCoreBoard has the Ethernet MAC address: ";
      cout << hex;
      cout << (int)((upper & 0x0000FF00) >>  8) << ".";
      cout << (int)((upper & 0x000000FF) >>  0) << ".";
      cout << (int)((lower & 0xFF000000) >> 24) << ".";
      cout << (int)((lower & 0x00FF0000) >> 16) << ".";
      cout << (int)((lower & 0x0000FF00) >>  8) << ".";
      cout << (int)((lower & 0x000000FF) >>  0);
      cout << dec;
      cout <<" . Do you want to change it? [y/n] ";
      cin >> input;
      cout << endl;
   
      if(input == "y")
      {
         cout << "Please enter the new SysCoreBoard Ethernet MAC: ";
         cin >> input;
         cout << endl;
         //TODO: convert to MAC
         control[boardnum].poke(ROC_MAC_ADDRESS_UPPER, a);
         control[boardnum].poke(ROC_MAC_ADDRESS_LOWER, a);
         break;
      }
      else if(input == "n")
      {
         break;
      }
      else
      {
         cout << "Wrong input!" << endl;
      }
   }
*/   
   
   cout << "SysCore is writing the configuration to its flash ..." << endl;
   if (control[boardnum].saveConfig())
       cout << "Done" << endl;
    else
       cout << "Fail" << endl;
   
   return 0;
}
