
var MBS;
var MyDisplay;



////////////// State class reflecting remote state and commnication:
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
		
		// workaraound: for the moment, mbs state is taken from dabc state
		if (state=="Running")
			this.fRunning=true;
		else
			this.fRunning=false;
		
		
	} else {
		console.log("UpdateDABCstate failed.");
	}
    console.log("UpdateDABCstate with ok=%s, value=%s, dabcstate=%s", ok, state, this.fDabcState);
    refreshcall();
}



MbsState.prototype.Update= function(callback){
	var pthis = this;
	//this.DabcParameter("PexDevice/MBSAcquisitionRunning", pthis.UpdateRunstate);
	//this.DabcParameter("PexReadout/PexorFileOn", pthis.UpdateFilestate);	
//	this.DabcParameter("PexDevice/PexorAcquisitionRunning", function(res,val) { pthis.UpdateRunstate(res,val); });
//	this.DabcParameter("PexReadout/PexorFileOn",function(res,val) { pthis.UpdateFilestate(res,val); })
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
	this.fUpdateTimer=null;
	this.fUpdateInterval=2000; // ms
	this.fGaugeEv = null;
	this.fGaugeDa = null;
	this.fTrendEv = null;
	this.fTrendDa = null;
	this.fLogDevice=null;
	this.fLogReadout=null;
}

// set up view elements of display:
MbsDisplay.prototype.BuildView = function(){
	
	
	
	this.fGaugeEv = new DABC.GaugeDrawElement();
	this.fTrendEv=new DABC.RateHistoryDrawElement();
	
	this.fGaugeDa = new DABC.GaugeDrawElement();
	this.fTrendDa=new DABC.RateHistoryDrawElement();
	
	this.fLogDevice= new DABC.LogDrawElement();
	this.fLogDevice.itemname = "../logger";
	this.fLogDevice.EnableHistory(100);
	this.fLogDevice.CreateFrames($("#DeviceInfo"));
	
	this.fLogReadout= new DABC.LogDrawElement();
	this.fLogReadout.itemname = "../ratf_log";
	this.fLogReadout.EnableHistory(100);
	this.fLogReadout.CreateFrames($("#ReadoutInfo"));
	
	this.SetRateGauges();
	
}


MbsDisplay.prototype.SetRateGauges = function(){
	
	this.fTrendEv.Clear();
	this.fTrendDa.Clear();
	this.fGaugeEv.itemname = "../EventRate";
	this.fGaugeEv.CreateFrames($("#EvRateDisplay"));
	this.fGaugeDa.itemname = "../DataRate";
	this.fGaugeDa.CreateFrames($("#DatRateDisplay"));
}

MbsDisplay.prototype.SetRateTrending = function(history){
	
	this.fTrendingHistory=history;
	this.fGaugeEv.Clear();
	this.fGaugeDa.Clear();
	this.fTrendEv.itemname="../EventRate";
	this.fTrendEv.EnableHistory(this.fTrendingHistory); 
	this.fTrendEv.CreateFrames($("#EvRateDisplay"));
	this.fTrendDa.itemname="../DataRate";
	this.fTrendDa.EnableHistory(this.fTrendingHistory);
	this.fTrendDa.CreateFrames($("#DatRateDisplay"));	
}



MbsDisplay.prototype.RefreshMonitor = function() {

	if (this.fTrending) {
		this.fTrendEv.force = true;
		
		this.fTrendEv.RegularCheck();
		this.fTrendEv.CheckResize(true);			
		this.fTrendDa.force = true;		
		this.fTrendDa.RegularCheck();
		this.fTrendDa.CheckResize(true);	
		
	} else {
		this.fGaugeEv.force = true;
		this.fGaugeEv.RegularCheck();
		this.fGaugeEv.CheckResize(true);	
		this.fGaugeDa.force = true;
		this.fGaugeDa.RegularCheck();
		this.fGaugeDa.CheckResize(true);

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
			console.log("SetTrending on");
			this.SetRateTrending(history); // todo: get interval from textbox
		}
	else
		{
			console.log("SetTrending off");
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
//			$("#buttonStartFile").prop('checked', true);
//			$("label[for='buttonStartFile']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-closethick MyButtonStyle\"</span>");
//			$("label[for='buttonStartFile']").attr("title", "Close output file");
    		$("#buttonStartFile").button("option", {icons: { primary: "ui-icon-closethick MyButtonStyle" }});
			$("#buttonStartFile").attr("title", "Close output file");
			
		} else {
			//console.log("RefreshView finds close file");
			$("#file_container").addClass("styleRed").removeClass("styleGreen");
//			$("#buttonStartFile").prop('checked', false);
//			 $("label[for='buttonStartFile']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-disk MyButtonStyle\"</span>");
//			 $("label[for='buttonStartFile']").attr("title", "Open lmd file for writing");
//			 
			 $("#buttonStartFile").button("option", {icons: { primary: "ui-icon-disk MyButtonStyle" }}); 
			 $("#buttonStartFile").attr("title", "Open lmd file for writing");
			  
			 
			 
			 
		}
	 
	 
	 if (this.fMonitoring) {
			$("#monitoring_container").addClass("styleGreen").removeClass("styleRed");
			 //$("label[for='Monitoring']").html("<span class=\"ui-button-text\">  Stop Monitoring </span>");
			 $("label[for='Monitoring']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-stop MyButtonStyle\"></span>");
			 $("label[for='Monitoring']").attr("title", "Stop frequent refresh");
		} else {
			$("#monitoring_container").addClass("styleRed").removeClass("styleGreen");
			//$("label[for='Monitoring']").html("<span class=\"ui-button-text\">  Start Monitoring </span>");
			$("label[for='Monitoring']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-play MyButtonStyle\"></span>");
			 $("label[for='Monitoring']").attr("title", "Activate frequent refresh");	
		}
	 
	 $("#Refreshtime").prop('disabled', this.fMonitoring);
	 
	 
	 if (this.fTrending) {
			 //$("label[for='Trending']").html("<span class=\"ui-button-text\">  Show Rate Gauges </span>");
			 $("label[for='Trending']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-circle-arrow-n MyButtonStyle\"></span>");//
			 //.addClass("MyButtonStyle");
			 $("label[for='Trending']").attr("title", "Show Rate Gauges");	
		} else {
			//$("label[for='Trending']").html("<span class=\"ui-button-text\">   Show Rate Trending </span>");
			$("label[for='Trending']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-image MyButtonStyle\"></span>");
			//.addClass("MyButtonStyle");
			$("label[for='Trending']").attr("title", "Show Rate Trending");		
		}
	 $("#Trendlength").prop('disabled', this.fTrending);
	 
	 console.log("RefreshView with dabc state = %s", this.fMbsState.fDabcState);
	 
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
	
	//////// first DABC generic commands
	// later this should be part of the framework...
//	$("#buttonStartDabc").button({text: false, icons: { primary: "ui-icon-play MyButtonStyle"}}).click(
//			function() {
//				var requestmsg = "Really Re-Start DABC Application?";
//				var response = confirm(requestmsg);
//				if (!response)
//					return;
//
//				MBS.DabcCommand("App/DoStart", "", function(
//						result) {
//					MyDisplay.SetStatusMessage(result ? "Start DABC command sent."
//							: "Start DABC FAILED.");
//					MyDisplay.RefreshMonitor();
//				});
//			});
//	
//	$("#buttonStopDabc").button({text: false, icons: { primary: "ui-icon-stop MyButtonStyle"}}).click(
//			function() {
//				var requestmsg = "Really Stop DABC Application?";
//				var response = confirm(requestmsg);
//				if (!response)
//					return;
//
//				MBS.DabcCommand("App/DoStop", "", function(
//						result) {
//					MyDisplay.SetStatusMessage(result ? "Stop DABC command sent."
//							: "Stop DABC  FAILED.");
//					MyDisplay.RefreshMonitor();
//				});
//			});
//	
//	$("#buttonConfigureDabc").button({text: false, icons: { primary: "ui-icon-power MyButtonStyle"}}).click(
//			function() {
//				var requestmsg = "Really Re-Configure DABC Application?";
//				var response = confirm(requestmsg);
//				if (!response)
//					return;
//
//				MBS.DabcCommand("App/DoConfigure", "", function(
//						result) {
//					MyDisplay.SetStatusMessage(result ? "Configure DABC command sent."
//							: "Configure DABC FAILED.");
//					MyDisplay.RefreshMonitor();
//				});
//			});
//	$("#buttonHaltDabc").button({text: false, icons: { primary: "ui-icon-cancel MyButtonStyle"}}).click(
//			function() {
//				var requestmsg = "Really Halt (shutdown) DABC Application?";
//				var response = confirm(requestmsg);
//				if (!response)
//					return;
//
//				MBS.DabcCommand("App/DoHalt", "", function(
//						result) {
//					MyDisplay.SetStatusMessage(result ? "Halt DABC command sent."
//							: "Halt DABC FAILED.");
//					MyDisplay.RefreshMonitor();
//				});
//			});
//	
	
///////////////////////////// pexor specific:	
	
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
//					if (result)
//						MBS.fRunning = true;
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
//					if (result)
//						MBS.fRunning = false;

					MyDisplay.RefreshMonitor();
				});
			});
	$("#buttonInitAcquisition").button({text: false, icons: { primary: "ui-icon-wrench MyButtonStyle"}}).click(function() {
		var requestmsg = "Really Initialize Acquisition?";
		var response = confirm(requestmsg);
		if (!response)
			return;

		MBS.DabcCommand("CmdMbs","cmd=\@startup",function(
				result) {
			MyDisplay.SetStatusMessage(result ? "Init Acquisition (@startup) command sent."
					: "Init Acquisition FAILED.");
			MyDisplay.RefreshMonitor();
			
		});

	});

	$("#buttonStartFile").button({text: false, icons: { primary: "ui-icon-disk MyButtonStyle"}});

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
						
					var requestmsg = "Really Start writing output file "
					+ datafilename + ", maxsize=" + datafilelimit +" ?";
				var response = confirm(requestmsg);
				if (!response)
					{
						event.preventDefault();
						return;
					}
				
					var options = "open file " + datafilename
					 + " size=" + datafilelimit + " -DISK"; // todo: switch between rfio and disk

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
						// todo: display output of result like in poland debug mode
						
						
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
						
	
	
	$("#buttonClearGosipLog").button({text: false, icons: { primary: "ui-icon-trash MyButtonStyle"}}).click(
			function() {
					MyDisplay.SetStatusMessage("Cleared gosip logoutput."); 
					document.getElementById("GosipLog").innerHTML="";
					
					 
			});
	 
	
	
// Use new jquery ui styled icon. However, here we would lose colors due to rendering
	$("#buttonUserGUI").button({text: false, icons: { primary: "ui-icon-poland MyButtonStyle"}}).click(
			function() {
					MyDisplay.SetStatusMessage("Launched gosip user gui."); 
					window.open('/GOSIP/Test/UI/','_blank');
				});

	
	
	// this works without replacing existing classes icons:
//	$("#buttonUserGUI").text("").append('<img src="img/PolandLogo.png"  height="24" width="25"/>').button()
//	.click(
//			function() {
//					MyDisplay.SetStatusMessage("Launched gosip user gui."); 
//					window.open('/GOSIP/Test/UI/','_blank');
//				});
	

	
	
	
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
    
    
    
 
    
    
    
    
	MyDisplay.RefreshView();
	

	
	
	
	
	$(document).tooltip();

});