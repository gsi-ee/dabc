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

#include "TArray.h"
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
   TBufferFile(TBuffer::kWrite),
   fOutBuffer(),
   fValue(),
   fObjMap(),
   fStack(),
   fErrorFlag(0),
   fExpectedChain(kFALSE),
   fCompressLevel(0)
{
   // Creates buffer object to serialize data into json.

   fBufSize = 1000000000;

   SetParent(0);
   SetBit(kCannotHandleMemberWiseStreaming);
   SetBit(kTextBasedStreaming);

   fOutBuffer.Capacity(10000);
   fValue.Capacity(1000);
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

   TBufferJSON buf;

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

   Info("WriteObject","Object %p", obj);

   TBufferFile::WriteObject(obj);
}

// TJSONStackObj is used to keep stack of object hierarchy,
// stored in TBuffer. For instance, data for parent class(es)
// stored in subnodes, but initial object node will be kept.

class TJSONStackObj : public TObject {
   public:
      TStreamerInfo*    fInfo;           //!
      TStreamerElement* fElem;           //! element in streamer info
      Int_t             fElemNumber;     //! number of streamer element in streamer info
      Bool_t            fIsStreamerInfo; //!
      Bool_t            fIsElemOwner;    //!
      Bool_t            fIsBaseClass;    //! indicate if element is base-class, ignored by post processing
      Bool_t            fIsPostProcessed;//! indicate that value is written
      Bool_t            fIsObjStarted;   //! indicate that object writing started, should be closed in postprocess
      Bool_t            fIsArray;        //! indicate if array object is used
      TObjArray         fValues;         //! raw values

      TJSONStackObj() :
         TObject(),
         fInfo(0),
         fElem(0),
         fElemNumber(0),
         fIsStreamerInfo(kFALSE),
         fIsElemOwner(kFALSE),
         fIsBaseClass(kFALSE),
         fIsPostProcessed(kFALSE),
         fIsObjStarted(kFALSE),
         fIsArray(kFALSE),
         fValues()
      { fValues.SetOwner(kTRUE); }

      virtual ~TJSONStackObj()
      {
         if (fIsElemOwner) delete fElem;
      }

      Bool_t IsStreamerInfo() const { return fIsStreamerInfo; }
      Bool_t IsStreamerElement() const { return !fIsStreamerInfo && (fElem!=0); }

      void PushValue(TString& v) { fValues.Add(new TObjString(v)); v.Clear(); }
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
void TBufferJSON::AppendOutput(const char* val, Bool_t first)
{
   if (first)
      for (Int_t n=0;n<=fStack.GetLast();n++)
         fOutBuffer.Append(" ");

   if (val!=0) fOutBuffer.Append(val);
}

//______________________________________________________________________________
void TBufferJSON::JsonStartElement()
{
   TJSONStackObj* stack = Stack();
   if ((stack!=0) && stack->IsStreamerElement() && !stack->fIsObjStarted) {

      if ((fValue.Length()>0) || (stack->fValues.GetLast()>=0))
         Error("JsonWriteObject", "Non-empty value buffer when start writing object");

      stack->fIsPostProcessed = kTRUE;
      stack->fIsObjStarted = kTRUE;

      if (!stack->fIsBaseClass) {
         AppendOutput(",\n");
         AppendOutput(stack->fElem->GetName(), kTRUE);
         AppendOutput(": ");
      }
   }
}


//______________________________________________________________________________
void TBufferJSON::JsonWriteObject(const void* obj, const TClass* cl, const char* extrafield)
{
   // Write object to buffer
   // If object was written before, only pointer will be stored
   // Return pointer to top xml node, representing object

   if (!cl) obj = 0;

   if (gDebug>0) Info("JsonWriteObject","Object %p class %s", obj, cl ? cl->GetName() : "null");

   // special handling for TArray classes - they should appear not as object but JSON array
   Bool_t isarray = strncmp("TArray", (cl ? cl->GetName() : ""), 6) == 0;
   if (isarray) isarray = ((TClass*)cl)->GetBaseClassOffset(TArray::Class()) == 0;

   if (!isarray) {
      JsonStartElement();
   }

   if (obj==0) { AppendOutput("null"); return; }


   if (!isarray) {
      // add element name which should correspond to the object

      for(unsigned n=0;n<fObjMap.size();n++)
         if (fObjMap[n] == obj) {
            AppendOutput(Form("{ $ref:id%u }", n));
            if (extrafield!=0) Error("JsonWriteObject", "Extra field %s was not recorded for object", extrafield);
            return;
         }

      fObjMap.push_back(obj);

      AppendOutput("{\n");

      AppendOutput("_classname: \"",kTRUE);
      AppendOutput(cl->GetName());
      AppendOutput("\"");

      if (extrafield!=0) {
         AppendOutput(",\n");
         AppendOutput(extrafield,kTRUE);
      }
   }

   if (gDebug>0)
      Info("JsonWriteObject","Starting object %p write for class: %s", obj, cl->GetName());

   TJSONStackObj* stack = PushStack();
   stack->fIsArray = isarray;

   if (((TClass*)cl)->GetBaseClassOffset(TCollection::Class())==0)
      JsonStreamCollection((TCollection*) obj, cl);
   else
      ((TClass*)cl)->Streamer((void*)obj, *this);

   if (gDebug>0)
      Info("JsonWriteObject","Done object %p write for class: %s", obj, cl->GetName());

   if (isarray) {
      if (stack->fValues.GetLast()==0)
         stack->fValues.Delete();
      else
         Error("JsonWriteObject","Problem when writing array");
   } else {
      if (stack->fValues.GetLast()>=0)
         Error("JsonWriteObject", "Non-empty values list when writing object");
   }

   PopStack();

   if (!isarray) {
      AppendOutput("\n");
      AppendOutput("}",kTRUE);
   }
}


//______________________________________________________________________________
void TBufferJSON::JsonStreamCollection(TCollection* col, const TClass* objClass)
{
   // store content of collection

   AppendOutput(",\n");

   AppendOutput("name: \"",kTRUE);
   AppendOutput(col->GetName());
   AppendOutput("\",\n");
   AppendOutput("array: [",kTRUE);

   bool islist = col->InheritsFrom(TList::Class());
   TString sopt;

   TIter iter(col);
   TObject* obj;
   Bool_t first = kTRUE;
   while ((obj = iter())!=0) {

      if (!first) {
         AppendOutput(",\n");
         AppendOutput("",kTRUE);
      }

      if (islist) sopt.Form("_option: \"%s\"", iter.GetOption());

      JsonWriteObject(obj, obj->IsA(), (islist ? sopt.Data() : 0));

      first = kFALSE;
   }

   AppendOutput("]");
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

   fExpectedChain = kFALSE;

   if (sinfo!=0) cl = sinfo->GetClass();

   if (cl==0) return;

   if (gDebug>0) Info("WorkWithClass","Class: %s", cl->GetName());

   TJSONStackObj* stack = Stack();

   if ((stack!=0) && stack->IsStreamerElement() && (stack->fElem->GetType() == TStreamerInfo::kObject))
      if (!stack->fIsObjStarted) {

         stack->fIsObjStarted = kTRUE;

         AppendOutput(",\n");
         AppendOutput(stack->fElem->GetName(), kTRUE);
         AppendOutput(": { \n");

         AppendOutput(" _classname: \"",kTRUE);
         AppendOutput(cl->GetName());
         AppendOutput("\"");
      }

   stack = PushStack();
   stack->fInfo = sinfo;
   stack->fIsStreamerInfo = kTRUE;


}

//______________________________________________________________________________
void TBufferJSON::DecrementLevel(TVirtualStreamerInfo* info)
{
   // Function is called from TStreamerInfo WriteBuffer and ReadBuffer functions
   // and decrease level in xml structure.

   fExpectedChain = kFALSE;

   if (gDebug>0)
      Info("DecrementLevel","Class: %s", (info ? info->GetClass()->GetName() : "custom"));

   TJSONStackObj* stack = Stack();

   if (stack->IsStreamerElement()) {
      if (gDebug>0)
         Info("DecrementLevel", "    Perform post-processing elem: %s", stack->fElem->GetName());

      PerformPostProcessing(stack);

      PopStack();  // remove stack of last element

      stack = Stack();
   }

   if (stack->fInfo != (TStreamerInfo*) info)
      Error("DecrementLevel", "    Mismatch of streamer info");

   PopStack();                       // back from data of stack info

   if (gDebug>0)
      Info("DecrementLevel","Class: %s done", (info ? info->GetClass()->GetName() : "custom"));
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
   // This is call-back from streamer which indicates
   // that class member will be streamed
   // Name of element used in JSON

   fExpectedChain = kFALSE;

   TJSONStackObj* stack = Stack();
   if (stack==0) {
      Error("WorkWithElement", "stack is empty");
      return;
   }

   if (gDebug>0)
      Info("WorkWithElement", "    Start element %s type %d", elem ? elem->GetName() : "---", elem? elem->GetType() : -1);

   if (stack->IsStreamerElement()) {
      // this is post processing

      if (gDebug>0)
         Info("WorkWithElement", "    Perform post-processing elem: %s", stack->fElem->GetName());

      PerformPostProcessing(stack);

      PopStack();                    // go level back
      stack = dynamic_cast<TJSONStackObj*> (fStack.Last());
   }

   fValue.Clear();

   if (stack==0) {
      Error("WorkWithElement", "Lost of stack");
      return;
   }

   Int_t comp_type = 0;

   if ((number>=0) && (elem==0)) {

      TStreamerInfo* info = stack->fInfo;
      if (!stack->IsStreamerInfo()) {
         Error("WorkWithElement", "Problem in Inc/Dec level");
         return;
      }

      comp_type = info->GetTypes()[number];

      elem = info->GetStreamerElementReal(number, 0);
   } else if (elem) {
      comp_type = elem->GetType();
   }

   if (elem==0) {
      Error("WorkWithElement", "streamer info returns elem = 0");
      return;
   }

   if (gDebug>0)
      Info("WorkWithElement", "    Element %s type %d", elem ? elem->GetName() : "---", elem? elem->GetType() : -1);

   Bool_t isBasicType = (elem->GetType()>0) && (elem->GetType()<20);

   fExpectedChain = isBasicType && (comp_type - elem->GetType() == TStreamerInfo::kOffsetL);

   if (fExpectedChain && (gDebug>0))
      Info("WorkWithElement",
           "    Expects chain for elem %s number %d",
            elem->GetName(), number);

   TClass* base_class = 0;

   if ((elem->GetType()==TStreamerInfo::kBase) ||
       ((elem->GetType()==TStreamerInfo::kTObject) && !strcmp(elem->GetName(), TObject::Class()->GetName())) ||
       ((elem->GetType()==TStreamerInfo::kTNamed) && !strcmp(elem->GetName(), TNamed::Class()->GetName())) )
         base_class = elem->GetClassPointer();

   if (base_class && (gDebug>0))
      Info("WorkWithElement",
           "   Expects base class %s with standard streamer",  base_class->GetName());

   stack = PushStack();
   stack->fElem = (TStreamerElement*) elem;
   stack->fElemNumber = number;
   stack->fIsElemOwner = (number<0);
   stack->fIsBaseClass = (base_class!=0);
}

//______________________________________________________________________________
void TBufferJSON::ClassBegin(const TClass* cl, Version_t)
{
   // Special function for custom streamers.
   // Should be called in the beginning

   WorkWithClass(0, cl);
}

//______________________________________________________________________________
void TBufferJSON::ClassEnd(const TClass*)
{
   // Special function for custom streamers.
   // Should be called at the end

   DecrementLevel(0);
}

//______________________________________________________________________________
void TBufferJSON::ClassMember(const char* name, const char* typeName, Int_t arrsize1, Int_t arrsize2)
{
   // Special function for custom streamers.
   // Provides possibility to mark name and type of stored data

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
void TBufferJSON::PerformPostProcessing(TJSONStackObj* stack, const TStreamerElement* elem)
{
   // Function is converts TObject and TString structures to more compact representation

   if ((elem==0) && stack->fIsPostProcessed) return;

   if (elem==0) elem = stack->fElem;
   if (elem==0) return;

   Info("PerformPostProcessing","Start element %s type %s", elem->GetName(), elem->GetTypeName());

   stack->fIsPostProcessed = kTRUE;

   // when element was written as separate object, close only braces and exit
   if (stack->fIsObjStarted) {
      AppendOutput("\n");
      AppendOutput("}", kTRUE);
      return;
   }

   const char* typname = stack->fIsBaseClass ? elem->GetName() : elem->GetTypeName();
   Bool_t isarray = (strncmp("TArray", typname, 6)==0);
   Bool_t istobject = (elem->GetType()==TStreamerInfo::kTObject) || (strcmp("TObject", typname)==0);




   if (!stack->fIsBaseClass) {
      AppendOutput(",\n");
      AppendOutput(elem->GetName(), kTRUE);
      AppendOutput(": ");
   }

   if (elem->GetType()==TStreamerInfo::kTString) {
      // in principle, we should just remove all kind of string length information

      Info("PerformPostProcessing", "Elem %s reformat string value = '%s'", elem->GetName(), fValue.Data());

      stack->fValues.Delete();
   } else
   if (istobject) {
      AppendOutput(",\n");
      if (stack->fValues.GetLast()!=0) {
         Error("PerformPostProcessing", "When storing TObject, more than 2 items are stored");
         AppendOutput("dummy: ", kTRUE);
      } else {
         AppendOutput("fUniqueID: ", kTRUE);
         AppendOutput(stack->fValues.At(0)->GetName());
         AppendOutput(",\n");
         AppendOutput("fBits: ", kTRUE);
      }

      stack->fValues.Delete();
   } else
   if (isarray) {

      Info("PerformPostProcessing", "TArray postprocessing");

      // work around for TArray classes - remove first element with array length

      // only for base class insert fN and fArray tags

      if (stack->fIsBaseClass && (stack->fValues.GetLast()==0)) {
         AppendOutput(",\n");
         AppendOutput("fN: ", kTRUE);
         AppendOutput(stack->fValues.At(0)->GetName());
         AppendOutput(",\n");
         AppendOutput("fArray: ", kTRUE);
      }

      stack->fValues.Delete();
   }


   if (stack->fIsBaseClass && !isarray && !istobject) {
      if (fValue.Length()!=0)
         Error("PerformPostProcessing", "Non-empty value for base class");
      return;
   }

   if (stack->fValues.GetLast()>=0) {
      AppendOutput("{ ");
      TString sbuf;
      for (Int_t n = 0;n <= stack->fValues.GetLast(); n++) {
         sbuf.Form("elem%d:",n);
         AppendOutput(sbuf.Data());
         AppendOutput(stack->fValues.At(n)->GetName());
         AppendOutput(", ");
      }
      sbuf.Form("elem%d:", stack->fValues.GetLast()+1);
      AppendOutput(sbuf.Data());
   }

   if (fValue.Length()==0) {
      AppendOutput("null");
   } else {
      AppendOutput(fValue.Data());
      fValue.Clear();
   }

   if (stack->fValues.GetLast()>=0)
      AppendOutput("}");
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

#define TJSONPushValue()                                 \
   if (fValue.Length() > 0) Stack()->PushValue(fValue);


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

   TJSONPushValue();

   JsonWriteBasic(*f);
}

//______________________________________________________________________________
void TBufferJSON::WriteDouble32 (Double_t *d, TStreamerElement * /*ele*/)
{
   // write a Double32_t to the buffer
   TJSONPushValue();

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


#define TJSONWriteArrayContent(vname, arrsize)     \
{                                                  \
   fValue.Append("[");                             \
   for(Int_t indx=0;indx<arrsize;indx++) {         \
      if (indx>0) fValue.Append(", ");             \
      JsonWriteBasic(vname[indx]);                 \
   }                                               \
   fValue.Append("]");                             \
}

// macro to write array, which include size
#define TBufferJSON_WriteArray(vname)              \
{                                                  \
   TJSONPushValue();                               \
   TJSONWriteArrayContent(vname, n);               \
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
#define TBufferJSON_WriteFastArray(vname)                                 \
{                                                                         \
   TJSONPushValue();                                                      \
   if (n<=0) { fValue.Append("[]"); return; }                             \
   TStreamerElement* elem = Stack(0)->fElem;                              \
   if ((elem!=0) && (elem->GetType()>TStreamerInfo::kOffsetL) &&          \
       (elem->GetType()<TStreamerInfo::kOffsetP) &&                       \
       (elem->GetArrayLength()!=n)) fExpectedChain = kTRUE;               \
   if (fExpectedChain) {                                                  \
      TStreamerInfo* info = Stack(1)->fInfo;                              \
      Int_t startnumber = Stack(0)->fElemNumber;                          \
      fExpectedChain = kFALSE;                                            \
      Int_t number(0), index(0);                                          \
      while (index<n) {                                                   \
        elem = info->GetStreamerElementReal(startnumber, number++);       \
        if (elem->GetType()<TStreamerInfo::kOffsetL) {                    \
          JsonWriteBasic(vname[index]);                                   \
          PerformPostProcessing(Stack(0), elem);                          \
          index++;                                                        \
        } else {                                                          \
          Int_t elemlen = elem->GetArrayLength();                         \
          TJSONWriteArrayContent((vname+index), elemlen);                 \
          index+=elemlen;                                                 \
          PerformPostProcessing(Stack(0), elem);                          \
        }                                                                 \
      }                                                                   \
   } else {                                                               \
      TJSONWriteArrayContent(vname, n);                                   \
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

   Bool_t usedefault = fExpectedChain;
   const Char_t* buf = c;
   if (!usedefault)
      for (int i=0;i<n;i++) {
         if (*buf < 27) { usedefault = kTRUE; break; }
         buf++;
      }
   if (usedefault) {
      // TODO - write as array of characters
      TBufferJSON_WriteFastArray(c);
   } else {
      TJSONPushValue();

      fValue.Append("\"");
      if ((c!=0) && (n>0)) fValue.Append(c, n);
      fValue.Append("\"");
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
void  TBufferJSON::WriteFastArray(void  *start,  const TClass *cl, Int_t n, TMemberStreamer *streamer)
{
   // Recall TBuffer function to avoid gcc warning message

   Info("WriteFastArray","void *start");

   if (streamer) {
      JsonStartElement();
      (*streamer)(*this, start, 0);
      return;
   }

   char *obj = (char*)start;
   if (!n) n=1;
   int size = cl->Size();

   if (n>1) {
      JsonStartElement();
      AppendOutput(" [ ");
   }

   for(Int_t j=0; j<n; j++,obj+=size) {
      // ((TClass*)cl)->Streamer(obj,*this);

      if (j>0) AppendOutput(",\n");

      JsonWriteObject(obj, cl);
   }

   if (n>1) {
      AppendOutput(" ]\n");
   }

}

//______________________________________________________________________________
Int_t TBufferJSON::WriteFastArray(void **start, const TClass *cl, Int_t n, Bool_t isPreAlloc, TMemberStreamer *streamer)
{
   // Recall TBuffer function to avoid gcc warning message

   Info("WriteFastArray","void **startp cl %s n %d streamer %p", cl->GetName(), n, streamer);

   if (streamer) {
      JsonStartElement();
      (*streamer)(*this,(void*)start,0);
      return 0;
   }

   Int_t res = 0;

   if (n>1) {
      JsonStartElement();
      AppendOutput(" [ ");
   }

   if (!isPreAlloc) {

      for (Int_t j=0;j<n;j++) {
         if (j>0) AppendOutput(",\n");
         res |= WriteObjectAny(start[j],cl);
      }

   } else {
      //case //-> in comment

      for (Int_t j=0;j<n;j++) {
         if (j>0) AppendOutput(",\n");

         if (!start[j]) start[j] = ((TClass*)cl)->New();
         // ((TClass*)cl)->Streamer(start[j],*this);
         JsonWriteObject(start[j], cl);
      }
   }

   if (n>1) {
      AppendOutput(" ]\n");
   }

   return res;
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

   TJSONPushValue();

   JsonWriteBasic(b);
}

//______________________________________________________________________________
void TBufferJSON::WriteChar(Char_t    c)
{
   // Writes Char_t value to buffer

   TJSONPushValue();

   JsonWriteBasic(c);
}

//______________________________________________________________________________
void TBufferJSON::WriteUChar(UChar_t   c)
{
   // Writes UChar_t value to buffer

   TJSONPushValue();

   JsonWriteBasic(c);
}

//______________________________________________________________________________
void TBufferJSON::WriteShort(Short_t   h)
{
   // Writes Short_t value to buffer

   TJSONPushValue();

   JsonWriteBasic(h);
}

//______________________________________________________________________________
void TBufferJSON::WriteUShort(UShort_t  h)
{
   // Writes UShort_t value to buffer

   TJSONPushValue();

   JsonWriteBasic(h);
}

//______________________________________________________________________________
void TBufferJSON::WriteInt(Int_t     i)
{
   // Writes Int_t value to buffer

   TJSONPushValue();

   JsonWriteBasic(i);
}

//______________________________________________________________________________
void TBufferJSON::WriteUInt(UInt_t    i)
{
   // Writes UInt_t value to buffer

   TJSONPushValue();

   JsonWriteBasic(i);
}

//______________________________________________________________________________
void TBufferJSON::WriteLong(Long_t    l)
{
   // Writes Long_t value to buffer

   TJSONPushValue();

   JsonWriteBasic(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteULong(ULong_t   l)
{
   // Writes ULong_t value to buffer

   TJSONPushValue();

   JsonWriteBasic(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteLong64(Long64_t  l)
{
   // Writes Long64_t value to buffer

   TJSONPushValue();

   JsonWriteBasic(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteULong64(ULong64_t l)
{
   // Writes ULong64_t value to buffer

   TJSONPushValue();

   JsonWriteBasic(l);
}

//______________________________________________________________________________
void TBufferJSON::WriteFloat(Float_t   f)
{
   // Writes Float_t value to buffer

   TJSONPushValue();

   JsonWriteBasic(f);
}

//______________________________________________________________________________
void TBufferJSON::WriteDouble(Double_t  d)
{
   // Writes Double_t value to buffer

   TJSONPushValue();

   JsonWriteBasic(d);
}

//______________________________________________________________________________
void TBufferJSON::WriteCharP(const Char_t *c)
{
   // Writes array of characters to buffer

   TJSONPushValue();

   fValue.Append("\"");
   fValue.Append(c);
   fValue.Append("\"");
}

//______________________________________________________________________________
void TBufferJSON::WriteTString(const TString &s)
{
   // Writes a TString
   Info("WriteTString", "Write string value");

   TJSONPushValue();

   fValue.Append("\"");
   fValue.Append(s);
   fValue.Append("\"");
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Char_t value)
{
   // converts Char_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%d",value);
   fValue.Append(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Short_t value)
{
   // converts Short_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%hd", value);
   fValue.Append(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Int_t value)
{
   // converts Int_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%d", value);
   fValue.Append(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Long_t value)
{
   // converts Long_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%ld", value);
   fValue.Append(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Long64_t value)
{
   // converts Long64_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), FLong64, value);
   fValue.Append(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Float_t value)
{
   // converts Float_t to string and add xml node to buffer

   char buf[200];
   snprintf(buf, sizeof(buf), fgFloatFmt, value);
   fValue.Append(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Double_t value)
{
   // converts Double_t to string and add xml node to buffer

   char buf[1000];
   snprintf(buf, sizeof(buf), fgFloatFmt, value);
   fValue.Append(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(Bool_t value)
{
   // converts Bool_t to string and add xml node to buffer

   fValue.Append(value ? "true" : "false");
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(UChar_t value)
{
   // converts UChar_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%u", value);
   fValue.Append(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(UShort_t value)
{
   // converts UShort_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%hu", value);
   fValue.Append(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(UInt_t value)
{
   // converts UInt_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%u", value);
   fValue.Append(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(ULong_t value)
{
   // converts ULong_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), "%lu", value);
   fValue.Append(buf);
}

//______________________________________________________________________________
void TBufferJSON::JsonWriteBasic(ULong64_t value)
{
   // converts ULong64_t to string and add xml node to buffer

   char buf[50];
   snprintf(buf, sizeof(buf), FULong64, value);
   fValue.Append(buf);
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
