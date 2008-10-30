#ifndef TEVENT_H
#define TEVENT_H

#include "TGo4EventElement.h"

class TRocFastEvent : public TGo4EventElement {
   public:
      TRocFastEvent();
      TRocFastEvent(const char* name);
      virtual ~TRocFastEvent();

      /** Method called by the framework to clear the event element. */
      void Clear(Option_t *t="");

      Int_t    fValue; 

   ClassDef(TRocFastEvent,1)
};
#endif //TEVENT_H



