(function(){

   if (typeof DABC == "object") {
      var e1 = new Error("DABC is already defined");
      e1.source = "dabc.js";
      throw e1;
   }

   if (typeof JSROOT != "object") {
      var e1 = new Error("dabc.js requires JSROOT to be already loaded");
      e1.source = "dabc.js";
      throw e1;
   }
   
   JSROOT.gStyle.DragAndDrop = true;

   DABC = {};

   DABC.version = "2.7.1";
   
   DABC.loadGauge = false;
   
   DABC.hpainter = null;  // hierarchy painter
   
   DABC.source_dir = function(){
      var scripts = document.getElementsByTagName('script');

      for (var n in scripts) {
         if (scripts[n]['type'] != 'text/javascript') continue;

         var src = scripts[n]['src'];
         if ((src == null) || (src.length == 0)) continue;

         var pos = src.indexOf("dabc.js");
         if (pos<0) continue;
         
         console.log("Set DABC.source_dir to " + src.substr(0, pos));
         return src.substr(0, pos);
      }
      return "";
   }(); 

   // =============================================================================

   DABC.DrawElement = function() {
      JSROOT.TBasePainter.call(this);
      this.itemname = "";               // full item name in hierarchy
      this.version = new Number(-1);    // check which version of element is drawn
      this.frameid = "";                // frame id of HTML element (ion most cases <div>), where object drawn
      this.is_dabc = true;              // indicate that this is dabc painter
      return this;
   }

   DABC.DrawElement.prototype = Object.create( JSROOT.TBasePainter.prototype );

// method called when item is activated (clicked)
// each item can react by itself
   DABC.DrawElement.prototype.ClickItem = function() { return; }

// method regularly called by the manager
// it is responsibility of item perform any action
   DABC.DrawElement.prototype.RegularCheck = function() { return; }

   DABC.DrawElement.prototype.NewHttpRequest = function(url, kind) {
      var item = this;
      return JSROOT.NewHttpRequest(url, kind, function(res) { item.RequestCallback(res); }); 
   }
   
   DABC.DrawElement.prototype.MonitoringEnabled = function() {
      if ('monitoring' in this) return this.monitoring; 

      var chkbox = document.getElementById("monitoring");
      return chkbox ? chkbox.checked : false;
   } 

   DABC.DrawElement.prototype.CreateFrames = function(frame) {
      this.frameid = $(frame).attr('id') + "_dummy";

      var entryInfo = 
         "<div id='" +this.frameid + "'>" + 
         "<h2> CreateFrames for item " + this.itemname + " not implemented </h2>"+
         "</div>";
      $(frame).empty();
      $(frame).append(entryInfo);
      
      this.SetDivId($(frame).attr('id'));
   }

   DABC.DrawElement.prototype.IsDrawn = function() {
      if (!this.frameid) return false;
      if (!document.getElementById(this.frameid)) return false;
      return true;
   }

   DABC.DrawElement.prototype.Clear = function() {
      // one should remove here all running requests
      // delete objects
      // remove GUI

      if (this.frameid.length > 0) {
         var elem = document.getElementById(this.frameid);
         if (elem!=null) elem.parentNode.removeChild(elem);
      }

      this.itemname = "";
      this.frameid = 0; 
      this.version = -1;
      this.frameid = "";
   }


   DABC.DrawElement.prototype.FullItemName = function() {
      // method should return absolute path of the item
      if ((this.itemname.length > 0) && (this.itemname[0] == '/')) return this.itemname;

      var curpath = document.location.pathname;
      // if (curpath.length == 0) curpath = document.location.pathname;

      return curpath + this.itemname; 
   }

// ========= start of CommandDrawElement

   DABC.CommandDrawElement = function() {
      DABC.DrawElement.call(this);
      this.req = null;
      this.jsonnode = null; // here is xml description of command, which should be first requested
      return this;
   }

   DABC.CommandDrawElement.prototype = Object.create( DABC.DrawElement.prototype );

   DABC.CommandDrawElement.prototype.CreateFrames = function(frame) {
      this.frameid = $(frame).attr('id') + "_command";

      var entryInfo = "<div id='" + this.frameid + "'/>";

      $(frame).empty();
      $(frame).append(entryInfo);
      
      this.SetDivId($(frame).attr('id'));

      this.ShowCommand();
      this.RequestCommand();
   }

   DABC.CommandDrawElement.prototype.NumArgs = function() {
      if (this.jsonnode==null) return 0;
      return this.jsonnode["numargs"];
   }

   DABC.CommandDrawElement.prototype.ArgName = function(n) {
      if (n>=this.NumArgs()) return "";
      return this.jsonnode["arg"+n];
   }

   DABC.CommandDrawElement.prototype.ArgKind = function(n) {
      if (n>=this.NumArgs()) return "";
      return this.jsonnode["arg"+n+"_kind"];
   }

   DABC.CommandDrawElement.prototype.ArgDflt = function(n) {
      if (n>=this.NumArgs()) return "";
      return this.jsonnode["arg"+n+"_dflt"];
   }

   DABC.CommandDrawElement.prototype.ArgMin = function(n) {
      if (n>=this.NumArgs()) return null;
      return this.jsonnode["arg"+n+"_min"];
   }

   DABC.CommandDrawElement.prototype.ArgMax = function(n) {
      if (n>=this.NumArgs()) return null;
      return this.jsonnode["arg"+n+"_max"];
   }

   DABC.CommandDrawElement.prototype.ShowCommand = function() {

      var frame = $("#" + this.frameid);

      frame.empty();

      frame.append("<h3>" + this.FullItemName() + "</h3>");

      if (this.jsonnode==null) {
         frame.append("request command definition...<br/>");
         return;
      } 

      var entryInfo = "<input id='" + this.frameid + "_button' type='button' title='Execute' value='Execute'/><br/>";

      for (var cnt=0;cnt<this.NumArgs();cnt++) {
         var argname = this.ArgName(cnt);
         var argkind = this.ArgKind(cnt);
         var argdflt = this.ArgDflt(cnt);

         var argid = this.frameid + "_arg" + cnt; 
         var argwidth = (argkind=="int") ? "80px" : "170px";

         entryInfo += "Arg: " + argname + " "; 
         entryInfo += "<input id='" + argid + "' style='width:" + argwidth + "' value='"+argdflt+"' argname = '" + argname + "'/>";    
         entryInfo += "<br/>";
      }

      entryInfo += "<div id='" +this.frameid + "_res'/>";

      frame.append(entryInfo);
      
      var pthis = this;
      
      $("#"+ this.frameid + "_button").click(function() { pthis.InvokeCommand(); });

      for (var cnt=0;cnt<this.NumArgs();cnt++) {
         var argid = this.frameid + "_arg" + cnt;
         var argkind = this.ArgKind(cnt);
         var argmin = this.ArgMin(cnt);
         var argmax = this.ArgMax(cnt);

         if ((argkind=="int") && (argmin!=null) && (argmax!=null))
            $("#"+argid).spinner({ min:argmin, max:argmax});
      }
   }


   DABC.CommandDrawElement.prototype.Clear = function() {

      DABC.DrawElement.prototype.Clear.call(this);

      if (this.req) this.req.abort(); 
      this.req = null;          
   }

   DABC.CommandDrawElement.prototype.ClickItem = function() {
   }

   DABC.CommandDrawElement.prototype.RegularCheck = function() {
   }

   DABC.CommandDrawElement.prototype.RequestCommand = function() {
      if (this.req) return;

      var url = this.itemname + "/get.json";

      this.req = this.NewHttpRequest(url, "text");

      this.req.send(null);
   }

   DABC.CommandDrawElement.prototype.InvokeCommand = function() {
      if (this.req) return;

      var resdiv = $("#" + this.frameid + "_res");
      if (resdiv) {
         resdiv.empty();
         resdiv.append("<h5>Send command to server</h5>");
      }

      var url = this.itemname + "/execute";

      for (var cnt=0;cnt<this.NumArgs();cnt++) {
         var argid = this.frameid + "_arg" + cnt;
         var argkind = this.ArgKind(cnt);
         var argmin = this.ArgMin(cnt);
         var argmax = this.ArgMax(cnt);

         var arginp = $("#"+argid);

         if (cnt==0) url+="?"; else url+="&";

         url += arginp.attr("argname");
         url += "=";
         if ((argkind=="int") && (argmin!=null) && (argmax!=null))
            url += arginp.spinner("value");
         else
            url += new String(arginp.val());
      }

      this.req = this.NewHttpRequest(url, "text");

      this.req.send(null);
   }

   DABC.CommandDrawElement.prototype.RequestCallback = function(arg) {
      // in any case, request pointer will be reseted
      // delete this.req;
      this.req = null;

      if (arg==null) {
         console.log("no response from server");
         return;
      }

      if (this.jsonnode==null) {
         this.jsonnode = JSON.parse(arg);
         this.ShowCommand();
         return;
      }

      var reply = JSON.parse(arg);
      if (typeof reply != 'object') {
         console.log("non-object in json response from server");
         return;
      }

      var resdiv = $("#" + this.frameid + "_res");
      if (resdiv) {
         resdiv.empty();
         resdiv.append("<h5>Get reply res=" + reply['_Result_'] + "</h5>");
      }
   }


// ========== start of HistoryDrawElement

   DABC.HistoryDrawElement = function(_clname) {
      DABC.DrawElement.call(this);

      this.clname = _clname;
      this.req = null;           // request to get raw data
      this.jsonnode = null;      // json object with current values 
      this.history = null;       // array with previous history entries
      this.hlimit = 0;           // maximum number of history entries
      this.force = true;
      this.request_name = "get.json";

      return this;
   }

   DABC.HistoryDrawElement.prototype = Object.create( DABC.DrawElement.prototype );

   DABC.HistoryDrawElement.prototype.EnableHistory = function(hlimit) {
      this.hlimit = hlimit;
   }

   DABC.HistoryDrawElement.prototype.isHistory = function() {
      return this.hlimit > 0;
   }

   DABC.HistoryDrawElement.prototype.Clear = function() {

      DABC.DrawElement.prototype.Clear.call(this);

      this.jsonnode = null;      // json object with current values 
      this.history = null;      // array with previous history
      this.hlimit = 100;         // maximum number of history entries  
      if (this.req) this.req.abort(); 
      this.req = null;          // this is current request
      this.force = true;
   }

   DABC.HistoryDrawElement.prototype.ClickItem = function() {
      if (this.req != null) return; 

      this.force = true;

      this.RegularCheck();
   }

   DABC.HistoryDrawElement.prototype.RegularCheck = function() {

      // no need to do something when req not completed
      if (this.req!=null) return;

      // do update when monitoring enabled
      if ((this.version >= 0) && !this.force) {
         if (!this.MonitoringEnabled()) return;
      }

      var url = "";
      if (this.itemname != "") url += this.itemname +"/";
      url += this.request_name + "?compact=0";

      if (this.version>0) url += "&version=" + this.version; 
      if (this.hlimit>0) url += "&history=" + this.hlimit;
      this.req = this.NewHttpRequest(url, "text");

      this.req.send(null);

      this.force = false;
   }

   DABC.HistoryDrawElement.prototype.ExtractField = function(name, kind, node) {

      if (!node) node = this.jsonnode;    
      if (!node || !(name in node)) return;

      if (kind=="number") return Number(node[name]); 
      if (kind=="time") {
         var d  = new Date(node[name]);
         return d.getTime() / 1000.;
      }

      return node[name];
   }

   DABC.HistoryDrawElement.prototype.ExtractSeries = function(name, kind) {

      // xml node must have attribute, which will be extracted
      var val = this.ExtractField(name, kind, this.jsonnode);
      if (val==null) return;

      var arr = new Array();
      arr.push(val);

      if (this.history) 
         for (var n=this.history.length-1;n>=0;n--) {
            var newval = this.ExtractField(name, kind, this.history[n]);
            if (newval!=null) val = newval;
            arr.push(val);
         }

      arr.reverse();
      return arr;
   }


   DABC.HistoryDrawElement.prototype.RequestCallback = function(arg) {
      // in any case, request pointer will be reseted
      // delete this.req;
      this.req = null;

      if (arg==null) {
         console.log("no response from server");
         return;
      }

      var top = JSON.parse(arg);

//    console.log("Get request callback " + top["_version"]);

      var new_version = Number(top["_version"]);

      var modified = (this.version != new_version);

      this.version = new_version;

      // this is xml node with all fields
      this.jsonnode = top;

      if (this.jsonnode == null) {
         console.log("Did not found node with item attributes");
         return;
      }

      // this is array with history entries 
      var arr = this.jsonnode["history"];

      if ((arr!=null) && (this.hlimit>0)) {

         // gap indicates that we could not get full history relative to provided version number
         var gap = this.jsonnode["history_gap"];

         // join both arrays with history entries
         if ((this.history == null) || (arr.length >= this.hlimit) || gap) {
            this.history = arr;
         } else
            if (arr.length>0) {
               modified = true;
               var total = this.history.length + arr.length; 
               if (total > this.hlimit) 
                  this.history.splice(0, total - this.hlimit);

               this.history = this.history.concat(arr);
            }

         // console.log("History length = " + this.history.length);
      }

      if (modified) this.DrawHistoryElement();
   }


   DABC.HistoryDrawElement.prototype.DrawHistoryElement = function()
   {
      // should be implemented in derived class
      alert("HistoryDrawElement.DrawHistoryElement not implemented for item " + this.itemname);
   }

// ======== end of DrawElement ======================

// ======== start of GaugeDrawElement ======================

   DABC.GaugeDrawElement = function() {
      DABC.HistoryDrawElement.call(this, "gauge");
      this.gauge = 0;
   }

   DABC.GaugeDrawElement.prototype = Object.create( DABC.HistoryDrawElement.prototype );

   DABC.GaugeDrawElement.prototype.CreateFrames = function(frame) {

      this.frameid = $(frame).attr('id') + "_gauge";
      this.min = 0;
      this.max = 10;
      this.gauge = null;

//    var entryInfo = "<div id='"+this.frameid+ "' class='200x160px'> </div> \n";
      var entryInfo = "<div id='"+this.frameid+ "'/>";
      $(frame).empty();
      $(frame).append(entryInfo);
      
      this.SetDivId($(frame).attr('id'));
   }

   DABC.GaugeDrawElement.prototype.DrawHistoryElement = function() {
      if (DABC.loadGauge) return this.DrawGauge();
      var element = this;
      JSROOT.loadScript(DABC.source_dir + 'raphael.2.1.0.min.js;' + 
                        DABC.source_dir + 'justgage.1.0.1.min.js', function() {
         DABC.loadGauge = true;
         element.DrawGauge();
      });
   }
  
   
   DABC.GaugeDrawElement.prototype.DrawGauge = function() {

      var val = this.ExtractField("value", "number");
      var min = this.ExtractField("min", "number");
      var max = this.ExtractField("max", "number");

      if (max!=null) this.max = max; 
      if (min!=null) this.min = min; 

      if (val > this.max) {
         if (this.gauge!=null) {
            this.gauge = null;
            $("#" + this.frameid).empty();
         }
         this.max = 1;
         var cnt = 0;
         while (val > this.max) 
            this.max *= (((cnt++ % 3) == 1) ? 2.5 : 2);
      }

      if (this.gauge==null) {
         this.gauge = new JustGage({
            id: this.frameid, 
            value: val,
            min: this.min,
            max: this.max,
            title: this.FullItemName()
         });
      } else {
         this.gauge.refresh(val);
      }
   }

// ======== end of GaugeDrawElement ======================

// ======== start of ImageDrawElement ======================

   DABC.ImageDrawElement = function() {
      DABC.DrawElement.call(this);
      this.imgcnt = 0;
   }

   DABC.ImageDrawElement.prototype = Object.create( DABC.DrawElement.prototype );

   DABC.ImageDrawElement.prototype.CreateFrames = function(frame) {

      this.topid = $(frame).attr('id');
      this.frameid = this.topid + "_image";
      
      this.DrawImage();
   }

   DABC.ImageDrawElement.prototype.RegularCheck = function() {
      if (this.MonitoringEnabled()) 
         this.DrawImage();
   }
   
   DABC.ImageDrawElement.prototype.DrawImage = function() {
      $("#"+this.topid).empty();

      var width = parseInt($("#"+this.topid).width());
      var height = parseInt($("#"+this.topid).height());
      if (height < 10) height = width*2/3;

      var url = this.itemname + "/root.png.gz?w=" + width + "&h=" + height + "&opt=col&dummy=" + this.imgcnt++;
      var entryInfo = 
         "<div id='"+this.frameid+ "'>" +
         "<img src='" + url + "' alt='some text' width='" + width + "'>" + 
         "</div>";
      $("#"+this.topid).append(entryInfo);
      
      this.SetDivId(this.topid);
   }

// ======== end of ImageDrawElement ======================


// ======== start of LogDrawElement ======================

   DABC.LogDrawElement = function() {
      DABC.HistoryDrawElement.call(this,"log");
   }

   DABC.LogDrawElement.prototype = Object.create( DABC.HistoryDrawElement.prototype );

   DABC.LogDrawElement.prototype.CreateFrames = function(frame) {
      this.frameid = $(frame).attr('id') + "_log";
      var entryInfo;
      if (this.isHistory()) {
         // var w = $(topid).width();
         var h = $(frame).height();
         var maxhstyle = "";
         if (h>10) maxhstyle = "; max-height:" + h + "px"; 
         entryInfo = "<div id='" + this.frameid + "' style='overflow:auto; font-family:monospace" + maxhstyle + "'/>";
      } else {
         entryInfo = "<div id='"+this.frameid+ "'/>";
      }
      $(frame).empty();
      $(frame).append(entryInfo);
      this.SetDivId($(frame).attr('id'));
   }

   DABC.LogDrawElement.prototype.DrawHistoryElement = function() {
      var element = $("#" + this.frameid);
      element.empty();

      if (this.isHistory()) {
         var txt = this.ExtractSeries("value","string");
         for (var i in txt)
            element.append("<PRE>"+txt[i]+"</PRE>");
      } else {
         var val = this.ExtractField("value");
         element.append(this.FullItemName() + "<br/>");
         element.append("<h5>"+val +"</h5>");
      }
   }

// ========= start of GenericDrawElement ===========================


   DABC.GenericDrawElement = function() {
      DABC.HistoryDrawElement.call(this,"generic");
      this.recheck = false;   // indicate that we want to redraw 
   }

   DABC.GenericDrawElement.prototype = Object.create( DABC.HistoryDrawElement.prototype );

   DABC.GenericDrawElement.prototype.CreateFrames = function(frame) {
      this.frameid = $(frame).attr('id') + "_generic";
      var entryInfo = "<div id='"+this.frameid+ "'/>";
      $(frame).append(entryInfo);
      this.SetDivId($(frame).attr('id'));
   }

   DABC.GenericDrawElement.prototype.DrawHistoryElement = function() {
      if (this.recheck) {
         console.log("here one need to draw element with real style " + this.FullItemName());
         this.recheck = false;

         if ('_kind' in this.jsonnode) {
            var itemname = this.itemname;
            var jsonnode = this.jsonnode;
            if (DABC.hpainter) {
               DABC.hpainter.clear();
               DABC.hpainter.display(itemname, "", jsonnode);
            }
            return;
         }
      }

      var element = $("#" + this.frameid);
      element.empty();
      element.append(this.FullItemName() + "<br/>");

      var ks = Object.keys(this.jsonnode);
      for (i = 0; i < ks.length; i++) {
         k = ks[i];
         element.append("<h5><PRE>" + ks[i] + " = " + this.jsonnode[ks[i]] + "</PRE></h5>");
      }
   }


  // ================ start of FesaDrawElement

   DABC.FesaDrawElement = function(_clname) {
      DABC.DrawElement.call(this);
      this.clname = _clname;     // FESA class name
      this.data = null;          // raw data
      this.root_obj = null;      // root object, required for draw
      this.root_painter = null;  // root object, required for draw
      this.req = null;           // request to get raw data
   }

   DABC.FesaDrawElement.prototype = Object.create( DABC.DrawElement.prototype );

   DABC.FesaDrawElement.prototype.Clear = function() {

      DABC.DrawElement.prototype.Clear.call(this);

      this.clname = "";         // ROOT class name
      this.root_obj = null;     // object itself, for streamer info is file instance
      this.root_painter = null;
      this.data = null;          // raw data
      if (this.req) this.req.abort(); 
      this.req = null;          // this is current request
      this.force = true;
   }

   DABC.FesaDrawElement.prototype.CreateFrames = function(frame) {
      this.frameid = $(frame).attr("id") + "_fesa";

      var entryInfo = "<div id='" + this.frameid + "'/>";
      $(frame).empty();
      $(frame).append(entryInfo);
      this.SetDivId($(frame).attr('id'));
   }

   DABC.FesaDrawElement.prototype.ClickItem = function() {
      if (this.req != null) return; 

      this.force = true;

      this.RegularCheck();
   }

   DABC.FesaDrawElement.prototype.RegularCheck = function() {

      // no need to do something when req not completed
      if (this.req!=null) return;

      // do update when monitoring enabled
      if ((this.version >= 0) && !this.force) {
         if (!this.MonitoringEnabled()) return;
      }

      var url = this.itemname + "/dabc.bin";

      if (this.version>0) url += "?version=" + this.version;

      this.req = this.NewHttpRequest(url, "bin");

      this.req.send(null);

      this.force = false;
   }


   DABC.FesaDrawElement.prototype.RequestCallback = function(arg) {
      // in any case, request pointer will be reseted
      // delete this.req;

      var bversion = new Number(0);
      if (this.req != 0) {
         var str = this.req.getResponseHeader("BVersion");
         if (str != null) {
            bversion = new Number(str);
            console.log("FESA data version is " + bversion);
         }
      }

      this.req = null;

      // console.log("Get response from server " + arg.length);

      if (this.version == bversion) {
         console.log("Same version of beam profile " + bversion);
         return;
      }

      if (arg == null) {
         alert("no data for beamprofile when expected");
         return;
      } 

      this.data = arg;
      this.version = bversion;

      console.log("FESA data length is " + this.data.length);

      var title = this.FullItemName() + "\nversion = " + this.version + ", size = " + this.data.length

      if (!this.ReconstructObject()) return;

      if (this.root_painter != null) {
         this.root_painter.RedrawPad();
      } else {
         this.root_painter = JSROOT.draw(this.frameid, this.root_obj);
      }
      $("#" + this.frameid).prop("title", title);
   }

   DABC.FesaDrawElement.prototype.ntou4 = function(b, o) {
      // convert (read) four bytes of buffer b into a uint32_t
      var n  = (b.charCodeAt(o)   & 0xff);
      n += (b.charCodeAt(o+1) & 0xff) << 8;
      n += (b.charCodeAt(o+2) & 0xff) << 16;
      n += (b.charCodeAt(o+3) & 0xff) << 24;
      return n;
   }

   DABC.FesaDrawElement.prototype.ReconstructObject = function()
   {
      if (this.clname != "2D") return false;

      if (this.root_obj == null) {
         this.root_obj = JSROOT.CreateTH2(16, 16);
         this.root_obj['fName']  = "BeamProfile";
         this.root_obj['fTitle'] = "Beam profile from FESA";
         this.root_obj['fOption'] = "col";
//       console.log("Create histogram");
      }

      if ((this.data==null) || (this.data.length != 16*16*4)) {
         alert("no proper data for beam profile");
         return false;
      }

      var o = 0;
      for (var iy=0;iy<16;iy++)
         for (var ix=0;ix<16;ix++) {
            var value = this.ntou4(this.data, o); o+=4;
            var bin = this.root_obj.getBin(ix+1, iy+1);
            this.root_obj.setBinContent(bin, value);
//          if (iy==5) console.log("Set content " + value);
         }

      return true;
   }

   // ========== start of RateHistoryDrawElement

   DABC.RateHistoryDrawElement = function() {
      DABC.HistoryDrawElement.call(this, "rate");
      this.root_painter = null;  // root painter, required for draw
      this.EnableHistory(100);
   }

   DABC.RateHistoryDrawElement.prototype = Object.create( DABC.HistoryDrawElement.prototype );

   DABC.RateHistoryDrawElement.prototype.Clear = function() {

      DABC.HistoryDrawElement.prototype.Clear.call(this);
      this.root_painter = null;  // root painter, required for draw
   }

   DABC.RateHistoryDrawElement.prototype.CreateFrames = function(frame) {

      this.frameid = $(frame).attr('id') + "_rate_history";

      var entryInfo = "<div id='" + this.frameid + "' style='width:100%; height: 100%'/>";
      $(frame).empty();
      $(frame).append(entryInfo);
      this.SetDivId($(frame).attr('id'));
   }

   DABC.RateHistoryDrawElement.prototype.DrawHistoryElement = function() {
      var title = this.itemname + "\nversion = " + this.version;
      if (this.history) title += ", history = " + this.history.length;

      var x = this.ExtractSeries("time", "time");
      var y = this.ExtractSeries("value", "number");

      if (x.length==1) {
         console.log("duplicate single point at time " + x[0]);
         x.push(x[0]+1);
         y.push(y[0]);
      } 

      // here we should create TGraph object

      var gr = JSROOT.CreateTGraph();
      gr['fLineColor'] = 2;
      gr['fLineWidth'] = 2;

      gr['fX'] = x;
      gr['fY'] = y;
      gr['fNpoints'] = x.length;

      JSROOT.AdjustTGraphRanges(gr);

      gr['fHistogram']['fTitle'] = this.FullItemName();
      if (gr['fHistogram']['fYaxis']['fXmin']>0)
         gr['fHistogram']['fYaxis']['fXmin'] = 0;
      else
         gr['fHistogram']['fYaxis']['fXmin'] *= 1.2;

      gr['fHistogram']['fYaxis']['fXmax'] *= 1.2;

      gr['fHistogram']['fXaxis']['fTimeDisplay'] = true;
      gr['fHistogram']['fXaxis']['fTimeFormat'] = "%H:%M:%S%F0"; // %FJanuary 1, 1970 00:00:00

      if (this.root_painter && this.root_painter.UpdateObject(gr)) {
         this.root_painter.RedrawPad();
      } else {
         this.root_painter = JSROOT.draw(this.frameid, gr, "L");
      }
      
      $("#" + this.frameid).prop("title", title);
   }
   
   DABC.RateHistoryDrawElement.prototype.CheckResize = function(force) {
      if (this.root_painter) this.root_painter.CheckResize(force);
   }

   DABC.CreateDrawElement = function(node, history_depth) {
      var kind = node["_kind"];
      var view = node["_view"];
      var history = node["_history"];
      if (!kind) kind = "";

      if (view == "png") 
         return new DABC.ImageDrawElement();
      
      // ROOT classes not supported
      if (kind.indexOf("ROOT.") == 0) return null;

      if (kind == "DABC.Command")
         return new DABC.CommandDrawElement();
      
      if (kind == "rate") { 
         if ((history_depth < 0) || !history)
            return new DABC.GaugeDrawElement();
         else 
            return new DABC.RateHistoryDrawElement();
      }
      
      if (kind == "log") {
         var elem = new DABC.LogDrawElement();
         if (history) {
            if (history_depth==0) elem.EnableHistory(100); else
            if (history_depth>0) elem.EnableHistory(history_depth);
         }
            
         return elem;
      }
      
      if (kind.indexOf("FESA.") == 0) 
         return new DABC.FesaDrawElement(kind.substr(5));
      
      return new DABC.GenericDrawElement();
   }
   
   // ============================================================

   DABC.HierarchyPainter = function(name, frameid) {
      JSROOT.HierarchyPainter.call(this, name, frameid);
   }

   DABC.HierarchyPainter.prototype = Object.create(JSROOT.HierarchyPainter.prototype);
   
   DABC.HierarchyPainter.prototype.ForEach = function(callback) {
      var ready = false;
      
      function ScanLevel(h, parent) {
         if (ready || (h==null)) return;
         if ((parent!=null) && !('_parent' in h)) h['_parent'] = parent;
         ready = callback(h);
         if (!ready && ('_childs' in h))
            for (var i in h['_childs'])
               ScanLevel(h['_childs'][i], h);
      }
      
      ScanLevel(this.h, null);
   }
   

   DABC.HierarchyPainter.prototype.CheckCanDo = function(node) {

      var cando = JSROOT.HierarchyPainter.prototype.CheckCanDo.call(this, node); 

      var kind = node["_kind"];
      var view = node["_view"];
      if (!kind) kind = "";

      if (view == "png") { cando.img1 = 'img_dabicon'; cando.display = true; } else
      if (kind == "rate") { cando.display = true; } else
      if (kind == "log") { cando.display = true; } else
      if (kind.indexOf("FESA.") == 0) { cando.display = true; } else         
      if (kind == "DABC.HTML") { cando.img1 = "img_globe"; cando.html = this.itemFullName(node) + "/"; cando.open = true; } else
      if (kind == "DABC.Application") cando.img1 = 'img_dabicon'; else
      if (kind == "DABC.Command") { cando.img1 = 'img_dabicon'; cando.display = true; cando.scan = false; } else
      if (kind == "GO4.Analysis") cando.img1 = 'img_go4icon'; else
      if (kind == "ROOT.TGo4AnalysisStatus") cando.img1 = 'img_go4icon';
      
      if ('_editor' in node) { cando.ctxt = true; cando.display = true; }
      if ('_player' in node) cando.display = true;
      
      return cando;
   }
   
   DABC.HierarchyPainter.prototype.CompleteOnline = function(ready_callback) {
      var scripts = "";

      this.ForEach(function(h) {
         if ('_autoload' in h)
            if (scripts.indexOf(h['_autoload'])<0) 
               scripts += h['_autoload'] + ";";
      });

      var painter = this;
      
      JSROOT.loadScript(scripts, function() {
         
         painter.ForEach(function(h) {
            if (!('_drawfunc' in h)) return;
            if (h._kind.indexOf('ROOT.')!=0) return;
            
            var typename = h._kind.slice(5);
            if (JSROOT.canDraw(typename)) return;
            
            var func = JSROOT.findFunction(h['_drawfunc']);
            if (func) JSROOT.addDrawFunc(typename, func);
         });   
         ready_callback();
      });
   }

   DABC.HierarchyPainter.prototype.RefreshHtml = function(force) {
      
      JSROOT.HierarchyPainter.prototype.RefreshHtml.call(this, force);

      $("#fast_buttons").empty();

      if (this.h==null) return;
      
      var cnt = 0;
      var painter = this;
      var statusitem = null, statusfuncname = null;
      
      this.ForEach(function(h) {
         if ((statusitem==null) && ('_status' in h)) {
            statusitem = painter.itemFullName(h);
            statusfuncname = h['_status'];
            return;
         }
      
         if (h['_kind'] != "DABC.Command") return;
         if (!('_fastcmd' in h)) return;
               
         var fullname = painter.itemFullName(h);
               
         cnt++;
         var html = "<button id='dabc_fastbtn_" + cnt + "' title='" + h['_name'] + "' item='" + fullname + "'></button>";
         $("#fast_buttons").append(html);
         $("#dabc_fastbtn_"+cnt)
               .text("")
               .append('<img height="16" src="' + h['_fastcmd'] + '" width="16"/>')
               .button()
               .click(function() { painter.ExecuteCommand($(this).attr('item')); });
      });
      
      if ((statusitem!=null) && (JSROOT.GetUrlOption('nostatus')==null)) {
         var func = JSROOT.findFunction(statusfuncname);
         if (func && $('#status-div').empty()) {
            this.CreateStatus(28);
            func('status-div', statusitem); 
         }
      }
   }
   
   DABC.HierarchyPainter.prototype.GetOnlineItem = function(item, callback) {

      if (!('_dabc_hist' in item))
         return JSROOT.HierarchyPainter.prototype.GetOnlineItem.call(this, item, callback);
      
      var url = this.itemFullName(item);
      if (url.length > 0) url += "/";
      url += 'get.json.gz?compact=3';

      var itemreq = JSROOT.NewHttpRequest(url, 'object', function(res) {
         
         var obj = null;
         if (res && res._kind=='ROOT.TH1D') {
            obj = JSROOT.CreateTH1(res.nbins);
            jQuery.extend(obj, { fName: res._name, fTitle: res._title });
            jQuery.extend(obj['fXaxis'], { fXmin: res.left,  fXmax: res.right });
            for (var i=0;i<res.nbins;i++)
               obj.fArray[i+1] = res.bins[i+3]; // 3 first items in array are not bins
         } else 
         if (res && res._kind=='ROOT.TH2D') {
            obj = JSROOT.CreateTH2(res.nbins1, res.nbins2);
            jQuery.extend(obj, { fName: res._name, fTitle: res._title });
            jQuery.extend(obj['fXaxis'], { fXmin: res.left1,  fXmax: res.right1 });
            jQuery.extend(obj['fYaxis'], { fXmin: res.left2,  fXmax: res.right2 });
            for (var i=0;i<res.nbins1;i++)
               for (var j=0;j<res.nbins2;j++)
                  obj['fArray'][i+1+(j+1)*(res.nbins1+2)] = res.bins[6+i+j*res.nbins1]; // 6 first items in array are not bins
         }
            

         if (typeof callback == 'function')
            callback(item, obj);
      });

      itemreq.send(null);

   }
   
   DABC.HierarchyPainter.prototype.display = function(itemname, options, call_back)
   {
      var node = this.Find(itemname);
      if ((node==null) || !this.CreateDisplay()) return;

      var mdi = this['disp'];

      var kind = node["_kind"];
      var view = node["_view"];
      var history = node["_history"];
      if (!kind) kind = "";

      var isdabc = false;

      mdi.ForEachPainter(function(p) {
         if ((p.GetItemName() == itemname) && p['is_dabc']) 
            { p.ClickItem(); isdabc = true; } 
      });

      if (isdabc) return;
      
      if (((kind.indexOf("ROOT.") == 0) && (view != "png")) || ('_player' in node))
         return JSROOT.HierarchyPainter.prototype.display.call(this, itemname, options, call_back);

      var elem = DABC.CreateDrawElement(node, options=='history' ? this.HistoryDepth() : -1);

      if (elem==null) return;
         
      elem.itemname = itemname;
 
      var frame = mdi.CreateFrame(itemname);
      elem.CreateFrames(frame);

      elem.RegularCheck();
      
      if (typeof call_back == 'function') call_back(elem);
   }
   
   DABC.HierarchyPainter.prototype.HistoryDepth = function(onlyurl) {
      var history = JSROOT.GetUrlOption("history");
      if ((history == "") || (history==null)) return 0; // 0 is default value means maximum history
      history = parseInt(history);
      return ((history == NaN) || (history<=0)) ? 0 : history;
   }
   

   DABC.HierarchyPainter.prototype.UpdateDabcElements = function()
   {
      var mdi = this['disp'];
      if (mdi==null) return;

      mdi.ForEachPainter(function(painter) {
         if (painter['is_dabc']) painter.RegularCheck();
      });

      if (this.IsMonitoring())
         this.updateAll();
   }

   DABC.HierarchyPainter.prototype.ExecuteCommand = function(cmditemname, args, call_back) {
      var url = cmditemname + "/execute";
      if (args!=null) url += "?" + args;
      
     var req = JSROOT.NewHttpRequest(url, "text", function(res) { 
        console.log(cmditemname+" done");
        if (typeof call_back=='function') call_back(res);
     });
     req.send(null);
   }
   
   DABC.HierarchyPainter.prototype.FillOnlineMenu = function(menu, onlineprop, itemname) {
      
      console.log("FillOnlineMenu " + itemname);
      
      var item = this.Find(itemname); 
      var painter = this;
      var opts = null;
      if (('_kind' in item) && (item._kind.indexOf("ROOT.")==0))
         opts = JSROOT.getDrawOptions(item._kind.substr(5), 'nosame');
      else if ('_history' in item) {
         opts = ['','history'];
      }
      
      var baseurl = onlineprop.server + onlineprop.itemname + "/";
      
      var drawurl = baseurl + "draw.htm", editorurl = baseurl + "draw.htm?opt=editor";
      
      var separ = "?";
      if (this.IsMonitoring()) { drawurl += separ + "monitoring=" + this.MonitoringInterval(); separ = "&"; }
      if (this.HistoryDepth()>0) { drawurl += separ + "history=" + this.HistoryDepth(); separ = "&"; }    
      
      if ('_kind' in item) {
         menu.addDrawMenu("Draw", opts, function(arg) { painter.display(itemname, arg); });
         menu.addDrawMenu("Draw in new window", opts, function(arg) { window.open(drawurl + separ + "opt=" + arg); });
      }
      
      if ((item!=null) && ('_editor' in item))
         menu.add("Editor", function() { window.open(editorurl); });
      
      if ('_player' in item)
         menu.add("Player", function() { painter.player(itemname); });
   }
   
   DABC.HierarchyPainter.prototype.CreateStatus = function(height) {
      $('#mainGUI').append('<div id="separator-status" class="separator" style="left:1px; right:1px;  height:4px; bottom:16px; cursor: ns-resize"></div>');
      $('#mainGUI').append('<div id="status-div" class="column" style="left:1px; right:1px; height:15px; bottom:1px"></div>');

      var painter = this;
      
      function adjustSize(height) {
         var h = JSROOT.touches ? 10 : 4;
         
         $('#left-div').css('bottom', height + h + 'px');
         $('#separator-div').css('bottom', height + h + 'px');
         $('#right-div').css('bottom', height + h + 'px');
         $('#separator-status').css('bottom', height + 'px').height(h);
         $('#status-div').height(height - 1);
      
         painter.CheckResize();
      }

      $("#separator-status").draggable({
         axis: "y" , zIndex: 100, cursor: "ns-resize",
         helper : function() { return $("#separator-status").clone().css('background-color','grey'); },
         stop: function(event,ui) { event.stopPropagation(); adjustSize($(window).height() - ui.position.top); }
      });
      
      adjustSize(height);
   }
   
   DABC.HierarchyPainter.prototype.updateOnOtherFrames = function(painter, obj) {
       // function should change object on other painters
      var mdi = this['disp'];
      if (mdi==null) return false;

      var isany = false;
      mdi.ForEachPainter(function(p, frame) {
         if ((p===painter) || (p.GetItemName() != painter.GetItemName())) return;
         mdi.ActivateFrame(frame);
         p.RedrawObject(obj);
         isany = true;
      });
      return isany;
   }

   DABC.HierarchyPainter.prototype.drawOnSuitableHistogram = function(painter, obj, is2d) {
      
      var mdi = this['disp'];
      if (mdi==null) return false;
      
      var kind = is2d ? "TH2" : "TH1";
      var found = false;
      
      mdi.ForEachPainter(function(p, frame) {
         if ((p==painter) || found) return;
         if (!('obj_typename' in p)) return;
         if (p.obj_typename.indexOf(kind)!=0) return;
         
         found = true;
         
         var p2 = JSROOT.draw(p.divid, obj, 'same');
         if (p2) p2.SetItemName(painter.GetItemName());
      });      
      
      return found;
   }
   
   
   // method for custom HADAQ-specific GUI, later could be moved into hadaq.js script 
   
   DABC.HadaqDAQControl = function(hpainter, itemname) {
      var mdi = hpainter.CreateDisplay();
      if (mdi == null) return null;

      var frame = mdi.FindFrame(itemname, true);
      if (frame==null) return null;
      
      var divid = frame.attr('id');
      
      var item = hpainter.Find(itemname);
      var calarr = [];
      if (item) {
         for (var n in item._parent._childs) {
            var name = item._parent._childs[n]._name;
            
            if ((name.indexOf("Input")==0) && (name.indexOf("TdcCal")>0)) {
               var fullname = hpainter.itemFullName(item._parent._childs[n]);
               calarr.push(fullname);               
            }
         }
      }
      
      frame.empty();
      
      var html = "<fieldset>" +
      		     "<legend>DAQ</legend>" +
      		     "<button class='hadaq_startfile'>Start file</button>" +
                 "<button class='hadaq_stopfile'>Stop file</button>" +
                 '<input class="hadaq_filename" type="text" name="filename" value="file.hld" style="margin-top:5px;"/><br/>' +
                 "<label class='hadaq_rate'>Rate: __undefind__</label><br/>"+
                 "<label class='hadaq_info'>Info: __undefind__</label>"+
      		     "</fieldset>" +
      		     "<fieldset>" +
      		     "<legend>Calibration</legend>";
      for (var n in calarr) {
         html += "<div class='hadaq_calibr' style='padding:2px;margin:2px'>" + calarr[n] + "</div>";
      }
      html+="</fieldset>";
      
      frame.html(html);
      
      frame.find(".hadaq_startfile").button().click(function() { 
         hpainter.ExecuteCommand(itemname+"/StartHldFile", "filename="+frame.find('.hadaq_filename').val()+"&maxsize=2000");
      });
      frame.find(".hadaq_stopfile").button().click(function() { 
         hpainter.ExecuteCommand(itemname+"/StopHldFile");
      });
      
      var inforeq = null;
      var firsttime = true;
      
      function UpdateDaqStatus(res) {
         if (res==null) return;
         for (var n in res._childs) {
            var item = res._childs[n];
            
            if (item._name=='HadaqInfo')
               frame.find('.hadaq_info').text("Info: " + item.value);                  
            if (item._name=='HadaqData')
               frame.find('.hadaq_rate')
                  .css("font-size","150%")
                  .text("Rate: " + item.value + " " + item.units+"/s");                  
         }
      }
      
      var handler = setInterval(function() {
         if ($("#"+divid+" .hadaq_info").length==0) {
            // if main element disapper (reset), stop handler 
            clearInterval(handler);
            return;
         }
         
         if (inforeq) return;
         
         var url = "multiget.json?items=[";
         url+="'" + itemname + "'";
         for (var n in calarr)
            url+= ",'" + calarr[n]+"/Status" + "'";
         url+="]";
         //url+="]&compact=3";
         
         function makehname(prefix, code, name) {
            var str = code.toString(16).toUpperCase();
            while (str.length<4) str = "0"+str;
            return "/"+prefix+"_"+str+"/"+prefix+"_"+str+"_"+name;
         }
         
         inforeq = JSROOT.NewHttpRequest(url, "object", function(res) {
            inforeq = null;
            if (res==null) return;
            UpdateDaqStatus(res[0].result);
            frame.find('.hadaq_calibr').each(function(index) {
               var info = res[index+1].result;
               
               if (firsttime) {
                  var code = "<div style='float:left'>";
                  code += "<button hist='" + calarr[index] + makehname("TRB", info.trb, "TdcDistr") + "' >"+info.trb.toString(16)+"</button>";
                  for (var j in info.tdc)
                     code+="<button hist='" + calarr[index] + makehname("TDC", info.tdc[j], "Channels") + "'>"+info.tdc[j].toString(16)+"</button>";
               
                  code +="</div>"; 
                  code+="<div class='hadaq_progress'></div>";
                  $(this).html(code);
                  $(this).find("button").button().click(function(){ 
                     console.log("Draw hist " + $(this).attr('hist'));
                     var drawframe = mdi.FindFrame(itemname+"_drawing", true);
                     drawframe.empty();
                     hpainter.display($(this).attr('hist'),"divid:"+drawframe.attr('id'));         
                  });
                  $(this).find(".hadaq_progress").progressbar({ value: info.progress });
               }
               
               var col = 'red';
               if (info.value=='Ready') col = 'green'; else
               if (info.value=='File') col = 'yellow'; 
               
               $(this).css('background-color', col);
               $(this).attr('title',"TRB:" + info.trb.toString(16) + " State: " + info.value + " Time:" + info.time);
               
               $(this).find(".hadaq_progress")
                  .attr("title", "progress: " + info.progress + "%")
                  .progressbar("option", "value", info.progress);
            });
            firsttime = false;
         });
         inforeq.send(null);
      }, 2000);
   }
   
})();
