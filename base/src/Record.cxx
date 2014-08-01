// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "dabc/Record.h"

#include "dabc/threads.h"
#include "dabc/logging.h"
#include "dabc/XmlEngine.h"
#include "dabc/ConfigBase.h"
#include "dabc/Manager.h"
#include "dabc/Configuration.h"

#include <map>
#include <string.h>
#include <fnmatch.h>

bool dabc::iostream::skip_object()
{
   if (!is_input()) return false;
   uint32_t storesz(0);
   read_uint32(storesz);

   return shift(((uint64_t) storesz)*8-4);
}


bool dabc::iostream::verify_size(uint64_t pos, uint64_t sz)
{
   uint64_t actualsz = size() - pos;

   if (actualsz==sz) return true;

   if (pos % 8 != 0) { EOUT("start position not 8-byte aligned!!!"); return false; }

   if (!is_real()) {
      // special case for size stream, we just shift until 8-byte alignment
      uint64_t rest = size() % 8;
      if (rest > 0) shift(8 - rest);
      return true;
   }

   if (sz % 8 != 0) { EOUT("size is not 8-byte aligned!!!"); return false; }

   if (actualsz > sz) {
      EOUT("Too many bytes %lu was accessed in stream compared to provided %lu", (long unsigned) actualsz, (long unsigned) sz);
      return false;
   }

   if (is_input()) return shift(sz-actualsz);

   while (actualsz < sz) {
      uint64_t buf(0);
      uint64_t portion = sz - actualsz;
      if (portion>8) portion = 8;
      write(&buf, portion);
      actualsz += portion;
   }
   return true;
}


uint64_t dabc::iostream::str_storesize(const std::string& str)
{
   uint64_t res = sizeof(int32_t);
   res += str.length() + 1; // null-terminated string will be stored
   while (res % 8 != 0) res++;
   return res;
}

bool dabc::iostream::write_str(const std::string& str)
{
   uint64_t sz = str_storesize(str);
   if (sz > maxstoresize()) {
      EOUT("Cannot store string in the buffer sz %lu  maxsize %lu  pos %lu real %s",
            (long unsigned) sz, (long unsigned) maxstoresize(), (long unsigned) size(), DBOOL(is_real()));
      return 0;
   }

   uint64_t pos = size();

   uint32_t storesz = sz / 8;
   write_uint32(storesz);
   write(str.c_str(), str.length()+1);

   return verify_size(pos, sz);
}

bool dabc::iostream::read_str(std::string& str)
{
   uint64_t pos = size();

   uint32_t storesz(0);
   read_uint32(storesz);
   uint64_t sz = ((uint64_t) storesz) * 8;

   if (sz > tmpbuf_size()) {
      EOUT("Cannot read complete string!!!");
      return false;
   }

   str.clear();
   str.append(tmpbuf()); // null-terminated string
   return verify_size(pos, sz);
}

// ===========================================================================

bool dabc::memstream::shift(uint64_t len)
{
   if (len > fRemains) return false;
   fCurr+=len;
   fRemains-=len;
   return true;
}

bool dabc::memstream::write(const void* src, uint64_t len)
{
   if (len > fRemains) return false;
   memcpy(fCurr, src, len);
   fCurr+=len;
   fRemains-=len;
   return true;
}


bool dabc::memstream::read(void* tgt, uint64_t len)
{
   if (len > fRemains) return false;
   memcpy(tgt, fCurr, len);
   fCurr+=len;
   fRemains-=len;
   return true;
}

void dabc::HXmlStore::CreateNode(const char *nodename)
{
   if (nodename==0) nodename = "item";

   // starts new xml node, will be closed by CloseNode
   buf.append(dabc::format("%*s<%s", compact() > 0 ? 0 : lvl * 2, "", nodename));
   if (first_node) { buf.append(" xmlns:dabc=\"http://dabc.gsi.de/xhtml\""); first_node = false; }
   numchilds.push_back(0);
   numflds.push_back(0);
   lvl++;

}

void dabc::HXmlStore::SetField(const char *field, const char *value)
{
   // set field (xml attribute) in current node

   buf.append(dabc::format(" %s=", field));

   int vlen = strlen(value);

   const char* v = value;
   const char* stop = value+vlen;

   if ((vlen > 1) && (value[0] == '\"') && (value[vlen-1] == '\"')) {
      v++; stop--;
   }

   buf.append("\"");

   while (v!=stop) {
      switch (*v) {
         case '<' : buf.append("&lt;"); break;
         case '>' : buf.append("&gt;"); break;
         case '&' : buf.append("&amp;"); break;
         case '\'' : buf.append("&apos;"); break;
         case '\"' : buf.append("&quot;"); break;
         default: buf.append(v, 1); break;
      }
      v++;
   }
   buf.append("\"");
}

void dabc::HXmlStore::BeforeNextChild(const char*)
{
   // called before next child node created
   if (numchilds.back()++ == 0) { buf.append(">"); NewLine(); }
}

void dabc::HXmlStore::CloseNode(const char *nodename)
{
   // called when node should be closed
   // depending from number of childs different xml format is applied

   lvl--;

   if (numchilds.back() > 0) {
      if (nodename==0) nodename = "item";
      buf.append(dabc::format("%*s</%s>", compact() > 0 ? 0 : lvl * 2, "", nodename));
      NewLine();
   } else {
      buf.append(dabc::format("/>"));
      NewLine();
   }
   numchilds.pop_back();
   numflds.pop_back();
}

// ===============================================================================


void dabc::HJsonStore::CreateNode(const char *nodename)
{
   // starts new json object, will be closed by CloseNode

   buf.append(dabc::format("%*s{", (compact() > 0) ? 0 : lvl * 4, ""));
   lvl++;
   numflds.push_back(0);
   numchilds.push_back(0);
   if (nodename!=0)
      SetField("_name", dabc::format("\"%s\"",nodename).c_str());
}

void dabc::HJsonStore::SetField(const char *field, const char *value)
{
   // set field (json field) in current node

   if (numflds.back() < 0) {
      EOUT("Not allowed to set fields (here %s) after any child was created", field);
      return;
   }

   if (numflds.back()++ > 0) buf.append(",");
   NewLine();
   buf.append(dabc::format("%*s\"%s\"", (compact() > 0) ? 0 : lvl * 4 - 2, "", field));
   switch(compact()) {
      case 2: buf.append(": "); break;
      case 3: buf.append(":"); break;
      default: buf.append(" : ");
   }
   buf.append(value);
}

void dabc::HJsonStore::CloseNode(const char *)
{
   // called when node should be closed
   // depending from number of childs different json format is applied

   CloseChilds();
   numchilds.pop_back();
   numflds.pop_back();
   lvl--;
   NewLine();
   buf.append(dabc::format("%*s}", (compact() > 0) ? 0 : lvl * 4, ""));
}

void dabc::HJsonStore::BeforeNextChild(const char* basename)
{
   // called before next child node created

   // first of all, close field and mark that we did it
   if (numflds.back() > 0) buf.append(",");
   numflds.back() = -1;

   if (numchilds.back()++ == 0) {
      NewLine();

      if (basename==0) basename = "_childs";

      buf.append(dabc::format("%*s\"%s\"", (compact() > 0) ? 0 : lvl * 4 - 2, "", basename));
      switch(compact()) {
         case 2: buf.append(": ["); break;
         case 3: buf.append(":["); break;
         default: buf.append(" : [");
      }
   } else {
      buf.append(",");
   }
   NewLine();
}

void dabc::HJsonStore::CloseChilds()
{
   if (numchilds.back() > 0) {
      NewLine();
      buf.append(dabc::format("%*s]", (compact() > 0) ? 0 : lvl * 4 - 2, ""));
      numchilds.back() = 0;
      numflds.back() = 1; // artificially mark that something was written
   }
}


// ===========================================================================


dabc::RecordField::RecordField() :
   fKind(kind_none),
   fModified(false),
   fTouched(false)
{
}

bool dabc::RecordField::cannot_modify()
{
   if (isreadonly()) return true;
   return false;
}

dabc::RecordField::RecordField(const RecordField& src)
{
   fKind = kind_none;
   fModified = false;
   fTouched = false;

   switch (src.fKind) {
      case kind_none: break;
      case kind_bool: SetBool(src.valueInt!=0); break;
      case kind_int: SetInt(src.valueInt); break;
      case kind_datime: SetDatime(src.valueUInt); break;
      case kind_uint: SetUInt(src.valueUInt); break;
      case kind_double: SetDouble(src.valueDouble); break;
      case kind_arrint: SetArrInt(src.valueInt, (int64_t*) src.arrInt); break;
      case kind_arruint: SetArrUInt(src.valueInt, (uint64_t*) src.arrUInt); break;
      case kind_arrdouble: SetArrDouble(src.valueInt, (double*) src.arrDouble); break;
      case kind_string: SetStr(src.valueStr); break;
      case kind_arrstr: SetArrStrDirect(src.valueInt, src.valueStr); break;
   }
}

dabc::RecordField::~RecordField()
{
   release();
}

uint64_t dabc::RecordField::StoreSize()
{
   sizestream s;
   Stream(s);
   return s.size();
}

bool dabc::RecordField::Stream(iostream& s)
{
   uint64_t pos = s.size();
   uint64_t sz = 0;

   uint32_t storesz(0), storekind(0), storeversion(0);

   if (s.is_output()) {
      sz = s.is_real() ? StoreSize() : 0;
      storesz = sz / 8;
      storekind = ((uint32_t)fKind & 0xffffff) | (storeversion << 24);
      s.write_uint32(storesz);
      s.write_uint32(storekind);
      switch (fKind) {
         case kind_none: break;
         case kind_bool:
         case kind_int: s.write_int64(valueInt); break;
         case kind_datime:
         case kind_uint: s.write_uint64(valueUInt); break;
         case kind_double: s.write_double(valueDouble); break;
         case kind_arrint:
            s.write_int64(valueInt);
            s.write(arrInt, valueInt*sizeof(int64_t));
            break;
         case kind_arruint:
            s.write_int64(valueInt);
            s.write(arrUInt, valueInt*sizeof(uint64_t));
            break;
         case kind_arrdouble:
            s.write_int64(valueInt);
            s.write(arrDouble, valueInt*sizeof(double));
            break;
         case kind_string:
            s.write(valueStr, strlen(valueStr)+1);
            break;
         case kind_arrstr: {
            int64_t fulllen = 0;
            char* ps = valueStr;
            for (int64_t n=0;n<valueInt;n++) {
               int len = strlen(ps) + 1;
               fulllen += len; ps += len;
            }
            s.write_int64(valueInt);
            s.write(valueStr, fulllen);
            break;
         }
      }
   } else {
      release();
      s.read_uint32(storesz);
      s.read_uint32(storekind);
      sz = ((uint64_t) storesz) * 8;
      // storeversion = storekind >> 24;


      fKind = (ValueKind) (storekind & 0xffffff);
      switch (fKind) {
         case kind_none: break;
         case kind_bool:
         case kind_int: s.read_int64(valueInt); break;
         case kind_datime:
         case kind_uint: s.read_uint64(valueUInt); break;
         case kind_double: s.read_double(valueDouble); break;
         case kind_arrint:
            s.read_int64(valueInt);
            arrInt = new int64_t[valueInt];
            s.read(arrInt, valueInt*sizeof(int64_t));
            break;
         case kind_arruint:
            s.read_int64(valueInt);
            arrUInt = new uint64_t[valueInt];
            s.read(arrUInt, valueInt*sizeof(uint64_t));
            break;
         case kind_arrdouble:
            s.read_int64(valueInt);
            arrDouble = new double[valueInt];
            s.read(arrDouble, valueInt*sizeof(double));
            break;
         case kind_string:
            valueStr = (char*) malloc((storesz-1)*8);
            s.read(valueStr, (storesz-1)*8);
            break;
         case kind_arrstr: {
            s.read_int64(valueInt);
            valueStr = (char*) malloc((storesz-2)*8);
            s.read(valueStr, (storesz-2)*8);
            break;
         }
      }
   }

   return s.verify_size(pos, sz);
}

void dabc::RecordField::release()
{
   switch (fKind) {
      case kind_none: break;
      case kind_bool: break;
      case kind_int: break;
      case kind_datime: break;
      case kind_uint: break;
      case kind_double: break;
      case kind_arrint: delete [] arrInt; break;
      case kind_arruint: delete [] arrUInt; break;
      case kind_arrdouble: delete [] arrDouble; break;
      case kind_string:
      case kind_arrstr: free(valueStr); break;
   }

   fKind = kind_none;
}

bool dabc::RecordField::AsBool(bool dflt) const
{
   switch (fKind) {
      case kind_none: return dflt;
      case kind_bool:
      case kind_int: return valueInt!=0;
      case kind_datime:
      case kind_uint: return valueUInt!=0;
      case kind_double: return valueDouble!=0.;
      case kind_arrint: if (valueInt>0) return arrInt[0]!=0; break;
      case kind_arruint: if (valueInt>0) return arrUInt[0]!=0; break;
      case kind_arrdouble: if (valueInt>0) return arrDouble[0]!=0; break;
      case kind_string:
      case kind_arrstr: {
         if (strcmp(valueStr,xmlTrueValue)==0) return true;
         if (strcmp(valueStr,xmlFalseValue)==0) return false;
         break;
      }
   }
   return dflt;
}

int64_t dabc::RecordField::AsInt(int64_t dflt) const
{
   switch (fKind) {
      case kind_none: return dflt;
      case kind_bool:
      case kind_int: return valueInt;
      case kind_datime:
      case kind_uint: return (int64_t) valueUInt;
      case kind_double: return (int64_t) valueDouble;
      case kind_arrint: if (valueInt>0) return arrInt[0]; break;
      case kind_arruint: if (valueInt>0) return (int64_t) arrUInt[0]; break;
      case kind_arrdouble: if (valueInt>0) return (int64_t) arrDouble[0]; break;
      case kind_string:
      case kind_arrstr: {
         long res;
         if (str_to_lint(valueStr, &res)) return res;
         break;
      }
   }
   return dflt;
}

uint64_t dabc::RecordField::AsUInt(uint64_t dflt) const
{
   switch (fKind) {
      case kind_none: return dflt;
      case kind_bool:
      case kind_int: return (uint64_t) valueInt;
      case kind_datime:
      case kind_uint: return valueUInt;
      case kind_double: return (uint64_t) valueDouble;
      case kind_arrint: if (valueInt>0) return (uint64_t) arrInt[0]; break;
      case kind_arruint: if (valueInt>0) return arrUInt[0]; break;
      case kind_arrdouble: if (valueInt>0) return (uint64_t) arrDouble[0]; break;
      case kind_string:
      case kind_arrstr: {
         long unsigned res;
         if (str_to_luint(valueStr, &res)) return res;
         break;
      }
   }
   return dflt;
}

double dabc::RecordField::AsDouble(double dflt) const
{
   switch (fKind) {
      case kind_none: return dflt;
      case kind_bool:
      case kind_int: return (double) valueInt;
      case kind_datime:
      case kind_uint: return (double) valueUInt;
      case kind_double: return valueDouble;
      case kind_arrint: if (valueInt>0) return (double) arrInt[0]; break;
      case kind_arruint: if (valueInt>0) return (double) arrUInt[0]; break;
      case kind_arrdouble: if (valueInt>0) return arrDouble[0]; break;
      case kind_string:
      case kind_arrstr: {
         double res;
         if (str_to_double(valueStr, &res)) return res;
         break;
      }
   }
   return dflt;
}

std::vector<int64_t> dabc::RecordField::AsIntVect() const
{
   std::vector<int64_t> res;

   switch (fKind) {
      case kind_none: break;
      case kind_bool:
      case kind_int: res.push_back(valueInt); break;
      case kind_datime:
      case kind_uint: res.push_back((int64_t) valueUInt); break;
      case kind_double: res.push_back((int64_t) valueDouble); break;
      case kind_arrint:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back(arrInt[n]);
         break;
      case kind_arruint:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back((int64_t) arrUInt[n]);
         break;
      case kind_arrdouble:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back((int64_t) arrDouble[n]);
         break;
      case kind_string: {
         long res0;
         if (str_to_lint(valueStr, &res0)) res.push_back(res0);
         break;
      }
      case kind_arrstr: {
         res.reserve(valueInt);
         long res0;
         char* p = valueStr;

         for (int64_t n=0;n<valueInt;n++) {
            if (!str_to_lint(p, &res0)) res0 = 0;
            res.push_back(res0);
            p += strlen(p)+1;
         }
         break;
      }

   }

   return res;
}

std::vector<uint64_t> dabc::RecordField::AsUIntVect() const
{
   std::vector<uint64_t> res;

   switch (fKind) {
      case kind_none: break;
      case kind_bool:
      case kind_int: res.push_back((uint64_t) valueInt); break;
      case kind_datime:
      case kind_uint: res.push_back(valueUInt); break;
      case kind_double: res.push_back((uint64_t) valueDouble); break;
      case kind_arrint:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back((uint64_t) arrInt[n]);
         break;
      case kind_arruint:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back(arrUInt[n]);
         break;
      case kind_arrdouble:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back((uint64_t) arrDouble[n]);
         break;
      case kind_string: {
         long unsigned res0;
         if (str_to_luint(valueStr, &res0)) res.push_back(res0);
         break;
      }
      case kind_arrstr: {
         res.reserve(valueInt);
         long unsigned res0;
         char* p = valueStr;

         for (int64_t n=0;n<valueInt;n++) {
            if (!str_to_luint(p, &res0)) res0 = 0;
            res.push_back(res0);
            p += strlen(p)+1;
         }
         break;
      }
   }

   return res;
}


std::vector<double> dabc::RecordField::AsDoubleVect() const
{
   std::vector<double> res;

   switch (fKind) {
      case kind_none: break;
      case kind_bool:
      case kind_int: res.push_back((double) valueInt); break;
      case kind_datime:
      case kind_uint: res.push_back((double) valueUInt); break;
      case kind_double: res.push_back(valueDouble); break;
      case kind_arrint:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back((double) arrInt[n]);
         break;
      case kind_arruint:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back((double) arrUInt[n]);
         break;
      case kind_arrdouble:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back(arrDouble[n]);
         break;
      case kind_string: {
         double res0;
         if (str_to_double(valueStr, &res0)) res.push_back(res0);
         break;
      }
      case kind_arrstr: {
         res.reserve(valueInt);
         double res0;
         char* p = valueStr;

         for (int64_t n=0;n<valueInt;n++) {
            if (!str_to_double(p, &res0)) res0 = 0.;
            res.push_back(res0);
            p += strlen(p)+1;
         }
         break;
      }
   }

   return res;
}

int dabc::RecordField::NeedXmlQuotes(const std::string& str)
{
   if (str.length()==0) return 1;

   if (str.find_first_of("\'\"<>&")!=std::string::npos) return 2;

   return (str.find_first_of(" ,[]")!=std::string::npos) ? 1 : 0;
}

std::string dabc::RecordField::ExpandXmlValue(const std::string& str)
{
   std::string res;

   for (unsigned n=0;n<str.length();n++)
      switch(str[n]) {
         case '\"': res.append("&quout;"); break;
         case '\'': res.append("&apos;"); break;
         case '<': res.append("&lt;"); break;
         case '>': res.append("&gt;"); break;
         case '&': res.append("&amp;"); break;
         default: res+=str[n]; break;
      }

   return res;
}

std::string dabc::RecordField::CompressXmlValue(const char* str, int len)
{
   std::string res;
   int n=0;
   while (n<len) {
      if (str[n]!='&') { res+=str[n++]; continue; }

      if ((len - n >= 7) && (strncmp(str+n, "&quout;", 7)==0)) { res+="\""; n+=7; continue; }
      if ((len - n >= 6) && (strncmp(str+n, "&apos;", 6)==0)) { res+="\'"; n+=6; continue; }
      if ((len - n >= 4) && (strncmp(str+n, "&lt;", 4)==0)) { res+="<"; n+=4; continue; }
      if ((len - n >= 4) && (strncmp(str+n, "&gt;", 4)==0)) { res+=">"; n+=4; continue; }
      if ((len - n >= 5) && (strncmp(str+n, "&amp;", 5)==0)) { res+="&"; n+=5; continue; }

      res+=str[n++];
   }

   return res;
}


std::string dabc::RecordField::AsStr(const std::string& dflt) const
{
   switch (fKind) {
      case kind_none: return dflt;
      case kind_bool: return valueInt!=0 ? xmlTrueValue : xmlFalseValue;
      case kind_int: return dabc::format("%ld", (long) valueInt);
      case kind_datime: {
         char sbuf[35];
         if (dabc::DateTime(valueUInt).AsJSString(sbuf, sizeof(sbuf), 3))
            return std::string(sbuf);
         return dflt;
      }
      case kind_uint: return dabc::format("%lu", (long unsigned) valueUInt);
      case kind_double: return dabc::format("%g", valueDouble);
      case kind_arrint:
      case kind_arruint:
      case kind_arrdouble:
      case kind_arrstr: {

         std::vector<std::string> vect = AsStrVect();
         if (vect.size()==0) return dflt;

         std::string res("[");
         for (unsigned n=0; n<vect.size();n++) {
            if (n>0) res.append(",");
            switch (NeedXmlQuotes(vect[n])) {
               case 1:
                  res.append("\'");
                  res.append(vect[n]);
                  res.append("\'");
                  break;
               case 2:
                  res.append("\'");
                  res.append(ExpandXmlValue(vect[n]));
                  res.append("\'");
                  break;
               default:
                  res.append(vect[n]);
                  break;
            }
         }
         res.append("]");
         return res;

         break;
      }
      case kind_string: {
         if (valueStr!=0) return valueStr;
         break;
      }
   }
   return dflt;
}

bool dabc::RecordField::NeedJsonReformat(const std::string& str)
{
   // here we have situation that both single and double quotes present
   // first try to define which kind quotes preceding with escape character
   bool is_last_escape(false);

   for (unsigned n=0;n<str.length();n++)

      switch (str[n]) {
         case '\\' : is_last_escape = !is_last_escape; break;
         case '\n' : return true; // new-line symbol should be replaced
         case '\"' :
            if (!is_last_escape) return true;
            is_last_escape = false;
            break;
         case 'n' :
            if (is_last_escape) return true; // string '\n' should be replaced by "\\n"
            break;
         default:
            is_last_escape = false;
            break;
      }

   return is_last_escape;
}

std::string dabc::RecordField::JsonReformat(const std::string& str)
{
   std::string res;

   bool is_last_escape(false);

   for (unsigned n=0;n<str.length();n++) {
      if (str[n] == '\n') {
         res += "\\\\n";
         continue;
      }

      if (str[n] == '\t') {
         res += "\\\\t";
         continue;
      }

      if (str[n] == '\\') is_last_escape = !is_last_escape;

      if ((str[n] == '\"') && !is_last_escape) res+='\\'; else
      if ((str[n] == 'n') && is_last_escape) res += '\\'; // when appearing \n, replace by \\n

      res += str[n];
   }

   if (is_last_escape) res.append("\\");
   return res;
}

std::string dabc::RecordField::AsJson() const
{
   switch (fKind) {
      case kind_none: return "null";
      case kind_bool: return valueInt!=0 ? "true" : "false";
      case kind_int: return dabc::format("%ld", (long) valueInt);
      case kind_datime: {
         char sbuf[35];
         if (dabc::DateTime(valueUInt).AsJSString(sbuf, sizeof(sbuf), 3))
            return dabc::format("\"%s\"", sbuf);
         break;
      }
      case kind_uint: return dabc::format("%lu", (long unsigned) valueUInt);
      case kind_double: return dabc::format("%g", valueDouble);
      case kind_arrint: {
         std::string res("[");
         for (int64_t n=0; n<valueInt;n++) {
            if (n>0) res.append(",");
            res.append(dabc::format("%ld", (long) arrInt[n]));
         }
         res.append("]");
         return res;
      }
      case kind_arruint: {
         std::string res("[");
         for (int64_t n=0; n<valueInt;n++) {
            if (n>0) res.append(",");
            res.append(dabc::format("%lu", (long unsigned) arrUInt[n]));
         }
         res.append("]");
         return res;
      }
      case kind_arrdouble: {
         std::string res("[");
         for (int64_t n=0; n<valueInt;n++) {
            if (n>0) res.append(",");
            res.append(dabc::format("%g", arrDouble[n]));
         }
         res.append("]");
         return res;
      }
      case kind_arrstr: {

         std::vector<std::string> vect = AsStrVect();

         std::string res("[");
         for (unsigned n=0; n<vect.size();n++) {
            if (n>0) res.append(",");
            res.append("\"");
            if (NeedJsonReformat(vect[n]))
               res.append(JsonReformat(vect[n]));
            else
               res.append(vect[n]);
            res.append("\"");
         }
         res.append("]");
         return res;
      }
      case kind_string: {
         if (valueStr==0) return "\"\"";
         std::string res("\"");
         if (NeedJsonReformat(valueStr))
            res.append(JsonReformat(valueStr));
         else
            res.append(valueStr);
         res.append("\"");
         return res;
      }
   }
   return "null";
}

std::vector<std::string> dabc::RecordField::AsStrVect() const
{
   std::vector<std::string> res;

   switch (fKind) {
      case kind_none: break;
      case kind_bool:
      case kind_int:
      case kind_datime:
      case kind_uint:
      case kind_double: res.push_back(AsStr()); break;
      case kind_arrint: {
         for (int64_t n=0; n<valueInt;n++)
            res.push_back(dabc::format("%ld", (long) arrInt[n]));
         break;
      }
      case kind_arruint: {
         for (int64_t n=0; n<valueInt;n++)
            res.push_back(dabc::format("%lu", (long unsigned) arrUInt[n]));
         break;
      }
      case kind_arrdouble: {
         for (int64_t n=0; n<valueInt;n++)
            res.push_back(dabc::format("%g", arrDouble[n]));
         break;
      }
      case kind_string: {
         if (!StrToStrVect(valueStr, res))
            res.push_back(valueStr);
          break;
      }
      case kind_arrstr: {
         char* p = valueStr;
         for (int64_t n=0;n<valueInt;n++) {
            res.push_back(p);
            p += strlen(p)+1;
         }
         break;
      }
   }

   return res;
}

bool dabc::RecordField::StrToStrVect(const char* str, std::vector<std::string>& vect, bool verbose)
{
   int len = strlen(str);

   if ((len<2) || (str[0]!='[') || (str[len-1]!=']')) return false;

   const char* pos = str + 1;

   while (*pos != 0) {
      while (*pos==' ') pos++; // exclude possible spaces in the begin
      if ((*pos=='\'') || (*pos == '\"')) {
         const char* p1 = strchr(pos+1, *pos);
         if (p1==0) {
            if (verbose) EOUT("Error syntax in array %s after char:%u - closing quote not found ", str, (unsigned) (pos - str));
            vect.clear();
            return false;
         }
         vect.push_back(CompressXmlValue(pos+1, p1 - pos - 1));
         pos = p1 + 1;
      } else {
         const char* p1 = strpbrk(pos+1, ",]");
         if (p1==0) {
            if (verbose) EOUT("Error syntax in array %s after char:%u - ',' or ']' not found ", str, (unsigned) (pos - str));
            vect.clear();
            return false;
         }
         int spaces = 0;
         while ((p1 - spaces > pos + 1) && (*(p1-spaces-1)==' ')) spaces++;
         vect.push_back(std::string(pos, p1 - pos - spaces));
         pos = p1;
      }
      while (*pos==' ') pos++; // exclude possible spaces at the end

      // this is end of the array
      if (*pos==']') break;

      if (*pos != ',') {
         if (verbose) EOUT("Error syntax in array %s char:%u - expected ',' ", str, (unsigned) (pos - str));
         break;
      }
      pos++;
   }

   return true;
}


bool dabc::RecordField::SetValue(const RecordField& src)
{
   switch (src.fKind) {
      case kind_none: return SetNull();
      case kind_bool: return SetBool(src.valueInt!=0);
      case kind_int: return SetInt(src.valueInt);
      case kind_datime: return SetDatime(src.valueUInt);
      case kind_uint: return SetUInt(src.valueUInt);
      case kind_double: return SetDouble(src.valueDouble);
      case kind_arrint: return SetArrInt(src.valueInt, (int64_t*) src.arrInt);
      case kind_arruint: return SetArrUInt(src.valueInt, (uint64_t*) src.arrUInt);
      case kind_arrdouble: return SetArrDouble(src.valueInt, (double*) src.arrDouble);
      case kind_string: return SetStr(src.valueStr);
      case kind_arrstr: return SetArrStr(src.valueInt, src.valueStr);
   }
   return false;
}


bool dabc::RecordField::SetNull()
{
   if (cannot_modify()) return false;
   if (fKind == kind_none) return modified(false);
   release();
   return modified();
}


bool dabc::RecordField::SetBool(bool v)
{
   if (cannot_modify()) return false;

   if ((fKind == kind_bool) && (valueInt == (v ? 1 : 0))) return modified(false);

   release();
   fKind = kind_bool;
   valueInt = v ? 1 : 0;

   return modified();
}

bool dabc::RecordField::SetInt(int64_t v)
{
   if (cannot_modify()) return false;
   if ((fKind == kind_int) && (valueInt == v)) return modified(false);
   release();
   fKind = kind_int;
   valueInt = v;
   return modified();
}

bool dabc::RecordField::SetDatime(uint64_t v)
{
   if (cannot_modify()) return false;
   if ((fKind == kind_datime) && (valueUInt == v)) return modified(false);

   release();
   fKind = kind_datime;
   valueUInt = v;

   return modified();
}

bool dabc::RecordField::SetDatime(const DateTime& v)
{
   return SetDatime(v.AsJSDate());
}


bool dabc::RecordField::SetUInt(uint64_t v)
{
   if (cannot_modify()) return false;

   if ((fKind == kind_uint) && (valueUInt == v)) return modified(false);

   release();
   fKind = kind_uint;
   valueUInt = v;

   return modified();
}

bool dabc::RecordField::SetDouble(double v)
{
   if (cannot_modify()) return false;

   if ((fKind == kind_double) && (valueDouble == v)) return modified(false);

   release();
   fKind = kind_double;
   valueDouble = v;

   return modified();
}

bool dabc::RecordField::SetStr(const std::string& v)
{
   if (cannot_modify()) return false;

   if ((fKind == kind_string) && (v==valueStr)) return modified(false);

   release();
   fKind = kind_string;
   valueStr = (char*) malloc(v.length()+1);
   strcpy(valueStr, v.c_str());

   return modified();
}

bool dabc::RecordField::SetStr(const char* v)
{
   if (cannot_modify()) return false;

   if ((fKind == kind_string) && (v!=0) && (strcmp(v,valueStr)==0)) return modified(false);

   release();
   fKind = kind_string;
   size_t len = v==0 ? 0 : strlen(v);
   valueStr = (char*) malloc(len+1);
   if (v!=0) strcpy(valueStr, v);
        else *valueStr = 0;

   return modified();
}

bool dabc::RecordField::SetStrVect(const std::vector<std::string>& vect)
{
   if (cannot_modify()) return false;

   if ((fKind == kind_arrstr) && (valueInt == (int64_t) vect.size())) {
      std::vector<std::string> vect0 = AsStrVect();
      if (vect0.size() == vect.size()) {
         bool diff = false;
         for (unsigned n=0;n<vect.size();n++)
            if (vect[n]!=vect0[n]) diff = true;
         if (!diff) return modified(false);
      }
   }

   release();
   fKind = kind_arrstr;

   valueInt = (int64_t) vect.size();

   size_t len = 0;
   for (unsigned n=0;n<vect.size();n++)
      len+=vect[n].length()+1;
   valueStr = (char*) malloc(len);
   char* p = valueStr;

   for (unsigned n=0;n<vect.size();n++) {
      strcpy(p, vect[n].c_str());
      p+=vect[n].length()+1;
   }

   return modified();

}

bool dabc::RecordField::SetArrInt(int64_t size, int64_t* arr, bool owner)
{
   if (cannot_modify() || (size<=0)) {
      if (owner) delete[] arr;
      return false;
   }

   if ((fKind == kind_arrint) && (size==valueInt))
      if (memcmp(arrInt, arr, size*sizeof(int64_t))==0)
      {
         if (owner) delete[] arr;
         return modified(false);
      }

   release();
   fKind = kind_arrint;
   valueInt = size;
   if (owner) {
      arrInt = arr;
   } else {
      arrInt = new int64_t[size];
      memcpy(arrInt, arr, size*sizeof(int64_t));
   }

   return modified();
}

bool dabc::RecordField::SetVectInt(const std::vector<int64_t>& v)
{
   int64_t* arr = 0;
   if (v.size()>0) {
      arr = new int64_t[v.size()];
      for (unsigned n=0;n<v.size();n++)
         arr[n] = v[n];
   }
   return SetArrInt(v.size(), arr, true);
}


bool dabc::RecordField::SetArrUInt(int64_t size, uint64_t* arr, bool owner)
{
   if (cannot_modify()) return false;
   if (size<=0) return false;

   if ((fKind == kind_arruint) && (valueInt == size))
      if (memcmp(arrUInt, arr, size*sizeof(uint64_t))==0) {
         if (owner) delete [] arr;
         return modified(false);
      }

   release();
   fKind = kind_arruint;
   valueInt = size;
   if (owner) {
      arrUInt = arr;
   } else {
      arrUInt = new uint64_t[size];
      memcpy(arrUInt, arr, size*sizeof(uint64_t));
   }

   return modified();
}

bool dabc::RecordField::SetVectUInt(const std::vector<uint64_t>& v)
{
   uint64_t* arr = 0;
   if (v.size()>0) {
      arr = new uint64_t[v.size()];
      for (unsigned n=0;n<v.size();n++)
         arr[n] = v[n];
   }
   return SetArrUInt(v.size(), arr, true);
}


bool dabc::RecordField::SetVectDouble(const std::vector<double>& v)
{
   double* arr = 0;
   if (v.size()>0) {
      arr = new double[v.size()];
      for (unsigned n=0;n<v.size();n++)
         arr[n] = v[n];
   }

   return SetArrDouble(v.size(), arr, true);
}


bool dabc::RecordField::SetArrDouble(int64_t size, double* arr, bool owner)
{
   if (cannot_modify()) return false;
   if (size<=0) return false;

   if ((fKind == kind_arrdouble) && (valueInt == size))
      if (memcmp(arrDouble, arr, size*sizeof(double))==0) {
         if (owner) delete[] arr;
         return modified(false);
      }

   release();
   fKind = kind_arrdouble;
   valueInt = size;
   if (owner) {
      arrDouble = arr;
   } else {
      arrDouble = new double[size];
      memcpy(arrDouble, arr, size*sizeof(double));
   }

   return modified();
}

bool dabc::RecordField::SetArrStr(int64_t size, char* arr, bool owner)
{
   if (cannot_modify()) return false;
   if (size<=0) return false;
   // TODO: check if content was not changed
   release();
   SetArrStrDirect(size, arr, owner);
   return modified();
}


void dabc::RecordField::SetArrStrDirect(int64_t size, char* arr, bool owner)
{
   fKind = kind_arrstr;
   valueInt = size;
   if (owner) {
      valueStr = arr;
   } else {
      int fullsize = 0;
      const char* p = arr;
      for (unsigned n=0;n<size;n++) {
         int len = strlen(p);
         fullsize += len+1;
         p+=len+1;
      }
      valueStr = (char*) malloc(fullsize);
      memcpy(valueStr, arr, fullsize);
   }
}


// =========================================================================

dabc::RecordFieldsMap::RecordFieldsMap() :
   fMap(),
   fChanged(false)
{
}

dabc::RecordFieldsMap::~RecordFieldsMap()
{
}

bool dabc::RecordFieldsMap::HasField(const std::string& name) const
{
   return fMap.find(name) != fMap.end();
}

bool dabc::RecordFieldsMap::RemoveField(const std::string& name)
{
   FieldsMap::iterator iter = fMap.find(name);
   if (iter==fMap.end()) return false;
   fMap.erase(iter);
   fChanged = true;
   return true;
}


std::string dabc::RecordFieldsMap::FieldName(unsigned n) const
{
   if (n>=fMap.size()) return "";

   for (FieldsMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (n==0) return iter->first;
      n--;
   }

   return "";
}

std::string dabc::RecordFieldsMap::FindFieldWhichStarts(const std::string& name)
{
   for (FieldsMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (iter->first.length() >= name.length())
         if (iter->first.compare(0, name.length(), name)==0) return iter->first;
   }

   return "";
}

bool dabc::RecordFieldsMap::WasChanged() const
{
   if (fChanged) return true;

   for (FieldsMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++)
      if (iter->second.IsModified()) return true;

   return false;
}

bool dabc::RecordFieldsMap::WasChangedWith(const std::string& prefix)
{
   // returns true when field with specified prefix was modified

   for (FieldsMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (iter->second.IsModified())
         if (iter->first.find(prefix)==0) return true;
   }

   return false;
}


void dabc::RecordFieldsMap::ClearChangeFlags()
{
   fChanged = false;
   for (FieldsMap::iterator iter = fMap.begin(); iter!=fMap.end(); iter++)
      iter->second.SetModified(false);
}

dabc::RecordFieldsMap* dabc::RecordFieldsMap::Clone()
{
   dabc::RecordFieldsMap* res = new dabc::RecordFieldsMap;

   for (FieldsMap::iterator iter = fMap.begin(); iter!=fMap.end(); iter++)
      res->Field(iter->first).SetValue(iter->second);

   return res;
}



uint64_t dabc::RecordFieldsMap::StoreSize(const std::string& nameprefix)
{
   sizestream s;
   Stream(s, nameprefix);
   return s.size();
}

bool dabc::RecordFieldsMap::Stream(iostream& s, const std::string& nameprefix)
{
   uint32_t storesz(0), storenum(0), storevers(0);
   uint64_t sz;

   uint64_t pos = s.size();

   if (s.is_output()) {
      sz = s.is_real() ? StoreSize(nameprefix) : 0;
      storesz = sz/8;
      storenum = (uint32_t) fMap.size();
      if (nameprefix.length()>0)
         for (FieldsMap::iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
            if (iter->first.find(nameprefix) != 0) storenum--;
         }

      s.write_uint32(storesz);
      s.write_uint32(storenum  | (storevers<<24));

      for (FieldsMap::iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
         if ((nameprefix.length()>0) && (iter->first.find(nameprefix) != 0)) continue;
         s.write_str(iter->first);
         iter->second.Stream(s);
      }

   } else {
      // depending on nameprefix argument, two strategy are followed
      // if nameprefix.empty(),  we need to detect changed, removed or new fields
      // at the end full list should be exactly to that we try to read from the stream
      // if not nameprefix.empty(), we need just to detect if fields with provided prefix are changed,
      // all other fields can remain as is

      s.read_uint32(storesz);
      sz = ((uint64_t) storesz)*8;
      s.read_uint32(storenum);
      // storevers = storenum >> 24;
      storenum = storenum & 0xffffff;

      // first clear touch flags
      for (FieldsMap::iterator iter = fMap.begin(); iter!=fMap.end(); iter++)
         iter->second.fTouched = false;

      for (uint32_t n=0;n<storenum;n++) {
         std::string name;
         s.read_str(name);
         RecordField& fld = fMap[name];
         fld.Stream(s);
         fld.fTouched = true;
      }

      FieldsMap::iterator iter = fMap.begin();
      // now we should remove all fields, which were not touched
      while (iter!=fMap.end()) {
         if (iter->second.fTouched) { iter++; continue; }
         if (!nameprefix.empty() && (iter->first.find(nameprefix) != 0)) { iter++; continue; }
         fMap.erase(iter++);
      }

   }

   return s.verify_size(pos, sz);
}


bool dabc::RecordFieldsMap::SaveInXml(XMLNodePointer_t node)
{
   for (FieldsMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {

      if (iter->first.empty() || (iter->first[0]=='#')) continue;

      std::string value = iter->second.AsStr();

      // if somebody use wrong symbol in parameter name, pack it differently
      if (iter->first.find_first_of(" #&\"\'!@%^*()=-\\/|~.,")!=iter->first.npos) {
         XMLNodePointer_t child = Xml::NewChild(node, 0, "dabc:field", 0);
         Xml::NewAttr(child, 0, "name", iter->first.c_str());
         Xml::NewAttr(child, 0, "value", value.c_str());
      } else {
         // add attribute
         Xml::NewAttr(node,0,iter->first.c_str(), value.c_str());
      }

   }
   return true;
}

bool dabc::RecordFieldsMap::SaveTo(HStore& res)
{
   for (FieldsMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {

      if (iter->first.empty() || (iter->first[0]=='#')) continue;

      // discard attributes, which using quotes or any special symbols in the names
      if (iter->first.find_first_of(" #&\"\'!@%^*()=-\\/|~.,") != std::string::npos) continue;

      res.SetField(iter->first.c_str(), iter->second.AsJson().c_str());
   }
   return true;
}

bool dabc::RecordFieldsMap::ReadFromXml(XMLNodePointer_t node, bool overwrite, const ResolveFunc& func)
{
   if (node==0) return false;

   for (XMLAttrPointer_t attr = Xml::GetFirstAttr(node); attr!=0; attr = Xml::GetNextAttr(attr)) {
      const char* attrname = Xml::GetAttrName(attr);
      const char* attrvalue = Xml::GetAttrValue(attr);
      if ((attrname==0) || (attrvalue==0)) continue;

      DOUT3("Cont:%p  attribute:%s overwrite:%s", this, attrname, DBOOL(overwrite));

      if (overwrite || !HasField(attrname))
         fMap[attrname].SetStr(func.Resolve(attrvalue));
   }

   for (XMLNodePointer_t child = Xml::GetChild(node); child!=0; child = Xml::GetNext(child)) {

      if (strcmp(Xml::GetNodeName(child), "dabc:field")!=0) continue;

      const char* attrname = Xml::GetAttr(child,"name");
      const char* attrvalue = Xml::GetAttr(child,"value");
      if ((attrname==0) || (attrvalue==0)) continue;

      if (overwrite || !HasField(attrname))
        fMap[attrname].SetStr(func.Resolve(attrvalue));
   }

   return true;
}

void dabc::RecordFieldsMap::CopyFrom(const RecordFieldsMap& src, bool overwrite)
{
   for (FieldsMap::const_iterator iter = src.fMap.begin(); iter!=src.fMap.end(); iter++)
      if (overwrite || !HasField(iter->first))
         fMap[iter->first] = iter->second;
}

void dabc::RecordFieldsMap::MoveFrom(RecordFieldsMap& src)
{
   std::vector<std::string> delfields;

   for (FieldsMap::iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (iter->second.IsProtected()) continue;
      if (!src.HasField(iter->first)) delfields.push_back(iter->first);
   }

   for (unsigned n=0;n<delfields.size();n++)
      RemoveField(delfields[n]);

   for (FieldsMap::iterator iter = src.fMap.begin(); iter!=src.fMap.end(); iter++) {
      // should we completely preserve protected fields???
      // if (iter->second.IsProtected()) continue;

      fMap[iter->first].SetValue(iter->second);
   }
}


void dabc::RecordFieldsMap::MakeAsDiffTo(const RecordFieldsMap& current)
{
   std::vector<std::string> delfields;

   for (FieldsMap::const_iterator iter = current.fMap.begin(); iter!=current.fMap.end(); iter++) {
      if (!HasField(iter->first))
         delfields.push_back(iter->first);
      else
         if (!iter->second.fModified) RemoveField(iter->first);
   }

   // we remember fields, which should be delete when we start to reconstruct history
   if (delfields.size()>0)
      Field("dabc:del").SetStrVect(delfields);
}

void dabc::RecordFieldsMap::ApplyDiff(const RecordFieldsMap& diff)
{
   for (FieldsMap::const_iterator iter = diff.fMap.begin(); iter!=diff.fMap.end(); iter++) {
      if (iter->first != "dabc:del") {
         fMap[iter->first] = iter->second;
         fMap[iter->first].fModified = true;
      } else {
         std::vector<std::string> delfields = iter->second.AsStrVect();
         for (unsigned n=0;n<delfields.size();n++)
            RemoveField(delfields[n]);
      }
   }
}


// ===============================================================================



dabc::RecordContainer::RecordContainer(const std::string& name, unsigned flags) :
   Object(0, name, flags | flAutoDestroy),
   fFields(new RecordFieldsMap)
{
}

dabc::RecordContainer::RecordContainer(Reference parent, const std::string& name, unsigned flags) :
   Object(MakePair(parent, name), flags | flAutoDestroy),
   fFields(new RecordFieldsMap)
{
}

dabc::RecordContainer::~RecordContainer()
{
   delete fFields; fFields = 0;
}

dabc::RecordFieldsMap* dabc::RecordContainer::TakeFieldsMap()
{
   dabc::RecordFieldsMap* res = fFields;
   fFields = new RecordFieldsMap;
   return res;
}

void dabc::RecordContainer::SetFieldsMap(RecordFieldsMap* newmap)
{
   delete fFields;
   fFields = newmap;
   if (fFields==0) fFields = new RecordFieldsMap;
}

void dabc::RecordContainer::Print(int lvl)
{
   DOUT1("%s : %s", ClassName(), GetName());

   for (unsigned n=0;n<fFields->NumFields();n++) {
      std::string name = fFields->FieldName(n);
      std::string value = fFields->Field(name).AsStr();
      DOUT1("   %s = %s", name.c_str(), value.c_str());
   }
}

dabc::XMLNodePointer_t dabc::RecordContainer::SaveInXmlNode(XMLNodePointer_t parent)
{
   XMLNodePointer_t node = Xml::NewChild(parent, 0, GetName(), 0);

   fFields->SaveInXml(node);

   return node;
}

bool dabc::RecordContainer::SaveTo(HStore& store)
{
   store.CreateNode(GetName());
   bool res = fFields->SaveTo(store);
   store.CloseNode(GetName());
   return res;
}

// ===========================================================================================


std::string dabc::Record::SaveToJson(unsigned mask)
{
   dabc::HJsonStore store(mask);
   if (SaveTo(store)) return store.GetResult();
   return "";
}

std::string dabc::Record::SaveToXml(unsigned mask)
{
   dabc::HXmlStore store(mask);
   if (SaveTo(store)) return store.GetResult();
   return "";
}

