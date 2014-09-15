DABC = {};

DABC.version = "2.6.7";

DABC.mgr = 0;

DABC.tree_limit = 200; // maximum number of elements drawn in the beginning

// ============= start of DrawElement ================================= 

DABC.DrawElement = function() {
   this.itemname = "";               // full item name in hierarchy
   this.version = new Number(-1);    // check which version of element is drawn
   this.frameid = "";                // frame id of HTML element (ion most cases <div>), where object drawn 
   return this;
}

//method called when item is activated (clicked)
//each item can react by itself
DABC.DrawElement.prototype.ClickItem = function() { return; }

// method regularly called by the manager
// it is responsibility of item perform any action
DABC.DrawElement.prototype.RegularCheck = function() { return; }


DABC.DrawElement.prototype.CreateFrames = function(topid,cnt) {
   this.frameid = "dabc_dummy_" + cnt;

   var entryInfo = 
      "<div id='" +this.frameid + "'>" + 
      "<h2> CreateFrames for item " + this.itemname + " not implemented </h2>"+
      "</div>"; 
   $(topid).append(entryInfo);
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

DABC.CommandDrawElement.prototype.CreateFrames = function(topid,cnt) {
   this.frameid = "dabc_command_" + cnt;

   var entryInfo = "<div id='" + this.frameid + "'/>";
   
   $(topid).empty();
   $(topid).append(entryInfo);
   
   this.ShowCommand();
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
   
   var entryInfo = "<input type='button' title='Execute' value='Execute' onclick=\"DABC.mgr.ExecuteCommand('" + this.itemname + "')\"/><br/>";

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

   var url = this.itemname + "get.json";

   this.req = DABC.mgr.NewHttpRequest(url, "text", this);

   this.req.send(null);
}

DABC.CommandDrawElement.prototype.InvokeCommand = function() {
   if (this.req) return;
   
   var resdiv = $("#" + this.frameid + "_res");
   if (resdiv) {
      resdiv.empty();
      resdiv.append("<h5>Send command to server</h5>");
   }
   
   var url = this.itemname + "execute";

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
   
   this.req = DABC.mgr.NewHttpRequest(url, "text", this);

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


//========== start of HistoryDrawElement

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

DABC.HistoryDrawElement.prototype.CreateFrames = function(topid, id) {
}

DABC.HistoryDrawElement.prototype.ClickItem = function() {
   if (this.req != null) return; 

   if (!this.IsDrawn()) 
      this.CreateFrames(DABC.mgr.NextCell(), DABC.mgr.cnt++);
   this.force = true;
   
   this.RegularCheck();
}

DABC.HistoryDrawElement.prototype.RegularCheck = function() {

   // no need to do something when req not completed
   if (this.req!=null) return;
 
   // do update when monitoring enabled
   if ((this.version >= 0) && !this.force) {
      var chkbox = document.getElementById("monitoring");
      if (!chkbox || !chkbox.checked) return;
   }
        
   var url = this.itemname + this.request_name + "?compact=3";

   if (this.version>0) url += "&version=" + this.version; 
   if (this.hlimit>0) url += "&history=" + this.hlimit;
   this.req = DABC.mgr.NewHttpRequest(url, "text", this);

   this.req.send(null);

   this.force = false;
}

DABC.HistoryDrawElement.prototype.ExtractField = function(name, kind, node) {
   
   if (!node) node = this.jsonnode;    
   if (!node) return;

   var val = node[name];
   if (!val) return;
   
   if (kind=="number") return Number(val); 
   if (kind=="time") {
      //return Number(val);
      var d  = new Date(val);
      return d.getTime() / 1000.;
   }
   
   return val;
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
      console.log("no xml response from server");
      return;
   }
   
   var top = JSON.parse(arg);

//   console.log("Get request callback " + top["_version"]);
   
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

// TODO: check how it works in different older browsers
DABC.GaugeDrawElement.prototype = Object.create( DABC.HistoryDrawElement.prototype );

DABC.GaugeDrawElement.prototype.CreateFrames = function(topid, id) {

   this.frameid = "dabc_gauge_" + id;
   this.min = 0;
   this.max = 10;
   this.gauge = null;
   
//   var entryInfo = "<div id='"+this.frameid+ "' class='200x160px'> </div> \n";
   var entryInfo = "<div id='"+this.frameid+ "'/>";
   $(topid).append(entryInfo);
}

DABC.GaugeDrawElement.prototype.DrawHistoryElement = function() {
   
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

//======== start of ImageDrawElement ======================

DABC.ImageDrawElement = function() {
   DABC.DrawElement.call(this);
}

// TODO: check how it works in different older browsers
DABC.ImageDrawElement.prototype = Object.create( DABC.DrawElement.prototype );

DABC.ImageDrawElement.prototype.CreateFrames = function(topid, id) {

   this.frameid = "dabc_image_" + id;
   
   var width = $(topid).width();
   
   var url = this.itemname + "root.png.gz?w=400&h=300&opt=col";
//   var entryInfo = "<div id='"+this.frameid+ "' class='200x160px'> </div> \n";
   var entryInfo = 
      "<div id='"+this.frameid+ "'>" +
      "<img src='" + url + "' alt='some text' width='" + width + "'>" + 
      "</div>";
   $(topid).append(entryInfo);
}


// ======== end of ImageDrawElement ======================


//======== start of LogDrawElement ======================

DABC.LogDrawElement = function() {
   DABC.HistoryDrawElement.call(this,"log");
}

DABC.LogDrawElement.prototype = Object.create( DABC.HistoryDrawElement.prototype );

DABC.LogDrawElement.prototype.CreateFrames = function(topid, id) {
   this.frameid = "dabc_log_" + id;
   var entryInfo;
   if (this.isHistory()) {
      // var w = $(topid).width();
      var h = $(topid).height();
      var maxhstyle = "";
      if (h>10) maxhstyle = "; max-height:" + h + "px"; 
      entryInfo = "<div id='" + this.frameid + "' style='overflow:auto; font-family:monospace" + maxhstyle + "'/>";
   } else {
      entryInfo = "<div id='"+this.frameid+ "'/>";
   }
   $(topid).append(entryInfo);
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

DABC.GenericDrawElement.prototype.CreateFrames = function(topid, id) {
   this.frameid = "dabc_generic_" + id;
   var entryInfo;
//   if (this.isHistory()) {
//      var w = $(topid).width();
//      var h = $(topid).height();
//      entryInfo = "<div id='" + this.frameid + "' style='overflow:auto; font-family:monospace; max-height:" + h + "px'/>";
//   } else {
      entryInfo = "<div id='"+this.frameid+ "'/>";
//   }
   $(topid).append(entryInfo);
}

DABC.GenericDrawElement.prototype.DrawHistoryElement = function() {
   if (this.recheck) {
      console.log("here one need to draw element with real style " + this.FullItemName());
      this.recheck = false;
      
      if ('_kind' in this.jsonnode) {
         var itemname = this.itemname;
         var jsonnode = this.jsonnode;
         DABC.mgr.ClearWindow();
         DABC.mgr.DisplayItem(itemname, jsonnode);
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

DABC.FesaDrawElement.prototype.CreateFrames = function(topid, id) {
   this.frameid = "dabcobj" + id;
   
   var entryInfo = "<div id='" + this.frameid + "'/>";
   $(topid).append(entryInfo);
   
   var render_to = "#" + this.frameid;
   var element = $(render_to);
      
   var fillcolor = 'white';
   var factor = 0.66666;
      
   d3.select(render_to).style("background-color", fillcolor);
   d3.select(render_to).style("width", "100%");

   var w = element.width(), h = w * factor; 

   this.vis = d3.select(render_to)
                   .append("svg")
                   .attr("width", w)
                   .attr("height", h)
                   .style("background-color", fillcolor);
      
   this.vis.append("svg:title").text(this.FullItemName());
}

DABC.FesaDrawElement.prototype.ClickItem = function() {
   if (this.req != null) return; 

   if (!this.IsDrawn()) 
      this.CreateFrames(DABC.mgr.NextCell(), DABC.mgr.cnt++);
   this.force = true;
   
   this.RegularCheck();
}

DABC.FesaDrawElement.prototype.RegularCheck = function() {

   
   // no need to do something when req not completed
   if (this.req!=null) return;
 
   // do update when monitoring enabled
   if ((this.version >= 0) && !this.force) {
      var chkbox = document.getElementById("monitoring");
      if (!chkbox || !chkbox.checked) return;
   }
        
   var url = this.itemname + "dabc.bin";
   
   if (this.version>0) url += "?version=" + this.version;

   this.req = DABC.mgr.NewHttpRequest(url, "bin", this);

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
   
   this.vis.select("title").text(this.FullItemName() + 
         "\nversion = " + this.version + ", size = " + this.data.length);

   if (!this.ReconstructObject()) return;
   
   if (this.root_painter != null) {
      this.root_painter.RedrawFrame();
   } else {
      this.root_painter = JSROOT.Painter.drawObjectInFrame(this.vis, this.root_obj);
      
      if (this.root_painter == -1) this.root_painter = null;
   }
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
//      console.log("Create histogram");
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
//         if (iy==5) console.log("Set content " + value);
      }
   
   return true;
}


//========== start of RateHistoryDrawElement

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

DABC.RateHistoryDrawElement.prototype.CreateFrames = function(topid, id) {

   
   this.frameid = "dabcobj" + id;
   
   var entryInfo = "<div id='" + this.frameid + "'/>";
   $(topid).append(entryInfo);
   
   var render_to = "#" + this.frameid;
   var element = $(render_to);
      
   var fillcolor = 'white';
   var factor = 0.66666;
      
   d3.select(render_to).style("background-color", fillcolor);
   d3.select(render_to).style("width", "100%");

   var w = element.width(), h = w * factor; 

   this.vis = d3.select(render_to)
                   .append("svg")
                   .attr("width", w)
                   .attr("height", h)
                   .style("background-color", fillcolor);
      
   this.vis.append("svg:title").text(this.itemname);
}


DABC.RateHistoryDrawElement.prototype.DrawHistoryElement = function() {

   this.vis.select("title").text(this.itemname + 
         "\nversion = " + this.version + ", history = " + (this.history ? this.history.length : 0));
   
   //console.log("Extract series");
   
   var x = this.ExtractSeries("time", "time");
   var y = this.ExtractSeries("value", "number");
   
   if (x.length==1) {
      console.log("duplicate single point at time " + x[0]);
      x.push(x[0]+1);
      y.push(y[0]);
   } 
   

   // here we should create TGraph object

   var gr = JSROOT.CreateTGraph();
   
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
   gr['fHistogram']['fXaxis']['fTimeFormat'] = "";
   // JSROOT.gStyle['TimeOffset'] = 0; // DABC uses UTC time, starting from 1/1/1970
   gr['fHistogram']['fXaxis']['fTimeFormat'] = "%H:%M:%S%F0"; // %FJanuary 1, 1970 00:00:00
   
   if (this.root_painter && this.root_painter.UpdateObject(gr)) {
      this.root_painter.RedrawFrame();
   } else {
      this.root_painter = JSROOT.Painter.drawObjectInFrame(this.vis, gr, "L");
      
      if (this.root_painter == -1) this.root_painter = null;
   }
}

// ======== start of RootDrawElement ======================

DABC.RootDrawElement = function(_clname) {
   DABC.DrawElement.call(this);
   this.clname = _clname;    // ROOT class name
   
   this.obj = null;          // object itself, for streamer info is file instance
   this.req = null;          // this is current request
   this.painter = null;      // pointer on painter, can be used for update
   this.version = 0;         // object version 
   this.object_size = 0;      // size of raw data, can be displayed
}

DABC.RootDrawElement.prototype = Object.create( DABC.DrawElement.prototype );

DABC.RootDrawElement.prototype.Clear = function() {
   
   DABC.DrawElement.prototype.Clear.call(this);

   this.obj = null;          // object itself, for streamer info is file instance
   if (this.req) this.req.abort(); 
   this.req = null;          // this is current request
   this.painter = null;      // pointer on painter, can be used for update
   this.version = 0;         // object version 
   this.object_size = 0;     // size of raw data, can be displayed
}

DABC.RootDrawElement.prototype.IsObjectDraw = function()
{
   // returns true when normal ROOT drawing should be used
   // when false, streamer info drawing is applied
   return this.clname != 'TStreamerInfoList'; 
}

DABC.RootDrawElement.prototype.CreateFrames = function(topid, id) {
   this.frameid = "dabcobj" + id;
   
   var entryInfo = ""; 
   if (this.IsObjectDraw()) {
      entryInfo += "<div id='" + this.frameid + "'/>";
   } else {
      entryInfo += "<div style='overflow:auto'><span id='" + this.frameid +"' class='dtree'></span></div>";
   }
   //entryInfo+="</div>";
   $(topid).append(entryInfo);
   
   if (this.IsObjectDraw()) {
      var render_to = "#" + this.frameid;
      var element = $(render_to);
      
      var fillcolor = 'white';
      var factor = 0.66666;
      
      d3.select(render_to).style("background-color", fillcolor);
      d3.select(render_to).style("width", "100%");

      var w = element.width(), h = w * factor; 

      this.vis = d3.select(render_to)
                   .append("svg")
                   .attr("width", w)
                   .attr("height", h)
                   .style("background-color", fillcolor);
      
      this.vis.append("svg:title").text(this.itemname);
      
//      console.log("create visual pane of width " + w + "  height " + h)
   }
   
//   $(topid).data('any', 10);
//   console.log("something = " + $(topid).data('any'));
   
}

DABC.RootDrawElement.prototype.ClickItem = function() {
   if (this.req != null) return; 

   // TODO: TCanvas update do not work in JSRootIO
   if (this.clname == "TCanvas") return;

   if (!this.IsDrawn()) 
      this.CreateFrames(DABC.mgr.NextCell(), DABC.mgr.cnt++);
      
   this.RegularCheck();
}


DABC.RootDrawElement.prototype.HasVersion = function(ver) {
   return this.obj && (this.version >= ver);
}

// if frame created and exists
DABC.RootDrawElement.prototype.DrawObject = function(newobj) {

   if (!this.IsObjectDraw()) {
      this.obj = newobj;
      var painter = new JSROOT.HierarchyPainter('sinfo', this.frameid);
      painter.ShowStreamerInfo(this.obj);
      return;
   }
   
   if (newobj != null) {
      if (this.painter && this.painter.UpdateObject(newobj)) {
         // if painter accepted object update, we need later just redraw frame
         newobj = null;
      } else { 
         this.obj = newobj;
         this.painter = null;
      }
   }
   
   if (this.obj == null) return;

   if (this.vis!=null) {
      var lbl = "";
      if (this.version > 0) lbl += "version = " + this.version + ", ";
      lbl += "size = " + this.object_size;
      this.vis.select("title").text(this.FullItemName() + "\n" + lbl);
   }

   if (this.painter != null) {
      this.painter.RedrawFrame();
   } else {
      this.painter = JSROOT.Painter.drawObjectInFrame(this.vis, this.obj);

      if (this.painter == -1) this.painter = null;

      // if (this.painter)  console.log("painter is created");
   }
}


DABC.RootDrawElement.prototype.RequestCallback = function(arg) {
   
   var mversion = null, bversion = null;
   
   if (this.req!=null) {
      mversion = this.req.getResponseHeader("MVersion");
      if (mversion!=null) mversion = new Number(mversion);
      bversion = this.req.getResponseHeader("BVersion");
      if (bversion!=null) bversion = new Number(bversion);
   }

   if (mversion == null) mversion = new Number(0);
   if (bversion == null) bversion = new Number(0);

   // in any case, request pointer will be reseted
   this.req = null;
   
   // console.log("Call back " + this.itemname);
   
   // if we got same version, do nothing - we are happy!!!
   if ((bversion > 0) && (this.version == bversion)) {
      console.log(" Get same version " + bversion + " of object " + this.itemname);
      return;
   } 
   
   var obj = JSROOT.parse(arg);

   this.version = bversion;
      
   this.object_size = arg.length;
      
   if (obj && ('_typename' in obj)) {
      // console.log("Get JSON object of " + obj['_typename']);
      this.DrawObject(obj);
   } 
}

DABC.RootDrawElement.prototype.RegularCheck = function() {

   // if item ready, verify that we want to send request again
   if (!this.IsDrawn()) return;
   
   var chkbox = document.getElementById("monitoring");
   if (!chkbox || !chkbox.checked) return;

   this.SendRequest();
}

DABC.RootDrawElement.prototype.SendRequest = function() {

   if (this.req!=null) return;
   
   // TODO: TCanvas update do not work in JSRootIO
   if (this.clname == "TCanvas") return;
   
   var url = this.itemname + "root.json.gz?compact=3";
   if (this.version>0) url += "&version=" + this.version;

   this.req = DABC.mgr.NewHttpRequest(url, "text", this);

   this.req.send(null);
}


// ======== end of RootDrawElement ======================



// ============= start of DABC.Manager =============== 

DABC.Manager = function(with_tree) {
   this.cnt = new Number(0);      // counter to create new element 
   this.arr = new Array();        // array of DrawElement
   this.with_tree = with_tree;
   
   this.hpainter = null;  // painter used to to draw hierarchy
   
   if (this.with_tree) this.CreateTable(2,2);

   // we could use ROOT drawing from beginning
   JSROOT.gStyle.OptimizeDraw = true;
   
   return this;
}

DABC.Manager.prototype.CreateTable = function(divx, divy)
{
   var tablecontents = "";
   var cnt = 0;
   
   var precx = Math.floor(100/divx);
   var precy = Math.floor(100/divy);
   
   tablecontents = "<table width='100%' height='100%'>";
   for (var i = 0; i < divy; i ++)
   {
      tablecontents += "<tr height='"+precy+"%'>";
      for (var j = 0; j < divx; j ++) {
         tablecontents += "<td id='draw_place_"+ cnt + "' width='" + precx 
                       + "%' height='"+precy+"%'>" + i + "," + j + "</td>";
         cnt++;
      }
      tablecontents += "</tr>";
   }
   tablecontents += "</table>";
   $("#dabc_draw").empty();
   document.getElementById("dabc_draw").innerHTML = tablecontents;

   this.table_counter = 0;
   this.table_number = divx*divy;
}

DABC.Manager.prototype.NextCell = function()
{
   var id = "#dabc_draw";
   if (this.with_tree) {
      var id = "#draw_place_" + this.table_counter;
      this.table_counter++;
      if (this.table_counter>=this.table_number) this.table_counter = 0;
   }
   $(id).empty();
   return id;
}


DABC.Manager.prototype.FindItem = function(itemname) {
   for (var i in this.arr) {
      if (this.arr[i].itemname == itemname) return this.arr[i];
   }
}

DABC.Manager.prototype.RemoveItem = function(item) {
   var indx = this.arr.indexOf(item);
   if (indx < 0) return;
   this.arr.splice(indx, 1);
   delete item;
}

DABC.Manager.prototype.empty = function() {
   return this.arr.length == 0;
}

// this is used for text request 
DABC.Manager.prototype.NewRequest = function() {
   var req;
   // For Safari, Firefox, and other non-MS browsers
   if (window.XMLHttpRequest) {
      try {
         req = new XMLHttpRequest();
      } catch (e) {
         req = false;
      }
   } else if (window.ActiveXObject) {
      // For Internet Explorer on Windows
      try {
         req = new ActiveXObject("Msxml2.XMLHTTP");
      } catch (e) {
         try {
            req = new ActiveXObject("Microsoft.XMLHTTP");
         } catch (e) {
            req = false;
         }
      }
   }

   return req;
}


DABC.Manager.prototype.NewHttpRequest = function(url, kind, item) {
   
//   var xhrcallback = function(res) {
//      item.RequestCallback(res);
//   }
   
   return JSROOT.NewHttpRequest(url, kind, function(res) { item.RequestCallback(res); }); 
}


DABC.Manager.prototype.UpdateComplexFields = function() {
   if (this.empty()) return;

   for (var i in this.arr) 
     this.arr[i].RegularCheck();
}

DABC.Manager.prototype.UpdateAll = function() {
   this.UpdateComplexFields();
}


DABC.Manager.prototype.DisplayItem = function(itemname, node)
{
   if (!node) {
      console.log("cannot find node for item " + itemname);
      return;
   } 
   
   var kind = node["_kind"];
   var history = node["_history"];
   var view = node["_view"];
   if (!kind) kind = "";

   var elem;
   
   if (view == "png") {
      elem = new DABC.ImageDrawElement();
      elem.itemname = itemname;
      elem.CreateFrames(this.NextCell(), this.cnt++);
      this.arr.push(elem);
      return;
   }
   
   if (kind == "DABC.Command") {
      elem = new DABC.CommandDrawElement();
      elem.itemname = itemname;
      elem.CreateFrames(this.NextCell(), this.cnt++);
      elem.RequestCommand();
      this.arr.push(elem);
      return;
   }

   // ratemeter
   if (kind == "rate") { 
      if ((history == null) || !document.getElementById("show_history").checked) {
         elem = new DABC.GaugeDrawElement();
         elem.itemname = itemname;
         elem.CreateFrames(this.NextCell(), this.cnt++);
         this.arr.push(elem);
         return;
      } else {
         elem = new DABC.RateHistoryDrawElement();
         elem.itemname = itemname;
         elem.CreateFrames(this.NextCell(), this.cnt++);
         this.arr.push(elem);
         return;
      }
   }
   
   if (kind == "log") {
      elem = new DABC.LogDrawElement();
      elem.itemname = itemname;

      if ((history != null) && document.getElementById("show_history").checked) 
         elem.EnableHistory(100);
      
      elem.CreateFrames(this.NextCell(), this.cnt++);
      this.arr.push(elem);
      return;
   }
   
   if (kind.indexOf("FESA.") == 0) {
      elem = new DABC.FesaDrawElement(kind.substr(5));
      elem.itemname = itemname;
      elem.CreateFrames(this.NextCell(), this.cnt++);
      this.arr.push(elem);
      elem.RegularCheck();
      return;
   }

   if (kind.indexOf("ROOT.") == 0) {
      // procesing of ROOT classes
      
      elem = new DABC.RootDrawElement(kind.substr(5));
      
      elem.itemname = itemname;
      elem.CreateFrames(this.NextCell(), this.cnt++);
      this.arr.push(elem);

      elem.SendRequest();
      return;
   }
   
   // create generic draw element, which just shows all attributes 
   elem = new DABC.GenericDrawElement();
   elem.itemname = itemname;
   elem.CreateFrames(this.NextCell(), this.cnt++);
   this.arr.push(elem);
   return elem;
}

DABC.Manager.prototype.ExecuteCommand = function(itemname)
{
   var elem = this.FindItem(itemname);
   if (elem) elem.InvokeCommand();
}

DABC.Manager.prototype.DisplayGeneric = function(itemname, recheck)
{
   var elem = new DABC.GenericDrawElement();
   elem.itemname = itemname;
   elem.CreateFrames(this.NextCell(), this.cnt++);
   if (recheck) {
      elem.recheck = true;
      elem.request_name = "h.json"; // we do not need element but its description
   }
   this.arr.push(elem);
}

DABC.Manager.prototype.DisplayHiearchy = function(holder) {
   if (this.hpainter!=null) return;
   
   this.hpainter = new JSROOT.HierarchyPainter('main', holder);
      
   // this.hpainter['ondisplay'] = null;
   
   this.hpainter['display'] = function(itemname) {
      var item = this.Find(itemname);
      
      if (item==null) {
         console.log("fail to find item " + itemname);
         return;
      }
      
      // FIXME: avoid usage of extra slashes
      DABC.mgr.DisplayItem(itemname + "/", item);
   }
   
   this.hpainter['clear'] = function() {
      DABC.mgr.ClearWindow();
   }
   
   
   this.hpainter['CheckCanDo'] = function(node, cando) {
      var kind = node["_kind"];
      var view = node["_view"];

      cando.expand = ('_more' in node);

      if (view == "png") { cando.img1 = 'httpsys/img/dabcicon.png'; cando.display = true; } else
      if (kind == "rate") { cando.display = true; } else
      if (kind == "log") { cando.display = true; } else
      if (kind == "DABC.HTML") { cando.img1 = JSROOT.source_dir+'img/globe.gif'; cando.open = true; } else
      if (kind == "DABC.Application") cando.img1 = 'httpsys/img/dabcicon.png'; else
      if (kind == "DABC.Command") { cando.img1 = 'httpsys/img/dabcicon.png'; cando.display = true; cando.scan = false; } else
      if (kind == "GO4.Analysis") cando.img1 = 'go4sys/icons/go4logo2_small.png'; else
         JSROOT.HierarchyPainter.prototype.CheckCanDo(node, cando);
   }
      
   this.hpainter.OpenOnline("h.json?compact=3");
}


DABC.Manager.prototype.ClearWindow = function()
{
   delete this.arr;
   this.arr = [];

   if (this.with_tree) {
      var num = $("#grid_spinner").spinner( "value" );
      this.CreateTable(num,num);
   }
}


DABC.Manager.prototype.ReloadSingleElement = function()
{
   if (this.arr.length == 0) return;

   var itemname = this.arr[this.arr.length-1].itemname;
   
   this.ClearWindow();
   
   this.DisplayGeneric(itemname, true);
}

// ============= end of DABC.Manager =============== 
