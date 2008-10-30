#ifndef DABC_XD_nameParser_H
#define DABC_XD_nameParser_H



#define DABC_VIS_VISIBLE     (1<<0) //VISIBLE
#define DABC_VIS_MONITOR     (1<<1) //MONITOR 
#define DABC_VIS_CHANGABLE  (1<<2)//CHANGABLE 
#define DABC_VIS_IMPORTANT  (1<<3) //IMPORTANT 
#define DABC_VIS_LOGGABLE  (1<<4) //write to log window 



namespace dabc{
namespace xd{

class nameParser
{
  public:
  
     /** type safety for record visility bits */
   typedef unsigned int visiblemask;
  
  
     /** type safe recordtype definitions. */
    enum recordtype {
     ATOMIC      = 0,
     GENERIC     = 1, 
     STATUS      = 2, 
     RATE        = 3, 
     HISTOGRAM   = 4,  
     MODULE      = 5, 
     PORT        = 6, 
     DEVICE      = 7, 
     QUEUE       = 8, 
     COMMANDDESC = 9,        
     INFO        = 10        
    };
    
    
   /** type safety for record status definitions */
    enum recordstat{
     NOTSPEC    = 0, 
     SUCCESS    = 1, 
     MESSAGE    = 2, 
     WARNING    = 3, 
     ERR        = 4, 
     FATAL      = 5  
    };


    
    /** type safety for record mode bits */
    enum recordmode{
     NOMODE    = 0 , 
     ANYMODE   = 1  
    }; 

  
  
  /**
   * Constructor and initialization
   */
  nameParser(){initParser(); parseQuality(0);}
  /**
   * Constructor passing a full namespec. Resets quality fields and parses fullname.
   * All fields will be set.
   * FullName: NameSpace/NodeSpec/ApplSpec/Name
   * NodeSpec: NodeName:NodeID
   * ApplSpec: [ApplNameSpace::]ApplNameID
   * ApplNameID: ApplName:ApplID
   */
  nameParser(const char *fullname);
  /**
   * Following getters return the fields
   */
  char * getNameSpace(){return NameSpace;}
  char * getNodeSpec(){return NodeSpec;} // NodeName:NodeID
  char * getNodeName(){return NodeName;}
  char * getNodeID(){return NodeID;}
  char * getApplSpec(){return ApplSpec;} // [ApplNameSpace::]ApplNameID
  char * getApplName(){return ApplName;}
  char * getApplNameSpace(){return ApplNameSpace;}
  char * getApplNameID(){return ApplNameID;} // ApplName:ApplID
  char * getApplID(){return ApplID;}
  char * getName(){return Name;}
  char * getFullName(){return FullName;}
  /**
   * Parses fullname. All fields will be set. Returns false in case of error.
   * This is normally used to enter a new DIM DABC service name.
   */
  bool parseFullName(const char *fullname=0);
  /**
   * Composes FullName from fields and returns it.
   * If a field has not been specified, NULL is returned.
   * Use this to get a new full name after setting some fields.
   */
  char * buildFullName();
  /**
   * Specify NameSpace field
   */
  void setNameSpace(const char *nameserver=0){strcpy(NameSpace,nameserver);}
  /**
   * Specify NodeSpec field. Must be NodeName:NodeID
   * Parse and fill NodeName and NodeID.
   */
  void setNodeSpec(const char *nodespec=0);
  /**
   * Specify NodeName. Build NodeSpec.
   */
  void setNodeName(const char *nodename=0);
  /**
   * Specify NodeID. Build NodeSpec.
   */
  void setNodeID(const char *nodeid=0);
  /**
   * Specify ApplSpec field. Must be [ApplNameSpace::]ApplName:ApplID.
   * Parse and fill ApplName and ApplID and optionally ApplNameSpace.
   */
  void setApplSpec(const char *applspec=0);
  /**
   * Specify ApplName. Build ApplSpec.
   */
  void setApplName(const char *applname=0);
  /**
   * Specify ApplNameID. Must be ApplName:ApplID. 
   * Parse an fill ApplName and ApplID, then build ApplSpec.
   */
  void setApplNameID(const char *applnameid=0);
  /**
   * Specify ApplID. Build ApplSpec.
   */
  void setApplID(const char *applid=0);
  /**
   * Specify ApplNameSpace. Build ApplSpec.
   */
  void setApplNameSpace(const char *applnamespace=0);
  /**
   * Specify Name
   */
  void setName(const char *name=0){strcpy(Name,name);}
  /**
   * Returns true if all fields are set, false otherwise. Used by buildFullName.
   */
  bool checkFields();
  /**
   * Following getters return the information from quality fields.
   * parseQuality or buildQuality must have been called before.
   */
  dabc::xd::nameParser::recordtype  getType(){return type;}
  dabc::xd::nameParser::recordstat getStatus(){return status;}
  dabc::xd::nameParser::visiblemask getVisibility(){return visibility;}
  dabc::xd::nameParser::recordmode getMode(){return mode;}
  bool isSuccess()    {return (status == dabc::xd::nameParser::SUCCESS);}
  bool isNotSpecified(){return (status == dabc::xd::nameParser::NOTSPEC);}
  bool isMessage()   {return (status == dabc::xd::nameParser::MESSAGE);}
  bool isWarning()   {return (status == dabc::xd::nameParser::WARNING);}
  bool isError()   {return (status == dabc::xd::nameParser::ERR);}
  bool isFatal()   {return (status == dabc::xd::nameParser::FATAL);}
 
  
  bool isAtomic()      {return (type   == dabc::xd::nameParser::ATOMIC);}
  bool isGeneric()     {return (type   == dabc::xd::nameParser::GENERIC);}
  bool isStatus()      {return (type   == dabc::xd::nameParser::STATUS);}
  bool isRate()        {return (type   == dabc::xd::nameParser::RATE);}
  bool isInfo()        {return (type   == dabc::xd::nameParser::INFO);}
  bool isHistogram()   {return (type   == dabc::xd::nameParser::HISTOGRAM);}
  bool isCommandDescriptor()  {return (type   == dabc::xd::nameParser::COMMANDDESC);}
  bool isHidden()      {return (visibility ==0);}
  bool isVisible()     {return ((visibility & DABC_VIS_VISIBLE) > 0);}
  bool isMonitor()     {return ((visibility & DABC_VIS_MONITOR) > 0);}
  bool isChangable()   {return ((visibility & DABC_VIS_CHANGABLE) > 0);}
  bool isImportant()   {return ((visibility & DABC_VIS_IMPORTANT) > 0);}
  bool isLoggable()   {return ((visibility & DABC_VIS_LOGGABLE) > 0);}
  /**
   * Returns composed quality longword.
   */
  int getQuality(){return (int)quality;}
  /**
   * Composes quality longword from fields and returns it.
   */
  int buildQuality(dabc::xd::nameParser::recordstat status, 
                    dabc::xd::nameParser::recordtype type, 
                    dabc::xd::nameParser::visiblemask visib, 
                    dabc::xd::nameParser::recordmode mode);
  
    /** set the type information in the quality word, but keep other bytes as is*/
    void setType(dabc::xd::nameParser::recordtype t);
        
    /** set visiblity information in the quality word, but keep other bytes as is*/
    void setVisibility(dabc::xd::nameParser::visiblemask mask);
    
   /** set status information in the quality word, but keep other bytes as is*/
    void setStatus(dabc::xd::nameParser::recordstat s);
    
   /** set mode information in the quality word, but keep other bytes as is*/
    void setMode(dabc::xd::nameParser::recordmode m);
        
  
  
  
  /**
   * Sets fields from quality longword
   */
  void parseQuality(unsigned int qual);
  char * toString(){return FullName;}
  void printFields();
  void newCommand(const char* name, const char* scope);
  void addArgument(const char* name, const char* type, const char* value, const char* required);
  char * getXmlString();
  
  /** deliver string for full variable or command name, containing the module name*/
  static const char* createFullParameterName(const char* modulename, const char* varname);
  
  /** takes fullname and writes name of module and reduced variable name to output chars*/
  static void parseFullParameterName(const char* fullname, char** modname, char** varname);

  /** derive command descriptor name from command name*/
  static const char* createCommandDescriptorName(const char* commandname);
  
   
     

private:
  void initParser();
  bool parseApplSpec();
  bool parseApplNameID();
  bool parseNodeSpec();
  void buildApplSpec();
  void buildNodeSpec();

private:
  char NameSpace[64];
  char NodeSpec[64];
  char NodeName[64];
  char NodeID[64];
  char ApplSpec[128];
  char ApplName[64];
  char ApplNameSpace[64];
  char ApplNameID[64];
  char ApplID[64];
  char Name[64];
  char FullName[256];
  char XmlString[1024];
  char ArgString[128];
  bool finalized;
  unsigned int quality;
  dabc::xd::nameParser::recordstat status;
  dabc::xd::nameParser::visiblemask visibility;
  dabc::xd::nameParser::recordtype type;
  dabc::xd::nameParser::recordmode mode;

  static char ParameterName[256];
  static char CommandDescriptorName[256];  
};

} // end namespace xd
} // end namespace dabc

#endif
