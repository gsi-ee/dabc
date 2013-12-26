#include "TRootSnifferStore.h"

void TRootSnifferStoreXml::CreateNode(Int_t lvl, const char* nodename)
{
   buf->Append(TString::Format("%*s<%s", lvl*2, "", nodename));
}

void TRootSnifferStoreXml::SetField(Int_t, const char* field, const char* value, Int_t)
{
   buf->Append(TString::Format(" %s=\"%s\"", field, value));
}

void TRootSnifferStoreXml::BeforeNextChild(Int_t, Int_t nchld, Int_t)
{
   if (nchld==0) buf->Append(">\n");
}

void TRootSnifferStoreXml::CloseNode(Int_t lvl, const char* nodename, Bool_t hadchilds)
{
   if (hadchilds)
      buf->Append(TString::Format("%*s</%s>\n", lvl*2, "", nodename));
   else
      buf->Append(TString::Format("/>\n"));
}

// ============================================================================

void TRootSnifferStoreJson::CreateNode(Int_t lvl, const char* nodename)
{
   buf->Append(TString::Format("%*s\"%s\" : {", lvl*4, "", nodename));
}

void TRootSnifferStoreJson::SetField(Int_t lvl, const char* field, const char* value, Int_t nfld)
{
   if (nfld==0) buf->Append("\n"); else buf->Append(",\n");
   buf->Append(TString::Format("%*s\"%s\" : \"%s\"", lvl*4+2, "", field, value));
}

void TRootSnifferStoreJson::BeforeNextChild(Int_t lvl, Int_t nchld, Int_t nfld)
{
   if (nchld==0) {
      if (nfld==0) buf->Append("\n"); else buf->Append(",\n");
      buf->Append(TString::Format("%*s\"childs\" : [\n", lvl*4+2, ""));
   } else {
      buf->Append(",\n");
   }
}

void TRootSnifferStoreJson::CloseNode(Int_t lvl, const char*, Bool_t hadchilds)
{
   if (hadchilds)
      buf->Append(TString::Format("\n%*s]", lvl*4+2, ""));
   buf->Append(TString::Format("\n%*s}", lvl*4, ""));
}

