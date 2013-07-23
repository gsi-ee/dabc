DABC = {};

DABC.version = "2.3.1";

DABC.mgr = 0;

DABC.dabc_tree = 0;   // variable to display hierarchy

DABC.load_root_js = 0; // 0 - not started, 1 - doing load, 2 - completed

DABC.ntou4 = function(b, o) {
   // convert (read) four bytes of buffer b into a uint32_t
   var n  = (b.charCodeAt(o)   & 0xff);
       n += (b.charCodeAt(o+1) & 0xff) << 8;
       n += (b.charCodeAt(o+2) & 0xff) << 16;
       n += (b.charCodeAt(o+3) & 0xff) << 24;
   return n;
}


DABC.UnpackBinaryHeader = function(arg) {
   if ((arg==null) || (arg.length < 20)) return null;
   
   var o = 0;
   var typ = DABC.ntou4(arg, o); o+=4;
   if (typ!=1) return null;
   
   var hdr = {};
   
   hdr['typ'] = typ;
   hdr['version'] = DABC.ntou4(arg, o); o+=4;
   hdr['master_version'] = DABC.ntou4(arg, o); o+=4;
   hdr['zipped'] = DABC.ntou4(arg, o); o+=4;
   hdr['payload'] = DABC.ntou4(arg, o); o+=4;
   
   hdr['rawdata'] = "";
   
   // if no payload specified, ignore
   if (hdr.payload == 0) return hdr;
   
   if (hdr.zipped == 0) {
      hdr['rawdata'] = arg.slice(o);
      if (hdr['rawdata'].length != hdr.payload)
         alert("mismatch between payload value and actual data length");
      return hdr;
   }

   var ziphdr = JSROOTIO.R__unzip_header(arg, o, true);
   
   if (!ziphdr) {
      alert("no zip header but it was specified!!!");
      return null;
   }
  
   var unzip_buf = JSROOTIO.R__unzip(ziphdr['srcsize'], arg, o, true);
   if (!unzip_buf) {
      alert("fail to unzip data");
      return false;
   } 
   
   hdr['rawdata'] = unzip_buf['unzipdata'];
   
   if (hdr['rawdata'].length != hdr.zipped)
      alert("mismatch between length of buffer before zip and actual data length");
   
   unzip_buf = null;
   return hdr;
}



DABC.DrawElement = function() {
   this.itemname = "";   // full item name in hierarhcy
   this.frameid = new Number(0);    // id of top frame, where item is drawn
   this.version = new Number(0);    // check which version of element is drawn
   this.frameid = "";
   return this;
}


// indicates if element operates with simple text fields, 
// which can be processed by SetValue() method
DABC.DrawElement.prototype.simple = function() { return false; }

// method regularly called by the manager
// it is responsibility of item perform any action
DABC.DrawElement.prototype.RegularCheck = function() { return; }

// method called when item is activated (clicked)
// each item can react by itself
DABC.DrawElement.prototype.ClickItem = function() { return; }


DABC.DrawElement.prototype.CreateFrames = function(topid,cnt) {
   this.frameid = "dabc_dummy_" + cnt;

   var entryInfo = 
      "<div id='" +this.frameid + "'>" + 
      "<h2> CreateFrames for item " + this.itemname + " not implemented </h2>"+
      "</div>"; 
   $(topid).append(entryInfo);
}

DABC.DrawElement.prototype.SetValue = function(val) {
   if (!val || (this.frameid.length==0)) return;
   document.getElementById(this.frameid).innerHTML = "Value = " + val;
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
   
   if (this.frameid.length>0) {
      var elem = document.getElementById(this.frameid);
      if (elem!=null) elem.parentNode.removeChild(elem);
   }
   
   this.itemname = "";
   this.frameid = 0; 
   this.version = 0;
   this.frameid = "";
}


// ======== end of DrawElement ======================

// ======== start of GaugeDrawElement ======================

DABC.GaugeDrawElement = function() {
   DABC.DrawElement.call(this);
   this.gauge = 0;
}

// TODO: check how it works in different older browsers
DABC.GaugeDrawElement.prototype = Object.create( DABC.DrawElement.prototype );

DABC.GaugeDrawElement.prototype.simple = function() { return true; }

DABC.GaugeDrawElement.prototype.CreateFrames = function(topid, id) {

   this.frameid = "dabc_gauge_" + id;

   var entryInfo = 
      "<div id='"+this.frameid+ "' class='200x160px'> </div> \n";
   $(topid).append(entryInfo);

   this.gauge = new JustGage({
      id: this.frameid, 
      value: 0,
      min: 0,
      max: 100,
      title: this.itemname
   }); 
}

DABC.GaugeDrawElement.prototype.SetValue = function(val) {
   if (val) this.gauge.refresh(val);
}

// ======== end of GaugeDrawElement ======================


//======== start of HierarchyDrawElement ======================

DABC.HierarchyDrawElement = function() {
   DABC.DrawElement.call(this);
   this.xmldoc = 0;
   this.ready = false;
   this.req = 0;             // this is current request
}

// TODO: check how it works in different older browsers
DABC.HierarchyDrawElement.prototype = Object.create( DABC.DrawElement.prototype );

DABC.HierarchyDrawElement.prototype.simple = function() { return false; }

DABC.HierarchyDrawElement.prototype.CreateFrames = function(topid, id) {

   this.frameid = topid;
   
}

DABC.HierarchyDrawElement.prototype.RegularCheck = function() {
   if (this.ready || this.req) return;
   
   var url = "h.xml";
   
   // if (this.version>0) url += "&ver=" + this.version;

   // $("#dabc_draw").append("<br> Create xml request");
   
   this.req = DABC.mgr.NewHttpRequest(url, true, false, this);

   this.req.send(null);
}

DABC.HierarchyDrawElement.prototype.nextNode = function(node)
{
   while (node && (node.nodeType!=1)) node = node.nextSibling;
   return node;
}

DABC.HierarchyDrawElement.prototype.createNode = function(nodeid, parentid, node, fullname) 
{
   node = this.nextNode(node);

   while (node) {

      // $("#dabc_draw").append("<br> Work with node " + node.nodeName);
      
      var kind = node.getAttribute("kind");
      var value = node.getAttribute("value");
      
      var html = "";
      
      var nodefullname  = fullname + "/" + node.nodeName; 
      
      var nodeimg = "";
      var node2img = "";
      
      if (kind) {
         html = "javascript: DABC.mgr.display('"+nodefullname+"');";

         // $("#dabc_draw").append("<br>See kind = " + kind);
         
         if (kind == "ROOT.Session") { nodeimg = source_dir+'img/globe.gif'; html = ""; }  else
         if (kind == "DABC.Application") { nodeimg = 'httpsys/img/dabcicon.png'; html = ""; }  else
         if (kind == "GO4.Analysis") { nodeimg = 'go4sys/icons/go4logo2_small.png'; html = ""; }  else
         if (kind.match(/\bROOT.TH1/)) nodeimg = source_dir+'img/histo.png'; else
         if (kind.match(/\bROOT.TH2/)) nodeimg = source_dir+'img/histo2d.png'; else  
         if (kind.match(/\bROOT.TH3/)) nodeimg = source_dir+'img/histo3d.png'; else
         if (kind == "ROOT.TCanvas") nodeimg = source_dir+'img/canvas.png'; else
         if (kind == "ROOT.TProfile") nodeimg = source_dir+'img/profile.png'; else
         if (kind.match(/\bROOT.TGraph/)) nodeimg = source_dir+'img/graph.png'; else
         if (kind == "ROOT.TTree") { nodeimg = source_dir+'img/tree.png'; html = ""; }  else
         if (kind == "ROOT.TFolder") { nodeimg = source_dir+'img/folder.gif'; node2img = source_dir+'img/folderopen.gif'; html = ""; }  else
         if (kind == "ROOT.TNtuple") { nodeimg = source_dir+'img/tree_t.png'; html = ""; }  else
         if (kind == "ROOT.TBranch") { nodeimg = source_dir+'img/branch.png'; html = ""; }  else
         if (kind.match(/\bROOT.TLeaf/)) { nodeimg = source_dir+'img/leaf.png'; html = ""; }  else
         if ((kind == "ROOT.TList") && (node.nodeName == "StreamerInfo")) nodeimg = source_dir+'img/question.gif'; 
      }
      
      if (node2img == "") node2img = nodeimg;
      
      DABC.dabc_tree.add(nodeid, parentid, node.nodeName, html, node.nodeName, "", nodeimg, node2img);
      
      nodeid = this.createNode(nodeid+1, nodeid, node.firstChild, nodefullname);
      
      node = this.nextNode(node.nextSibling);
   }
   
   return nodeid;
}


DABC.HierarchyDrawElement.prototype.TopNode = function() 
{
   if (!this.xmldoc) return;
   
   var lvl1 = this.nextNode(this.xmldoc.firstChild);
   
   if (lvl1) return this.nextNode(lvl1.firstChild);
}


DABC.HierarchyDrawElement.prototype.FindNode = function(fullname, top) {

//   $("#dabc_draw").append("<br> Serching for " + fullname);

   if (!top) top = this.TopNode();
   if (!top || (fullname.length==0)) return;
   if (fullname[0] != '/') return;
   
   var pos = fullname.indexOf("/", 1);
   var localname = pos>0 ? fullname.substr(1, pos-1) : fullname.substr(1);  
   var child = this.nextNode(top.firstChild);
   while (child) {
      if (child.nodeName == localname) break;
      child = this.nextNode(child.nextSibling);
   }
   
   if (!child  || (pos<=0)) return child;
   
   return this.FindNode(fullname.substr(pos), child);
   
}


DABC.HierarchyDrawElement.prototype.RequestCallback = function(arg) {
   this.req = 0;

   if (arg==null) { this.ready = false; return; }

   // $("#dabc_draw").append("<br> Get XML request callback "+ver);

   this.xmldoc = arg;
   
   // $("#dabc_draw").append("<br> xml doc is there");
   
   var top = this.TopNode();
   
   if (!top) { 
      $("#dabc_draw").append("<br> XML top node not found");
      return;
   }

   var top_version = top.getAttribute("dabc:version");

   if (!top_version) {
      $("#dabc_draw").append("<br> dabc:version not found");
      return;
   } else {
      this.version = top_version;
      // $("#dabc_draw").append("<br> found dabc:version " + this.version);
      // $("#dabc_draw").append("<br> found node " + top.nodeName);
   }
   
   this.createNode(0, -1, top.firstChild, "");

   var content = "<p><a href='javascript: DABC.dabc_tree.openAll();'>open all</a> | <a href='javascript: DABC.dabc_tree.closeAll();'>close all</a> | <a href='javascript: DABC.mgr.ReloadTree();'>reload</a> | <a href='javascript: DABC.mgr.ClearWindow();'>clear</a> </p>";
   content += DABC.dabc_tree;
   $("#" + this.frameid).html(content);
   
   DABC.dabc_tree.openAll();
   
   this.ready = true;
}


DABC.HierarchyDrawElement.prototype.Clear = function() {
   
   DABC.DrawElement.prototype.Clear.call(this);
   
   this.xmldoc = null;
   this.ready = false;
   if (this.req != null) this.req.abort();
   this.req = null;
}


// ======== end of HierarchyDrawElement ======================

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
      
   this.vis.append("svg:title").text(this.itemname);
}

DABC.FesaDrawElement.prototype.ClickItem = function() {
   if (this.req != null) return; 

   if (!this.IsDrawn()) 
      this.CreateFrames(DABC.mgr.NextCell(), DABC.mgr.cnt++);
   this.force = true;
   
   this.RegularCheck();
}

DABC.FesaDrawElement.prototype.RegularCheck = function() {

   if (DABC.load_root_js==0) {
      DABC.load_root_js = 1;
      AssertPrerequisites(function() {
         // $("#dabc_draw").append("<br> load main scripts done");
         DABC.load_root_js = 2; 
      }, true);
   }
   
   // in any case, complete JSRootIO is required before we could start 
   if (DABC.load_root_js < 2) return;
   
   // no need to do something when req not completed
   if (this.req!=null) return;
 
   // do update when monitoring enabled
   if ((this.version > 0) && !this.force) {
      var chkbox = document.getElementById("monitoring");
      if (!chkbox || !chkbox.checked) return;
   }
        
   var url = "getbinary?" + this.itemname;
   
   if (this.version>0) url += "&ver=" + this.version;

   this.req = DABC.mgr.NewHttpRequest(url, true, true, this);

   this.req.send(null);

   this.force = false;
}


DABC.FesaDrawElement.prototype.RequestCallback = function(arg) {
   // in any case, request pointer will be reseted
   // delete this.req;
   this.req = null;
   
   console.log("Get response from server " + arg.length);
   
   var hdr = DABC.UnpackBinaryHeader(arg);
   
   if (hdr == null) {
      alert("cannot extract binary header");
      return;
   }
   
   if (this.version == hdr.version) {
      console.log("Same version of beam profile " + ver);
      return;
   }
   
   if (hdr.rawdata == null) {
      alert("no data for beamprofile when expected");
      return;
   } 

   this.data = hdr.rawdata;
   this.version = hdr.version;

   console.log("Data length is " + this.data.length);

   

   
   this.vis.select("title").text(this.itemname + 
         "\nversion = " + this.version + ", size = " + this.data.length);

   if (!this.ReconstructObject()) return;
   
   if (this.root_painter != null) {
      this.root_painter.RedrawFrame();
   } else {
      this.root_painter = JSROOTPainter.drawObjectInFrame(this.vis, this.root_obj);
      
      if (this.root_painter == -1) this.root_painter = null;
   }
}


DABC.FesaDrawElement.prototype.ReconstructObject = function()
{
   if (this.clname != "2D") return false;
   
   if (this.root_obj == null) {
      this.root_obj = JSROOTPainter.Create2DHisto(16, 16);
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
         var value = DABC.ntou4(this.data, o); o+=4;
         var bin = this.root_obj.getBin(ix+1, iy+1);
         this.root_obj.setBinContent(bin, value);
//         if (iy==5) console.log("Set content " + value);
      }
   
   return true;
}



// ======== start of RootDrawElement ======================

DABC.RootDrawElement = function(_clname) {
   DABC.DrawElement.call(this);

   this.clname = _clname;    // ROOT class name
   this.obj = null;          // object itself, for streamer info is file instance
   this.sinfo = null;        // used to refer to the streamer info record
   this.req = null;          // this is current request
   this.first_draw = true;   // one should enable flag only when all ROOT scripts are loaded
   this.painter = null;      // pointer on painter, can be used for update
   
   this.raw_data = null;    // raw data kept in the item when object cannot be reconstructed immediately
   this.raw_data_version = 0;   // verison of object in the raw data, will be copied into object when completely reconstructed
   this.raw_data_size = 0;      // size of raw data, can be displayed
   this.need_master_version = 0; // this is version, required for the master item (streamer info)
   
   this.StateEnum = {
         stInit        : 0,
         stWaitRequest : 1,
         stWaitSinfo   : 2,
         stReady       : 3
   };
   
   this.state = this.StateEnum.stInit;   
}

DABC.RootDrawElement.prototype = Object.create( DABC.DrawElement.prototype );

DABC.RootDrawElement.prototype.Clear = function() {
   
   DABC.DrawElement.prototype.Clear.call(this);

   this.clname = "";         // ROOT class name
   this.obj = null;          // object itself, for streamer info is file instance
   this.sinfo = null;        // used to refer to the streamer info record
   if (this.req) this.req.abort(); 
   this.req = null;          // this is current request
   this.first_draw = true;   // one should enable flag only when all ROOT scripts are loaded
   this.painter = null;      // pointer on painter, can be used for update
}



DABC.RootDrawElement.prototype.CreateFrames = function(topid, id) {
   this.frameid = "dabcobj" + id;
   
   this.first_draw = true;
   
   var entryInfo = ""; 
   if (this.sinfo) {
      entryInfo += "<div id='" + this.frameid + "'/>";
   } else {
      entryInfo += "<h5><a> Streamer Infos </a>&nbsp; </h5>";
      entryInfo += "<div style='overflow:auto'><h6>Streamer Infos</h6><span id='" + this.frameid +"' class='dtree'></span></div>";
   }
   //entryInfo+="</div>";
   $(topid).append(entryInfo);
   
   if (this.sinfo) {
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
//   $("#dabc_draw").append("<br>something = " + $(topid).data('any'));
   
}

DABC.RootDrawElement.prototype.ClickItem = function() {
   if (this.state != this.StateEnum.stReady) return; 

   // TODO: TCanvas update do not work in JSRootIO
   if (this.clname == "TCanvas") return;

   if (!this.IsDrawn()) 
      this.CreateFrames(DABC.mgr.NextCell(), DABC.mgr.cnt++);
      
   this.state = this.StateEnum.stInit;
   this.RegularCheck();
}

// force item to get newest version of the object
DABC.RootDrawElement.prototype.Update = function() {
   if (this.state != this.StateEnum.stReady) return;
   this.state = this.StateEnum.stInit;
   this.RegularCheck();
}

DABC.RootDrawElement.prototype.HasVersion = function(ver) {
   return this.obj && (this.version >= ver);
}

// if frame created and exists
DABC.RootDrawElement.prototype.DrawObject = function() {
   if (this.obj == null) return;

   if (this.sinfo) {
   
      if (this.vis!=null)
        this.vis.select("title").text(this.itemname + 
                                      "\nversion = " + this.version + ", size = " + this.raw_data_size);
      
      if (this.painter != null) {
         this.painter.RedrawFrame();
      } else {
//         if (gStyle) gStyle.AutoStat = true;
//                else $("#dabc_draw").append("<br>no gStyle");

         this.painter = JSROOTPainter.drawObjectInFrame(this.vis, this.obj);
         
         if (this.painter == -1) this.painter = null;

         // if (this.painter)  $("#dabc_draw").append("<br>painter is created");
      }
   } else {
      gFile = this.obj;
      JSROOTPainter.displayStreamerInfos(this.obj.fStreamerInfo.fStreamerInfos, "#" + this.frameid);
      gFile = 0;
   }
   
   this.first_draw = false;
}

DABC.RootDrawElement.prototype.ReconstructRootObject = function() {
   if (!this.raw_data) {
      this.state = this.StateEnum.stInit;
      return;
   }

   var obj = {};
   
   obj['_typename'] = 'JSROOTIO.' + this.clname;

//   $("#dabc_draw").append("<br>Calling JSROOTIO function");

   gFile = this.sinfo.obj;

   if (this.clname == "TCanvas") {
      obj = JSROOTIO.ReadTCanvas(this.raw_data, 0);
      if (obj && obj['fPrimitives']) {
         if (obj['fName'] == "") obj['fName'] = "anyname";
      }
   } else 
   if (JSROOTIO.GetStreamer(this.clname)) {
      JSROOTIO.GetStreamer(this.clname).Stream(obj, this.raw_data, 0);
      JSROOTCore.addMethods(obj);

   } else {
      $("#dabc_draw").append("<br>!!!!! streamer not found !!!!!!!" + this.clname);
   }
   
   gFile = null;

   if (this.painter && this.painter.UpdateObject(obj)) {
      // if painter accepted object update, we need later just redraw frame
      obj = null;
   } else { 
      this.obj = obj;
      this.painter = null;
   }
   
   this.state = this.StateEnum.stReady;
   this.version = this.raw_data_version;
   
   this.raw_data = null;
   this.raw_data_version = 0;
   
   this.DrawObject();
}

DABC.RootDrawElement.prototype.RequestCallback = function(arg) {
   // in any case, request pointer will be reseted
   // delete this.req;
   this.req = null;
   
   if (this.state != this.StateEnum.stWaitRequest) {
      alert("item not in wait request state");
      this.state = this.StateEnum.stInit;
      return;
   }
   
   var hdr = DABC.UnpackBinaryHeader(arg);
   
   if (hdr==null) {
      $("#dabc_draw").append("<br> RootDrawElement get error " + this.itemname + "  reload list");
      this.state = this.StateEnum.stInit;
      // most probably, objects structure is changed, therefore reload it
      DABC.mgr.ReloadTree();
      return;
   }

   // if we got same version, do nothing - we are happy!!!
   if ((hdr.version > 0) && (this.version == hdr.version)) {
      this.state = this.StateEnum.stReady;
      // $("#dabc_draw").append("<br> Get same version " + ver + " of object " + this.itemname);
      if (this.first_draw) this.DrawObject();
      return;
   } 
   
   // $("#dabc_draw").append("<br> RootDrawElement get callback " + this.itemname + " sz " + arg.length + "  this.version = " + this.version + "  newversion = " + ver);

   if (!this.sinfo) {
      
      delete this.obj; 
      
      // we are doing sreamer infos
      if (gFile) $("#dabc_draw").append("<br> gFile already exists???"); 
      this.obj = new JSROOTIO.RootFile;

      gFile = this.obj;
      this.obj.fStreamerInfo.ExtractStreamerInfo(hdr.rawdata);
      this.version = hdr.version;
      this.state = this.StateEnum.stReady;
      this.raw_data = null;
      // this indicates that object was clicked and want to be drawn
      this.DrawObject();
         
      gFile = null;

      if (!this.obj) alert("Cannot reconstruct streamer infos!!!");

      // with streamer info one could try to update complex fields
      DABC.mgr.UpdateComplexFields();

      return;
   } 

   this.raw_data_version = hdr.version;
   this.raw_data = hdr.rawdata;
   this.raw_data_size = hdr.rawdata.length;
   this.need_master_version = hdr.master_version;
   
   if (this.sinfo && !this.sinfo.HasVersion(this.need_master_version)) {
      // $("#dabc_draw").append("<br> Streamer info is required of vers " + this.need_master_version);
      this.state = this.StateEnum.stWaitSinfo;
      this.sinfo.Update();
      return;
   }
   
   this.ReconstructRootObject();
}

DABC.RootDrawElement.prototype.RegularCheck = function() {

   if (DABC.load_root_js==0) {
      DABC.load_root_js = 1;
      AssertPrerequisites(function() {
         // $("#dabc_draw").append("<br> load main scripts done");
         DABC.load_root_js = 2; 
      }, true);
   }
   
   // in any case, complete JSRootIO is required before we could start 
   if (DABC.load_root_js < 2) return;
   
   switch (this.state) {
     case this.StateEnum.stInit: break;
     case this.StateEnum.stWaitRequest: return;
     case this.StateEnum.stWaitSinfo: { 
        // $("#dabc_draw").append("<br> item " + this.itemname+ " requires streamer info ver " + this.need_master_version  +  "  available is = " + this.sinfo.version);

        if (this.sinfo.HasVersion(this.need_master_version)) {
           this.ReconstructRootObject();
        } else {
           // $("#dabc_draw").append("<br> version is not ok");
        }
        return;
     }
     case this.StateEnum.stReady: {
        // if item ready, verify that we want to send request again
        if (!this.IsDrawn()) return;
        var chkbox = document.getElementById("monitoring");
        if (!chkbox || !chkbox.checked) return;
        
        // TODO: TCanvas update do not work in JSRootIO
        if (this.clname == "TCanvas") return;
        
        break;
     }
     default: return;
   }
   
   // $("#dabc_draw").append("<br> checking request for " + this.itemname + (this.sinfo.ready ? " true" : " false"));

   var url = "getbinary?" + this.itemname;
   
   if (this.version>0) url += "&ver=" + this.version;

   this.req = DABC.mgr.NewHttpRequest(url, true, true, this);

//   $("#dabc_draw").append("<br> Send request " + url);

   this.req.send(null);
   
   this.state = this.StateEnum.stWaitRequest;
}


// ======== end of RootDrawElement ======================



// ============= start of DABC.Manager =============== 

DABC.Manager = function() {
   this.cnt = new Number(0);            // counter to create new element 
   this.arr = new Array();  // array of DrawElement

   DABC.dabc_tree = new dTree('DABC.dabc_tree');
   DABC.dabc_tree.config.useCookies = false;
   
   this.CreateTable(3,3);
   
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
   var id = "#draw_place_" + this.table_counter;
   this.table_counter++;
   if (this.table_counter>=this.table_number) this.table_counter = 0;
   $(id).empty();
   return id;
}


DABC.Manager.prototype.FindItem = function(_item) {
   for (var i in this.arr) {
      if (this.arr[i].itemname == _item) return this.arr[i];
   }
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


DABC.Manager.prototype.NewHttpRequest = function(url, async, isbin, item) {
   var xhr = new XMLHttpRequest();
   
   xhr.dabc_item = item;
   xhr.isbin = isbin;

//   if (typeof ActiveXObject == "function") {
   if (window.ActiveXObject) {
      // $("#dabc_draw").append("<br> Create IE request");
      
      xhr.onreadystatechange = function() {
         // $("#dabc_draw").append("<br> Ready IE request");
         if (this.readyState != 4) return;
         if (!this.dabc_item || (this.dabc_item==0)) return;
         
         if (this.status == 200 || this.status == 206) {
            if (this.isbin) {
               var filecontent = new String("");
               var array = new VBArray(this.responseBody).toArray();
               for (var i = 0; i < array.length; i++) {
                  filecontent = filecontent + String.fromCharCode(array[i]);
               }
               
               // $("#dabc_draw").append("<br> IE response ver = " + ver);

               this.dabc_item.RequestCallback(filecontent);
               delete filecontent;
               filecontent = null;
            } else {
               this.dabc_item.RequestCallback(this.responseXML);
            }
         } else {
            this.dabc_item.RequestCallback(null);
         } 
         this.dabc_item = null;
      }
      
      xhr.open('POST', url, async);
      
   } else {
      
      xhr.onreadystatechange = function() {
         //$("#dabc_draw").append("<br> Response request "+this.readyState);

         if (this.readyState != 4) return;
         if (!this.dabc_item || (this.dabc_item==0)) return;
         
         if (this.status == 0 || this.status == 200 || this.status == 206) {
            if (this.isbin) {
               var HasArrayBuffer = ('ArrayBuffer' in window && 'Uint8Array' in window);
               var Buf;
               if (HasArrayBuffer && 'mozResponse' in this) {
                  Buf = this.mozResponse;
               } else if (HasArrayBuffer && this.mozResponseArrayBuffer) {
                  Buf = this.mozResponseArrayBuffer;
               } else if ('responseType' in this) {
                  Buf = this.response;
               } else {
                  Buf = this.responseText;
                  HasArrayBuffer = false;
               }
               if (HasArrayBuffer) {
                  var filecontent = new String("");
                  var bLen = Buf.byteLength;
                  var u8Arr = new Uint8Array(Buf, 0, bLen);
                  for (var i = 0; i < u8Arr.length; i++) {
                     filecontent = filecontent + String.fromCharCode(u8Arr[i]);
                  }
                  delete u8Arr;
               } else {
                  var filecontent = Buf;
               }
               this.dabc_item.RequestCallback(filecontent);

               delete filecontent;
               filecontent = null;
            } else {
               this.dabc_item.RequestCallback(this.responseXML);
            }
         } else {
            this.dabc_item.RequestCallback(null);
         }
         this.dabc_item = 0;
      }

      xhr.open('POST', url, async);
      
      if (xhr.isbin) {
         var HasArrayBuffer = ('ArrayBuffer' in window && 'Uint8Array' in window);
         if (HasArrayBuffer && 'mozResponseType' in xhr) {
            xhr.mozResponseType = 'arraybuffer';
         } else if (HasArrayBuffer && 'responseType' in xhr) {
            xhr.responseType = 'arraybuffer';
         } else {
            //XHR binary charset opt by Marcus Granado 2006 [http://mgran.blogspot.com]
            xhr.overrideMimeType("text/plain; charset=x-user-defined");
         }
      }
   }
   return xhr;
}

// This method used to update all simple fields together
DABC.Manager.prototype.UpdateSimpleFields = function() {
   if (this.empty()) return;

   var url = "chartreq.htm?";
   var cnt = 0;

   for (var i in this.arr) {
      if (!this.arr[i].simple()) continue;
      if (cnt++>0) url+="&";
      url += this.arr[i].itemname;
   }

   if (cnt==0) return;

   var req = this.NewRequest();
   if (!req) return;

   // $("#dabc_draw").append("<br> Send request " + url);

   req.open("POST", url, false);
   req.send();

   var repl = JSON && JSON.parse(req.responseText) || $.parseJSON(req.responseText);

   // $("#dabc_draw").append("<br> Get reply " + req.responseText);

   req = 0;
   if (!repl) return;

   for (var indx in repl) {
      var elem = this.FindItem(repl[indx].name);
      // $("#dabc_draw").append("<br> Check reply for " + repl[indx].name);

      if (elem) elem.SetValue(repl[indx].value);
   }
}

DABC.Manager.prototype.UpdateComplexFields = function() {
   if (this.empty()) return;

   for (var i in this.arr) 
     this.arr[i].RegularCheck();
}

DABC.Manager.prototype.UpdateAll = function() {
   this.UpdateSimpleFields();
   this.UpdateComplexFields();
}

DABC.Manager.prototype.display = function(itemname) {
   if (!itemname) return;

/*   
   $("#dabc_draw").append("<br> display click "+itemname);
   if (JSROOT.elements) $("#dabc_draw").append("<br> elements are there ");
   var topid = JSROOT.elements.generateNewFrame("report");
   var elem = new JSROOT.DrawElement();
   JSROOT.elements.initElement(topid, elem, true);
   elem.makeCollapsible();
   elem.setInfo("Any Element");
   elem.appendText("<br>somethiung to see");
   elem.appendText("<br>somethiung to see");
   $("#dabc_draw").append("<br> source "+JSROOT.source_dir );
*/  
   var xmlnode = this.FindXmlNode(itemname);
   if (!xmlnode) {
      $("#dabc_draw").append("<br> cannot find xml node "+itemname);
      return;
   }
   
   var kind = xmlnode.getAttribute("kind");
   if (!kind) return;

//   $("#dabc_draw").append("<br> find kind of node "+itemname + " " + kind);
   
   var elem = this.FindItem(itemname);

   if (elem) {
      elem.ClickItem();
      return;
   }

   // ratemeter
   if (kind == "rate") {
      elem = new DABC.GaugeDrawElement();
      elem.itemname = itemname;
      elem.CreateFrames(this.NextCell(), this.cnt++);
      elem.SetValue(xmlnode.getAttribute("value"));
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
   
   
   // any non-ROOT is ignored for the moment
   if (kind.indexOf("ROOT.") != 0) return; 
         
   var sinfoname = this.FindMasterName(itemname, xmlnode);
   
   var sinfo = this.FindItem(sinfoname);
   
   if (sinfoname && !sinfo) {
      sinfo = new DABC.RootDrawElement(kind.substr(5));
      sinfo.itemname = sinfoname;
      this.arr.push(sinfo);
      // mark sinfo item as ready - it will not be requested until is not really required
      sinfo.state = sinfo.StateEnum.stReady;
   }
      
   elem = new DABC.RootDrawElement(kind.substr(5));
   elem.itemname = itemname;
   elem.sinfo = sinfo;
   elem.CreateFrames(this.NextCell(), this.cnt++);
   this.arr.push(elem);
   
   elem.RegularCheck();
}

DABC.Manager.prototype.DisplayHiearchy = function(holder) {
   var elem = this.FindItem("ObjectsTree");
   
   if (elem) return;

   // $("#dabc_draw").append("<br> start2");

   elem = new DABC.HierarchyDrawElement();
   
   elem.itemname = "ObjectsTree";

   elem.CreateFrames(holder, this.cnt++);
   
   this.arr.push(elem);
   
   elem.RegularCheck();
}

DABC.Manager.prototype.ReloadTree = function() 
{
   var elem = this.FindItem("ObjectsTree");
   if (!elem) return;
   
   elem.ready = false;
   elem.RegularCheck();
}

DABC.Manager.prototype.ClearWindow = function()
{
   $("#dialog").empty();
   
   var elem = this.FindItem("ObjectsTree");

   for (var i=0;i<this.arr.lentgh;i++) {
      if (this.arr[i] == elem) continue;
      this.arr[i].Clear();
   }
   
   this.arr.splice(0, this.arr.length);
   this.arr.push(elem);
   
   this.CreateTable(3,3);
   
   elem.ready = false;
   elem.RegularCheck();
}


DABC.Manager.prototype.FindXmlNode = function(itemname) {
   var elem = this.FindItem("ObjectsTree");
   if (!elem) return;
   return elem.FindNode(itemname);
}

/** \brief Method finds element in structure, which must be loaded before item itself can be loaded
 *   In case of ROOT objects it is StreamerInfo */

DABC.Manager.prototype.FindMasterName = function(itemname, itemnode) {
   if (!itemnode) return;
   
   var master = itemnode.getAttribute("master");
   if (!master) return;
   
   var newname = itemname;

   while (newname) {
      var separ = newname.lastIndexOf("/");
      if (separ<=0) {
         alert("Name " + itemname + " is not long enouth for master " + itemnode.getAttribute("master"));
         return;
      }
      newname = newname.substr(0, separ);
      
      if (master.indexOf("../")<0) break;
      master = master.substr(3);
   }
   
   return newname + "/" + master;
}



// ============= end of DABC.Manager =============== 
