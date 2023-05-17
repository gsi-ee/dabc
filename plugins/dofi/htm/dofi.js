
"use strict";

let MyDOFI, MyDisplay;

const DofiCommands = {
	DOFI_NOP: 0,
	DOFI_RESET: 1,
	DOFI_READ: 2,
	DOFI_WRITE: 3,
	DOFI_SETBIT: 4,
	DOFI_CLEARBIT: 5,
	DOFI_RESET_SCALER: 6,
	DOFI_ENABLE_SCALER: 7,
	DOFI_DISABLE_SCALER: 8,
	DOFI_CONFIGURE: 9
}


/** number of input or output:*/
var DOFI_NUM_CHANNELS = 64,




	/** registers for invert, delay, signal length forming, start here: */
	DOFI_SIGNALCTRL_BASE = 0x0,

	/* signal control register bits:
	 * |63......40|39.....16|15....1|0|
	 * | -length- | -delay- |xxxxxxx|invert|
	 *
	 * 24 bit length values 0 .16777215 (TODO units!)
	 * 24 bit delay valuse  0 ...16777215 (units?)
	 * 1 bit inverted yes or no
	 * */

	/** registers to set AND relation between this output channel and input channels (bit indexed);
	 * starting from this address:*/
	DOFI_ANDCTRL_BASE = 0x40,

	/** registers to set OR relation between this output channel and input channels (bit indexed)
	 * starting from tis address*/
	DOFI_ORCTRL_BASE = 0x80,

	/* address of input scalers from here:*/
	DOFI_INPUTSCALER_BASE = 0xC0,

	/* address of output scalers from here:*/
	DOFI_OUTPUTSCALER_BASE = 0x100;

let DOFI_TIME_UNIT = 10n;


////////////// State class reflecting remote state and communication:

class DofiState {

	constructor() {
		this.fVerbose = false;
		this.fHexmode = false;
		this.fFileName = "test.dof";
		this.fSignalControl = [];
		this.fOutputANDBits = [];
		this.fOutputORBits = [];
		this.fInputScalers = [];
		this.fOutputScalers = [];
		this.fInputScalersBefore = [];
		this.fOutputScalersBefore = [];
		this.fInputRate = [];
		this.fOutputRate = [];
		for (var i = 0; i < DOFI_NUM_CHANNELS; i++) {
			this.fSignalControl.push(BigInt(0));
			this.fSignalControl.push(BigInt(0));
			this.fOutputANDBits.push(BigInt(0));
			this.fOutputORBits.push(BigInt(0));
			this.fInputScalers.push(BigInt(0));
			this.fOutputScalers.push(BigInt(0));
			this.fInputScalersBefore.push(BigInt(0));
			this.fOutputScalersBefore.push(BigInt(0));
			this.fInputRate.push(0.0);
			this.fOutputRate.push(0.0);
		}


	}

	/////////////////////////// functions for manipulating data elements here:
	SetInputInvert(ch, on) {
		let channel = Number(ch);
		let enable = Boolean(on);
		if (channel < DOFI_NUM_CHANNELS) {
			enable ? (this.fSignalControl[channel] |= 1n) : (this.fSignalControl[channel] &= ~1n);
		}
	}

	IsInputInvert(ch) {
		let channel = Number(ch);
		let rev = Boolean(false);
		if (channel < DOFI_NUM_CHANNELS) {
			rev = Boolean((this.fSignalControl[channel] & 0x1n) == 0x1n);
		}
		/*console.log("signalcontrol ["+channel+"]="+Number(this.fSignalControl[channel]));	
		console.log("IsInputInvert ["+channel+"]="+rev);*/


		return rev;
	}


	SetInputDelay(ch, d) {
		let channel = Number(ch);
		let mask = BigInt(0);
		let delay = BigInt(d) / DOFI_TIME_UNIT;
		if (channel < DOFI_NUM_CHANNELS) {
			mask = (0xFFFFFFn << 16n);
			this.fSignalControl[channel] &= ~mask;
			this.fSignalControl[channel] |= (delay & 0xFFFFFFn) << 16n;
		}
	}

	GetInputDelay(ch) {
		let channel = Number(ch);
		let value = Number(-1);
		if (channel < DOFI_NUM_CHANNELS) {
			value = Number(DOFI_TIME_UNIT * BigInt.asIntN(24, (this.fSignalControl[channel] >> 16n) & 0xFFFFFFn));

		}
		return value;
	}

	SetInputLength(ch, l) {
		let channel = Number(ch);
		let mask = BigInt(0);
		let len = BigInt(l) / DOFI_TIME_UNIT;

		if (channel < DOFI_NUM_CHANNELS) {
			mask = (0xFFFFFFn << 40n);
			//console.log("SetInputLength("+channel+","+len+" old register value:"+this.fSignalControl[channel] );
			this.fSignalControl[channel] &= ~mask;
			this.fSignalControl[channel] |= (len & 0xFFFFFFn) << 40n;

			//console.log("SetInputLength("+channel+","+len+" sets register value:"+this.fSignalControl[channel] );
		}
	}




	GetInputLength(ch) {
		let channel = Number(ch);
		let value = Number(-1);
		if (channel < DOFI_NUM_CHANNELS) {
			value = Number(DOFI_TIME_UNIT * BigInt.asIntN(24, (this.fSignalControl[channel] >> 40n) & 0xFFFFFFn));
		}
		return value;
	}

	AddOutputAND(ouch, inch) {

		if (ouch < DOFI_NUM_CHANNELS && inch < DOFI_NUM_CHANNELS) {
			//let outchannel = BigInt(ouch);
			let inchannel = BigInt(inch);
			let mask = BigInt(1n << inchannel);
			this.fOutputANDBits[ouch] |= mask;
			console.log("AddOutputAND(" + ouch + "," + inch + ") sets register value:" + this.fOutputANDBits[ouch]);
		}
	}


	ClearOutputAND(ouch, inch) {
		if (ouch < DOFI_NUM_CHANNELS && inch < DOFI_NUM_CHANNELS) {
			//let outchannel = BigInt(ouch);
			let inchannel = BigInt(inch);
			let mask = BigInt(1n << inchannel);
			this.fOutputANDBits[ouch] &= ~mask;
			console.log("ClearOutputAND(" + ouch + "," + inch + ") sets register value:" + this.fOutputANDBits[ouch]);
		}
	}

	IsOutputAND(ouch, inch) {
		let rev = Boolean(false);
		if (ouch < DOFI_NUM_CHANNELS && inch < DOFI_NUM_CHANNELS) {
			//let outchannel = BigInt(ouch);
			let inchannel = BigInt(inch);
			let mask = BigInt(1n << inchannel);
			rev = ((this.fOutputANDBits[ouch] & mask) == mask);
		}
		return rev;
	}


	AddOutputOR(ouch, inch) {

		if (ouch < DOFI_NUM_CHANNELS && inch < DOFI_NUM_CHANNELS) {
			//let outchannel = BigInt(ouch);
			let inchannel = BigInt(inch);
			let mask = BigInt(1n << inchannel);
			this.fOutputORBits[ouch] |= mask;
			console.log("AddOutputOR(" + ouch + "," + inch + ") sets register value:" + this.fOutputORBits[ouch]);
		}
	}


	ClearOutputOR(ouch, inch) {
		if (ouch < DOFI_NUM_CHANNELS && inch < DOFI_NUM_CHANNELS) {
			let outchannel = BigInt(ouch);
			let inchannel = BigInt(inch);
			let mask = BigInt(1n << inchannel);
			this.fOutputORBits[ouch] &= ~mask;
			console.log("ClearOutputOR(" + ouch + "," + inch + ") sets register value:" + this.fOutputORBits[ouch]);
		}
	}

	IsOutputOR(ouch, inch) {
		let rev = Boolean(false);
		if (ouch < DOFI_NUM_CHANNELS && inch < DOFI_NUM_CHANNELS) {
			//let outchannel = BigInt(ouch);
			let inchannel = BigInt(inch);
			let mask = BigInt(1n << inchannel);
			rev = ((this.fOutputORBits[ouch] & mask) == mask);
		}
		return rev;
	}








	/////////////// below funtions for communication with the web server JAM:



	/*DabcCommand(cmd, option) {
		let pre = "../",
			suf = "/execute",
			fullcom = pre + cmd + suf;
		if (option) fullcom += "?" + option;

		return JSROOT.httpRequest(fullcom, "text")
			.then(reply => { console.log('Reply = ' + reply); return true; });
	}

	DabcParameter(par) {
		let pre = "../",
			suf = "/get.json",
			fullcom = pre + par + suf;

		return JSROOT.httpRequest(fullcom, "text")
			.then(reply => {
				if (!reply) return null;
				let obj = JSON.parse(reply);
				return obj ? obj.value : null;
			});
	}*/

	DofiCommand(cmd, address, value, repeat, hex, verbose, filename) {

		let cmdurl = "/dofi/DofiCommand/execute";
		let cmdtext = cmdurl + "?COMMAND=" + cmd;
		cmdtext += "&VERBOSE=" + verbose;
		cmdtext += "&ADDRESS=" + address;
		cmdtext += "&VALUE=" + value;
		cmdtext += "&REPEAT=" + repeat;
		cmdtext += "&HEXFORMAT=" + hex;
		cmdtext += "&FILENAME=\'" + filename + "\'";
		//console.log(cmdtext);

		return JSROOT.httpRequest(cmdtext, "object")
			.then(reply => {
				//if(this.fVerbose) {console.log('Reply = ' + reply);}
				let numresults = 0;
				var results = [];
				var addrs = [];
				if (reply['NUMRESULTS'] != null) {
					numresults = Number(reply['NUMRESULTS']);

					for (var i = 0; i < numresults; ++i) {
						let vfield = "VALUE_" + i;
						let afield = "ADDRESS_" + i;
						let value = 0;
						let add = 0;
						value = BigInt(reply[vfield]);
						results.push(value);
						add = Number(reply[afield]);
						addrs.push(add);
					} // numresults
					//console.log('Reply1 = ' + numresults);
					if (numresults == 0) return true;
					return { numresults, results, addrs };
				} // if (reply['NUMRESULTS']  
				//console.log('Reply2 = ' + numresults);
				return true;
			})
			.catch(() => {
				console.log("Dofi command failed - no  results");
				return { numresults: 0, results: [], addrs: 0 };
			}); //then


	}






	GetRegisters() {

		let prscale = this.GetScalers();
		let prset = this.GetSetup();
		return Promise.all([prscale, prset]).then(arr => { return arr[0] + arr[1]; });
	}

	GetSetup() {

		var cmd = DofiCommands.DOFI_READ;
		var base;
		//console.log("Get setup called");

		base = DOFI_ANDCTRL_BASE;
		//cmd, address, value, repeat, hex, verbose, filename,
		let pr1 = this.DofiCommand(cmd, base, 0, DOFI_NUM_CHANNELS, this.fHexmode, this.fVerbose, "")
			.then(rev => {
				if (this.fVerbose) {
					console.log("Got " + rev.numresults + " result values.");
					console.log(rev.addrs);
					console.log(rev.results);
				}
				this.fOutputANDBits = rev.results;
				//return true;
			})
			.catch(() => {
				console.log("Get Setup failed for AND registers.");
				return false;
			})

		base = DOFI_ORCTRL_BASE;
		//cmd, address, value, repeat, hex, verbose, filename,
		let pr2 = this.DofiCommand(cmd, base, 0, DOFI_NUM_CHANNELS, this.fHexmode, this.fVerbose, "")
			.then(rev => {
				if (this.fVerbose) {
					console.log("Got " + rev.numresults + " result values.");
					console.log(rev.addrs);
					console.log(rev.results);
				}
				this.fOutputORBits = rev.results;
				//return true;
			})
			.catch(() => {
				console.log("Get Setup failed forOR registers.");
				return false;
			})


		base = DOFI_SIGNALCTRL_BASE;
		//cmd, address, value, repeat, hex, verbose, filename,
		let pr3 = this.DofiCommand(cmd, base, 0, DOFI_NUM_CHANNELS, this.fHexmode, this.fVerbose, "")
			.then(rev => {
				if (this.fVerbose) {
					console.log("Got " + rev.numresults + " result values.");
					console.log(rev.addrs);
					console.log(rev.results);
				}


				this.fSignalControl = rev.results;

				//return true;
			})
			.catch(() => {
				console.log("Get Setup failed for input signal registers.");
				return false;
			})



		return Promise.all([pr1, pr2, pr3]).then(arr => { return arr[0] + arr[1] + arr[2]; });
	}


	GetScalers() {

		var cmd = DofiCommands.DOFI_READ;
		var base;


		//console.log("GetScalers called");

		// first try with scalers only:
		base = DOFI_INPUTSCALER_BASE;
		//cmd, address, value, repeat, hex, verbose, filename,
		let pr1 = this.DofiCommand(cmd, base, 0, DOFI_NUM_CHANNELS, this.fHexmode, this.fVerbose, "")
			.then(rev => {
				if (this.fVerbose) {
					console.log("Got " + rev.numresults + " result values.");
					console.log(rev.addrs);
					console.log(rev.results);
				}
				this.fInputScalers = rev.results;
				//return true;
			})
			.catch(() => {
				console.log("Get Registers failed for input scalers.");
				return false;
			})

		base = DOFI_OUTPUTSCALER_BASE;
		//cmd, address, value, repeat, hex, verbose, filename,
		let pr2 = this.DofiCommand(cmd, base, 0, DOFI_NUM_CHANNELS, this.fHexmode, this.fVerbose, "")
			.then(rev => {
				if (this.fVerbose) {
					console.log("Got " + rev.numresults + " result values.");
					console.log(rev.addrs);
					console.log(rev.results);
				}
				this.fOutputScalers = rev.results;
				//return true;
			})
			.catch(() => {
				console.log("Get Registers failed for output scalers.");
				return false;
			})

		return Promise.all([pr1, pr2]).then(arr => { return arr[0] + arr[1]; });

	}
	
	
	SingleCommand(cmd) {
		
		return this.DofiCommand(cmd, 0, 0, 1, this.fHexmode, this.fVerbose, "")
						.then(() => {
							if (this.fVerbose) {
								console.log("Executed single command: " + cmd + " )");
							}
						return true;
						})
						.catch(() => {
							console.log("Single command "+cmd+" failed!");
							return false;
						});
		}
	

	SetRegisters(options) {

		var cmd = DofiCommands.DOFI_WRITE;
		var base;
		//console.log("SetRegisters with options:"+options);


		// TODO: special mode to set all registers?



		// TODO: only write entries that were really changed, like parameter editor JAM23
		let promises = [];
		let len = options.length;
		for (let index = 0; index < len; index++) {
			let entry = options[index];
			let key = entry[0];
			let value = entry[1];
			let address = 0;
			console.log("SetRegisters found key" + key + " with value:" + value);

			let delim = key.lastIndexOf("_") + 1;
			let suffix = key.substring(delim);
			let i = Number(suffix);
			if (key.includes("in_invert")) {

				//console.log("got suffix:"+suffix+", number="+i);
				this.SetInputInvert(i, value);
				address = DOFI_SIGNALCTRL_BASE + i;
				promises[index] =
					this.DofiCommand(cmd, address, this.fSignalControl[i], 1, this.fHexmode, this.fVerbose, "")
						.then(() => {
							if (this.fVerbose) {
								console.log("Wrote to register " + address + ":" + this.fSignalControl[i] + " (" + key + "=" + value + ")");
							}
						})
						.catch(() => {
							console.log("Set Registers failed for " + key + "=" + value);
						});



			}
			else if (key.includes("in_delay")) {
				this.SetInputDelay(i, value);
				address = DOFI_SIGNALCTRL_BASE + i;
				promises[index] =
					this.DofiCommand(cmd, address, this.fSignalControl[i], 1, this.fHexmode, this.fVerbose, "")
						.then(() => {
							if (this.fVerbose) {
								console.log("Wrote to register " + address + ":" + this.fSignalControl[i] + " (" + key + "=" + value + ")");
							}
						})
						.catch(() => {
							console.log("Set Registers failed for " + key + "=" + value);
						});


			}
			else if (key.includes("in_length")) {
				this.SetInputLength(i, value);
				address = DOFI_SIGNALCTRL_BASE + i;
				promises[index] =
					this.DofiCommand(cmd, address, this.fSignalControl[i], 1, this.fHexmode, this.fVerbose, "")
						.then(() => {
							if (this.fVerbose) {
								console.log("Wrote to register " + address + ":" + this.fSignalControl[i] + " (" + key + "=" + value + ")");
							}
						})
						.catch(() => {
							console.log("Set Registers failed for " + key + "=" + value);
						});

			}

			else if (key.includes("matrix_and")) {
				let dash = suffix.lastIndexOf("-");
				let ouch = suffix.substring(dash + 1);
				let inch = suffix.substring(0, dash);
				let oc = Number(ouch);
				let ic = Number(inch);
				let enable = Boolean(value);
				if (enable) {
					this.AddOutputAND(oc, ic)
				}
				else {
					this.ClearOutputAND(oc, ic);
				}
				//this.SetInputLength(i,value);
				address = DOFI_ANDCTRL_BASE + oc;
				promises[index] =
					this.DofiCommand(cmd, address, this.fOutputANDBits[oc], 1, this.fHexmode, this.fVerbose, "")
						.then(() => {
							if (this.fVerbose) {
								console.log("Wrote to register " + address + ":" + this.fOutputANDBits[oc] + " (" + key + "=" + value + ")");
							}
						})
						.catch(() => {
							console.log("Set Registers failed for " + key + "=" + value);
						});

			}

			else if (key.includes("matrix_or")) {
				let dash = suffix.lastIndexOf("-");
				let ouch = suffix.substring(dash + 1);
				let inch = suffix.substring(0, dash);
				let oc = Number(ouch);
				let ic = Number(inch);
				let enable = Boolean(value);
				if (enable) {
					this.AddOutputOR(oc, ic)
				}
				else {
					this.ClearOutputOR(oc, ic);
				}
				address = DOFI_ORCTRL_BASE + oc;
				promises[index] =
					this.DofiCommand(cmd, address, this.fOutputORBits[oc], 1, this.fHexmode, this.fVerbose, "")
						.then(() => {
							if (this.fVerbose) {
								console.log("Wrote to register " + address + ":" + this.fOutputORBits[oc] + " (" + key + "=" + value + ")");
							}
						})
						.catch(() => {
							console.log("Set Registers failed for " + key + "=" + value);
						});

			}




		} // for


		return Promise.all(promises).then(() => { return true; })
			.catch(() => { return false; });

	

	}

	SaveScalers() {
		for (var i = 0; i < DOFI_NUM_CHANNELS; ++i) {
			this.fInputScalersBefore[i] = this.fInputScalers[i];
			this.fOutputScalersBefore[i] = this.fOutputScalers[i];
		}
	}


	EvaluateRates(deltatime) {
		{
			if (deltatime == 0) return;
			for (var i = 0; i < DOFI_NUM_CHANNELS; ++i) {
				this.fInputRate[i] = 1000.0 * Number(this.fInputScalers[i] - this.fInputScalersBefore[i]) / deltatime;
				this.fOutputRate[i] = 1000.0 * Number(this.fOutputScalers[i] - this.fOutputScalersBefore[i]) / deltatime;
			}
			//console.log(" EvaluateRates: deltatime="+deltatime +", rate[0]="+this.fInputRate[0]);
		}
	}


	Update() {
		let pr1 = this.UpdateMonitor(0);
		let pr2 = this.UpdateSetup();
		return Promise.all([pr1, pr2]).then(arr => { return arr[0] + arr[1]; });
	}

	UpdateSetup() {
		let pr = this.GetSetup().then(() => {
			//console.log("Get Registers is ok. "); 
		})
			.catch(() => console.log("Update setup state failed."));

		return pr;
	}


	UpdateMonitor(deltatime) {
		this.SaveScalers();
		let pr = this.GetScalers().then(() => {


			//console.log("Get Scalers is ok. "); 
			this.EvaluateRates(deltatime);
		})
			.catch(() => console.log("Update setup state failed."));
		return pr;
	}


} // class


/////////////// DISPLAY class to manage current view:

class DofiDisplay extends JSROOT.BasePainter {


	checkResize() { }

	constructor(dom, state) {
		super(dom);
		this.fDofiState = state;
		this.fDoCommandConfirm = true;
		this.fMonitoring = false;
		this.fUpdateTimer = null;
		this.fUpdateInterval = 2000; // ms
		this.fChanges = ["dummy", "init"];
		this.BuildView();
	}

	// set up view elements of display:
	BuildView() {

let dom = this.selectDom();
		dom.select("#dofi_command_container").classed("styleBlue", true);


//	$("#dofi_command_container").addClass("styleBlue"); // TODO: remove jquery for d3


		// select first tab of logic matrix tables	
		document.getElementById("OR").style.display = "block";
		let tablinks = document.getElementsByClassName("tablinks");
		tablinks[0].className += " active";
		
		
		
		


		this.RefreshAll(); // will also fetch data from server
	}



	SelectMatrixTab(evt, tabName) {
		var i, tabcontent, tablinks;
		let dom = this.selectDom();
		tabcontent = document.getElementsByClassName("tabcontent");
		for (i = 0; i < tabcontent.length; i++) {
			tabcontent[i].style.display = "none";
		}
		tablinks = document.getElementsByClassName("tablinks");
		for (i = 0; i < tablinks.length; i++) {
			tablinks[i].className = tablinks[i].className.replace(" active", "");
		}
		document.getElementById(tabName).style.display = "block";
		evt.currentTarget.className += " active";
	}


	RefreshAll() {
		return this.fDofiState.Update().then(() => this.RefreshView());
	}


	RefreshMonitor(deltatime) {

		//console.log("RefreshMonitor with dt="+deltatime);
		return this.fDofiState.UpdateMonitor(deltatime).then(() => {
			this.SetStatusMessage("Refreshed Scalers");
			this.FillScalersTable();

		});
	}



	/** @summary add identifier of changed element to list, make warning sign visible */
	MarkChanged(key) {
		let dom = this.selectDom();
		// first avoid duplicate keys:
		if (this.fChanges.indexOf(key) >= 0) return;
		this.fChanges.push(key);
		console.log("Mark changed :%s", key);
		dom.select("#status_container").classed("styleMagenta", true).classed("styleYellow", false);
		//$("#status_container").addClass("styleMagenta").removeClass("styleYellow");
		//this.selectDom().select(".buttonChangedParameter").style("display", null);// show warning sign
	}

	/** @summary clear changes flag */
	ClearChanges() {
		let dom = this.selectDom();
		this.fChanges = []; //
		dom.select("#status_container").classed("styleYellow", true).classed("styleMagenta", false);
		//$("#status_container").addClass("styleYellow").removeClass("styleMagenta");
		//this.selectDom().select(".buttonChangedParameter").style("display", "none"); // hide warning sign
	}


	EvaluateChanges() {
		let elem = ["dummy", "val"];
		let changelist = [elem];
		let dom = this.selectDom(),
			len = this.fChanges.length;
		changelist = [];
		for (let index = 0; index < len; index++) {
			//let cursor=changes.pop();
			let key = this.fChanges[index];
			console.log("Evaluate change key:%s", key);
			// here mapping of key to editor field:
			// first look if we have checkbox:
			let val = 0;
			let element = dom.select("." + key.toString());
			if (element.property("type") == "checkbox") {
				element.property("checked") ? val = 1 : val = 0;
			}
			else {
				val = dom.select("." + key.toString()).property("value");
			}
			changelist.push([key, val]);
		}// for index
		//console.log("Resulting option string:%s", optionstring);

		return changelist;
	}


	ApplySetup() {

		if (this.fDoCommandConfirm) {
			let requestmsg = "Really Apply GUI Setup?",
				response = this.Confirm(requestmsg);
			if (!response)
				return;
		}

		let changes = this.EvaluateChanges();
		this.fDofiState.SetRegisters(changes).then(() => {
			this.AddLogText("Setup has been applied",true);
			this.UpdateElementsSize();
			this.SetStatusMessage("Applied setup data.");
			this.ClearChanges();
			return true;
		})
			.catch(() => {
				this.AddLogText("Apply setup failed",true);
			});



	}


	ResetDofi() {

		if (this.fDoCommandConfirm) {
			let requestmsg = "Really Reset DOFI spi logic??",
				response = this.Confirm(requestmsg);
			if (!response)
				return;
		}


		this.SetStatusMessage("Reset dofi logic.");
		return this.fDofiState.SingleCommand(DofiCommands.DOFI_RESET);


		return true;
	}

	ChangeScalers(enabled) {
		let state = "disable";
		if (enabled) state = "enable";

		if (this.fDoCommandConfirm) {
			let requestmsg = "Really " + state + " DOFI scalers?",
				response = this.Confirm(requestmsg);
			if (!response)
				return;
		}
		let com = (enabled ? DofiCommands.DOFI_ENABLE_SCALER :DofiCommands.DOFI_DISABLE_SCALER); 

		this.SetStatusMessage("changing dofi scalers to " + state + ".");		
		return this.fDofiState.SingleCommand(com);
	}


	ClearScalers() {

		if (this.fDoCommandConfirm) {
			let requestmsg = "Really Clear DOFI scalers?",
				response = this.Confirm(requestmsg);
			if (!response)
				return;
		}
		this.SetStatusMessage("Clearing dofi scalers.");
		return this.fDofiState.SingleCommand(DofiCommands.DOFI_RESET_SCALER);
	}


	ChangeMonitoring(on) {

		this.fMonitoring = on;
		if (on) {
			this.SetStatusMessage("Starting monitoring timer with " + this.fUpdateInterval + " ms.");
			this.fUpdateTimer = window.setInterval(function() { MyDisplay.RefreshMonitor(MyDisplay.fUpdateInterval) }, MyDisplay.fUpdateInterval);
		} else {
			window.clearInterval(this.fUpdateTimer);
			this.SetStatusMessage("Stopped monitoring timer.");
		}
		this.RefreshButtons();
	}



	SetCommandConfirm(on) {
		this.fDoCommandConfirm = on;
		this.RefreshButtons();
	}

RefreshButtons() {
	let dom = this.selectDom();
if (this.fMonitoring) {
			//$("#displaymode_container").addClass("styleGreen").removeClass("styleRed");
			dom.select("#displaymode_container").classed("styleGreen", true).classed("styleRed", false);
			$("label[for='Monitoring']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-stop MyButtonStyle\"></span>");
			$("label[for='Monitoring']").attr("title", "Stop frequent refresh");
			$("#Refreshtime").spinner("disable");


		} else {
			//$("#displaymode_container").addClass("styleRed").removeClass("styleGreen");
			dom.select("#displaymode_container").classed("styleRed", true).classed("styleGreen", false);
			$("label[for='Monitoring']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-play MyButtonStyle\"></span>");
			$("label[for='Monitoring']").attr("title", "Activate frequent refresh");
			$("#Refreshtime").spinner("enable");

		}





		if (this.fDoCommandConfirm) {
			$("label[for='ConfirmCommandToggle']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-comment MyButtonStyle\"></span>");
			$("label[for='ConfirmCommandToggle']").attr("title", "Command Confirmation Mode is ON");
		} else {
			$("label[for='ConfirmCommandToggle']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-alert MyButtonStyle\"></span>");
			$("label[for='ConfirmCommandToggle']").attr("title", "Command Confirmation Mode is OFF");
		}

		if (this.fDofiState.fVerbose) {
			$("label[for='VerboseBox']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-volume-on MyButtonStyle\"></span>");//
			$("label[for='VerboseBox']").attr("title", "VerboseMode is ON");
		} else {
			$("label[for='VerboseBox']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-volume-off MyButtonStyle\"></span>");
			$("label[for='VerboseBox']").attr("title", "VerboseMode is OFF");
		}

		if (this.fDofiState.fHexmode) {
			$("label[for='HexMode']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-caret-1-n MyButtonStyle\"></span>");//
			$("label[for='HexMode']").attr("title", "Hex Mode is ON");
		} else {
			$("label[for='HexMode']").html("<span class=\"ui-button-icon-primary ui-icon ui-icon-caret-1-s MyButtonStyle\"></span>");
			$("label[for='HexMode']").attr("title", "Hex Mode is OFF");
		}


}

	RefreshView() {

   	    this.RefreshButtons();		
		this.FillScalersTable();
		this.FillInputsTable();
		this.FillMatrixTables();
		this.ClearChanges();

	}


	SetStatusMessage(info) {
		let d = new Date();
		//txt = d.toLocaleString('de-DE') + "  >\n" + info;
		let tim = "Last refresh: " + d.toLocaleString('de-DE');
		document.getElementById("status_time").innerHTML = tim;
		document.getElementById("status_message").innerHTML = info;
	}


	AddLogText(info, withdate) {
		var ddd = "",txt="";
		let d;
		if(withdate)
		{
			d = new Date(),
			txt = d.toLocaleString('de-DE') + "  >" + info;
		}
		else
		{	
			txt=info;
		}
		ddd += "<pre>";
		ddd += txt;
		ddd += "</pre>";
		document.getElementById("logging").innerHTML += ddd;
		this.UpdateElementsSize();

	}


	UpdateElementsSize() {

		$("#content_log").scrollTop($("#content_log")[0].scrollHeight - $("#content_log").height());

		//~ $("#QFWModeCombo").selectmenu("option", "width", $('#QFW-table').width());
		//~ $("#DacModeCombo").selectmenu("option", "width",
		//~ $('#DAC-Cali-table').width());
		return true;
	}


	ClearLogWindow() {

		document.getElementById("logging").innerHTML = "Welcome to DOFI Web GUI!<br/>  -    v0.53, 17-May 2023 by S. Linev/ J. Adamzewski-Musch (JAM)<br/>";
		return this.UpdateElementsSize();



	}



	Confirm(msg) {
		if (this.fDoCommandConfirm)
			return (confirm(msg));
		else
			return true;
	}

	Dump() {

		this.SetStatusMessage("Dumping setup data:");

		this.fDofiState.Update().then(() => {
			this.AddLogText("Input Scalers:",true);
			for (var i = 0; i < DOFI_NUM_CHANNELS; i++) {
				this.AddLogText(" >" + i + ":" + this.fDofiState.fInputScalers[i] + " -",false);
			}
			this.AddLogText("Output Scalers:",true);
			for (var i = 0; i < DOFI_NUM_CHANNELS; i++) {
				this.AddLogText(" >" + i + ":" + this.fDofiState.fOutputScalers[i] + " -",false);
			}

			// missing:
			//~ this.fDofiState.fSignalControl[i];
			//~ this.fDofiState.fSignalControl[i]);
			//~ this.fDofiState.fOutputANDBits[i]);
			//~ this.fDofiState.fOutputORBits[i]);
			//~ this.fDofiState.fInputScalerBefore[i]);
			//~ this.fDofiState.fOutputScalersBefore[i]);


			this.UpdateElementsSize();
			return true;
		})
			.catch(() => { this.AddLogText("Dumping failed!",true); return false; })


		//~ .then(() => MyDisplay.SetStatusMessage("Init Acquisition (@startup) command sent."))
		//~ .catch(() => MyDisplay.SetStatusMessage("Init Acquisition FAILED."))

		return true;
	}


	Configure() {

		this.SetStatusMessage("Configuring...");

		var configfilename = prompt(
			"Please type name of configuration file (*.dof) on server"
			, "initlab.dof");
		if (!configfilename)
			return;

		this.SetStatusMessage("Apply configuration file " + configfilename + " ...");
		var res = false;
		var cmd = DofiCommands.DOFI_CONFIGURE;
		//cmd, address, value, repeat, hex, verbose, filename,
		this.fDofiState.DofiCommand(cmd, 0, 0, 0, 0, 0, configfilename)
			.then(() => { res = true })
			.catch(() => { res = false });

		this.SetStatusMessage("Configuration with " + configfilename
			+ (res ? " - OK" : " -Failed!"));

	}


	FillScalersTable() {
		let dom = this.selectDom();
		dom.selectAll(".scaler_values tbody").html("");

		for (let i = 0; i < DOFI_NUM_CHANNELS; i++) {
			if (MyDOFI.fHexmode) {
				dom.select(".scaler_values tbody").append("tr")
					.html(`<td class="scaler_channel" >${i}</td>
                               <td class='scaler_input' >0x${MyDOFI.fInputScalers[i].toString(16)}</td>
                                <td class='scaler_output'>0x${MyDOFI.fOutputScalers[i].toString(16)}</td>
                                <td class='scaler_inrate'>${MyDOFI.fInputRate[i]}</td>
                                <td class='scaler_outrate'>${MyDOFI.fOutputRate[i]}</td>`);
			}
			else {
				dom.select(".scaler_values tbody").append("tr")
					.html(`<td class="scaler_channel" >${i}</td>
                               <td class='scaler_input' >${MyDOFI.fInputScalers[i]}</td>
                                <td class='scaler_output'>${MyDOFI.fOutputScalers[i]}</td>
                                <td class='scaler_inrate'>${MyDOFI.fInputRate[i]}</td>
                                <td class='scaler_outrate'>${MyDOFI.fOutputRate[i]}</td>`);
			}
		} // for i
	}



	FillInputsTable() {
		let dom = this.selectDom();
		dom.selectAll(".input_signals tbody").html("");

		for (let i = 0; i < DOFI_NUM_CHANNELS; i++) {
			let checkstring = (MyDOFI.IsInputInvert(i) ? "checked" : "");
			// JAM 17-05-23: hexmode not working for all fields here. big numbers are invisible in spinbox? TODO
			/*if (MyDOFI.fHexmode) {


				dom.select(".input_signals tbody").append("tr")
					.html(`<td class="insignal_channel" >${i}</td>
                               <td class='insignal_invert'><input   type="checkbox" ${checkstring} class='in_invert_${i}></td>
                                <td class='insignal_delay'>0x<input   type="number" value="${MyDOFI.GetInputDelay(i).toString(16)}" class='in_delay_${i}' min="0" max="167772150" size="9"></td>
                                <td class='insignal_length'>0x<input  type="number" value="${MyDOFI.GetInputLength(i).toString(16)}" class='in_length_${i}'  min="0" max="167772150" size="9"</td>
                                `);

			}
			else {*/
				dom.select(".input_signals tbody").append("tr")
					.html(`<td class="insignal_channel" >${i}</td>
                               <td class='insignal_invert'> <input   type="checkbox" ${checkstring} class='in_invert_${i}'></td>
                                <td class='insignal_delay'><input   type="number" value="${MyDOFI.GetInputDelay(i)}" class='in_delay_${i}' min="0" max="167772150" size="9"></td>
                                <td class='insignal_length'><input  type="number" value="${MyDOFI.GetInputLength(i)}" class='in_length_${i}' min="0" max="167772150" size="9"</td>
                                `);
			//}
		} // for i

		
		dom.select(" .input_signals tbody").selectAll("input").on("change", function() { MyDisplay.MarkChanged(d3_select(this).attr('class')); });
		this.ClearChanges();
	}



	FillMatrixTables() {
		let dom = this.selectDom();

		dom.selectAll(".logic_matrix_or thead").html("");
		dom.selectAll(".logic_matrix_or tbody").html("");
		dom.select(".logic_matrix_or thead").append("tr").append("th").html("In");
		for (let j = 0; j < DOFI_NUM_CHANNELS; j++) {
			dom.select(".logic_matrix_or thead").select("tr").append("th").classed("outmatrix_channel", true).
				html(`${j}`);
		}

		for (let i = 0; i < DOFI_NUM_CHANNELS; i++) {
			dom.select(".logic_matrix_or tbody").append("tr").classed(`yheader_or_${i}`, true).append("td").classed("inmatrix_channel", true).html(`${i}`);

			for (let j = 0; j < DOFI_NUM_CHANNELS; j++) {
				let checkstring = (MyDOFI.IsOutputOR(j, i) ? "checked" : "");
				//let checkstring="checked";
				//dom.select(".logic_matrix_and tbody")
				dom.select(`.yheader_or_${i}`).insert("td").classed("matrix_check", true)
					.html(`<input type="checkbox" ${checkstring} class="matrix_or_${i}-${j}">`);

			}
		}
	dom.select(" .logic_matrix_or tbody").selectAll("input").on("change", function() { MyDisplay.MarkChanged(d3_select(this).attr('class')); });




		dom.selectAll(".logic_matrix_and thead").html("");
		dom.selectAll(".logic_matrix_and tbody").html("");
		dom.select(".logic_matrix_and thead").append("tr").append("th").html("In");
		for (let j = 0; j < DOFI_NUM_CHANNELS; j++) {
			dom.select(".logic_matrix_and thead").select("tr").append("th").classed("outmatrix_channel", true).
				html(`${j}`);
		}

		for (let i = 0; i < DOFI_NUM_CHANNELS; i++) {
			dom.select(".logic_matrix_and tbody").append("tr").classed(`yheader_and_${i}`, true).append("td").classed("inmatrix_channel", true).html(`${i}`);

			for (let j = 0; j < DOFI_NUM_CHANNELS; j++) {
				let checkstring = (MyDOFI.IsOutputAND(j, i) ? "checked" : "");
				//let checkstring="checked";
				dom.select(`.yheader_and_${i}`).insert("td").classed("matrix_check", true)
					.html(`<input type="checkbox" ${checkstring} class="matrix_and_${i}-${j}">`);

			}
		}


		dom.select(" .logic_matrix_and tbody").selectAll("input").on("change", function() { MyDisplay.MarkChanged(d3_select(this).attr('class')); });

	}

}
// class DofiState

////////////////////////////////////////////////////////////////////////////
//////// the main() :
MyDOFI = new DofiState();
MyDisplay = new DofiDisplay("dofi_top_frame", MyDOFI);
MyDisplay.ClearLogWindow();
MyDisplay.SetStatusMessage("Started DOFI GUI");



//MyDisplay.ChangeMonitoring(false);
///////////////////////////// DOFI specific, still jquery for the moment:

$("#HexMode").button().click(function() {
	MyDisplay.fDofiState.fHexmode = $(this).is(':checked');
	MyDisplay.RefreshView(true); // refresh everything with new number base
});

$("#VerboseBox").button({showLabel: true, icon: " ui-icon-volume-off MyButtonStyle" }).click(function() {
	MyDisplay.fDofiState.fVerbose = $(this).is(':checked');
	MyDisplay.RefreshButtons(); // refresh everything with new number base
});

$("#buttonShow").button({ showLabel: true, icon: "ui-icon-refresh MyButtonStyle" }).click(function() {
	MyDisplay.SetStatusMessage("Get dofi registers");
	MyDisplay.RefreshAll();
});

$("#buttonApply").button({ showLabel: true, icon: " ui-icon-arrowthick-1-w MyButtonStyle" })
	.click(
		function() {
			MyDisplay.ApplySetup();
		});

$("#buttonConfigure").button({ showLabel: true, icon: " ui-icon-folder-open MyButtonStyle" })
	.click(function() {
		MyDisplay.Configure();
	});

$("#buttonDataDump")
	.button({ showLabel: true, icon: " ui-icon-document MyButtonStyle" })
	.click(
		function() {
			MyDisplay.Dump();
		});

$("#buttonClear")
	.button({ showLabel: true, icon: "ui-icon-trash MyButtonStyle" })
	.click(
		function() {
			MyDisplay.ClearLogWindow();
		});



$("#buttonReset").button({ showLabel: false, icon: "ui-icon-wrench MyButtonStyle" }).click(function() {
	MyDisplay.ResetDofi();
});

$("#buttonScalerClear").button({ showLabel: false, icon: "ui-icon-arrowreturnthick-1-s" }).click(function() {
	MyDisplay.ClearScalers();
});



$("#buttonScalerStop").button({ showLabel: false, icon: "ui-icon-stop MyButtonStyle" }).click(function() {
	MyDisplay.ChangeScalers(false);
});

$("#buttonScalerGo").button({ showLabel: false, icon: "ui-icon-seek-next  MyButtonStyle" }).click(function() {
	MyDisplay.ChangeScalers(true);
});




///////////////////////////////////////////////////////// JAM23 for monitor timer here:
$("#Monitoring").checkboxradio({ icon: false })
	.click(function() {
		MyDisplay.fUpdateInterval = 1000 * parseInt(document.getElementById("Refreshtime").value);
		MyDisplay.ChangeMonitoring($(this).is(':checked'));
	});
//////////////////////////


$("#ConfirmCommandToggle").checkboxradio({ icon: false }).click(
	function() {
		let doconfirm = $('#ConfirmCommandToggle').is(':checked');
		MyDisplay.SetCommandConfirm(doconfirm);
		//console.log("Command confirm is " + doconfirm);

	});


$('#Refreshtime').spinner({
	min: 1,
	max: 120,
	step: 1
}).val(MyDisplay.fUpdateInterval / 1000);


MyDisplay.RefreshView();


$(document).tooltip();



//});
