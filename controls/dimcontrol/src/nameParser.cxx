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
#include <iostream>
#include <unistd.h>
using namespace std;
#include <string>
#include "dimc/nameParser.h"

char dimc::nameParser::ParameterName[256];
char dimc::nameParser::CommandDescriptorName[256];

dimc::nameParser::nameParser(const char *fullname)
{
  parseQuality(0);
  parseFullName(fullname);
}
void dimc::nameParser::initParser(){
  NameSpace[0]=0;
  NodeSpec[0]=0;
  NodeName[0]=0;
  NodeID[0]=0;
  ApplSpec[0]=0;
  ApplName[0]=0;
  ApplNameID[0]=0;
  ApplNameSpace[0]=0;
  ApplID[0]=0;
  Name[0]=0;
  FullName[0]=0;
}
void dimc::nameParser::parseQuality(unsigned int qual){
  quality   = qual;
  status    =  (dimc::nameParser::recordstat) (quality       & 0xff);
  type      = (dimc::nameParser::recordtype) ((quality>>8)  & 0xff);
  visibility= (dimc::nameParser::visiblemask)  ((quality>>16) & 0xff);
  mode      = (dimc::nameParser::recordmode) ((quality>>24) & 0xff);
}

int dimc::nameParser::buildQuality (dimc::nameParser::recordstat s,
                                        dimc::nameParser::recordtype t,
                                        dimc::nameParser::visiblemask v,
                                        dimc::nameParser::recordmode m)
{
  status=s;
  type=t;
  visibility=v;
  mode=m;
  quality= ((unsigned int) s) |((unsigned int) (t) << 8) | ((unsigned int)(v) << 16) |((unsigned int) (m) << 24);
  return quality;
}

void dimc::nameParser::setType(dimc::nameParser::recordtype t)
{
    int q=quality;
    q = (q & 0xFFFF00FF); // clear old type byte 2
    int rt=((int) (t) << 8);
    q = (q | rt); // set new type
    quality=q;
    type=t;
}

void dimc::nameParser::setVisibility(dimc::nameParser::visiblemask mask)
{
    int q=quality;
    q = (q & 0xFF00FFFF); // clear old visibility byte 2
    int rmask=((int) (mask) << 16);
    q = (q | rmask); // set new vis mask
    quality=q;
    visibility=mask;

}

void dimc::nameParser::setStatus(dimc::nameParser::recordstat s)
{
    int q=quality; // no getter method for quality in dim c++ interface!
    q = (q & 0xFFFFFF00); // clear old status byte 1
    int rs=(int) (s);
    q = (q | rs); // set new status
    quality=q;
    status=s;
}

void dimc::nameParser::setMode(dimc::nameParser::recordmode m)
{
    int q=quality;
    q = (q & 0x00FFFFFF); // clear old mode byte 3
    int rm=((int) (m) << 24);
    q = (q | rm); // set new mode mask
    quality=q;
    mode=m;
}




bool dimc::nameParser::checkFields(){
  if(strlen(NameSpace)==0) return false;
  if(strlen(NodeName)==0) return false;
  if(strlen(NodeID)==0) return false;
  if(strlen(ApplName)==0) return false;
  if(strlen(ApplID)==0) return false;
  if(strlen(Name)==0) return false;
  return true;
}
char* dimc::nameParser::buildFullName()
{ // check if all fields are there
  if(!checkFields()) return NULL;
  buildNodeSpec();
  buildApplSpec();
  sprintf(FullName,"%s/%s/%s/%s",NameSpace,NodeSpec,ApplSpec,Name);
  return FullName;
}
void dimc::nameParser::buildNodeSpec(){
  sprintf(NodeSpec,"%s:%s",NodeName,NodeID);
}
void dimc::nameParser::buildApplSpec(){
  sprintf(ApplNameID,"%s:%s",ApplName,ApplID);
  if(strlen(ApplNameSpace) > 0) sprintf(ApplSpec,"%s::%s",ApplNameSpace,ApplNameID);
  else strcpy(ApplSpec,ApplNameID);
}
void dimc::nameParser::setNodeSpec(const char *nodespec){
  strcpy(NodeSpec,nodespec);
  parseNodeSpec();
}
void dimc::nameParser::setNodeName(const char *nodename){
  strcpy(NodeName,nodename);
  buildNodeSpec();
}
void dimc::nameParser::setNodeID(const char *nodeid){
  strcpy(NodeID,nodeid);
  buildNodeSpec();
}
void dimc::nameParser::setApplSpec(const char *applspec){
  strcpy(ApplSpec,applspec);
  parseApplSpec();
}
void dimc::nameParser::setApplName(const char *applname){
  strcpy(ApplName,applname);
  buildApplSpec();
}
void dimc::nameParser::setApplNameID(const char *applnameid){
  strcpy(ApplNameID,applnameid);
  parseApplNameID();
  buildApplSpec();
}
void dimc::nameParser::setApplID(const char *applid){
  strcpy(ApplID,applid);
  buildApplSpec();
}
void dimc::nameParser::setApplNameSpace(const char *applnamespace){
  strcpy(ApplNameSpace,applnamespace);
  buildApplSpec();
}
bool dimc::nameParser::parseApplSpec(){
  char *p1;
  p1=strstr(ApplSpec,"::");
  if(p1){  // namespace specified
    strcpy(ApplNameSpace,ApplSpec);
    *p1++ = 0; p1++;
    strcpy(ApplNameID,p1);
  }
  else { // no namespace
    strcpy(ApplNameID,ApplSpec);
    ApplNameSpace[0]=0;
  }
  return parseApplNameID();
}
bool dimc::nameParser::parseApplNameID(){
  char *p2;
  strcpy(ApplName,ApplNameID);
  p2=ApplName;
  while(1)  {
    while((*p2 != 0)&(*p2 != ':'))p2++;
    if(*p2 == 0) break; // error
    *p2++ = 0;
    strcpy(ApplID,p2);
    return true;
  }
  return false;
}
bool dimc::nameParser::parseNodeSpec(){
  char *p2;
  strcpy(NodeName,NodeSpec);
  p2=NodeName;
  while(1)  {
    while((*p2 != 0)&(*p2 != ':'))p2++;
    if(*p2 == 0) break; // error
    *p2++ = 0;
    strcpy(NodeID,p2);
    return true;
  }
  return false;
}

bool dimc::nameParser::parseFullName(const char *fullname){
  char *p1,*p2;
  char fn[512];
  if(fullname) strcpy(fn,fullname);
  else strcpy(fn,FullName);
  initParser();
  p1=fn;
  p2=fn;
  // get fields between the slashes
  while(1){
    while((*p2 != 0)&(*p2 != '/'))p2++;
    if(*p2 == 0) break; // error
    *p2++ =0;
    strcpy(NameSpace,p1);
    p1=p2;
    while((*p2 != 0)&(*p2 != '/'))p2++;
    if(*p2 == 0) break; // error
    *p2++ =0;
    strcpy(NodeSpec,p1);
    p1=p2;
    while((*p2 != 0)&(*p2 != '/'))p2++;
    if(*p2 == 0) break; // error
    *p2++ =0;
    strcpy(ApplSpec,p1);
    p1=p2;
    strcpy(Name,p1);
    // parse subfields
    if(!parseNodeSpec())break;
    if(!parseApplSpec())break;
    buildFullName();
    return true;
  }
  /*
  cout << "Error: " <<
  NameSpace << "/" <<
  NodeName << ":" <<
  NodeID << "/" <<
  ApplNameSpace << "::" <<
  ApplName << ":" <<
  ApplID << "/" <<
  Name << endl;
  */

  strcpy(FullName,"");
  return false;
}

void dimc::nameParser::printFields() {
  cout << "Full:  " << FullName << endl;
  cout << "Fields: " <<
  NameSpace << "/" <<
  NodeName << ":" <<
  NodeID << "/" <<
  ApplNameSpace << "::" <<
  ApplName << ":" <<
  ApplID << "/" <<
  Name << endl;

  cout << "NodeSpec: " <<
  NodeSpec << "  ApplSpec: " <<
  ApplSpec << "  ApplNameID: " <<
  ApplNameID << endl;
}
void dimc::nameParser::newCommand(const char* name, const char* scope){
  char temp[128];
  finalized=false;
  strcpy(XmlString,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  sprintf(temp,"<command name=\"%s\" scope=\"%s\">\n",name,scope);
  strcat(XmlString,temp);
}
void dimc::nameParser::addArgument(const char* name, const char* type, const char* value, const char* required){
  char temp[128];
  if(!finalized) {
    sprintf(temp,"<argument name=\"%s\" type=\"%s\" value=\"%s\" required=\"%s\"/>\n",name,type,value,required);
    strcat(XmlString,temp);
  }
}
char * dimc::nameParser::getXmlString(){
  if(!finalized) strcat(XmlString,"</command>\n");
  finalized=true;
  return XmlString;

}

const char* dimc::nameParser::createCommandDescriptorName(const char* commandname)
{
 snprintf(CommandDescriptorName, sizeof(CommandDescriptorName), "%s_",commandname);
 return CommandDescriptorName;
}

const char* dimc::nameParser::createFullParameterName(const char* modulename, const char* varname)
{
   snprintf(ParameterName, sizeof(ParameterName), "%s.%s",varname,modulename);
   return ParameterName;
}

void dimc::nameParser::parseFullParameterName(const char* fullname, char** modname, char** varname)
{
   strncpy(ParameterName,fullname,sizeof(ParameterName));
   char* colon=strstr(ParameterName,".");
   if(colon==0)
      {
       *modname=0;
       *varname=0;
      }
   else
      {
        *modname=colon+1;
        *colon=0;
        *varname=ParameterName;
      }
    //cout <<"parseFullParameterName with modname="<<*modname<<", varname="<<*varname << endl;

}

