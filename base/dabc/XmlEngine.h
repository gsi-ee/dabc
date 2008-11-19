#ifndef DABC_XmlEngine
#define DABC_XmlEngine

#ifndef DABC_string
#include "dabc/string.h"
#endif

namespace dabc {

   typedef void* XMLNodePointer_t;
   typedef void* XMLNsPointer_t;
   typedef void* XMLAttrPointer_t;
   typedef void* XMLDocPointer_t;

   class XmlInputStream;
   class XmlOutputStream;

   class XmlEngine {
      public:
         XmlEngine();
         virtual ~XmlEngine();

         bool              HasAttr(XMLNodePointer_t xmlnode, const char* name);
         const char*       GetAttr(XMLNodePointer_t xmlnode, const char* name);
         int               GetIntAttr(XMLNodePointer_t node, const char* name);
         XMLAttrPointer_t  NewAttr(XMLNodePointer_t xmlnode, XMLNsPointer_t,
                                   const char* name, const char* value);
         XMLAttrPointer_t  NewIntAttr(XMLNodePointer_t xmlnode, const char* name, int value);
         void              FreeAttr(XMLNodePointer_t xmlnode, const char* name);
         void              FreeAllAttr(XMLNodePointer_t xmlnode);
         XMLAttrPointer_t  GetFirstAttr(XMLNodePointer_t xmlnode);
         XMLAttrPointer_t  GetNextAttr(XMLAttrPointer_t xmlattr);
         const char*       GetAttrName(XMLAttrPointer_t xmlattr);
         const char*       GetAttrValue(XMLAttrPointer_t xmlattr);
         XMLNodePointer_t  NewChild(XMLNodePointer_t parent, XMLNsPointer_t ns,
                                    const char* name, const char* content = 0);
         XMLNsPointer_t    NewNS(XMLNodePointer_t xmlnode, const char* reference, const char* name = 0);
         XMLNsPointer_t    GetNS(XMLNodePointer_t xmlnode);
         const char*       GetNSName(XMLNsPointer_t ns);
         const char*       GetNSReference(XMLNsPointer_t ns);
         void              AddChild(XMLNodePointer_t parent, XMLNodePointer_t child);
         void              AddChildFirst(XMLNodePointer_t parent, XMLNodePointer_t child);
         bool              AddComment(XMLNodePointer_t parent, const char* comment);
         bool              AddDocComment(XMLDocPointer_t xmldoc, const char* comment);
         bool              AddRawLine(XMLNodePointer_t parent, const char* line);
         bool              AddDocRawLine(XMLDocPointer_t xmldoc, const char* line);
         bool              AddStyleSheet(XMLNodePointer_t parent,
                                         const char* href,
                                         const char* type = "text/css",
                                         const char* title = 0,
                                         int alternate = -1,
                                         const char* media = 0,
                                         const char* charset = 0);
         bool              AddDocStyleSheet(XMLDocPointer_t xmldoc,
                                            const char* href,
                                            const char* type = "text/css",
                                            const char* title = 0,
                                            int alternate = -1,
                                            const char* media = 0,
                                            const char* charset = 0);
         void              UnlinkNode(XMLNodePointer_t node);
         void              FreeNode(XMLNodePointer_t xmlnode);
         void              UnlinkFreeNode(XMLNodePointer_t xmlnode);
         const char*       GetNodeName(XMLNodePointer_t xmlnode);
         const char*       GetNodeContent(XMLNodePointer_t xmlnode);
         XMLNodePointer_t  GetChild(XMLNodePointer_t xmlnode);
         XMLNodePointer_t  GetParent(XMLNodePointer_t xmlnode);
         XMLNodePointer_t  GetNext(XMLNodePointer_t xmlnode);
         void              ShiftToNext(XMLNodePointer_t &xmlnode, bool tonode = true);
         bool              IsEmptyNode(XMLNodePointer_t xmlnode);
         void              SkipEmpty(XMLNodePointer_t &xmlnode);
         void              CleanNode(XMLNodePointer_t xmlnode);
         XMLDocPointer_t   NewDoc(const char* version = "1.0");
         void              AssignDtd(XMLDocPointer_t xmldoc, const char* dtdname, const char* rootname);
         void              FreeDoc(XMLDocPointer_t xmldoc);
         void              SaveDoc(XMLDocPointer_t xmldoc, const char* filename, int layout = 1);
         void              DocSetRootElement(XMLDocPointer_t xmldoc, XMLNodePointer_t xmlnode);
         XMLNodePointer_t  DocGetRootElement(XMLDocPointer_t xmldoc);
         XMLDocPointer_t   ParseFile(const char* filename, bool showerr = true);
         bool              ValidateVersion(XMLDocPointer_t doc, const char* version = 0);
         void              SaveSingleNode(XMLNodePointer_t xmlnode, std::string* res, int layout = 1);
         XMLNodePointer_t  ReadSingleNode(const char* src);
      protected:
         char*             Makestr(const char* str);
         char*             Makenstr(const char* start, int len);
         XMLNodePointer_t  AllocateNode(int namelen, XMLNodePointer_t parent);
         XMLAttrPointer_t  AllocateAttr(int namelen, int valuelen, XMLNodePointer_t xmlnode);
         XMLNsPointer_t    FindNs(XMLNodePointer_t xmlnode, const char* nsname);
         void              TruncateNsExtension(XMLNodePointer_t xmlnode);
         void              UnpackSpecialCharacters(char* target, const char* source, int srclen);
         void              OutputValue(char* value, XmlOutputStream* out);
         void              SaveNode(XMLNodePointer_t xmlnode, XmlOutputStream* out, int layout, int level);
         XMLNodePointer_t  ReadNode(XMLNodePointer_t xmlparent, XmlInputStream* inp, int& resvalue);
         void              DisplayError(int error, int linenumber);
   };
}

#endif
