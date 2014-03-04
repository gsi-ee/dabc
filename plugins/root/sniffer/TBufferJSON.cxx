// $Id$
// Author: Sergey Linev  4.03.2014

/*************************************************************************
 * Copyright (C) 1995-2004, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//________________________________________________________________________
//
// Class for serializing/deserializing object to/from xml.
// It redefines most of TBuffer class function to convert simple types,
// array of simple types and objects to/from xml.
// Instead of writing a binary data it creates a set of xml structures as
// nodes and attributes
// TBufferJSON class uses streaming mechanism, provided by ROOT system,
// therefore most of ROOT and user classes can be stored to xml. There are
// limitations for complex objects like TTree, which can not be yet converted to xml.
//________________________________________________________________________


#include "TBufferJSON.h"

#include "Compression.h"

#include "TObjArray.h"
#include "TROOT.h"
#include "TClass.h"
#include "TClassTable.h"
#include "TDataType.h"
#include "TExMap.h"
#include "TMethodCall.h"
#include "TStreamerInfo.h"
#include "TStreamerElement.h"
#include "TProcessID.h"
#include "TFile.h"
#include "TMemberStreamer.h"
#include "TStreamer.h"
#include "TStreamerInfoActions.h"

#ifdef R__VISUAL_CPLUSPLUS
#define FLong64    "%I64d"
#define FULong64   "%I64u"
#else
#define FLong64    "%lld"
#define FULong64   "%llu"
#endif

ClassImp(TBufferJSON);


const char* TBufferJSON::fgFloatFmt = "%e";

//______________________________________________________________________________
TBufferJSON::TBufferJSON() :
   TBufferFile(),
   fOutBuffer(),
   fObjMap(),
   fIsAnyValue(kFALSE),
   fStack(),
   fErrorFlag(0),
   fExpectedChain(kFALSE),
   fExpectedBaseClass(0),
   fCompressLevel(0)

{
   // Default constructor
}

//______________________________________________________________________________
TBufferJSON::TBufferJSON(TBuffer::EMode mode) :
   TBufferFile(mode),
   fOutBuffer(),
   fObjMap(),
   fIsAnyValue(kFALSE),
   fStack(),
   fErrorFlag(0),
   fExpectedChain(kFALSE),
   fExpectedBaseClass(0),
   fCompressLevel(0)
{
   // Creates buffer object to serailize/deserialize data to/from xml.
   // Mode should be either TBuffer::kRead or TBuffer::kWrite.

   fBufSize = 1000000000;

   SetParent(0);
   SetBit(kCannotHandleMemberWiseStreaming);
   SetBit(kTextBasedStreaming);
}

//______________________________________________________________________________
TBufferJSON::~TBufferJSON()
{
   // destroy xml buffer

   fStack.Delete();
}

//______________________________________________________________________________
TString TBufferJSON::ConvertToJSON(const TObject* obj)
{
   // converts object, inherited from TObject class, to JSON string

   return ConvertToJSON(obj, obj ? obj->IsA() : 0);
}

//______________________________________________________________________________
TString TBufferJSON::ConvertToJSON(const void* obj, const TClass* cl)
{
   // converts any type of object to XML string

   TBufferJSON buf(TBuffer::kWrite);

   return buf.JsonWriteAny(obj, cl);
}

//______________________________________________________________________________
TString TBufferJSON::JsonWriteAny(const void* obj, const TClass* cl)
{
   // Convert object of any class to xml structures
   // Return pointer on top xml element

   fErrorFlag = 0;

   fOutBuffer.Clear();

   JsonWriteObject(obj, cl);

   return fOutBuffer;
}


//______________________________________________________________________________
void TBufferJSON::WriteObject(const TObject *obj)
{
   // Convert object into xml structures.
   // !!! Should be used only by TBufferJSON itself.
   // Use ConvertToXML() methods to convert your object to xml
   // Redefined here to avoid gcc 3.x warning

   TBufferFile::WriteObject(obj);
}

// TJSONStackObj is used to keep stack of object hierarchy,
// stored in TBuffer. For instance, data for parent class(es)
// stored in subnodes, but initial object node will be kept.

class TJSONStackObj : public TObject {
   public:
      TJSONStackObj() :
         TObject(),
         fInfo(0),
         fElem(0),
         fElemNumber(0),
         fIsStreamerInfo(kFALSE),
         fIsElemOwner(kFALSE)
      {}

      virtual ~TJSONStackObj()
      {
         if (fIsElemOwner) delete fElem;
      }

      Bool_t IsStreamerInfo() const { return fIsStreamerInfo; }

      TStreamerInfo*    fInfo;
      TStreamerElement* fElem;
      Int_t             fElemNumber;
      Bool_t            fIsStreamerInfo;
      Bool_t            fIsElemOwner;
};

//______________________________________________________________________________
TJSONStackObj* TBufferJSON::PushStack(Bool_t simple)
{
   // add new level to xml stack

   TJSONStackObj* stack = new TJSONStackObj();
   fStack.Add(stack);
   return stack;
}

//______________________________________________________________________________
TJSONStackObj* TBufferJSON::PopStack()
{
   // remove one level from xml stack

   TObject* last = fStack.Last();
   if (last!=0) {
      fStack.Remove(last);
      delete last;
      fStack.Compress();
   }
   return dynamic_cast<TJSONStackObj*> (fStack.Last());
}

//______________________________________________________________________________
TJSONStackObj* TBufferJSON::Stack(Int_t depth)
{
   // return xml stack object of specified depth

   TJSONStackObj* stack = 0;
   if (depth<=fStack.GetLast())
      stack = dynamic_cast<TJSONStackObj*> (fStack.At(fStack.GetLast()-depth));
   return stack;
}

//______________________________________________________________________________
void TBufferJSON::SetCompressionAlgorithm(Int_t algorithm)
{
   // See comments for function SetCompressionSettings
   if (algorithm < 0 || algorithm >= ROOT::kUndefinedCompressionAlgorithm) algorithm = 0;
   if (fCompressLevel < 0) {
      // if the level is not defined yet use 1 as a default
      fCompressLevel = 100 * algorithm + 1;
   } else {
      int level = fCompressLevel % 100;
      fCompressLevel = 100 * algorithm + level;
   }
}

//______________________________________________________________________________
void TBufferJSON::SetCompressionLevel(Int_t level)
{
   // See comments for function SetCompressionSettings
   if (level < 0) level = 0;
   if (level > 99) level = 99;
   if (fCompressLevel < 0) {
      // if the algorithm is not defined yet use 0 as a default
      fCompressLevel = level;
   } else {
      int algorithm = fCompressLevel / 100;
      if (algorithm >= ROOT::kUndefinedCompressionAlgorithm) algorithm = 0;
      fCompressLevel = 100 * algorithm + level;
   }
}

//______________________________________________________________________________
void TBufferJSON::SetCompressionSettings(Int_t settings)
{
   // Used to specify the compression level and algorithm:
   //  settings = 100 * algorithm + level
   //
   //  level = 0 no compression.
   //  level = 1 minimal compression level but fast.
   //  ....
   //  level = 9 maximal compression level but slower and might use more memory.
   // (For the currently supported algorithms, the maximum level is 9)
   // If compress is negative it indicates the compression level is not set yet.
   //
   // The enumeration ROOT::ECompressionAlgorithm associates each
   // algorithm with a number. There is a utility function to help
   // to set the value of compress. For example,
   //   ROOT::CompressionSettings(ROOT::kLZMA, 1)
   // will build an integer which will set the compression to use
   // the LZMA algorithm and compression level 1.  These are defined
   // in the header file Compression.h.

   fCompressLevel = settings;
}

//______________________________________________________________________________
void TBufferJSON::AppendOutput(const char* val)
{
   if (val!=0) fOutBuffer.Append(val);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteObject(const void* obj, const TClass* cl)
{
   // Write object to buffer
   // If object was written before, only pointer will be stored
   // Return pointer to top xml node, representing object

   if (!cl) obj = 0;

   if (obj==0) { AppendOutput("{}\n"); return; }

   if (gDebug>1)
      Info("JsonWriteObject","Done write for class: %s", cl ? cl->GetName() : "null");

   for(unsigned n=0;n<fObjMap.size();n++)
      if (fObjMap[n] == obj) {
         AppendOutput(Form("{ $ref:id%u }\n", n));
         return;
      }

   fObjMap.push_back(obj);

   AppendOutput(" {\n");

   // TString clname = XmlConvertClassName(cl->GetName());
   // fXML->NewAttr(objnode, 0, xmlio::ObjClass, clname);

   ((TClass*)cl)->Streamer((void*)obj, *this);

   AppendOutput(" }\n");
}

//______________________________________________________________________________
void TBufferJSON::IncrementLevel(TVirtualStreamerInfo* info)
{
   // Function is called from TStreamerInfo WriteBuffer and Readbuffer functions
   // and indent new level in xml structure.
   // This call indicates, that TStreamerInfo functions starts streaming
   // object data of correspondent class

   WorkWithClass((TStreamerInfo*)info);
}

//______________________________________________________________________________
void  TBufferJSON::WorkWithClass(TStreamerInfo* sinfo, const TClass* cl)
{
   // Prepares buffer to stream data of specified class

   fIsAnyValue = kFALSE;
   fExpectedChain = kFALSE;

   if (sinfo!=0) cl = sinfo->GetClass();

   if (cl==0) return;

   if (gDebug>2) Info("IncrementLevel","Class: %s", cl->GetName());

   if (fExpectedBaseClass == cl) {
      // normal situation - do nothing

   } else {

      // not a normal situation
      Error("WorkWithClass", "Provided class %p do not correspond with expected %p", cl, fExpectedBaseClass);
   }
   fExpectedBaseClass = 0;

   TJSONStackObj* stack = PushStack();
   stack->fInfo = sinfo;
   stack->fIsStreamerInfo = kTRUE;
}

//______________________________________________________________________________
void TBufferJSON::DecrementLevel(TVirtualStreamerInfo* info)
{
   // Function is called from TStreamerInfo WriteBuffer and Readbuffer functions
   // and decrease level in xml structure.

   fIsAnyValue = kFALSE;
   fExpectedChain = kFALSE;

   if (gDebug>2)
      Info("DecrementLevel","Class: %s", (info ? info->GetClass()->GetName() : "custom"));

   TJSONStackObj* stack = Stack();

   if (!stack->IsStreamerInfo()) {
      // PerformPostProcessing();
      if (!fIsAnyValue) AppendOutput(" null");
      PopStack();  // remove stack of last element
   }

   PopStack();                       // back from data of stack info
}

//______________________________________________________________________________
void TBufferJSON::SetStreamerElementNumber(Int_t number)
{
   // Function is called from TStreamerInfo WriteBuffer and Readbuffer functions
   // and add/verify next element of xml structure
   // This calls allows separate data, correspondent to one class member, from another

   WorkWithElement(0, number);
}

//______________________________________________________________________________
void TBufferJSON::WorkWithElement(TStreamerElement* elem, Int_t number)
{
   //to be documented by Sergey

   fExpectedChain = kFALSE;
   fExpectedBaseClass = 0;

   TJSONStackObj* stack = Stack();
   if (stack==0) {
      Error("SetStreamerElementNumber", "stack is empty");
      return;
   }

   if (!stack->IsStreamerInfo()) {   // this is not a first element

      // TODO: probably for TString one could make post processing as before
      // PerformPostProcessing();

      if (!fIsAnyValue) AppendOutput(" null");
      AppendOutput(",\n");

      PopStack();                    // go level back
      stack = dynamic_cast<TJSONStackObj*> (fStack.Last());
   }

   fIsAnyValue = kFALSE;

   if (stack==0) {
      Error("SetStreamerElementNumber", "Lost of stack");
      return;
   }

   Int_t comp_type = 0;

   if ((number>=0) && (elem==0)) {

      TStreamerInfo* info = stack->fInfo;
      if (!stack->IsStreamerInfo()) {
         Error("SetStreamerElementNumber", "Problem in Inc/Dec level");
         return;
      }

      comp_type = info->GetTypes()[number];

      elem = info->GetStreamerElementReal(number, 0);
   } else if (elem) {
      comp_type = elem->GetType();
   }

   if (elem==0) {
      Error("SetStreamerElementNumber", "streamer info returns elem = 0");
      return;
   }

   if (gDebug>4) Info("SetStreamerElementNumber", "    Next element %s", elem->GetName());

   Bool_t isBasicType = (elem->GetType()>0) && (elem->GetType()<20);

   fExpectedChain = isBasicType && (comp_type - elem->GetType() == TStreamerInfo::kOffsetL);

   if (fExpectedChain && (gDebug>3))
      Info("SetStreamerElementNumber",
           "    Expects chain for elem %s number %d",
            elem->GetName(), number);

   if ((elem->GetType()==TStreamerInfo::kBase) ||
       ((elem->GetType()==TStreamerInfo::kTNamed) && !strcmp(elem->GetName(), TNamed::Class()->GetName())))
      fExpectedBaseClass = elem->GetClassPointer();

   if (fExpectedBaseClass && (gDebug>3))
      Info("SetStreamerElementNumber",
           "   Expects base class %s with standard streamer",
               fExpectedBaseClass->GetName());

   if (fExpectedBaseClass==0) {
      AppendOutput(elem->GetName());
      AppendOutput(" : ");
   }


   stack = PushStack();
   stack->fElem = (TStreamerElement*) elem;
   stack->fElemNumber = number;
   stack->fIsElemOwner = (number<0);
}

//______________________________________________________________________________
void TBufferJSON::ClassBegin(const TClass* cl, Version_t)
{
   //to be documented by Sergey
   WorkWithClass(0, cl);
}

//______________________________________________________________________________
void TBufferJSON::ClassEnd(const TClass*)
{
   //to be documented by Sergey
   DecrementLevel(0);
}

//______________________________________________________________________________
void TBufferJSON::ClassMember(const char* name, const char* typeName, Int_t arrsize1, Int_t arrsize2)
{
   //to be documented by Sergey
   if (typeName==0) typeName = name;

   if ((name==0) || (strlen(name)==0)) {
      Error("ClassMember","Invalid member name");
      fErrorFlag = 1;
      return;
   }

   TString tname = typeName;

   Int_t typ_id = -1;

   if (strcmp(typeName,"raw:data")==0)
      typ_id = TStreamerInfo::kMissing;

   if (typ_id<0) {
      TDataType *dt = gROOT->GetType(typeName);
      if (dt!=0)
         if ((dt->GetType()>0) && (dt->GetType()<20))
            typ_id = dt->GetType();
   }

   if (typ_id<0)
      if (strcmp(name, typeName)==0) {
         TClass* cl = TClass::GetClass(tname.Data());
         if (cl!=0) typ_id = TStreamerInfo::kBase;
      }

   if (typ_id<0) {
      Bool_t isptr = kFALSE;
      if (tname[tname.Length()-1]=='*') {
         tname.Resize(tname.Length()-1);
         isptr = kTRUE;
      }
      TClass* cl = TClass::GetClass(tname.Data());
      if (cl==0) {
         Error("ClassMember","Invalid class specifier %s", typeName);
         fErrorFlag = 1;
         return;
      }

      if (cl->IsTObject())
         typ_id = isptr ? TStreamerInfo::kObjectp : TStreamerInfo::kObject;
      else
         typ_id = isptr ? TStreamerInfo::kAnyp : TStreamerInfo::kAny;

      if ((cl==TString::Class()) && !isptr)
         typ_id = TStreamerInfo::kTString;
   }

   TStreamerElement* elem = 0;

   if (typ_id == TStreamerInfo::kMissing) {
      elem = new TStreamerElement(name,"title",0, typ_id, "raw:data");
   } else


   if (typ_id==TStreamerInfo::kBase) {
      TClass* cl = TClass::GetClass(tname.Data());
      if (cl!=0) {
         TStreamerBase* b = new TStreamerBase(tname.Data(), "title", 0);
         b->SetBaseVersion(cl->GetClassVersion());
         elem = b;
      }
   } else

   if ((typ_id>0) && (typ_id<20)) {
      elem = new TStreamerBasicType(name, "title", 0, typ_id, typeName);
   } else

   if ((typ_id==TStreamerInfo::kObject) ||
       (typ_id==TStreamerInfo::kTObject) ||
       (typ_id==TStreamerInfo::kTNamed)) {
      elem = new TStreamerObject(name, "title", 0, tname.Data());
   } else

   if (typ_id==TStreamerInfo::kObjectp) {
      elem = new TStreamerObjectPointer(name, "title", 0, tname.Data());
   } else

   if (typ_id==TStreamerInfo::kAny) {
      elem = new TStreamerObjectAny(name, "title", 0, tname.Data());
   } else

   if (typ_id==TStreamerInfo::kAnyp) {
      elem = new TStreamerObjectAnyPointer(name, "title", 0, tname.Data());
   } else

   if (typ_id==TStreamerInfo::kTString) {
      elem = new TStreamerString(name, "title", 0);
   }

   if (elem==0) {
      Error("ClassMember","Invalid combination name = %s type = %s", name, typeName);
      fErrorFlag = 1;
      return;
   }

   if (arrsize1>0) {
      elem->SetArrayDim(arrsize2>0 ? 2 : 1);
      elem->SetMaxIndex(0, arrsize1);
      if (arrsize2>0)
         elem->SetMaxIndex(1, arrsize2);
   }

   // we indicate that there is no streamerinfo
   WorkWithElement(elem, -1);
}

//______________________________________________________________________________
void TBufferJSON::PerformPostProcessing()
{
   // Function is converts TObject and TString structures to more compact representation

}

//______________________________________________________________________________
TClass* TBufferJSON::ReadClass(const TClass*, UInt_t*)
{
   // suppressed function of TBuffer

   return 0;
}

//______________________________________________________________________________
void TBufferJSON::WriteClass(const TClass*)
{
   // suppressed function of TBuffer

}

//______________________________________________________________________________
Int_t TBufferJSON::CheckByteCount(UInt_t /*r_s */, UInt_t /*r_c*/, const TClass* /*cl*/)
{
   // suppressed function of TBuffer

   return 0;
}

//______________________________________________________________________________
Int_t  TBufferJSON::CheckByteCount(UInt_t, UInt_t, const char*)
{
   // suppressed function of TBuffer

   return 0;
}

//______________________________________________________________________________
void TBufferJSON::SetByteCount(UInt_t, Bool_t)
{
   // suppressed function of TBuffer

}

//______________________________________________________________________________
void TBufferJSON::SkipVersion(const TClass *cl)
{
   // Skip class version from I/O buffer.
   ReadVersion(0,0,cl);
}
   
//______________________________________________________________________________
Version_t TBufferJSON::ReadVersion(UInt_t *start, UInt_t *bcnt, const TClass * /*cl*/)
{
   // read version value from buffer

   Version_t res = 0;

   if (start) *start = 0;
   if (bcnt) *bcnt = 0;

   if (gDebug>2) Info("ReadVersion","Version = %d", res);

   return res;
}

//______________________________________________________________________________
UInt_t TBufferJSON::WriteVersion(const TClass *cl, Bool_t /* useBcnt */)
{
   // Copies class version to buffer, but not writes it to xml
   // Version will be written with next I/O operation or
   // will be added as attribute of class tag, created by IncrementLevel call

   if (fExpectedBaseClass!=cl)
      fExpectedBaseClass = 0;

   if (gDebug>2)
      Info("WriteVersion", "Class: %s, version = %d",
           cl->GetName(), cl->GetClassVersion());

   return 0;
}

//______________________________________________________________________________
void* TBufferJSON::ReadObjectAny(const TClass*)
{
   // Read object from buffer. Only used from TBuffer

   return 0;
}

//______________________________________________________________________________
void TBufferJSON::SkipObjectAny()
{
   // Skip any kind of object from buffer
   // Actually skip only one node on current level of xml structure
}

//______________________________________________________________________________
void TBufferJSON::WriteObjectClass(const void *actualObjStart, const TClass *actualClass)
{
   // Write object to buffer. Only used from TBuffer

   if (gDebug>2)
      Info("WriteObject","Class %s", (actualClass ? actualClass->GetName() : " null"));

   JsonWriteObject(actualObjStart, actualClass);
}


// macro to read array, which include size attribute
#define TBufferJSON_ReadArray(tname, vname)                   \
{                                                             \
   if (!vname) return 0;                                      \
   return 1;                                                  \
}

//______________________________________________________________________________
void TBufferJSON::ReadFloat16 (Float_t *, TStreamerElement * /*ele*/)
{
   // read a Float16_t from the buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadDouble32 (Double_t *, TStreamerElement * /*ele*/)
{
   // read a Double32_t from the buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadWithFactor(Float_t *, Double_t /* factor */, Double_t /* minvalue */)
{
   // Read a Double32_t from the buffer when the factor and minimun value have been specified
   // see comments about Double32_t encoding at TBufferFile::WriteDouble32().
   // Currently TBufferJSON does not optimize space in this case.
   
}

//______________________________________________________________________________
void TBufferJSON::ReadWithNbits(Float_t *, Int_t /* nbits */)
{
   // Read a Float16_t from the buffer when the number of bits is specified (explicitly or not)
   // see comments about Float16_t encoding at TBufferFile::WriteFloat16().
   // Currently TBufferJSON does not optimize space in this case.
   
}

//______________________________________________________________________________
void TBufferJSON::ReadWithFactor(Double_t *, Double_t /* factor */, Double_t /* minvalue */)
{
   // Read a Double32_t from the buffer when the factor and minimun value have been specified
   // see comments about Double32_t encoding at TBufferFile::WriteDouble32().   
   // Currently TBufferJSON does not optimize space in this case.
   
}

//______________________________________________________________________________
void TBufferJSON::ReadWithNbits(Double_t *, Int_t /* nbits */)
{
   // Read a Double32_t from the buffer when the number of bits is specified (explicitly or not)
   // see comments about Double32_t encoding at TBufferFile::WriteDouble32().   
   // Currently TBufferJSON does not optimize space in this case.
   
}

//______________________________________________________________________________
void TBufferJSON::WriteFloat16 (Float_t *f, TStreamerElement * /*ele*/)
{
   // write a Float16_t to the buffer
   JsonWriteBasic(*f);
}

//______________________________________________________________________________
void TBufferJSON::WriteDouble32 (Double_t *d, TStreamerElement * /*ele*/)
{
   // write a Double32_t to the buffer
   JsonWriteBasic(*d);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArray(Bool_t    *&b)
{
   // Read array of Bool_t from buffer

   TBufferJSON_ReadArray(Bool_t,b);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArray(Char_t    *&c)
{
   // Read array of Char_t from buffer

   TBufferJSON_ReadArray(Char_t,c);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArray(UChar_t   *&c)
{
   // Read array of UChar_t from buffer

   TBufferJSON_ReadArray(UChar_t,c);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArray(Short_t   *&h)
{
   // Read array of Short_t from buffer

   TBufferJSON_ReadArray(Short_t,h);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArray(UShort_t  *&h)
{
   // Read array of UShort_t from buffer

   TBufferJSON_ReadArray(UShort_t,h);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArray(Int_t     *&i)
{
   // Read array of Int_t from buffer

   TBufferJSON_ReadArray(Int_t,i);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArray(UInt_t    *&i)
{
   // Read array of UInt_t from buffer

   TBufferJSON_ReadArray(UInt_t,i);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArray(Long_t    *&l)
{
   // Read array of Long_t from buffer

   TBufferJSON_ReadArray(Long_t,l);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArray(ULong_t   *&l)
{
   // Read array of ULong_t from buffer

   TBufferJSON_ReadArray(ULong_t,l);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArray(Long64_t  *&l)
{
   // Read array of Long64_t from buffer

   TBufferJSON_ReadArray(Long64_t,l);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArray(ULong64_t *&l)
{
   // Read array of ULong64_t from buffer

   TBufferJSON_ReadArray(ULong64_t,l);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArray(Float_t   *&f)
{
   // Read array of Float_t from buffer

   TBufferJSON_ReadArray(Float_t,f);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArray(Double_t  *&d)
{
   // Read array of Double_t from buffer

   TBufferJSON_ReadArray(Double_t,d);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArrayFloat16(Float_t  *&f, TStreamerElement * /*ele*/)
{
   // Read array of Float16_t from buffer

   TBufferJSON_ReadArray(Float_t,f);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadArrayDouble32(Double_t  *&d, TStreamerElement * /*ele*/)
{
   // Read array of Double32_t from buffer

   TBufferJSON_ReadArray(Double_t,d);
}

// macro to read array from xml buffer
#define TBufferJSON_ReadStaticArray(vname)                          \
{                                                                   \
   if (!vname) return 0;                                            \
   return 1;                                                        \
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArray(Bool_t    *b)
{
   // Read array of Bool_t from buffer

   TBufferJSON_ReadStaticArray(b);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArray(Char_t    *c)
{
   // Read array of Char_t from buffer

   TBufferJSON_ReadStaticArray(c);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArray(UChar_t   *c)
{
   // Read array of UChar_t from buffer

   TBufferJSON_ReadStaticArray(c);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArray(Short_t   *h)
{
   // Read array of Short_t from buffer

   TBufferJSON_ReadStaticArray(h);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArray(UShort_t  *h)
{
   // Read array of UShort_t from buffer

   TBufferJSON_ReadStaticArray(h);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArray(Int_t     *i)
{
   // Read array of Int_t from buffer

   TBufferJSON_ReadStaticArray(i);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArray(UInt_t    *i)
{
   // Read array of UInt_t from buffer

   TBufferJSON_ReadStaticArray(i);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArray(Long_t    *l)
{
   // Read array of Long_t from buffer

   TBufferJSON_ReadStaticArray(l);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArray(ULong_t   *l)
{
   // Read array of ULong_t from buffer

   TBufferJSON_ReadStaticArray(l);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArray(Long64_t  *l)
{
   // Read array of Long64_t from buffer

   TBufferJSON_ReadStaticArray(l);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArray(ULong64_t *l)
{
   // Read array of ULong64_t from buffer

   TBufferJSON_ReadStaticArray(l);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArray(Float_t   *f)
{
   // Read array of Float_t from buffer

   TBufferJSON_ReadStaticArray(f);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArray(Double_t  *d)
{
   // Read array of Double_t from buffer

   TBufferJSON_ReadStaticArray(d);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArrayFloat16(Float_t  *f, TStreamerElement * /*ele*/)
{
   // Read array of Float16_t from buffer

   TBufferJSON_ReadStaticArray(f);
}

//______________________________________________________________________________
Int_t TBufferJSON::ReadStaticArrayDouble32(Double_t  *d, TStreamerElement * /*ele*/)
{
   // Read array of Double32_t from buffer

   TBufferJSON_ReadStaticArray(d);
}

// macro to read content of array, which not include size of array
// macro also treat situation, when instead of one single array chain
// of several elements should be produced
#define TBufferJSON_ReadFastArray(vname)                                  \
{                                                                         \
   if (n<=0) return;                                                      \
   if (!vname) return;                                                    \
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(Bool_t    *b, Int_t n)
{
   // read array of Bool_t from buffer

   TBufferJSON_ReadFastArray(b);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(Char_t    *c, Int_t n)
{
   // read array of Char_t from buffer

  TBufferJSON_ReadFastArray(c);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(UChar_t   *c, Int_t n)
{
   // read array of UChar_t from buffer

   TBufferJSON_ReadFastArray(c);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(Short_t   *h, Int_t n)
{
   // read array of Short_t from buffer

   TBufferJSON_ReadFastArray(h);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(UShort_t  *h, Int_t n)
{
   // read array of UShort_t from buffer

   TBufferJSON_ReadFastArray(h);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(Int_t     *i, Int_t n)
{
   // read array of Int_t from buffer

   TBufferJSON_ReadFastArray(i);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(UInt_t    *i, Int_t n)
{
   // read array of UInt_t from buffer

   TBufferJSON_ReadFastArray(i);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(Long_t    *l, Int_t n)
{
   // read array of Long_t from buffer

   TBufferJSON_ReadFastArray(l);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(ULong_t   *l, Int_t n)
{
   // read array of ULong_t from buffer

   TBufferJSON_ReadFastArray(l);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(Long64_t  *l, Int_t n)
{
   // read array of Long64_t from buffer

   TBufferJSON_ReadFastArray(l);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(ULong64_t *l, Int_t n)
{
   // read array of ULong64_t from buffer

   TBufferJSON_ReadFastArray(l);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(Float_t   *f, Int_t n)
{
   // read array of Float_t from buffer

   TBufferJSON_ReadFastArray(f);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(Double_t  *d, Int_t n)
{
   // read array of Double_t from buffer

   TBufferJSON_ReadFastArray(d);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArrayFloat16(Float_t  *f, Int_t n, TStreamerElement * /*ele*/)
{
   // read array of Float16_t from buffer

   TBufferJSON_ReadFastArray(f);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArrayWithFactor(Float_t  *f, Int_t n, Double_t /* factor */, Double_t /* minvalue */)
{
   // read array of Float16_t from buffer

   TBufferJSON_ReadFastArray(f);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArrayWithNbits(Float_t  *f, Int_t n, Int_t /*nbits*/)
{
   // read array of Float16_t from buffer

   TBufferJSON_ReadFastArray(f);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArrayDouble32(Double_t  *d, Int_t n, TStreamerElement * /*ele*/)
{
   // read array of Double32_t from buffer

   TBufferJSON_ReadFastArray(d);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArrayWithFactor(Double_t  *d, Int_t n, Double_t /* factor */, Double_t /* minvalue */)
{
   // read array of Double32_t from buffer

   TBufferJSON_ReadFastArray(d);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArrayWithNbits(Double_t  *d, Int_t n, Int_t /*nbits*/)
{
   // read array of Double32_t from buffer

   TBufferJSON_ReadFastArray(d);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(void  *start, const TClass *cl, Int_t n, TMemberStreamer *s, const TClass *onFileClass)
{
   // redefined here to avoid warning message from gcc

   TBufferFile::ReadFastArray(start, cl, n, s, onFileClass);
}

//______________________________________________________________________________
void TBufferJSON::ReadFastArray(void **startp, const TClass *cl, Int_t n, Bool_t isPreAlloc, TMemberStreamer *s, const TClass *onFileClass)
{
   // redefined here to avoid warning message from gcc

   TBufferFile::ReadFastArray(startp, cl, n, isPreAlloc, s, onFileClass);
}


#define TJSONWriteArrayContent(vname, arrsize)      \
{                                                  \
   for(Int_t indx=0;indx<arrsize;indx++)           \
     JsonWriteBasic(vname[indx]);                   \
}

// macro to write array, which include size
#define TBufferJSON_WriteArray(vname)                         \
{                                                             \
   AppendOutput("[");                                         \
   fIsAnyValue = kFALSE;                                      \
   TJSONWriteArrayContent(vname, n);                          \
   AppendOutput("]");                                         \
}

//______________________________________________________________________________
void TBufferJSON::WriteArray(const Bool_t    *b, Int_t n)
{
   // Write array of Bool_t to buffer

   TBufferJSON_WriteArray(b);
}

//______________________________________________________________________________
void TBufferJSON::WriteArray(const Char_t    *c, Int_t n)
{
   // Write array of Char_t to buffer

   TBufferJSON_WriteArray(c);
}

//______________________________________________________________________________
void TBufferJSON::WriteArray(const UChar_t   *c, Int_t n)
{
   // Write array of UChar_t to buffer

   TBufferJSON_WriteArray(c);
}

//______________________________________________________________________________
void TBufferJSON::WriteArray(const Short_t   *h, Int_t n)
{
   // Write array of Short_t to buffer

   TBufferJSON_WriteArray(h);
}

//______________________________________________________________________________
void TBufferJSON::WriteArray(const UShort_t  *h, Int_t n)
{
   // Write array of UShort_t to buffer

   TBufferJSON_WriteArray(h);
}

//______________________________________________________________________________
void TBufferJSON::WriteArray(const Int_t     *i, Int_t n)
{
   // Write array of Int_ to buffer

   TBufferJSON_WriteArray(i);
}

//______________________________________________________________________________
void TBufferJSON::WriteArray(const UInt_t    *i, Int_t n)
{
   // Write array of UInt_t to buffer

   TBufferJSON_WriteArray(i);
}

//______________________________________________________________________________
void TBufferJSON::WriteArray(const Long_t    *l, Int_t n)
{
   // Write array of Long_t to buffer

   TBufferJSON_WriteArray(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteArray(const ULong_t   *l, Int_t n)
{
   // Write array of ULong_t to buffer

   TBufferJSON_WriteArray(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteArray(const Long64_t  *l, Int_t n)
{
   // Write array of Long64_t to buffer

   TBufferJSON_WriteArray(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteArray(const ULong64_t *l, Int_t n)
{
   // Write array of ULong64_t to buffer

   TBufferJSON_WriteArray(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteArray(const Float_t   *f, Int_t n)
{
   // Write array of Float_t to buffer

   TBufferJSON_WriteArray(f);
}

//______________________________________________________________________________
void TBufferJSON::WriteArray(const Double_t  *d, Int_t n)
{
   // Write array of Double_t to buffer

   TBufferJSON_WriteArray(d);
}

//______________________________________________________________________________
void TBufferJSON::WriteArrayFloat16(const Float_t  *f, Int_t n, TStreamerElement * /*ele*/)
{
   // Write array of Float16_t to buffer

   TBufferJSON_WriteArray(f);
}

//______________________________________________________________________________
void TBufferJSON::WriteArrayDouble32(const Double_t  *d, Int_t n, TStreamerElement * /*ele*/)
{
   // Write array of Double32_t to buffer

   TBufferJSON_WriteArray(d);
}

// write array without size attribute
// macro also treat situation, when instead of one single array
// chain of several elements should be produced
#define TBufferJSON_WriteFastArray(vname)                                  \
{                                                                         \
   if (n<=0) return;                                                      \
   TStreamerElement* elem = Stack(0)->fElem;                              \
   if ((elem!=0) && (elem->GetType()>TStreamerInfo::kOffsetL) &&          \
       (elem->GetType()<TStreamerInfo::kOffsetP) &&                       \
       (elem->GetArrayLength()!=n)) fExpectedChain = kTRUE;               \
   if (fExpectedChain) {                                                  \
      TStreamerInfo* info = Stack(1)->fInfo;                              \
      Int_t startnumber = Stack(0)->fElemNumber;                          \
      fExpectedChain = kFALSE;                                            \
      Int_t number(0), index(0);                                          \
      AppendOutput("[");                                                  \
      while (index<n) {                                                   \
        elem = info->GetStreamerElementReal(startnumber, number++);       \
        if (elem->GetType()<TStreamerInfo::kOffsetL) {                    \
          fIsAnyValue = index>0;                                          \
          JsonWriteBasic(vname[index]);                                   \
          index++;                                                        \
        } else {                                                          \
          Int_t elemlen = elem->GetArrayLength();                         \
          AppendOutput(index>0 ? ", [" : "[]");                           \
          fIsAnyValue = kFALSE;                                           \
          TJSONWriteArrayContent((vname+index), elemlen);                 \
          index+=elemlen;                                                 \
          AppendOutput("]");                                              \
        }                                                                 \
      }                                                                   \
      AppendOutput("]");                                                  \
   } else {                                                               \
      AppendOutput("[");                                                  \
      fIsAnyValue = kFALSE;                                               \
      TJSONWriteArrayContent(vname, n);                                   \
      AppendOutput("]");                                                  \
   }                                                                      \
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArray(const Bool_t    *b, Int_t n)
{
   // Write array of Bool_t to buffer

   TBufferJSON_WriteFastArray(b);
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArray(const Char_t    *c, Int_t n)
{
   // Write array of Char_t to buffer
   // If array does not include any special characters,
   // it will be reproduced as CharStar node with string as attribute

   Bool_t usedefault = (n==0) || fExpectedChain;
   const Char_t* buf = c;
   if (!usedefault)
      for (int i=0;i<n;i++) {
         if (*buf < 27) { usedefault = kTRUE; break; }
         buf++;
      }
   if (usedefault) {
      TBufferJSON_WriteFastArray(c);
   } else {
      Char_t* buf2 = new Char_t[n+1];
      memcpy(buf2, c, n);
      buf2[n] = 0;
      JsonWriteValue(buf2, kTRUE);
      delete[] buf2;
   }
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArray(const UChar_t   *c, Int_t n)
{
   // Write array of UChar_t to buffer

   TBufferJSON_WriteFastArray(c);
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArray(const Short_t   *h, Int_t n)
{
   // Write array of Short_t to buffer

   TBufferJSON_WriteFastArray(h);
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArray(const UShort_t  *h, Int_t n)
{
   // Write array of UShort_t to buffer

   TBufferJSON_WriteFastArray(h);
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArray(const Int_t     *i, Int_t n)
{
   // Write array of Int_t to buffer

   TBufferJSON_WriteFastArray(i);
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArray(const UInt_t    *i, Int_t n)
{
   // Write array of UInt_t to buffer

   TBufferJSON_WriteFastArray(i);
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArray(const Long_t    *l, Int_t n)
{
   // Write array of Long_t to buffer

   TBufferJSON_WriteFastArray(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArray(const ULong_t   *l, Int_t n)
{
   // Write array of ULong_t to buffer

   TBufferJSON_WriteFastArray(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArray(const Long64_t  *l, Int_t n)
{
   // Write array of Long64_t to buffer

   TBufferJSON_WriteFastArray(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArray(const ULong64_t *l, Int_t n)
{
   // Write array of ULong64_t to buffer

   TBufferJSON_WriteFastArray(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArray(const Float_t   *f, Int_t n)
{
   // Write array of Float_t to buffer

   TBufferJSON_WriteFastArray(f);
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArray(const Double_t  *d, Int_t n)
{
   // Write array of Double_t to buffer

   TBufferJSON_WriteFastArray(d);
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArrayFloat16(const Float_t  *f, Int_t n, TStreamerElement * /*ele*/)
{
   // Write array of Float16_t to buffer

   TBufferJSON_WriteFastArray(f);
}

//______________________________________________________________________________
void TBufferJSON::WriteFastArrayDouble32(const Double_t  *d, Int_t n, TStreamerElement * /*ele*/)
{
   // Write array of Double32_t to buffer

   TBufferJSON_WriteFastArray(d);
}

//______________________________________________________________________________
void  TBufferJSON::WriteFastArray(void  *start,  const TClass *cl, Int_t n, TMemberStreamer *s)
{
   // Recall TBuffer function to avoid gcc warning message

   TBufferFile::WriteFastArray(start, cl, n, s);
}

//______________________________________________________________________________
Int_t TBufferJSON::WriteFastArray(void **startp, const TClass *cl, Int_t n, Bool_t isPreAlloc, TMemberStreamer *s)
{
   // Recall TBuffer function to avoid gcc warning message

   return TBufferFile::WriteFastArray(startp, cl, n, isPreAlloc, s);
}

//______________________________________________________________________________
void TBufferJSON::StreamObject(void *obj, const type_info &typeinfo, const TClass* /* onFileClass */ )
{
   // stream object to/from buffer

   StreamObject(obj, TClass::GetClass(typeinfo));
}

//______________________________________________________________________________
void TBufferJSON::StreamObject(void *obj, const char *className, const TClass* /* onFileClass */ )
{
   // stream object to/from buffer

   StreamObject(obj, TClass::GetClass(className));
}

void TBufferJSON::StreamObject(TObject *obj)
{
   // stream object to/from buffer

   StreamObject(obj, obj ? obj->IsA() : TObject::Class());
}

//______________________________________________________________________________
void TBufferJSON::StreamObject(void *obj, const TClass *cl, const TClass* /* onfileClass */ )
{
   // stream object to/from buffer

   if (gDebug>1)
      Info("StreamObject","Class: %s", (cl ? cl->GetName() : "none"));

   JsonWriteObject(obj, cl);
}

//______________________________________________________________________________
void TBufferJSON::ReadBool(Bool_t    &)
{
   // Reads Bool_t value from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadChar(Char_t    &)
{
   // Reads Char_t value from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadUChar(UChar_t   &)
{
   // Reads UChar_t value from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadShort(Short_t   &)
{
   // Reads Short_t value from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadUShort(UShort_t  &)
{
   // Reads UShort_t value from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadInt(Int_t     &)
{
   // Reads Int_t value from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadUInt(UInt_t    &)
{
   // Reads UInt_t value from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadLong(Long_t    &)
{
   // Reads Long_t value from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadULong(ULong_t   &)
{
   // Reads ULong_t value from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadLong64(Long64_t  &)
{
   // Reads Long64_t value from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadULong64(ULong64_t &)
{
   // Reads ULong64_t value from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadFloat(Float_t   &)
{
   // Reads Float_t value from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadDouble(Double_t  &)
{
   // Reads Double_t value from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadCharP(Char_t    *)
{
   // Reads array of characters from buffer
}

//______________________________________________________________________________
void TBufferJSON::ReadTString(TString & /*s*/)
{
   // Reads a TString
}


//______________________________________________________________________________
void TBufferJSON::WriteBool(Bool_t    b)
{
   // Writes Bool_t value to buffer

   JsonWriteBasic(b);
}

//______________________________________________________________________________
void TBufferJSON::WriteChar(Char_t    c)
{
   // Writes Char_t value to buffer

   JsonWriteBasic(c);
}

//______________________________________________________________________________
void TBufferJSON::WriteUChar(UChar_t   c)
{
   // Writes UChar_t value to buffer

   JsonWriteBasic(c);
}

//______________________________________________________________________________
void TBufferJSON::WriteShort(Short_t   h)
{
   // Writes Short_t value to buffer

   JsonWriteBasic(h);
}

//______________________________________________________________________________
void TBufferJSON::WriteUShort(UShort_t  h)
{
   // Writes UShort_t value to buffer

   JsonWriteBasic(h);
}

//______________________________________________________________________________
void TBufferJSON::WriteInt(Int_t     i)
{
   // Writes Int_t value to buffer

   JsonWriteBasic(i);
}

//______________________________________________________________________________
void TBufferJSON::WriteUInt(UInt_t    i)
{
   // Writes UInt_t value to buffer

   JsonWriteBasic(i);
}

//______________________________________________________________________________
void TBufferJSON::WriteLong(Long_t    l)
{
   // Writes Long_t value to buffer

   JsonWriteBasic(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteULong(ULong_t   l)
{
   // Writes ULong_t value to buffer

   JsonWriteBasic(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteLong64(Long64_t  l)
{
   // Writes Long64_t value to buffer

   JsonWriteBasic(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteULong64(ULong64_t l)
{
   // Writes ULong64_t value to buffer

   JsonWriteBasic(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteFloat(Float_t   f)
{
   // Writes Float_t value to buffer

   JsonWriteBasic(f);
}

//______________________________________________________________________________
void TBufferJSON::WriteDouble(Double_t  d)
{
   // Writes Double_t value to buffer

   JsonWriteBasic(d);
}

//______________________________________________________________________________
void TBufferJSON::WriteCharP(const Char_t *c)
{
   // Writes array of characters to buffer

   JsonWriteValue(c, kTRUE);
}

//______________________________________________________________________________
void TBufferJSON::WriteTString(const TString &s)
{
   // Writes a TString

   JsonWriteValue(s.Data() ? s.Data() : "", kTRUE);
   //TO BE IMPLEMENTED
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Char_t value)
{
   // converts Char_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%d",value);
   JsonWriteValue(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Short_t value)
{
   // converts Short_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%hd", value);
   JsonWriteValue(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Int_t value)
{
   // converts Int_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%d", value);
   JsonWriteValue(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Long_t value)
{
   // converts Long_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%ld", value);
   JsonWriteValue(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Long64_t value)
{
   // converts Long64_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), FLong64, value);
   JsonWriteValue(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Float_t value)
{
   // converts Float_t to string and add xml node to buffer

   char buf[200];
   snprintf(buf, sizeof(buf), fgFloatFmt, value);
   JsonWriteValue(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Double_t value)
{
   // converts Double_t to string and add xml node to buffer

   char buf[1000];
   snprintf(buf, sizeof(buf), fgFloatFmt, value);
   JsonWriteValue(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Bool_t value)
{
   // converts Bool_t to string and add xml node to buffer

   JsonWriteValue(value ? "true" : "false");
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(UChar_t value)
{
   // converts UChar_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%u", value);
   JsonWriteValue(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(UShort_t value)
{
   // converts UShort_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%hu", value);
   JsonWriteValue(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(UInt_t value)
{
   // converts UInt_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%u", value);
   JsonWriteValue(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(ULong_t value)
{
   // converts ULong_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%lu", value);
   JsonWriteValue(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(ULong64_t value)
{
   // converts ULong64_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), FULong64, value);
   JsonWriteValue(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteValue(const char* value, Bool_t quotes)
{
   // write value create xml node with specified name and adds it to stack node

   if (fIsAnyValue) AppendOutput(", ");

   if (quotes) AppendOutput("\"");
   AppendOutput(value);
   if (quotes) AppendOutput("\"");

   fIsAnyValue = kTRUE;
}


void TBufferJSON::SetFloatFormat(const char* fmt)
{
   // set printf format for float/double members, default "%e"

   if (fmt==0) fmt = "%e";
   fgFloatFmt = fmt;
}

const char* TBufferJSON::GetFloatFormat()
{
   // return current printf format for float/double members, default "%e"

   return fgFloatFmt;
}

//______________________________________________________________________________
Int_t TBufferJSON::ApplySequence(const TStreamerInfoActions::TActionSequence &sequence, void *obj) 
{
   // Read one collection of objects from the buffer using the StreamerInfoLoopAction.
   // The collection needs to be a split TClonesArray or a split vector of pointers.
   
   TVirtualStreamerInfo *info = sequence.fStreamerInfo;
   IncrementLevel(info);
   
   if (gDebug) {
      //loop on all active members
      TStreamerInfoActions::ActionContainer_t::const_iterator end = sequence.fActions.end();
      for(TStreamerInfoActions::ActionContainer_t::const_iterator iter = sequence.fActions.begin();
          iter != end;
          ++iter) {      
         // Idea: Try to remove this function call as it is really needed only for XML streaming.
         SetStreamerElementNumber((*iter).fConfiguration->fElemId);
         (*iter).PrintDebug(*this,obj);
         (*iter)(*this,obj);
      }
      
   } else {
      //loop on all active members
      TStreamerInfoActions::ActionContainer_t::const_iterator end = sequence.fActions.end();
      for(TStreamerInfoActions::ActionContainer_t::const_iterator iter = sequence.fActions.begin();
          iter != end;
          ++iter) {      
         // Idea: Try to remove this function call as it is really needed only for XML streaming.
         SetStreamerElementNumber((*iter).fConfiguration->fElemId);
         (*iter)(*this,obj);
      }
   }
   
   DecrementLevel(info);
   return 0;
}

//______________________________________________________________________________
Int_t TBufferJSON::ApplySequenceVecPtr(const TStreamerInfoActions::TActionSequence &sequence, void *start_collection, void *end_collection) 
{
   // Read one collection of objects from the buffer using the StreamerInfoLoopAction.
   // The collection needs to be a split TClonesArray or a split vector of pointers.
   
   TVirtualStreamerInfo *info = sequence.fStreamerInfo;
   IncrementLevel(info);
   
   if (gDebug) {
      //loop on all active members
      TStreamerInfoActions::ActionContainer_t::const_iterator end = sequence.fActions.end();
      for(TStreamerInfoActions::ActionContainer_t::const_iterator iter = sequence.fActions.begin();
          iter != end;
          ++iter) {      
         // Idea: Try to remove this function call as it is really needed only for XML streaming.
         SetStreamerElementNumber((*iter).fConfiguration->fElemId);
         (*iter).PrintDebug(*this,*(char**)start_collection);  // Warning: This limits us to TClonesArray and vector of pointers.
         (*iter)(*this,start_collection,end_collection);
      }
      
   } else {
      //loop on all active members
      TStreamerInfoActions::ActionContainer_t::const_iterator end = sequence.fActions.end();
      for(TStreamerInfoActions::ActionContainer_t::const_iterator iter = sequence.fActions.begin();
          iter != end;
          ++iter) {      
         // Idea: Try to remove this function call as it is really needed only for XML streaming.
         SetStreamerElementNumber((*iter).fConfiguration->fElemId);
         (*iter)(*this,start_collection,end_collection);
      }
   }
   
   DecrementLevel(info);
   return 0;
}

//______________________________________________________________________________
Int_t TBufferJSON::ApplySequence(const TStreamerInfoActions::TActionSequence &sequence, void *start_collection, void *end_collection) 
{
   // Read one collection of objects from the buffer using the StreamerInfoLoopAction.
   
   TVirtualStreamerInfo *info = sequence.fStreamerInfo;
   IncrementLevel(info);
   
   TStreamerInfoActions::TLoopConfiguration *loopconfig = sequence.fLoopConfig;
   if (gDebug) {
      
      // Get the address of the first item for the PrintDebug.
      // (Performance is not essential here since we are going to print to
      // the screen anyway).
      void *arr0 = loopconfig->GetFirstAddress(start_collection,end_collection);
      // loop on all active members
      TStreamerInfoActions::ActionContainer_t::const_iterator end = sequence.fActions.end();
      for(TStreamerInfoActions::ActionContainer_t::const_iterator iter = sequence.fActions.begin();
          iter != end;
          ++iter) {      
         // Idea: Try to remove this function call as it is really needed only for XML streaming.
         SetStreamerElementNumber((*iter).fConfiguration->fElemId);
         (*iter).PrintDebug(*this,arr0);
         (*iter)(*this,start_collection,end_collection,loopconfig);
      }
      
   } else {
      //loop on all active members
      TStreamerInfoActions::ActionContainer_t::const_iterator end = sequence.fActions.end();
      for(TStreamerInfoActions::ActionContainer_t::const_iterator iter = sequence.fActions.begin();
          iter != end;
          ++iter) {      
         // Idea: Try to remove this function call as it is really needed only for XML streaming.
         SetStreamerElementNumber((*iter).fConfiguration->fElemId);
         (*iter)(*this,start_collection,end_collection,loopconfig);
      }
   }
   
   DecrementLevel(info);
   return 0;
}
