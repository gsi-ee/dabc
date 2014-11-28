
var MBS;
var MyDisplay;



////////////// State class reflecting remote state and communication:
function MbsState() {
	this.fRunning = false;
	this.fFileOpen = false;
	this.fFileName = "run0.lmd";
	this.fFileInterval = 1000;
	this.fLogging = false;
	this.fLogData = null;

}

MbsState.prototype.DabcCommand = function(cmd, option, callback) {
	var xmlHttp = new XMLHttpRequest();
	var pre = "../";
	var suf = "/execute";
	var fullcom = pre + cmd + suf + "?" + option;
	console.log(fullcom);
	xmlHttp.open('GET', fullcom, true);
	xmlHttp.onreadystatechange = function() {

		if (xmlHttp.readyState == 4) {
			console.log("DabcCommand completed.");
			var reply = JSON.parse(xmlHttp.responseText);
			console.log("Reply= %s", reply);
			callback(true); // todo: evaluate return values of reply
		}
	}
	xmlHttp.send(null);
};


MbsState.prototype.DabcParameter = function(par, callback) {
	var xmlHttp = new XMLHttpRequest();
	var pre = "../";
	var suf = "/get.json";
	var fullcom = pre + par + suf; 
	console.log(fullcom);
	xmlHttp.open('GET', fullcom, true); 
	xmlHttp.onreadystatechange = function() {

		if (xmlHttp.readyState == 4) {
			//console.log("DabcParameter request completed.");
			var reply = JSON.parse(xmlHttp.responseText);
			 if (typeof reply != 'object') {
		         console.log("non-object in json response from server");
		         return;
		      }
			 
//			 var val= false;
//			 val=(reply['value']==="true") ? true : false;
			 //console.log("response=%s, Reply= %s, value=%s", xmlHttp.responseText, reply, val);
			 //console.log("name=%s, value= %s state=%s", reply['_name'], reply['value'], val);
			callback("true",reply['value']); 
		}
	}
	xmlHttp.send(null);
}; 




MbsState.prototype.GosipCommand = function(cmd, command_callback) {
// this is variation of code found in gosip polandsetup gui:
	
	var cmdurl="/GOSIP/Test/CmdGosip/execute";
	var xmlHttp = new XMLHttpRequest();
	var sfp=0; 
	var dev=0;
	
	var cmdtext = cmdurl + "?sfp=" + sfp + "&dev=" + dev;
//
//	if (log)
    cmdtext += "&log=1";
//
	cmdtext += "&cmd=\'" + cmd + "\'";
//
	console.log(cmdtext);
//
	xmlHttp.open('GET', cmdtext, true);
//
	var pthis = this;

	xmlHttp.onreadystatechange = function() {
		// console.log("onready change " + xmlHttp.readyState); 
		if (xmlHttp.readyState == 4) {
			var reply = JSON.parse(xmlHttp.responseText);
			var ddd = "";
			if (!reply || (reply["_Result_"] != 1)) {
				command_callback(false, null,ddd);
				return;
			}

			if (reply['log'] != null) {
				
				// console.log("log length = " + Setup.fLogData.length); 
				for ( var i in reply['log']) {

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



MbsState.prototype.UpdateRunstate = function(ok, state){
	//console.log("UpdateRunstate with ok=%s, value=%s", ok, state);
	if (ok=="true") {
		this.fRunning = (state==true);
	} else {
		console.log("UpdateRunstate failed.");
	}
}

MbsState.prototype.UpdateFilestate = function(ok, state){
	
	if (ok=="true") {
		this.fFileOpen = (state==true);
		} else {
		console.log("UpdateFilestate failed.");
	}
	//console.log("UpdateFilestate with ok=%s, value=%s, fileopen=%s, typeofFileopen=%s", ok, state, this.fFileOpen, typeof(this.fFileOpen));
}

MbsState.prototype.UpdateDABCstate = function(ok, state, refreshcall){
// DABC states as strings:	
//	"Halted"
//	"Ready"
//    "Running"
//    "Failure";
//    "Transition";	
	
	
	if (ok=="true") {
		this.fDabcState = state;
		
	} else {
		console.log("UpdateDABCstate failed.");
	}
    //console.log("UpdateDABCstate with ok=%s, value=%s, dabcstate=%s", ok, state, this.fDabcState);
    refreshcall();
}



MbsState.prototype.Update= function(callback){
	var pthis = this;
	// TEST:
	this.DabcParameter("MbsAcqRunning", function(res,val) { pthis.UpdateRunstate(res,val); });
	this.DabcParameter("MbsFileOpen",function(res,val) { pthis.UpdateFilestate(res,val); })
//	
	this.DabcParameter("../../web-mbs/App/State",function(res,val) { pthis.UpdateDABCstate(res,val, callback); })
	
	//this.fDabcState="Running";
	//callback(); // will be done when last parameter update response has been processed
}


/////////////// DISPLAY class to manage current view:
function MbsDisplay(state){
	this.fMbsState=state;
	this.fMonitoring=false;
	this.fTrending=false;
	this.fTrendingHistory=100;
	this.fFileLogMode=4;
	this.fLoggingHistory=100;
	this.fMbsLoggingHistory=100;
	this.fUpdateTimer=null;
	this.fUpdateInterval=2000; // ms
	this.fGaugeEv = null;
	this.fGaugeDa = null;
	this.fGaugeSv = null;
	this.fTrendEv = null;
	this.fTrendDa = null;
	this.fTrendSv = null;
	this.fLogDevice=null;
	this.fLogReadout=null;
}

// set up view elements of display:
MbsDisplay.prototype.BuildView = function(){
	
	
	
	this.fGaugeEv = new DABC.GaugeDrawElement();
	this.fTrendEv=new DABC.RateHistoryDrawElement();
	
	this.fGaugeDa = new DABC.GaugeDrawElement();
	this.fTrendDa=new DABC.RateHistoryDrawElement();
	
	this.fGaugeSv = new DABC.GaugeDrawElement();
	this.fTrendSv=new DABC.RateHistoryDrawElement();
	
	this.fLogDevice= new DABC.LogDrawElement();
	this.SetMbsLog();
	
	this.fLogReadout= new DABC.LogDrawElement();
	this.SetFileLogMode(4, 100);	
	this.SetRateGauges();
	
}


MbsDisplay.prototype.SetRateGauges = function(){
	
	this.fTrendEv.Clear();
	this.fTrendDa.Clear();
	this.fTrendSv.Clear();
	this.fGaugeEv.itemname = "../EventRate";
	this.fGaugeEv.CreateFrames($("#EvRateDisplay"));
	this.fGaugeDa.itemname = "../DataRate";
	this.fGaugeDa.CreateFrames($("#DatRateDisplay"));
	this.fGaugeSv.itemname = "../ServerRate";
	this.fGaugeSv.CreateFrames($("#SrvRateDisplay"));
	
	
}

MbsDisplay.prototype.SetRateTrending = function(history){
	
	this.fTrendingHistory=history;
	this.fGaugeEv.Clear();
	this.fGaugeDa.Clear();
	this.fGaugeSv.Clear();
	this.fTrendEv.itemname="../EventRate";
	this.fTrendEv.EnableHistory(this.fTrendingHistory); 
	this.fTrendEv.CreateFrames($("#EvRateDisplay"));
	this.fTrendDa.itemname="../DataRate";
	this.fTrendDa.EnableHistory(this.fTrendingHistory);
	this.fTrendDa.CreateFrames($("#DatRateDisplay"));
	this.fTrendSv.itemname="../ServerRate";
	this.fTrendSv.EnableHistory(this.fTrendingHistory);
	this.fTrendSv.CreateFrames($("#SrvRateDisplay"));	
	
}


MbsDisplay.prototype.ClearDaqLog = function(){
	this.fLogDevice.Clear();
	this.fLogDevice.itemname = "../logger";
	this.fLogDevice.EnableHistory(0);
	this.fLogDevice.CreateFrames($("#DeviceInfo"));
	
}

MbsDisplay.prototype.SetFileLogMode= function(mode, history){
	
	if(history==0) history=this.fLoggingHistory;
	if(mode==0) mode= this.fFileLogMode;
	this.fLogReadout.Clear();
	//console.log("SetFileLogMode with mode="+mode+" , history="+history);
	switch (mode) {	
	case "1":
		// "rate" 
		this.fLogReadout.itemname = "../rate_log";	
		break;		
	case "2":
		// "rash" 
		this.fLogReadout.itemname = "../rash_log";	
		break;	
	case "3":
		// "rast" 
		this.fLogReadout.itemname = "../rast_log";	
		break;	
	case "4":
	default:
		// "ratf" 
		this.fLogReadout.itemname = "../ratf_log";	
		break;		
		
		
	};
	this.fLogReadout.EnableHistory(history);
	this.fLoggingHistory=history;
	this.fFileLogMode=mode;
	this.fLogReadout.CreateFrames($("#ReadoutInfo"));
	
}

MbsDisplay.prototype.SetMbsLog= function(){
	
	//console.log("SetMbsLog with value:"+history);
	this.fLogDevice.Clear();
	this.fLogDevice.itemname = "../logger";
	this.fLogDevice.EnableHistory(this.fMbsLoggingHistory);
	this.fLogDevice.CreateFrames($("#DeviceInfo"));
}


MbsDisplay.prototype.RefreshMonitor = function() {

	if (this.fTrending) {
		this.fTrendEv.force = true;		
		this.fTrendEv.RegularCheck();
		this.fTrendEv.CheckResize(true);			
		this.fTrendDa.force = true;		
		this.fTrendDa.RegularCheck();
		this.fTrendDa.CheckResize(true);
		this.fTrendSv.force = true;		
		this.fTrendSv.RegularCheck();
		this.fTrendSv.CheckResize(true);
		
		
		
		
	} else {
		this.fGaugeEv.force = true;
		this.fGaugeEv.RegularCheck();
		this.fGaugeEv.CheckResize(true);	
		this.fGaugeDa.force = true;
		this.fGaugeDa.RegularCheck();
		this.fGaugeDa.CheckResize(true);
		this.fGaugeSv.force = true;
		this.fGaugeSv.RegularCheck();
		this.fGaugeSv.CheckResize(true);
		

	}

	this.fLogDevice.force = true;
	this.fLogDevice.RegularCheck();
	this.fLogReadout.force = true;
	this.fLogReadout.RegularCheck();
	
	// optionally adjust scrollbars at info logs:
	$("#daq_log").scrollTop($("#daq_log")[0].scrollHeight - $("#daq_log").height());
	$("#file_log").scrollTop($("#file_log")[0].scrollHeight - $("#file_log").height());
	
	
	pthis = this;
	this.fMbsState.Update(function() {
		pthis.RefreshView()
	});
}





MbsDisplay.prototype.ChangeMonitoring = function(on){
	
    this.fMonitoring=on;
	if(on)
		{
		 this.SetStatusMessage("Starting monitoring timer with "+ this.fUpdateInterval + " ms.");
		 this.fUpdateTimer=window.setInterval(function(){MyDisplay.RefreshMonitor()}, this.fUpdateInterval);
		}
	else
		{
			window.clearInterval(this.fUpdateTimer);
			this.SetStatusMessage("Stopped monitoring timer.");
		}
	
}


MbsDisplay.prototype.SetTrending = function(on,history){
	this.fTrending=on;
	if(on)
		{
			//console.log("SetTrending on");
			this.SetRateTrending(history); // todo: get interval from textbox
		}
	else
		{
			//console.log("SetTrending off");
			this.SetRateGauges();
		}
	
}

MbsDisplay.prototype.RefreshView = function(){

	
	 if (this.fMbsState.fRunning) {
			$("#mbs_container").addClass("styleGreen").removeClass("styleRed");
		} else {
			$("#mbs_container").addClass("styleRed").removeClass("styleGreen");
		}
	 //console.log("RefreshView typeof fileopen=%s, value=%s globalvalue=%s", typeof(this.fMbsState.fFileOpen), 
	//		 this.fMbsState.fFileOpen, Pexor.fFileOpen);
	 
	 if (this.fMbsState.fFileOpen) {
		 	//console.log("RefreshView finds open file");
			$("#file_container").addClass("styleGreen").removeClass("styleRed");
    		$("#buttonStartFile").button("option", {icons: { primary: "ui-icon-closethick MyButtonStyle" }});
			$("#buttonStartFile").attr("title", "Close output file");
			
		} else {
			//console.log("RefreshView finds close file");
			$("#file_container").addClass("styleRed").removeClass("styleGreen");
			 $("#buttonStartFile").button("option", {icons: { primary: "ui-icon-seek-next MyButtonStyle" }}); 
			 $("#buttonStartFile").attr("title", "Open lmd file for writing");
			  
			 
			 
			 
		}
	 
	 
	 if (this.fMonitoring) {
			$("#monitoring_container").addClass("styleGreen").removeClass("styleRed");
			 $("label[for='Monitoring']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-stop MyButtonStyle\"></span>");
			 $("label[for='Monitoring']").attr("title", "Stop frequent refresh");
		} else {
			$("#monitoring_container").addClass("styleRed").removeClass("styleGreen");
			$("label[for='Monitoring']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-play MyButtonStyle\"></span>");
			 $("label[for='Monitoring']").attr("title", "Activate frequent refresh");	
		}
	 
	 $("#Refreshtime").prop('disabled', this.fMonitoring);
	 
	 
	 if (this.fTrending) {
			 $("label[for='Trending']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-circle-arrow-n MyButtonStyle\"></span>");//
			 $("label[for='Trending']").attr("title", "Show Rate Gauges");	
		} else {
			$("label[for='Trending']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-image MyButtonStyle\"></span>");
			$("label[for='Trending']").attr("title", "Show Rate Trending");		
		}
	 $("#Trendlength").prop('disabled', this.fTrending);
	 
	 if($("#FileRFIO").is(':checked')){
		 $("label[for='FileRFIO']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-link MyButtonStyle\"></span>");
		 $("label[for='FileRFIO']").attr("title",  "Write to RFIO server. Must be connected first with command connect rfio -DISK or -ARCHIVE.");
	 }
	 else
		 {
		 $("label[for='FileRFIO']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-disk MyButtonStyle\"></span>");
		 $("label[for='FileRFIO']").attr("title", "Write to local disk");
		 }
		 
		 
	 if($("#FileAutoMode").is(':checked')){
		 
		 $("label[for='FileAutoMode']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-star MyButtonStyle\"></span>");
		 $("label[for='FileAutoMode']").attr("title",  "Automatic file numbering. Names of the form namexxx.lmd are created with consecutive numbers xxx.");

	 } 
	 else
		 {
		 $("label[for='FileAutoMode']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-document MyButtonStyle\"></span>");
		 $("label[for='FileAutoMode']").attr("title",  "Use exact file name. Will return error if file already exists.");

		 }
	 
	 
	// console.log("RefreshView with dabc state = %s", this.fMbsState.fDabcState);
	 
//	 if (this.fMbsState.fDabcState=="Running") { 
//		//	console.log("RefreshView finds Running state");
//			$("#dabc_container").addClass("styleGreen").removeClass("styleRed").removeClass("styleYellow").removeClass("styleBlue");
//			 
//		} 
//	 else if (this.fMbsState.fDabcState=="Ready") {
//		 //	console.log("RefreshView finds Ready state");
//			$("#dabc_container").addClass("styleYellow").removeClass("styleRed").removeClass("styleGreen").removeClass("styleBlue");			 
//		} 
//	 else if ((this.fMbsState.fDabcState=="Failure") || (this.fMbsState.fDabcState=="Transition")) {
//		 //console.log("RefreshView finds Failure state");
//		 $("#dabc_container").addClass("styleBlue").removeClass("styleYellow").removeClass("styleRed").removeClass("styleGreen");			 
//		} 	 
//	 else {
//		 //console.log("RefreshView finds other state");
//			$("#dabc_container").addClass("styleRed").removeClass("styleGreen").removeClass("styleYellow").removeClass("styleBlue");
//		}
	 

	
};


MbsDisplay.prototype.SetStatusMessage= function(info) {
	var d = new Date();
	var txt = d.toLocaleString() + "  >" + info;
	document.getElementById("status_message").innerHTML = txt;
}





$(function() {

	MBS = new MbsState();
	MyDisplay=new MbsDisplay(MBS);
	MyDisplay.BuildView();
	MyDisplay.ChangeMonitoring(false);
	
	
///////////////////////////// mbs specific:	
	
	$("#buttonStartAcquisition").button({text: false, icons: { primary: "ui-icon-play MyButtonStyle"}}).click(
			function() {
				var requestmsg = "Really Start Acquisition?";
				var response = confirm(requestmsg);
				if (!response)
					return;

				MBS.DabcCommand("CmdMbs", "cmd=sta acq", function(
						result) {
					MyDisplay.SetStatusMessage(result ? "Start Acquisition command sent."
							: "Start Acquisition FAILED.");
					MyDisplay.RefreshMonitor();
				});
			});

	$("#buttonStopAcquisition").button({text: false, icons: { primary: "ui-icon-stop MyButtonStyle"}}).click(
			function() {

				var requestmsg = "Really Stop Acquisition?";
				var response = confirm(requestmsg);
				if (!response)
					return;
				MBS.DabcCommand("CmdMbs", "cmd=sto acq", function(
						result) {
					MyDisplay.SetStatusMessage(result ? "Stop Acquisition command sent."
							: "Stop Acquisition FAILED.");
					MyDisplay.RefreshMonitor();
				});
			});
	
////////// startup command will not work at the moment:	
//	$("#buttonInitAcquisition").button({text: false, icons: { primary: "ui-icon-wrench MyButtonStyle"}}).click(function() {
//		var requestmsg = "Really Initialize Acquisition?";
//		var response = confirm(requestmsg);
//		if (!response)
//			return;
//
//		MBS.DabcCommand("CmdMbs","cmd=\@startup",function(
//				result) {
//			MyDisplay.SetStatusMessage(result ? "Init Acquisition (@startup) command sent."
//					: "Init Acquisition FAILED.");
//			MyDisplay.RefreshMonitor();
//			
//		});
//
//	});

	$("#buttonStartFile").button({text: false, icons: { primary: "ui-icon-seek-next MyButtonStyle"}});
	
	$("#FileAutoMode").button({text: false, icons: { primary: "ui-icon-star MyButtonStyle"}}).click(function() {
		MyDisplay.RefreshView();
	}
			);
	
	$("#FileRFIO").button({text: false, icons: { primary: "ui-icon-link MyButtonStyle"}}).click(function() {
			MyDisplay.RefreshView();}
			);

	$("#lmd_file_form").submit(
				function(event) {
					
					var checked=MBS.fFileOpen;
					
					if(checked)
						{
						var requestmsg = "Really Stop writing output file "
							+ MBS.fFileName + " ?";
					var response = confirm(requestmsg);
					if (!response)
						{
							event.preventDefault();
							return;
						}
					MBS.DabcCommand("CmdMbs","cmd=clo fi",function(
							result) {
						MyDisplay.SetStatusMessage(result ? "Stop File command sent."
								: "Stop File FAILED.");
						MyDisplay.RefreshMonitor();
					});
						
						
						}
					else
						{							
						var datafilename=document.getElementById("Filename").value;
						var datafilelimit=document.getElementById("Filesize").value;
						var options = "cmd= open file " + datafilename
						 + " size=" + datafilelimit; 
						if($("#FileRFIO").is(':checked'))
							options += " -RFIO";
						else
							options += " -DISK";
						
						if($("#FileAutoMode").is(':checked'))
							options += " -AUTO";
					var requestmsg = "Really Start writing output file with "
					+ options+" ?";
				var response = confirm(requestmsg);
				if (!response)
					{
						event.preventDefault();
						return;
					}
				
					var options = "cmd= open file " + datafilename
					 + " size=" + datafilelimit; 
					if($("#FileRFIO").is(':checked'))
						options += " -RFIO";
					else
						options += " -DISK";
					
					if($("#FileAutoMode").is(':checked'))
						options += " -AUTO";
					
					MBS.DabcCommand("CmdMbs", options,function(
							result) {
						MyDisplay.SetStatusMessage(result ? "Start File command sent: "+options
								: "Start File FAILED.");
						if (result)
							{
							MBS.fFileName = datafilename;
							}
						MyDisplay.RefreshMonitor();
						
					
					});
						}
					event.preventDefault();
				}
					);
	
	

	$("#Monitoring").button({text: false, icons: { primary: "ui-icon-play MyButtonStyle"}}).click(function() {		
		MyDisplay.fUpdateInterval=1000*parseInt(document.getElementById("Refreshtime").value);
		MyDisplay.ChangeMonitoring($(this).is(':checked'));
		MyDisplay.RefreshView();
	});
	
	
	$("#buttonRefresh").button({text: false, icons: { primary: "ui-icon-refresh MyButtonStyle"}}).click(
			function() {
					MyDisplay.RefreshMonitor();
				});
	
	
	$("#Trending").button({text: false, icons: { primary: "ui-icon-image MyButtonStyle"}}).click(function() {
		MyDisplay.SetTrending($(this).is(':checked'), parseInt(document.getElementById("Trendlength").value));
		MyDisplay.RefreshView();
	});
	
	
	
	$("#buttonExecuteGosip").button({text: false, icons: { primary: "ui-icon-gear MyButtonStyle"}});
	

	
	
	   $( "#gosipcmd_form" ).submit(
				function(event) {
					var sfp=0;
					var dev=0;
					var log=false;
					var cmdpar=document.getElementById("commandGosip").value;
					var requestmsg = "Really Execute  gosipcmd "+ cmdpar;
					var response = confirm(requestmsg);
					if (!response){
						event.preventDefault();
						return;
						}
					MBS.GosipCommand(cmdpar, function(ok,
							logout, result) {
						MyDisplay.SetStatusMessage(ok ? "gosip command send."
								: "gosip command FAILED.");
						if(ok)
							{
								document.getElementById("GosipLog").innerHTML += logout;
								//console.log(logout);
							}
						// NOTE: GosipLog is inner text div, but scrollbars are put to container gosip_log!
						$("#gosip_log").scrollTop($("#gosip_log")[0].scrollHeight - $("#gosip_log").height());
					  	//console.log("scrollheight="+ $("#gosip_log")[0].scrollHeight + ", height=" + $("#gosip_log").height() + ", scrolltop=" + $("#gosip_log").scrollTop());				
						MyDisplay.RefreshMonitor();
					});
					
					event.preventDefault();
				});
	   
	   
	   
	   $("#buttonExecuteMBS").button({text: false, icons: { primary: "ui-icon-gear MyButtonStyle"}});
				
			
	   $( "#mbscmd_form" ).submit(
				function(event) {
					var sfp=0;
					var dev=0;
					var log=false;
					var cmdpar="cmd="+document.getElementById("commandMBS").value;
					var requestmsg = "Really Execute  mbs command: "+ cmdpar;
					var response = confirm(requestmsg);
					if (!response){
						event.preventDefault();
						return;
						}
					
					MBS.DabcCommand("CmdMbs", cmdpar,function(
							result) {
						MyDisplay.SetStatusMessage(result ? "Send MBS command: "+cmdpar
								: "MBS command  FAILED.");
						MyDisplay.RefreshMonitor();
					});
					event.preventDefault();
				});
	   
//	   $("#buttonClearMbsLog").button({text: false, icons: { primary: "ui-icon-trash MyButtonStyle"}}).click(
//				function() {
//						MyDisplay.SetStatusMessage("Cleared mbs logoutput.");
//						var history=MyDisplay.fMbsLoggingHistory;
//						MyDisplay.fMbsLoggingHistory=0;
//						MyDisplay.SetMbsLog();
//						MyDisplay.RefreshMonitor();
//						MyDisplay.fMbsLoggingHistory=history;
//						MyDisplay.SetMbsLog();
//						 
//				});				
	
	
	$("#buttonClearGosipLog").button({text: false, icons: { primary: "ui-icon-trash MyButtonStyle"}}).click(
			function() {
					MyDisplay.SetStatusMessage("Cleared gosip logoutput."); 
					document.getElementById("GosipLog").innerHTML="";
					
					 
			});
	 
	
	
// Use new jquery ui styled icon
	$("#buttonUserGUI").button({text: false, icons: { primary: "ui-icon-poland MyButtonStyle"}}).click(
			function() {
					MyDisplay.SetStatusMessage("Launched gosip user gui."); 
					window.open('/GOSIP/Test/UI/','_blank');
				});

	    $("#logmodes").buttonset();
	    
	    $("input[name='filelogmode']").on("change", function () {
	    	var history=document.getElementById("Loglength").value;
	    	console.log("logmodes got value="+this.value +", loglength="+history);	    	
	    	MyDisplay.SetFileLogMode(this.value,history);
	    });
		
	
	
    $('#Refreshtime').spinner({
        min: 1,
        max: 120,
        step: 1
    });

    $('#Trendlength').spinner({
        min: 10,
        max: 10000,
        step: 10
    });
	
    $('#Filesize').spinner({
        min: 100,
        max: 4000,
        step: 100
    });
    
    $('#Loglength').spinner({
        min: 10,
        max: 100000,
        step: 10,
        spin: function( event, ui ) {MyDisplay.fLoggingHistory=ui.value;},
    	stop: function( event, ui ) {	MyDisplay.SetFileLogMode(0,0);}
    });
    
    $('#MbsLoglength').spinner({
        min: 10,
        max: 100000,
        step: 10,
        spin: function( event, ui ) {MyDisplay.fMbsLoggingHistory=ui.value;},
    	stop: function( event, ui ) {	MyDisplay.SetMbsLog();}
    });
 
    
	MyDisplay.RefreshView();
	

	
	
	
	
	$(document).tooltip();

});