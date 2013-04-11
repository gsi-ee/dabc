#ifndef ROC_BOARDSVECTOR_H
#define ROC_BOARDSVECTOR_H

#include <vector>

#include <string>

#include "roc/Message.h"
#include "roc/Board.h"
#include "nxyter/FebBase.h"

namespace roc {
  
  /** This is helper class to accomodate collection of 
    *  roc::Board objects and supply classes, which are 
    *  required for combiner module in dabc. 
    */

   typedef std::vector<roc::Message> MessagesVector;


   struct BoardRec {
      roc::Board*      brd;
      std::string      devname;
      std::string      rocname;
      std::string      febskind;
      nxyter::FebBase* feb0;
      nxyter::FebBase* feb1;
      bool             get4;
      uint32_t         gpio_mask_save;
      uint32_t         gpio_mask_on;
      uint32_t         gpio_mask_off;
      BoardRec(roc::Board* _brd = 0) :
         brd(_brd), devname(), rocname(), febskind(), feb0(0), feb1(0), get4(false),
         gpio_mask_save(0), gpio_mask_on(0), gpio_mask_off(0) {}
      BoardRec(const BoardRec& rec) :
         brd(rec.brd), devname(rec.devname), 
         rocname(rec.rocname), febskind(rec.febskind),
         feb0(rec.feb0), feb1(rec.feb1), get4(rec.get4),
         gpio_mask_save(rec.gpio_mask_save), gpio_mask_on(rec.gpio_mask_on), gpio_mask_off(rec.gpio_mask_off) {}
   };


   class BoardsVector : public std::vector<BoardRec> {
      protected:
         std::vector<std::string> fDLMDevs;
      public:
         BoardsVector() : std::vector<BoardRec>() {}
         virtual ~BoardsVector() {}

         void addRoc(const std::string& roc, const std::string& febs);
         bool setBoard(unsigned n, roc::Board* brd, std::string devname);
         void addDLMDev(const std::string& dlm) { fDLMDevs.push_back(dlm); }
 
         void returnBoards();

         void produceSystemMessage(uint32_t id);

         roc::Board* brd(unsigned n) { return at(n).brd; }
         
         // returns number of adc channels on the feb
         int numAdc(unsigned n, unsigned nfeb);

         MessagesVector* readoutExtraMessages();

         void autoped_switch(bool on);
         
         void issueDLM(int code);

         bool isAnyGet4();

         void ResetAllGet4();

      protected:

         void autoped_issue_system_message(roc::Board* brd, uint32_t type);
         void autoped_setnxmode(nxyter::FebBase* feb, bool testtrig);

         void readFeb(uint32_t rocid, uint32_t febid, nxyter::FebBase* feb, MessagesVector* vect);

   };
}

#endif
