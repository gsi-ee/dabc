DABC = {};

DABC.version = "2.1.2";

DABC.mgr = 0;

DABC.dabc_tree = 0;   // variable to display hierarchy

DABC.load_root_js = 0; // 0 - not started, 1 - doing load, 2 - completed

DABC.DrawElement = function() {
   this.itemname = "";   // full item name in hierarhcy
   this.frameid = new Number(0);    // id of top frame, where item is drawn
   this.version = new Number(0);    // check which version of element is drawn
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
   if (!val || !this.frameid) return;
   document.getElementById(this.frameid).innerHTML = "Value = " + val;
}

DABC.DrawElement.prototype.IsDrawn = function() {
   if (!this.frameid) return false;
   
   if (!document.getElementById(this.frameid)) return false;
   
   return true;
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

   // $("#report").append("<br> Create xml request");
   
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

      // $("#report").append("<br> Work with node " + node.nodeName);
      
      var kind = node.getAttribute("kind");
      var value = node.getAttribute("value");
      
      var html = "";
      
      var nodefullname  = fullname + "/" + node.nodeName; 
      
      var nodeimg = "";
      var node2img = "";
      
      if (kind) {
         html = "javascript: DABC.mgr.display('"+nodefullname+"');";

         // $("#report").append("<br>See kind = " + kind);
         
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

//   $("#report").append("<br> Serching for " + fullname);

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


DABC.HierarchyDrawElement.prototype.RequestCallback = function(arg, ver, mver) {
   this.req = 0;

   if ((ver<0) || !arg) { this.ready = false; return; }

   // $("#report").append("<br> Get XML request callback "+ver);

   this.version = ver;
   
   this.xmldoc = arg;
   
   // $("#report").append("<br> xml doc is there");
   
   var top = this.TopNode();
   
   if (!top) { 
      $("#report").append("<br> XML top node not found");
      return;
   }

   var top_version = top.getAttribute("dabc:version");

   if (!top_version) {
      $("#report").append("<br> dabc:version not found");
      return;
   } else {
      this.version = top_version;
      // $("#report").append("<br> found dabc:version " + this.version);
      // $("#report").append("<br> found node " + top.nodeName);
   }

   
   this.createNode(0, -1, top.firstChild, "");

   var content = "<p><a href='javascript: DABC.dabc_tree.openAll();'>open all</a> | <a href='javascript: DABC.dabc_tree.closeAll();'>close all</a> | <a href='javascript: DABC.mgr.ReloadTree();'>reload</a> </p>";
   content += DABC.dabc_tree;
   $("#" + this.frameid).html(content);
   
   DABC.dabc_tree.openAll();
   
   this.ready = true;
}

// ======== end of HierarchyDrawElement ======================



// ======== start of RootDrawElement ======================

DABC.RootDrawElement = function(_clname) {
   DABC.DrawElement.call(this);

   this.clname = _clname;    // ROOT class name
   this.obj = null;          // object itself, for streamer info is file instance
   this.sinfo = null;        // used to refer to the streamer info record
   this.req = null;          // this is current request
   this.titleid = "";     
   this.drawid = 0;       // numeric id of pad, where ROOT object is drawn
   this.first_draw = true;  // one should enable flag only when all ROOT scripts are loaded
   
   this.raw_data = null;  // raw data kept in the item when object cannot be reconstructed immediately
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

// TODO: check how it works in different older browsers
DABC.RootDrawElement.prototype = Object.create( DABC.DrawElement.prototype );

DABC.RootDrawElement.prototype.CreateFrames = function(topid, id) {
   this.drawid = id;
   this.frameid = "histogram" + this.drawid;
   this.titleid = "uid_accordion_" + this.drawid; 
   
   this.first_draw = true;
   
   var entryInfo = "";
   if (this.sinfo) {
      entryInfo = "<h5 id='"+this.titleid+"'><a id='"+this.titleid + "_text'>" + this.itemname + "</a></h5>";
      entryInfo += "<div id='" + this.frameid + "'>";
   } else {
      entryInfo = "<h5 id=\""+this.titleid+"\"><a> Streamer Infos </a>&nbsp; </h5><div><br>";
      entryInfo += "<h6>Streamer Infos</h6><span id='" + this.frameid +"' class='dtree'></span></div><br>";

   }
   $(topid).append(entryInfo);
   
//   $("#report").append("is " + $("#"+this.titleid).schemaTypeInfo); 
}

DABC.RootDrawElement.prototype.ClickItem = function() {
   if (this.state != this.StateEnum.stReady) return; 

   // TODO: TCanvas update do not work in JSRootIO
   if (this.clname == "TCanvas") return;

   if (!this.IsDrawn()) 
      this.CreateFrames("#report", this.cnt++);
      
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
   if ((this.drawid==0) || !this.obj) return;

   var child = document.getElementById(this.titleid + "_text");
   // if (child) $("#report").append("<br>child " + child.innerHTML);
   if (child) child.innerHTML = 
      this.itemname + ",   version = " + this.version + ", size = " + this.raw_data_size; 
   
   if (this.sinfo) 
      JSROOTPainter.drawObject(this.obj, this.drawid);
   else {
      gFile = this.obj;
      JSROOTPainter.displayStreamerInfos(this.obj.fStreamerInfo.fStreamerInfos, "#" + this.frameid);
      gFile = 0;
   }
   
   if (this.first_draw) addCollapsible("#"+this.titleid);
   this.first_draw = false;
}

DABC.RootDrawElement.prototype.UnzipRawData = function() {
   
   // method returns true if unpacked data can be used
   
   if (!this.raw_data) return false;

   var ziphdr = gFile.R__unzip_header(this.raw_data, 0, true);
   
   if (!ziphdr) return true;
  
   var unzip_buf = gFile.R__unzip(ziphdr['srcsize'], this.raw_data, 0, ziphdr['tgtsize']);
   if (!unzip_buf) {
      alert("fail to unzip data");
      return false;
   } 
   
   this.raw_data = unzip_buf['unzipdata'];
   // $("#report").append("<br>unpzip buffer " + this.raw_data_size + " to length "+ this.raw_data.length);
   unzip_buf = null;
   return true;
}


DABC.RootDrawElement.prototype.ReconstructRootObject = function() {
   if (!this.raw_data) {
      this.state = this.StateEnum.stInit;
      return;
   }

   this.obj = {};
   this.obj['_typename'] = 'JSROOTIO.' + this.clname;

   gFile = this.sinfo.obj;

   // one need gFile to unzip data
   if (!this.UnzipRawData()) return;

   if (this.clname == "TCanvas") {
      this.obj = JSROOTIO.ReadTCanvas(this.raw_data, 0);
      if (this.obj && this.obj['fPrimitives']) {
         if (this.obj['fName'] == "") this.obj['fName'] = "anyname";
      }
   } else 
   if (JSROOTIO.GetStreamer(this.clname)) {
      JSROOTIO.GetStreamer(this.clname).Stream(this.obj, this.raw_data, 0);
      JSROOTCore.addMethods(this.obj);
   } else {
      $("#report").append("<br>!!!!! streamer not found !!!!!!!" + this.clname);
   }

   this.state = this.StateEnum.stReady;
   this.version = this.raw_data_version;
   
   this.raw_data = null;
   this.raw_data_version = 0;
   
   this.DrawObject();
   
}

DABC.RootDrawElement.prototype.RequestCallback = function(arg, ver, mver) {
   // in any case, request pointer will be reseted
   // delete this.req;
   this.req = 0;
   
   if (this.state != this.StateEnum.stWaitRequest) {
      alert("item not in wait request state");
      this.state = this.StateEnum.stInit;
      return;
   }
   
   if ((ver < 0) || !arg) {
      $("#report").append("<br> RootDrawElement get error " + this.itemname + "  reload list");
      this.state = this.StateEnum.stInit;
      // most probably, objects structure is changed, therefore reload it
      DABC.mgr.ReloadTree();
      return;
   }

   // if we got same version, do nothing - we are happy!!!
   if ((ver > 0) && (this.version == ver)) {
      this.state = this.StateEnum.stReady;
      $("#report").append("<br> Get same version " + ver + " of object " + this.itemname);
      if (this.first_draw) this.DrawObject();
      return;
   } 
   
   // $("#report").append("<br> RootDrawElement get callback " + this.itemname + " sz " + arg.length + "  this.version = " + this.version + "  newversion = " + ver);

   if (!this.sinfo) {
      
      delete this.obj; 
      
      // we are doing sreamer infos
      // if (gFile) $("#report").append("<br> gFile already exists???"); else 
      this.obj = new JSROOTIO.RootFile;

      // one need gFile to unzip data
      gFile = this.obj;
      
      this.raw_data = arg;
      
      if (this.UnzipRawData()) {
         this.obj.fStreamerInfo.ExtractStreamerInfo(this.raw_data);
         this.version = ver;
         this.state = this.StateEnum.stReady;
         this.raw_data = null;
         // this indicates that object was clicked and want to be drawn
         this.DrawObject();
      } else {
         this.obj = null;
      }
         
      gFile = null;

      // $("#report").append("<br> Unpacked streamer infos of version " + ver);
      if (!this.obj) $("#report").append("<br> No file in streamer infos!!!");

      // with streamer info one could try to update complex fields
      DABC.mgr.UpdateComplexFields();

      return;
   } 

   this.raw_data_version = ver;
   this.raw_data = arg;
   this.raw_data_size = arg.length;
   
   if (mver) this.need_master_version = mver;
   
   if (this.sinfo && !this.sinfo.HasVersion(this.need_master_version)) {
      // $("#report").append("<br> Streamer info is required of vers " + this.need_master_version);
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
         DABC.load_root_js = 2; 
         // $("#report").append("<br> load all JSRootIO scripts");
         DABC.mgr.UpdateComplexFields();
      });
   }

   // in any case, complete JSRootIO is required before we could start 
   if (DABC.load_root_js < 2) return;
   
   switch (this.state) {
     case this.StateEnum.stInit: break;
     case this.StateEnum.stWaitRequest: return;
     case this.StateEnum.stWaitSinfo: { 
        // $("#report").append("<br> item " + this.itemname+ " requires streamer info ver " + this.need_master_version  +  "  available is = " + this.sinfo.version);

        if (this.sinfo.HasVersion(this.need_master_version)) {
           // $("#report").append("<br> try to reconstruct");
           this.ReconstructRootObject();
        } else {
           // $("#report").append("<br> version is not ok");
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
   
   // $("#report").append("<br> checking request for " + this.itemname + (this.sinfo.ready ? " true" : " false"));

   var url = "getbinary?" + this.itemname;
   
   if (this.version>0) url += "&ver=" + this.version;

   this.req = DABC.mgr.NewHttpRequest(url, true, true, this);

   // $("#report").append("<br> Send request " + url);

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
   
   return this;
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
      // $("#report").append("<br> Create IE request");
      
      xhr.onreadystatechange = function() {
         // $("#report").append("<br> Ready IE request");
         if (this.readyState != 4) return;
         if (!this.dabc_item || (this.dabc_item==0)) return;
         
         if (this.status == 200 || this.status == 206) {
            if (this.isbin) {
               var filecontent = new String("");
               var array = new VBArray(this.responseBody).toArray();
               for (var i = 0; i < array.length; i++) {
                  filecontent = filecontent + String.fromCharCode(array[i]);
               }

               var ver = this.getResponseHeader("Content-Version");
               if (!ver) {
                  ver = -1;
                  $("#report").append("<br> Response version not specified");
               } 
               
               var mver = this.getResponseHeader("Master-Version");
               
               // $("#report").append("<br> IE response ver = " + ver);

               this.dabc_item.RequestCallback(filecontent, Number(ver), Number(mver));
               delete filecontent;
               filecontent = null;
            } else {
               this.dabc_item.RequestCallback(this.responseXML, 0);
            }
         } else {
            this.dabc_item.RequestCallback(null, -1);
         } 
         this.dabc_item = null;
      }
      
      xhr.open('POST', url, async);
      
   } else {
      
      xhr.onreadystatechange = function() {
         //$("#report").append("<br> Response request "+this.readyState);

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

               var ver = this.getResponseHeader("Content-Version");
               if (!ver) {
                  ver = -1;
                  $("#report").append("<br> Response version not specified");
               } 
               
               var mver = this.getResponseHeader("Master-Version");

               this.dabc_item.RequestCallback(filecontent, Number(ver), Number(mver));

               delete filecontent;
               filecontent = null;
            } else {
               this.dabc_item.RequestCallback(this.responseXML, 0);
            }
         } else {
            this.dabc_item.RequestCallback(null, -1);
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

   // $("#report").append("<br> Send request " + url);

   req.open("POST", url, false);
   req.send();

   var repl = JSON && JSON.parse(req.responseText) || $.parseJSON(req.responseText);

   // $("#report").append("<br> Get reply " + req.responseText);

   req = 0;
   if (!repl) return;

   for (var indx in repl) {
      var elem = this.FindItem(repl[indx].name);
      // $("#report").append("<br> Check reply for " + repl[indx].name);

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
   
//   $("#report").append("<br> display click "+itemname);
  
   var xmlnode = this.FindXmlNode(itemname);
   if (!xmlnode) {
      $("#report").append("<br> cannot find xml node "+itemname);
      return;
   }
   
   var kind = xmlnode.getAttribute("kind");
   if (!kind) return;

//   $("#report").append("<br> find kind of node "+itemname + " " + kind);
   
   var elem = this.FindItem(itemname);

   if (elem) {
      elem.ClickItem();
      return;
   }

   // ratemeter
   if (kind == "rate") {
      elem = new DABC.GaugeDrawElement();
      elem.itemname = itemname;
      elem.CreateFrames("#report", this.cnt++);
      elem.SetValue(xmlnode.getAttribute("value"));
      this.arr.push(elem);
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
   elem.CreateFrames("#report", this.cnt++);
   this.arr.push(elem);
   
   elem.RegularCheck();
}

DABC.Manager.prototype.DisplayHiearchy = function(holder) {
   var elem = this.FindItem("ObjectsTree");
   
   if (elem) return;

   // $("#report").append("<br> start2");

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
