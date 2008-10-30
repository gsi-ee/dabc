#include <iostream>
#include <unistd.h>
using namespace std;
#include <string>
#include <nameParser.h>

char dabc::xd::nameParser::ParameterName[256];
char dabc::xd::nameParser::CommandDescriptorName[256];    

dabc::xd::nameParser::nameParser(const char *fullname)
{
  parseQuality(0);
  parseFullName(fullname);
}
void dabc::xd::nameParser::initParser(){
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
void dabc::xd::nameParser::parseQuality(unsigned int qual){
  quality   = qual;
  status    =  (dabc::xd::nameParser::recordstat) (quality       & 0xff);
  type      = (dabc::xd::nameParser::recordtype) ((quality>>8)  & 0xff);
  visibility= (dabc::xd::nameParser::visiblemask)  ((quality>>16) & 0xff);
  mode      = (dabc::xd::nameParser::recordmode) ((quality>>24) & 0xff);
}

int dabc::xd::nameParser::buildQuality (dabc::xd::nameParser::recordstat s, 
                                        dabc::xd::nameParser::recordtype t, 
                                        dabc::xd::nameParser::visiblemask v, 
                                        dabc::xd::nameParser::recordmode m)
{
  status=s;
  type=t;
  visibility=v;
  mode=m;
  quality= ((unsigned int) s) |((unsigned int) (t) << 8) | ((unsigned int)(v) << 16) |((unsigned int) (m) << 24);
  return quality;
}

void dabc::xd::nameParser::setType(dabc::xd::nameParser::recordtype t)
{
    int q=quality;  
    q = (q & 0xFFFF00FF); // clear old type byte 2  
    int rt=((int) (t) << 8);
    q = (q | rt); // set new type 
    quality=q;
    type=t;   
}
        
void dabc::xd::nameParser::setVisibility(dabc::xd::nameParser::visiblemask mask)
{
    int q=quality;   
    q = (q & 0xFF00FFFF); // clear old visibility byte 2  
    int rmask=((int) (mask) << 16);
    q = (q | rmask); // set new vis mask 
    quality=q;
    visibility=mask;     
   
}
    
void dabc::xd::nameParser::setStatus(dabc::xd::nameParser::recordstat s)
{
    int q=quality; // no getter method for quality in dim c++ interface!  
    q = (q & 0xFFFFFF00); // clear old status byte 1  
    int rs=(int) (s);
    q = (q | rs); // set new status 
    quality=q;
    status=s;    
}
    
void dabc::xd::nameParser::setMode(dabc::xd::nameParser::recordmode m)
{
    int q=quality;   
    q = (q & 0x00FFFFFF); // clear old mode byte 3  
    int rm=((int) (m) << 24);
    q = (q | rm); // set new mode mask 
    quality=q;
    mode=m;   
}
      



bool dabc::xd::nameParser::checkFields(){
  if(strlen(NameSpace)==0) return false;
  if(strlen(NodeName)==0) return false;
  if(strlen(NodeID)==0) return false;
  if(strlen(ApplName)==0) return false;
  if(strlen(ApplID)==0) return false;
  if(strlen(Name)==0) return false;
  return true;
}
char* dabc::xd::nameParser::buildFullName()
{ // check if all fields are there
  if(!checkFields()) return NULL;
  buildNodeSpec();
  buildApplSpec();
  sprintf(FullName,"%s/%s/%s/%s",NameSpace,NodeSpec,ApplSpec,Name);
  return FullName;
}
void dabc::xd::nameParser::buildNodeSpec(){
  sprintf(NodeSpec,"%s:%s",NodeName,NodeID);
}
void dabc::xd::nameParser::buildApplSpec(){
  sprintf(ApplNameID,"%s:%s",ApplName,ApplID);
  if(strlen(ApplNameSpace) > 0) sprintf(ApplSpec,"%s::%s",ApplNameSpace,ApplNameID);
  else strcpy(ApplSpec,ApplNameID);
}
void dabc::xd::nameParser::setNodeSpec(const char *nodespec){
  strcpy(NodeSpec,nodespec);
  parseNodeSpec();
}
void dabc::xd::nameParser::setNodeName(const char *nodename){
  strcpy(NodeName,nodename);
  buildNodeSpec();
}
void dabc::xd::nameParser::setNodeID(const char *nodeid){
  strcpy(NodeID,nodeid);
  buildNodeSpec();
}
void dabc::xd::nameParser::setApplSpec(const char *applspec){
  strcpy(ApplSpec,applspec);
  parseApplSpec();
}
void dabc::xd::nameParser::setApplName(const char *applname){
  strcpy(ApplName,applname);
  buildApplSpec();
}
void dabc::xd::nameParser::setApplNameID(const char *applnameid){
  strcpy(ApplNameID,applnameid);
  parseApplNameID();
  buildApplSpec();
}
void dabc::xd::nameParser::setApplID(const char *applid){
  strcpy(ApplID,applid);
  buildApplSpec();
}
void dabc::xd::nameParser::setApplNameSpace(const char *applnamespace){
  strcpy(ApplNameSpace,applnamespace);
  buildApplSpec();
}
bool dabc::xd::nameParser::parseApplSpec(){
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
bool dabc::xd::nameParser::parseApplNameID(){
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
bool dabc::xd::nameParser::parseNodeSpec(){
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

bool dabc::xd::nameParser::parseFullName(const char *fullname){
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
  cout << "Error: " << 
  NameSpace << "/" <<
  NodeName << ":" <<
  NodeID << "/" <<
  ApplNameSpace << "::" <<
  ApplName << ":" <<
  ApplID << "/" <<
  Name << endl;
  strcpy(FullName,"");
  return false;
}

void dabc::xd::nameParser::printFields() {
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
void dabc::xd::nameParser::newCommand(const char* name, const char* scope){
  char temp[128];
  finalized=false;
  strcpy(XmlString,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  sprintf(temp,"<command name=\"%s\" scope=\"%s\">\n",name,scope);
  strcat(XmlString,temp);
}
void dabc::xd::nameParser::addArgument(const char* name, const char* type, const char* value, const char* required){
  char temp[128];
  if(!finalized) {
    sprintf(temp,"<argument name=\"%s\" type=\"%s\" value=\"%s\" required=\"%s\"/>\n",name,type,value,required);
    strcat(XmlString,temp);
  }
}
char * dabc::xd::nameParser::getXmlString(){
  if(!finalized) strcat(XmlString,"</command>\n");
  finalized=true;
  return XmlString;

}

const char* dabc::xd::nameParser::createCommandDescriptorName(const char* commandname)
{
 snprintf(CommandDescriptorName, sizeof(CommandDescriptorName), "%s_",commandname);  
 return CommandDescriptorName;      
}

const char* dabc::xd::nameParser::createFullParameterName(const char* modulename, const char* varname)
{
   snprintf(ParameterName, sizeof(ParameterName), "%s.%s",varname,modulename);  
   return ParameterName;
} 
  
void dabc::xd::nameParser::parseFullParameterName(const char* fullname, char** modname, char** varname)
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

