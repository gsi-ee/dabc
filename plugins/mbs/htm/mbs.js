let MBS, MyDisplay;

////////////// State class reflecting remote state and communication:
function MbsState() {
   this.fSetupLoaded=false;
   this.fRunning = false;
   this.fFileOpen = false;
   this.fFileName = "run0.lmd";
   this.fFileInterval = 1000;
   this.fLogging = false;
   this.fLogData = null;
}

MbsState.prototype.DabcCommand = function(cmd, option) {
   let pre = "../",
       suf = "/execute",
       fullcom = pre + cmd + suf;
   if (option) fullcom +=  "?" + option;

   return JSROOT.httpRequest(fullcom, "text")
                .then(reply => { console.log('Reply = ' + reply); return true; });
}

MbsState.prototype.DabcParameter = function(par) {
   let pre = "../",
       suf = "/get.json",
       fullcom = pre + par + suf;

   return JSROOT.httpRequest(fullcom, "text")
                .then(reply => {
                   if (!reply) return null;
                   let obj = JSON.parse(reply);
                   return obj ? obj.value : null;
               });
}

MbsState.prototype.GosipCommand = function(cmd, command_callback) {
// this is variation of code found in gosip polandsetup gui:

   let cmdurl="/GOSIP/Test/CmdGosip/execute",
       xmlHttp = new XMLHttpRequest(),
       sfp = 0,
       dev = 0,

       cmdtext = cmdurl + "?sfp=" + sfp + "&dev=" + dev;

   cmdtext += "&log=1";
   cmdtext += "&cmd=\'" + cmd + "\'";
   console.log(cmdtext);
   xmlHttp.open('GET', cmdtext, true);

   xmlHttp.onreadystatechange = function() {
      // console.log("onready change " + xmlHttp.readyState);
      if (xmlHttp.readyState == 4) {
         let reply = JSON.parse(xmlHttp.responseText), ddd = "";

         if (!reply || (reply["_Result_"] != 1)) {
            command_callback(false, null, ddd);
            return;
         }

         if (reply['log'] != null) {

            // console.log("log length = " + Setup.fLogData.length);
            for ( let i in reply['log']) {

               if (reply['log'][i].search("\n") >= 0)
                  console.log("found 1");
               if (reply['log'][i].search("\\n") >= 0)
                  console.log("found 2");
               if (reply['log'][i].search("\\\n") >= 0)
                  console.log("found 3");

               ddd += "<pre>";
               ddd += reply['log'][i].replace(/\\n/g, "<br/>");
               ddd += "</pre>";
            }
            //console.log(ddd);
         }

         command_callback(true, ddd, reply["res"]);
      }
   };

   xmlHttp.send(null);
}

MbsState.prototype.Update = function() {

   // TEST:
   this.DabcParameter("MbsSetupLoaded").then(state => { this.fSetupLoaded = (state==true);})
                                       .catch(() => console.log("UpdateSetupstate failed."));
   this.DabcParameter("MbsAcqRunning").then(state => {this.fRunning = (state==true); })
                                      .catch(() => console.log("UpdateRunstate failed."));
   this.DabcParameter("MbsFileOpen").then(state => { this.fFileOpen = (state==true); })
                                    .catch(() => console.log("UpdateFilestate failed."));
   this.DabcParameter("MbsHistoryDepth").then(val => { MyDisplay.fLoggingHistory = val; })
                                        .catch(() => console.log("UpdateHistoryDepth failed."));

   this.DabcParameter("MbsRateInterval").then(val => { MyDisplay.fRateInterval = val; })
                                        .catch(() => console.log("UpdateRateInterval failed."));

   // DABC states as strings:
   //   "Halted"
   //   "Ready"
   //    "Running"
   //    "Failure";
   //    "Transition";
   return this.DabcParameter("../../web-mbs/App/State").then(state => { this.fDabcState = state; })
                                                       .catch(() => console.log("UpdateDABCstate failed."));
}


/////////////// DISPLAY class to manage current view:
function MbsDisplay(state){
   this.fMbsState = state;
   this.fDoCommandConfirm = true;
   this.fShowGosip = false;
   this.fMonitoring = false;
   this.fTrending = false;
   this.fTrendingHistory = 100;
   this.fFileLogMode = 4;
   this.fLoggingHistory = 100;
   this.fRateInterval = 1;
   this.fMbsLoggingHistory = 500;
   this.fUpdateTimer = null;
   this.fUpdateInterval = 2000; // ms
   this.fMiddlerightPos = 0;
   this.fMiddleWidth = 0;
}

// set up view elements of display:
MbsDisplay.prototype.BuildView = function() {
   let hpainter = new JSROOT.HierarchyPainter('root', null),
       disp = new JSROOT.CustomDisplay();

   disp.addFrame("EvRateDisplay", "EventRate");
   disp.addFrame("DatRateDisplay", "DataRate");
   disp.addFrame("SrvRateDisplay", "ServerRate");
   disp.addFrame("DeviceInfo", "logger");
   // use same frame for different items
   disp.addFrame("ReadoutInfo", "rate_log");
   disp.addFrame("ReadoutInfo", "rash_log");
   disp.addFrame("ReadoutInfo", "rast_log");
   disp.addFrame("ReadoutInfo", "ratf_log");

   hpainter.setDisplay(disp);

   this.hpainter = null;

   hpainter.openOnline("../").then(() => {
      this.hpainter = hpainter;
      this.SetTrending(false, 300);
      hpainter.display("logger");
      this.SetFileLogMode(4, 0, 0);   // init log window, but keep history and interval from server
   });
}


MbsDisplay.prototype.SetRateGauges = function(){
   this.hpainter.display("EventRate", "gauge");
   this.hpainter.display("DataRate", "gauge");
   this.hpainter.display("ServerRate", "gauge");
}


MbsDisplay.prototype.SetRateTrending = function(history) {
   this.fTrendingHistory = history;

   this.hpainter.display("EventRate", "line");
   this.hpainter.display("DataRate", "line");
   this.hpainter.display("ServerRate", "line");
}


MbsDisplay.prototype.SetFileLogMode = function(mode, history, deltat){

   if(history == 0)
      history = document.getElementById("Loglength").value;

   if(deltat == 0)
      deltat = document.getElementById("Loginterval").value;

   this.fLoggingHistory=history;
   this.fRateInterval=deltat;

   if(mode != 0) this.fFileLogMode=mode;

   console.log("SetFileLogMode with mode="+mode+" , history="+history +" , deltat="+deltat);
   switch (this.fFileLogMode) {
   case "1":
      // "rate"
      this.hpainter.display("rate_log");
      break;
   case "2":
      // "rash"
      this.hpainter.display("rash_log");
      break;
   case "3":
      // "rast"
      this.hpainter.display("rast_log");
      break;
   case "4":
   default:
      // "ratf"
      this.hpainter.display("ratf_log");
      break;
   };

   if(mode != 0) return; // only change display mode, do not start new trending history

   MBS.DabcCommand("CmdSetRateInterval", "time="+this.fRateInterval)
      .then(() => MyDisplay.SetStatusMessage("CmdSetRateInterval sent."))
      .catch(() => MyDisplay.SetStatusMessage("CmdSetRateInterval FAILED."));


   MBS.DabcCommand("CmdSetHistoryDepth", "entries="+this.fLoggingHistory)
      .then(() => MyDisplay.SetStatusMessage("CmdSetHistoryDepth sent."))
      .catch(() => MyDisplay.SetStatusMessage("CmdSetHistoryDepth FAILED."));
}


MbsDisplay.prototype.RefreshMonitor = function() {

   if (!this.hpainter) return;

   this.hpainter.updateItems();

   // optionally adjust scrollbars at info logs:
   $("#daq_log").scrollTop($("#daq_log")[0].scrollHeight - $("#daq_log").height());
   $("#file_log").scrollTop($("#file_log")[0].scrollHeight - $("#file_log").height());

   this.fMbsState.Update().then(() => this.RefreshView());
}


MbsDisplay.prototype.ChangeMonitoring = function(on) {

   this.fMonitoring = on;
   if(on) {
       this.SetStatusMessage("Starting monitoring timer with "+ this.fUpdateInterval + " ms.");
       this.fUpdateTimer = window.setInterval(function(){MyDisplay.RefreshMonitor()}, this.fUpdateInterval);
   } else {
       window.clearInterval(this.fUpdateTimer);
       this.SetStatusMessage("Stopped monitoring timer.");
   }
}

MbsDisplay.prototype.SetTrending = function(on,history) {
   this.fTrending = on;
   if(on) {
     console.log("SetTrending on");
     this.SetRateTrending(history); // todo: get interval from textbox
   } else {
     console.log("SetTrending off");
     this.SetRateGauges();
   }
}

MbsDisplay.prototype.SetCommandConfirm = function(on) {
   this.fDoCommandConfirm = on;
}


MbsDisplay.prototype.ShowGosipPanel = function(on){

    if (!on) {
         $('#gosip_container').hide();
         $('#gosip_log').hide();
         // $('#daq_log').height("400px"); // this does not allow to set
         // automatic stretch to bottom
         $('#daq_log').css({
            bottom : "2.7em",
            height : "auto"
         });

      } else {
         $('#gosip_container').show();
         $('#gosip_log').show();
         // $('#daq_log').height("270px"); }
         $('#daq_log').css({
            bottom : "auto",
            height : "198px"
         });
      }
    this.fShoWGosip=on;
}


MbsDisplay.prototype.RefreshView = function() {

   if (this.fMbsState.fRunning) {
      $("#mbs_container").addClass("styleGreen").removeClass("styleRed").removeClass("styleYellow");
   } else {
      if (this.fMbsState.fSetupLoaded) {
         $("#mbs_container").addClass("styleYellow").removeClass("styleRed").removeClass("styleGreen");
      } else {
         $("#mbs_container").addClass("styleRed").removeClass("styleGreen").removeClass("styleYellow");
      }
   }
    //console.log("RefreshView typeof fileopen=%s, value=%s globalvalue=%s", typeof(this.fMbsState.fFileOpen),
   //       this.fMbsState.fFileOpen, Pexor.fFileOpen);

   if (this.fMbsState.fFileOpen) {
      //console.log("RefreshView finds open file");
      $("#file_container").addClass("styleGreen").removeClass("styleRed");
      $("#buttonStartFile").button("option", {icon:  "ui-icon-closethick MyButtonStyle" });
      $("#buttonStartFile").attr("title", "Close output file");
      $("#FileAutoMode").prop('disabled', true);
      $("#FileRFIO").prop('disabled', true); 
      $("#DirectFSQ").prop('disabled', true);
      $("#Filesize").spinner("disable");

   } else {
      //console.log("RefreshView finds close file");
      $("#file_container").addClass("styleRed").removeClass("styleGreen");
      $("#buttonStartFile").button("option", {icon:  "ui-icon-seek-next MyButtonStyle" });
      $("#buttonStartFile").attr("title", "Open lmd file for writing");
      $("#FileAutoMode").prop('disabled', false);
      $("#FileRFIO").prop('disabled', false);
      $("#DirectFSQ").prop('disabled', false);
      $("#Filesize").spinner("enable");
   }

   if (this.fMonitoring) {
      $("#monitoring_container").addClass("styleGreen").removeClass("styleRed");
      $("label[for='Monitoring']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-stop MyButtonStyle\"></span>");
      $("label[for='Monitoring']").attr("title", "Stop frequent refresh");
      $("#Refreshtime").spinner("disable");
      $("#Loglength").spinner("disable");
      $("#Loginterval").spinner("disable");

   } else {
      $("#monitoring_container").addClass("styleRed").removeClass("styleGreen");
      $("label[for='Monitoring']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-play MyButtonStyle\"></span>");
      $("label[for='Monitoring']").attr("title", "Activate frequent refresh");
      $("#Refreshtime").spinner("enable");
      $("#Loglength").spinner("enable");
      $("#Loginterval").spinner("enable");

   }

   if (!this.fTrending) {
      $("label[for='Trending']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-image MyButtonStyle\"></span>");//
      $("label[for='Trending']").attr("title", "Rates are displayed as gauges. Press to switch to trending graphs.");
      $("#Trendlength").spinner("enable");
   } else {
      $("label[for='Trending']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-circle-arrow-n MyButtonStyle\"></span>");
      $("label[for='Trending']").attr("title", "Rates are displayed as trending graphs. Press to switch to gauges.");
      $("#Trendlength").spinner("disable");
   }

   if($("#FileRFIO").is(':checked')){
      $("label[for='FileRFIO']").html("<span class=\"ui-icon ui-icon-link MyButtonStyle\"></span>");
      $("label[for='FileRFIO']").attr("title",  "Write to RFIO or FSQ server is enabled. Must be connected first with command 'connect rfio -DISK or -ARCHIVE' ; or 'connect fsq' if direct fsq is checked.");
   } else {
      $("label[for='FileRFIO']").html("<span class=\"ui-icon ui-icon-disk MyButtonStyle\"></span>");
      $("label[for='FileRFIO']").attr("title", "Write to local disk is enabled.");
   }
   
   
   

   if($("#FileAutoMode").is(':checked')){
      $("label[for='FileAutoMode']").html("<span class=\"ui-icon ui-icon-star MyButtonStyle\"></span>");
      $("label[for='FileAutoMode']").attr("title",  "Automatic file numbering is enabled. Names of the form namexxx.lmd are created with consecutive numbers xxx.");
   } else {
      $("label[for='FileAutoMode']").html("<span class=\"ui-icon ui-icon-document MyButtonStyle\"></span>");
      $("label[for='FileAutoMode']").attr("title",  "Use exact file name is enabled. Will return error if file already exists.");
   }
   
  // DirectFSQ
    if($("#DirectFSQ").is(':checked')){
      $("label[for='DirectFSQ']").html("<span class=\"ui-icon ui-icon-plus MyButtonStyle\"></span>");
      $("label[for='DirectFSQ']").attr("title",  "Direct FSQ transport is enabled. Must be connected first with command connect fsq");
   } else {
      $("label[for='DirectFSQ']").html("<span class=\"ui-icon ui-icon-minus MyButtonStyle\"></span>");
      $("label[for='DirectFSQ']").attr("title", "Direct FSQ transport is disabled. Remote storage may use RFIO or disk server after connect rfio -DISK or -ARCHIVE");
   }
   

   if (this.fDoCommandConfirm) {
      $("label[for='ConfirmCommandToggle']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-comment MyButtonStyle\"></span>");//
      $("label[for='ConfirmCommandToggle']").attr("title", "Command Confirmation Mode is ON");
   } else {
      $("label[for='ConfirmCommandToggle']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-alert MyButtonStyle\"></span>");
      $("label[for='ConfirmCommandToggle']").attr("title", "Command Confirmation Mode is OFF");
   }

   $('#Loglength').val(MyDisplay.fLoggingHistory);
   $('#Loginterval').val(Math.round(MyDisplay.fRateInterval));

   // console.log("RefreshView with dabc state = %s",
   // this.fMbsState.fDabcState);

//    if (this.fMbsState.fDabcState=="Running") {
//      //   console.log("RefreshView finds Running state");
//         $("#dabc_container").addClass("styleGreen").removeClass("styleRed").removeClass("styleYellow").removeClass("styleBlue");
//
//      }
//    else if (this.fMbsState.fDabcState=="Ready") {
//       //   console.log("RefreshView finds Ready state");
//         $("#dabc_container").addClass("styleYellow").removeClass("styleRed").removeClass("styleGreen").removeClass("styleBlue");
//      }
//    else if ((this.fMbsState.fDabcState=="Failure") || (this.fMbsState.fDabcState=="Transition")) {
//       //console.log("RefreshView finds Failure state");
//       $("#dabc_container").addClass("styleBlue").removeClass("styleYellow").removeClass("styleRed").removeClass("styleGreen");
//      }
//    else {
//       //console.log("RefreshView finds other state");
//         $("#dabc_container").addClass("styleRed").removeClass("styleGreen").removeClass("styleYellow").removeClass("styleBlue");
//      }
}


MbsDisplay.prototype.SetStatusMessage= function(info) {
   let d = new Date(),
       txt = d.toLocaleString() + "  >" + info;
   document.getElementById("status_message").innerHTML = txt;
}

MbsDisplay.prototype.Confirm = function(msg) {
   if(this.fDoCommandConfirm)
      return (confirm(msg));
   else
      return true;
}

MBS = new MbsState();
MyDisplay = new MbsDisplay(MBS);
MyDisplay.BuildView();
MyDisplay.ChangeMonitoring(true);

///////////////////////////// mbs specific:

$("#buttonStartAcquisition").button({showLabel: false, icon:  "ui-icon-play MyButtonStyle"}).click(
      function() {
         let requestmsg = "Really Start Acquisition?",
             response = MyDisplay.Confirm(requestmsg);
         if (!response)
            return;

         MBS.DabcCommand("CmdMbs", "cmd=sta acq")
            .then(() => MyDisplay.SetStatusMessage("Start Acquisition command sent."))
            .catch(() => MyDisplay.SetStatusMessage("Start Acquisition FAILED."))
            .finally(() => MyDisplay.RefreshMonitor());
      });

$("#buttonStopAcquisition").button({showLabel: false, icon:  "ui-icon-stop MyButtonStyle"}).click(
      function() {

         let requestmsg = "Really Stop Acquisition?",
             response = MyDisplay.Confirm(requestmsg);
         if (!response)
            return;
         MBS.DabcCommand("CmdMbs", "cmd=sto acq")
            .then(() => MyDisplay.SetStatusMessage("Stop Acquisition command sent."))
            .catch(() => MyDisplay.SetStatusMessage("Stop Acquisition FAILED."))
            .finally(() => MyDisplay.RefreshMonitor());
      });

////////// startup command will not work at the moment:
$("#buttonStartupAcquisition").button({showLabel: false, icon:  "ui-icon-arrowthick-1-n MyButtonStyle"}).click(function() {
   let requestmsg = "Really Initialize Acquisition?",
       response = MyDisplay.Confirm(requestmsg);
   if (!response)
      return;

   MBS.DabcCommand("CmdMbs","cmd=\@startup")
      .then(() => MyDisplay.SetStatusMessage("Init Acquisition (@startup) command sent."))
      .catch(() => MyDisplay.SetStatusMessage("Init Acquisition FAILED."))
      .finally(() => MyDisplay.RefreshMonitor());
});

$("#buttonShutdownAcquisition").button({showLabel: false, icon:  "ui-icon-arrowthick-1-s MyButtonStyle"}).click(function() {
   let requestmsg = "Really Shut down Acquisition?",
       response = MyDisplay.Confirm(requestmsg);
   if (!response)
      return;

   MBS.DabcCommand("CmdMbs","cmd=\@shutdown")
      .then(() => MyDisplay.SetStatusMessage("Shutdwon Acquisition (@shutdown) command sent."))
      .catch(() => MyDisplay.SetStatusMessage("Shutdwon Acquisition FAILED."))
      .finally(() => MyDisplay.RefreshMonitor());
});



$("#buttonStartFile").button({showLabel: false, icon:  "ui-icon-seek-next MyButtonStyle"});

$("#FileAutoMode").checkboxradio({icon: false})
                  .click(function() { MyDisplay.RefreshView(); });

$("#FileRFIO").checkboxradio({icon: false})
              .click(function() { MyDisplay.RefreshView();} );
              
$("#DirectFSQ").checkboxradio({icon: false})
              .click(function() { MyDisplay.RefreshView();} );              
              

$("#lmd_file_form").submit(
   function(event) {

      let checked = MBS.fFileOpen;

      if (checked) {
         let requestmsg = "Really Stop writing output file " + MBS.fFileName + " ?",
             response = MyDisplay.Confirm(requestmsg);
         if (!response) {
            event.preventDefault();
            return;
         }
         MBS.DabcCommand("CmdMbs", "cmd=clo fi")
            .then(() => MyDisplay.SetStatusMessage("Stop File command sent."))
            .catch(() => MyDisplay.SetStatusMessage("Stop File FAILED."))
            .finally(() => MyDisplay.RefreshMonitor());
      }
      else {
         let datafilename = document.getElementById("Filename").value,
             datafilelimit = document.getElementById("Filesize").value,
             options = "cmd= open file " + datafilename + " size=" + datafilelimit;
         if ($("#FileRFIO").is(':checked'))
            {
                 if ($("#DirectFSQ").is(':checked'))
                    options += " -FSQ";
                 else
                    options += " -RFIO";  
            }
            else
            {
                    options += " -DISK";
            }
         if ($("#FileAutoMode").is(':checked'))
            options += " -AUTO";
         let requestmsg = "Really Start writing output file with "  + options + " ?",
             response = MyDisplay.Confirm(requestmsg);
         if (!response) {
            event.preventDefault();
            return;
         }

        

         MBS.DabcCommand("CmdMbs", options)
            .then(() => {
                MyDisplay.SetStatusMessage("Start File command sent: " + options);
                MBS.fFileName = datafilename;
             }).catch(() => MyDisplay.SetStatusMessage("Start File FAILED."))
            .finally(() => MyDisplay.RefreshMonitor());

      }
      event.preventDefault();
   }
);


$("#Monitoring").checkboxradio({icon: false})
   .click(function() {
   MyDisplay.fUpdateInterval=1000*parseInt(document.getElementById("Refreshtime").value);
   MyDisplay.ChangeMonitoring($(this).is(':checked'));
   MyDisplay.RefreshView();
});


$("#buttonRefresh").button({showLabel: false, icon:  "ui-icon-refresh MyButtonStyle"}).click(
      function() {
            MyDisplay.RefreshMonitor();
         });


$("#Trending").checkboxradio({icon: false})
.prop('checked', MyDisplay.fTrending)
.click(function() {
   MyDisplay.SetTrending($(this).is(':checked'), parseInt(document.getElementById("Trendlength").value));
   MyDisplay.RefreshView();
});

$("#buttonExecuteGosip").button({showLabel: false, icon:  "ui-icon-gear MyButtonStyle"});

   $( "#gosipcmd_form" ).submit(
         function(event) {
            let cmdpar = document.getElementById("commandGosip").value,
                requestmsg = "Really Execute  gosipcmd "+ cmdpar,
                response = MyDisplay.Confirm(requestmsg);
            if (!response){
               event.preventDefault();
               return;
            }
            MBS.GosipCommand(cmdpar, function(ok, logout, result) {
               MyDisplay.SetStatusMessage(ok ? "gosip command send."
                     : "gosip command FAILED.");
               if(ok) document.getElementById("GosipLog").innerHTML += logout;
               // NOTE: GosipLog is inner text div, but scrollbars are put to container gosip_log!
               $("#gosip_log").scrollTop($("#gosip_log")[0].scrollHeight - $("#gosip_log").height());
                 //console.log("scrollheight="+ $("#gosip_log")[0].scrollHeight + ", height=" + $("#gosip_log").height() + ", scrolltop=" + $("#gosip_log").scrollTop());
               MyDisplay.RefreshMonitor();
            });

            event.preventDefault();
         });


   $("#buttonExecuteMBS").button({showLabel: false, icon:  "ui-icon-gear MyButtonStyle"});


   $( "#mbscmd_form" ).submit(
         function(event) {
            let cmdpar = "cmd="+document.getElementById("commandMBS").value,
                requestmsg = "Really Execute  mbs command: "+ cmdpar,
                response = MyDisplay.Confirm(requestmsg);
            if (!response){
               event.preventDefault();
               return;
            }

            MBS.DabcCommand("CmdMbs", cmdpar)
               .then(() => MyDisplay.SetStatusMessage("Send MBS command: "+cmdpar))
               .catch(() => MyDisplay.SetStatusMessage("MBS command  FAILED."))
               .finally(() => MyDisplay.RefreshMonitor());

            event.preventDefault();
         });



   $("#ShowGosipToggle").checkboxradio({icon: false})
   .prop('checked', MyDisplay.fShowGosip)
   .click(
         function() {
            MyDisplay.ShowGosipPanel($('#ShowGosipToggle').is(':checked'));
         });



$("#buttonClearGosipLog").button({showLabel: false, icon:  "ui-icon-trash MyButtonStyle"}).click(
      function() {
            MyDisplay.SetStatusMessage("Cleared gosip logoutput.");
            document.getElementById("GosipLog").innerHTML="";


      });


// Use new jquery ui styled icon
$("#buttonUserGUI").button({showLabel: false, icon:  "ui-icon-poland MyButtonStyle"}).click(
      function() {
            MyDisplay.SetStatusMessage("Launched gosip user gui.");
            window.open('/GOSIP/Test/UI/','_blank');
         });

    $("#logmodes").buttonset();

    $("input[name='filelogmode']").on("change", function () {
       MyDisplay.SetFileLogMode(this.value, 0 ,0);
    });

    $("#ConfirmCommandToggle").checkboxradio({icon: false}).click(
         function() {
            let doconfirm = $('#ConfirmCommandToggle').is(':checked');
            MyDisplay.SetCommandConfirm(doconfirm);
            MyDisplay.RefreshView();
            console.log("Command confirm is " + doconfirm);

         });


 $('#Refreshtime').spinner({
     min: 1,
     max: 120,
     step: 1
 }).val(MyDisplay.fUpdateInterval/1000);

 $('#Trendlength').spinner({
     min: 10,
     max: 3600,
     step: 10
 }).val(MyDisplay.fTrendingHistory);

 $('#Filesize').spinner({
     min: 100,
     max: 2000,
     step: 100
 });

 $('#Loglength').spinner({
     min: 10,
     max: 100000,
     step: 10,
//        spin: function( event, ui ) {console.log("Loglength spin"); MyDisplay.fLoggingHistory=ui.value;},
//       stop: function( event, ui ) {console.log("Loglength stop");},
     change: function( event, ui ) {MyDisplay.SetFileLogMode(0,0,0);}
 }).val(MyDisplay.fLoggingHistory);

 $('#Loginterval').spinner({
     min: 1,
     max: 100000,
     step: 1,
//        spin: function( event, ui ) {console.log("Loginterval spin"); MyDisplay.fRateInterval=ui.value;},
//       stop: function( event, ui )  {console.log("Loginterval stop");},
    change: function( event, ui ){MyDisplay.SetFileLogMode(0,0,0);}

 }).val(MyDisplay.fRateInterval);

 $('#MbsLoglength').spinner({
     min: 10,
     max: 100000,
     step: 10,
     spin: function( event, ui ) {  MyDisplay.fMbsLoggingHistory = ui.value;},
     stop: function( event, ui ) {  MyDisplay.hpainter.display("logger"); }
 }).val(MyDisplay.fMbsLoggingHistory);



 //$('#gosip_container').resizable().draggable().selectable();


 $("#mbs_container").resizable({
     handles: 'e',
     //helper: "ui-resizable-helper",
     minWidth: 400,
     start: function( event, ui ) {
        MyDisplay.fMiddleWidth=$("#file_log").width();
        let middleleft = $("#file_log").position().left;
        MyDisplay.fMiddlerightPos=middleleft+MyDisplay.fMiddleWidth; // remember position of right edge middle column
        console.log("left="+middleleft+", right="+MyDisplay.fMiddlerightPos+", width="+MyDisplay.fMiddleWidth);
     },

     resize: function( event, ui ) {
        $("#file_log").css({
           left: (ui.size.width +2),
           right: MyDisplay.fMiddlerightPos,
           width:  (MyDisplay.fMiddlerightPos -  (ui.size.width -2))
         });
        $("#file_container").css({
           left: (ui.size.width +2),
           right:  MyDisplay.fMiddlerightPos,
           width: (MyDisplay.fMiddlerightPos -  (ui.size.width -2))
         });
        $("#filelog_mode_container").css({
           left: (ui.size.width +2),
           right:  MyDisplay.fMiddlerightPos,
           width: (MyDisplay.fMiddlerightPos -  (ui.size.width -2))
         });
        $("#daq_log").css({
           width: (ui.size.width)
         });
        $("#gosip_container").css({
           width: (ui.size.width)
         });
        $("#gosip_log").css({
           width: (ui.size.width)
         });

        // need to set absolute pixels for right column, too, otherwise we will have gap after next window resize...
        $("#monitoring_container").css({
           left: MyDisplay.fMiddlerightPos +4
         });
        $("#ratemode_container").css({
           left: MyDisplay.fMiddlerightPos +4
         });
        $("#rate_container").css({
           left: MyDisplay.fMiddlerightPos +4
         });
        }
    });


 $("#file_container").resizable({
     handles: 'e',
     //helper: "ui-resizable-helper",
     minWidth: 400,
     start: function( event, ui ) {
        MyDisplay.fMiddleWidth = ui.size.width;
        MyDisplay.fMiddlerightPos = $("#monitoring_container").position().left; // remember position of right edge middle column
        console.log("start resize file container - right="+MyDisplay.fMiddlerightPos);

     },

     resize: function( event, ui ) {

        $("#filelog_mode_container").css({
           width: (ui.size.width)
         });
        $("#file_log").css({
           width: (ui.size.width)
         });

        $("#monitoring_container").css({
           left: (MyDisplay.fMiddlerightPos - (MyDisplay.fMiddleWidth - ui.size.width))
         });
        $("#ratemode_container").css({
           left: (MyDisplay.fMiddlerightPos - (MyDisplay.fMiddleWidth - ui.size.width))
         });
        $("#rate_container").css({
           left: (MyDisplay.fMiddlerightPos - (MyDisplay.fMiddleWidth - ui.size.width))
         });


        // need to set absolute pixels for left column, too, otherwise we will have gap after next window resize...
        $("#mbs_container").css({
           right: MyDisplay.fMiddlerightPos-2-MyDisplay.fMiddleWidth
         });
        $("#daq_log").css({
           right: MyDisplay.fMiddlerightPos - 2  -MyDisplay.fMiddleWidth
         });
        $("#gosip_container").css({
           right: MyDisplay.fMiddlerightPos - 2  -MyDisplay.fMiddleWidth
         });
        $("#gosip_log").css({
           right: MyDisplay.fMiddlerightPos - 2  -MyDisplay.fMiddleWidth
         });


        }
    });

MyDisplay.ShowGosipPanel(false);
//MyDisplay.RefreshView();

MyDisplay.RefreshMonitor();

$(document).tooltip();

