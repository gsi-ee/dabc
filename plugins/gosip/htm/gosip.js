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
   this.fLogging = false;
   this.fLogData = null;
   
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

PolandSetup.prototype.SetTriggerMaster  = function(on)
{
   this.fTriggerMode = on ? (this.fTriggerMode | 2) : (this.fTriggerMode & ~2);
}

PolandSetup.prototype.IsTriggerMaster = function()
{
  return ((this.fTriggerMode & 2) === 2);
}

PolandSetup.prototype.SetFesaMode = function(on)
{
  this.fTriggerMode = on ? (this.fTriggerMode | 1) : (this.fTriggerMode & ~1);
}

PolandSetup.prototype.IsFesaMode = function()
{
  return (this.fTriggerMode & 1) === 1;
}

PolandSetup.prototype.SetInternalTrigger = function (on)
{
   this.fInternalTrigger = on ?  (this.fInternalTrigger | 1) : (this.fInternalTrigger & ~1);
}

PolandSetup.prototype.IsInternalTrigger = function()
{
  return (this.fInternalTrigger & 1) === 1;
}

PolandSetup.prototype.GetCalibrationTime = function()
{
   return (this.fDACCalibTime*POLAND_TIME_UNIT)/1000.;
}

PolandSetup.prototype.SetCalibrationTime = function (ms)
{
   this.fDACCalibTime = 1000 * ms / POLAND_TIME_UNIT;
}

PolandSetup.prototype.GetStepTime = function(loop)
{
   return this.fTimes[loop]*POLAND_TIME_UNIT;
}

PolandSetup.prototype.SetStepTime = function(loop, us)
{
   this.fTimes[loop] = us / POLAND_TIME_UNIT;
}


PolandSetup.prototype.RefreshQFW = function(base)
{
   var options = document.getElementById("QFWModeCombo").options;
   for (var i = 0; i < options.length; i++) 
      options[i].selected = (options[i].value == this.fQFWMode);
   $("#QFWModeCombo").selectmenu('refresh', true);
   
   if (!base) base = 16;
   var pre = base==16 ? "0x" : "";

   document.getElementById("TS1Loop").value = pre + this.fSteps[0].toString(base);
   document.getElementById("TS2Loop").value = pre + this.fSteps[1].toString(base);
   document.getElementById("TS3Loop").value = pre + this.fSteps[2].toString(base);
   document.getElementById("TS1Time").value = this.GetStepTime(0).toString();
   document.getElementById("TS2Time").value = this.GetStepTime(1).toString();
   document.getElementById("TS3Time").value = this.GetStepTime(2).toString();
   
   document.getElementById("MasterTrigger").checked = this.IsTriggerMaster();
   document.getElementById("FESAMode").checked = this.IsFesaMode();
   document.getElementById("InternalTrigger").checked = this.IsInternalTrigger();
}

PolandSetup.prototype.EvaluateQFW = function() 
{
   var options = document.getElementById("QFWModeCombo").options;
   for (var i = 0; i < options.length; i++) 
      if (options[i].selected) this.fQFWMode = Number(options[i].value);

   this.fSteps[0] = parseInt(document.getElementById("TS1Loop").value);
   this.fSteps[1] = parseInt(document.getElementById("TS2Loop").value);
   this.fSteps[2] = parseInt(document.getElementById("TS3Loop").value);
   this.SetStepTime(0, parseFloat(document.getElementById("TS1Time").value));
   this.SetStepTime(1, parseFloat(document.getElementById("TS2Time").value));
   this.SetStepTime(2, parseFloat(document.getElementById("TS3Time").value));
   
   this.SetTriggerMaster(document.getElementById("MasterTrigger").checked);
   this.SetFesaMode(document.getElementById("FESAMode").checked);
   this.SetInternalTrigger(document.getElementById("InternalTrigger").checked);
}

PolandSetup.prototype.RefreshDAC = function(base)
{
   var options = document.getElementById("DacModeCombo").options;
   for (var i = 0; i < options.length; i++) 
      options[i].selected = (options[i].value == this.fDACMode);
   $("#DacModeCombo").selectmenu('refresh', true);

   if (!base) base = 16;
   var pre = base==16 ? "0x" : "";

   document.getElementById("DacOffset").value = pre + this.fDACOffset.toString(base);
   document.getElementById("DacDeltaOffset").value = pre + this.fDACDelta.toString(base);
   document.getElementById("DacCalibTime").value = this.GetCalibrationTime().toString();
   
   for (var i=0;i<POLAND_DAC_NUM;i++) {
      var elem = document.getElementById("DacEdit" + (i+1));
      
      elem.value = pre + this.fDACValue[i].toString(base);
   }
}

PolandSetup.prototype.EvaluateDAC = function()
{
   var options = document.getElementById("DacModeCombo").options;
   for (var i = 0; i < options.length; i++) 
      if (options[i].selected) this.fDACMode = Number(options[i].value);
   
   if(this.fDACMode==4)
     this.fDACAllValue = parseInt(document.getElementById("DacStart").value);
  else
     this.fDACStartValue = parseInt(document.getElementById("DacStart").value);

  this.fDACOffset = parseInt(document.getElementById("DacOffset").value);
  this.fDACDelta = parseInt(document.getElementById("DacDeltaOffset").value);
  this.SetCalibrationTime(parseFloat(document.getElementById("DacCalibTime").value));

  if(this.fDACMode == 1)
     for (var i=0;i<POLAND_DAC_NUM;i++) {
        var elem = document.getElementById("DacEdit" + (i+1));
        this.fDACValue[i] = parseInt(elem.value);
     }
}

PolandSetup.prototype.RefreshCounters = function(base)
{
   if (!base) base = 16;
   var pre = base==16 ? "0x" : "";
   
   document.getElementById("TriggerLbl").innerHTML = "<h2>"+pre+this.fEventCounter.toString(base)+"</h2>";

   var txt = "";
   
   for (var i=0;i<this.fErrorCounter.length;i++)
      txt+="<h4>"+pre + this.fErrorCounter[i].toString(base)+"</h4>";

   document.getElementById("ErrorsLbl").innerHTML = txt;
}


PolandSetup.prototype.GosipCommand = function(cmd, command_callback)
{
   var xmlHttp = new XMLHttpRequest();
   
   var cmdtext = this.CmdUrl + "?sfp=" + this.fSFP + "&dev=" + this.fDEV;
   
   if (this.fLogging) cmdtext+="&log=1";
   
   cmdtext+="&cmd=\'" + cmd + "\'";
   
   console.log(cmdtext);
   
   xmlHttp.open('GET', cmdtext, true);
   
   var pthis = this;
   
   xmlHttp.onreadystatechange = function () {
      // console.log("onready change " + xmlHttp.readyState); 
      if (xmlHttp.readyState == 4) {
         var reply = JSON.parse(xmlHttp.responseText);
      
         if (!reply || (reply["_Result_"]!=1)) {
            command_callback(false, null);
            return;
         }
         
         if (reply['log']!=null) {
            var ddd = "";
            // console.log("log length = " + Setup.fLogData.length); 
            for (var i in reply['log']) {
               
               if (reply['log'][i].search("\n")>=0) console.log("found 1");
               if (reply['log'][i].search("\\n")>=0) console.log("found 2");
               if (reply['log'][i].search("\\\n")>=0) console.log("found 3");
               
               ddd += "<pre>";
               ddd += reply['log'][i].replace(/\\n/g,"<br/>");
               ddd += "</pre>";
            }

            document.getElementById("logging").innerHTML += ddd;
         }
         
         command_callback(true, reply["res"]);
      }
   };
   
   xmlHttp.send(null);
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
   
   var pthis = this;
   
   this.GosipCommand("[" + regs.toString() + "]", function(isok, res) {

      if (isok && (res.length != regs.length)) {
         console.log("return length mismatch");
         isok = false;
      }
      
      if (isok) {
         var indx = 0;
         
         pthis.fInternalTrigger = Number(res[indx++]);
         pthis.fTriggerMode = Number(res[indx++]);
         pthis.fEventCounter = Number(res[indx++]);
         pthis.fQFWMode = Number(res[indx++]);

         for (var i = 0; i < POLAND_TS_NUM; ++i)
         {
            pthis.fSteps[i] = Number(res[indx++]);
            pthis.fTimes[i] = Number(res[indx++]);
         }
         for (var e = 0; e < POLAND_ERRCOUNT_NUM; ++e)
         {
            pthis.fErrorCounter[e] = Number(res[indx++]);
         }

         pthis.fDACMode = Number(res[indx++]);
         pthis.fDACCalibTime = Number(res[indx++]);
         pthis.fDACOffset = Number(res[indx++]);
         pthis.fDACStartValue = Number(res[indx++]);

         for (var d = 0; d < POLAND_DAC_NUM; ++d)
         {
            pthis.fDACValue[d] = Number(res[indx++]);
         }
      }
      callback(isok);
   });
}



PolandSetup.prototype.SetRegisters = function(kind, callback)
{
   // one could set "QFW", "DAC" or all registers 
   
   
   // write register values from strucure with gosipcmd
   
   var regs = new Array();

   if (kind=="QFW") {
      // reading of registers on QFW page
   
      if ((this.fSFP >= 0) && (this.fDEV >= 0)) {
         // update trigger modes only in single device
         regs.push([POLAND_REG_INTERNAL_TRIGGER, this.fInternalTrigger]);
         regs.push([POLAND_REG_MASTERMODE, this.fTriggerMode]);
      }

      regs.push([POLAND_REG_QFW_MODE, this.fQFWMode]);
   
      for (var i = 0; i < POLAND_TS_NUM; ++i)
      {
         regs.push([POLAND_REG_STEPS_BASE + 4 * i, this.fSteps[i]]);
         regs.push([POLAND_REG_TIME_BASE + 4 * i, this.fTimes[i]]);
      }
   }
   
   if (kind == "DAC") {
      // reading of registers on DAC page
      
      regs.push([POLAND_REG_DAC_MODE, this.fDACMode]);

      switch(this.fDACMode)
      {
        case 1:
          // manual settings:
          for (var d = 0; d < POLAND_DAC_NUM; ++d)
          {
             regs.push( [POLAND_REG_DAC_BASE_WRITE + 4 * d, this.fDACValue[d]]);
          }
          break;
        case 2:
          // test structure:
          // no more actions needed
          break;
        case 3:
          // issue calibration:
          regs.push( [POLAND_REG_DAC_CAL_STARTVAL , this.fDACStartValue]);
          regs.push( [POLAND_REG_DAC_CAL_OFFSET ,  this.fDACOffset]);
          regs.push( [POLAND_REG_DAC_CAL_DELTA ,  this.fDACDelta]);
          regs.push( [POLAND_REG_DAC_CAL_TIME ,  this.fDACCalibTime]);

          break;
        case 4:
          // all same constant value mode:
          regs.push( [POLAND_REG_DAC_ALLVAL , this.fDACAllValue]);
          break;

        default:
          console.log("!!! ApplyDAC Never come here - undefined DAC mode " + this.fDACMode);
          break;

      };

      regs.push([POLAND_REG_DAC_PROGRAM , 1]);
      regs.push([POLAND_REG_DAC_PROGRAM , 0]);
   }
   
   
   var cmdtext = "[";
   
   for (var i=0;i<regs.length;i++) {
      if (i>0) cmdtext += ",";
      cmdtext += "\"-w " + regs[i][0] + " " + regs[i][1] + "\"";  
   }
   
   cmdtext += "]";
   
   this.GosipCommand(cmdtext, function(isok, res) {
      if (isok && (res.length != regs.length)) {
         console.log("return length mismatch");
         isok = false;
      }
      
      callback(isok);
   });
}


PolandSetup.prototype.DumpData = function(callback) 
{
   
}
