#include "mbs/LmdInput.h"

#include <string.h>
#include <stdlib.h>

#include "dabc/logging.h"
#include "dabc/Buffer.h"
#include "dabc/FileIO.h"
#include "dabc/Manager.h"

#include "mbs/MbsTypeDefs.h"

mbs::LmdInput::LmdInput(const char* fname, 
                        int nummulti,
                        int firstmulti) :
   dabc::DataInput(),
   fFileName(fname ? fname : ""),
   fNumMultiple(nummulti),
   fFirstMultiple(firstmulti),
   fFilesList(0),
   fFile(),
   fCurrentFileName()
{
}

mbs::LmdInput::~LmdInput()
{
   CloseFile(); 
   if (fFilesList) {
      delete fFilesList;
      fFilesList = 0;
   }
}

bool mbs::LmdInput::Init()
{
   if (fFileName.length()==0) return false;
   
   if (fFilesList!=0) {
      EOUT(("Files list already exists"));
      return false;   
   }
   
   if (strpbrk(fFileName.c_str(),"*?")!=0)
      fFilesList = dabc::Manager::Instance()->ListMatchFiles("", fFileName.c_str());
   else 
   if (fNumMultiple<=0) {
//      std::string mask = fFileName;
//      mask += "_*.lmd";
//      fFilesList = dabc::Manager::Instance()->ListMatchFiles("", mask.c_str());
//      
//      DOUT1(("Try to find files with mask %s", mask.c_str()));
//   } else
//   if (fNumMultiple == 0) {
      fFilesList = new dabc::Folder(0,"FilesList", true);
      new dabc::Basic(fFilesList, fFileName.c_str());
   } else {
      int number = fFirstMultiple;
      
      fFilesList = new dabc::Folder(0,"FilesList", true);
      
      while (number<fNumMultiple) {
         std::string fname;
         dabc::formats(fname, "%s_%04d.lmd", fFileName.c_str(), number);
         new dabc::Basic(fFilesList, fname.c_str());
         number++;
      }
   }
   
   return OpenNextFile();
}

bool mbs::LmdInput::OpenNextFile()
{
   fFile.Close(); 
   fCurrentFileName = "";
       
   if ((fFilesList==0) || (fFilesList->NumChilds()==0)) return false;
    
   const char* nextfilename = fFilesList->GetChild(0)->GetName();
   
   bool res = fFile.OpenRead(nextfilename);
   
   if (!res) 
      EOUT(("Cannot open file %s for reading, errcode:%u", nextfilename, fFile.LastError()));
   else 
      fCurrentFileName = nextfilename;
       
   delete fFilesList->GetChild(0);
   
   return res; 
}
         

bool mbs::LmdInput::CloseFile()
{
   fFile.Close(); 
   
   return true;
}
         
unsigned mbs::LmdInput::Read_Size()
{
   // get size of the buffer which should be read from the file  
   
   if (!fFile.IsReadMode()) 
      if (!OpenNextFile()) return di_EndOfStream;
   
   return 64*1024;
}

unsigned mbs::LmdInput::Read_Complete(dabc::Buffer* buf)
{
   unsigned numev = 0;
   uint32_t bufsize = 0;

   do { 
    
       if (!fFile.IsReadMode()) return di_Error;
       
       bufsize = buf->GetDataSize();
       
       numev = fFile.ReadBuffer(buf->GetDataLocation(), bufsize);
       
       if (numev==0) {
          DOUT1(("File %s return 0 numev for buffer %u - end of file", fCurrentFileName.c_str(), buf->GetDataSize())); 
          if (!OpenNextFile()) return di_EndOfStream;
       }
       
   } while (numev==0);
   
   buf->SetDataSize(bufsize); 
   buf->SetTypeId(mbs::mbt_MbsEvs10_1);
   
   return di_Ok; 
}
