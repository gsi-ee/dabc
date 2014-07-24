var POLAND_TS_NUM = 3,
    POLAND_DAC_NUM = 32,
    POLAND_ERRCOUNT_NUM = 8,
    POLAND_TIME_UNIT = 0.02,

    POLAND_REG_TRIGCOUNT        = 0x000000,
    POLAND_REG_RESET            = 0x200000,
    POLAND_REG_STEPS_BASE       = 0x200014,
    POLAND_REG_STEPS_TS1        = 0x200014,
    POLAND_REG_STEPS_TS2        = 0x200018,
    POLAND_REG_STEPS_TS3        = 0x20001C,

    POLAND_REG_TIME_BASE        = 0x200020,
    POLAND_REG_TIME_TS1         = 0x200020,
    POLAND_REG_TIME_TS2         = 0x200024,
    POLAND_REG_TIME_TS3         = 0x200028,

    POLAND_REG_QFW_MODE         = 0x200004,

    POLAND_REG_DAC_MODE         = 0x20002C,
    POLAND_REG_DAC_PROGRAM      = 0x200030,
    POLAND_REG_DAC_BASE_WRITE   = 0x200050,
    POLAND_REG_DAC_BASE_READ    = 0x200180,

    POLAND_REG_DAC_ALLVAL       = 0x2000d4,
    POLAND_REG_DAC_CAL_STARTVAL = 0x2000d0,
    POLAND_REG_DAC_CAL_OFFSET   = 0x200034,
    POLAND_REG_DAC_CAL_DELTA    = 0x20000c,
    POLAND_REG_DAC_CAL_TIME     = 0x200038,

    POLAND_REG_INTERNAL_TRIGGER = 0x200040,
    POLAND_REG_DO_OFFSET        = 0x200044,
    POLAND_REG_OFFSET_BASE      = 0x200100,
    POLAND_REG_MASTERMODE       = 0x200048,
    POLAND_REG_ERRCOUNT_BASE    = 0x200;



function PolandSetup(cmdurl) {
   
   // address used to access gosip command
   this.CmdUrl = cmdurl;
   
   this.fSFP = 0;
   this.fDEV = 0;
   
   this.fSteps = new Array;
   this.fTimes = new Array;
   for (var i=0;i<POLAND_TS_NUM;i++) {
      this.fSteps.push(0);
      this.fTimes.push(0);
   } 

   this.fInternalTrigger = 0;
   this.fTriggerMode = 0;
   this.fQFWMode = 0;

   this.fEventCounter = 0;
   this.fErrorCounter = new Array;
   for (var i=0;i<POLAND_ERRCOUNT_NUM;i++)
      this.fErrorCounter.push(0);

   /* DAC values and settings:*/
   this.fDACMode = 0;
   this.fDACValue = new Array;
   for (var i=0;i<POLAND_DAC_NUM;i++)
      this.fDACValue.push(i);

   this.fDACAllValue = 0;
   this.fDACStartValue = 0;
   this.fDACOffset = 0;
   this.fDACDelta = 0;
   this.fDACCalibTime = 0;
}

PolandSetup.prototype.GetCalibrationTime = function()
{
   return (this.fDACCalibTime*POLAND_TIME_UNIT)/1000.;
}


PolandSetup.prototype.RefreshDAC = function(base)
{
  //std::cout << "PolandGui::RefreshDAC"<< std::endl;
   
   if (!base) base = 16;
   
   var pre = base==16 ? "0x" : "";

   document.getElementById("DacOffset").value = pre + this.fDACOffset.toString(base);
   document.getElementById("DacDeltaOffset").value = pre + this.fDACDelta.toString(base);
   document.getElementById("DacCalibTime").value = this.GetCalibrationTime().toString();
   
   for (var i=1;i<=POLAND_DAC_NUM;i++) {
      var elem = document.getElementById("DacEdit" + i)
      
      elem.value = pre + this.fDACValue[i-1].toString(base);
   }
}

PolandSetup.prototype.ReadRegisters = function(callback)
{
   var regs = new Array();
   regs.push(POLAND_REG_INTERNAL_TRIGGER, POLAND_REG_MASTERMODE, POLAND_REG_TRIGCOUNT, POLAND_REG_QFW_MODE); 

   for (var i = 0; i < POLAND_TS_NUM; ++i)
   {
     regs.push(POLAND_REG_STEPS_BASE + 4 * i);
     regs.push(POLAND_REG_TIME_BASE + 4 * i);
   }

   for (var e = 0; e < POLAND_ERRCOUNT_NUM; ++e)
   {
     regs.push(POLAND_REG_ERRCOUNT_BASE + 4 * e);
   }

   regs.push(POLAND_REG_DAC_MODE, POLAND_REG_DAC_CAL_TIME, POLAND_REG_DAC_CAL_OFFSET, POLAND_REG_DAC_CAL_STARTVAL);

   for (var d = 0; d < POLAND_DAC_NUM; ++d)
   { 
      regs.push(POLAND_REG_DAC_BASE_READ + 4 * d);
   }

   
   var xmlHttp = new XMLHttpRequest();
   
   var cmdtext = this.CmdUrl + "?sfp=" + this.fSFP + "&dev=" + this.fDEV;
   
   cmdtext+="&cmd=\'[" + regs.toString() + "]\'";
   
   console.log(cmdtext);
   
   xmlHttp.open('GET', cmdtext, true);
   
   var pthis = this;
   
   xmlHttp.onreadystatechange = function () {
      // console.log("onready change " + xmlHttp.readyState); 
      if (xmlHttp.readyState == 4) {
         var reply = JSON.parse(xmlHttp.responseText);
      
         if (!reply || (reply["_Result_"]!=1)) {
            callback(false);
            return;
         }
         
         var res = reply["res"];
         if (res.length != regs.length) {
            console.log("return length mismatch");
            callback(false);
            return;
         }

         var indx = 0;
         
         pthis.fInternalTrigger = res[indx++];
         pthis.fTriggerMode = res[indx++];
         pthis.fEventCounter = res[indx++];
         pthis.fQFWMode = res[indx++];

         for (var i = 0; i < POLAND_TS_NUM; ++i)
         {
            pthis.fSteps[i] = res[indx++];
            pthis.fTimes[i] = res[indx++];
         }
         for (var e = 0; e < POLAND_ERRCOUNT_NUM; ++e)
         {
            pthis.fErrorCounter[e] = res[indx++];
         }

         pthis.fDACMode = res[indx++];
         pthis.fDACCalibTime = res[indx++];
         pthis.fDACOffset = res[indx++];
         pthis.fDACStartValue = res[indx++];

         for (var d = 0; d < POLAND_DAC_NUM; ++d)
         {
            pthis.fDACValue[d] = res[indx++];
         }
         
         callback(true);
      }
   };
   
   xmlHttp.send(null);
}
