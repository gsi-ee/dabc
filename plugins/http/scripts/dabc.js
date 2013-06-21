DABC = {};

DABC.version = "2.1.1";

DABC.mgr = 0;

DABC.DrawElement = function() {
   this.itemname = "";   // full item name in hierarhcy
   this.frameid = 0;    // id of top frame, where item is drawn
   this.version = 0;    // check which version of element is drawn
   this.ready = false;   // indicate that element completed with drawing
   return this;
}

// indicates if element operates with simple text fields, 
// which can be processed by SetValue() method
DABC.DrawElement.prototype.simple = function() { return false; }

// method regularly called by the manager
// it is responsibility of item itself create request and perform update
DABC.DrawElement.prototype.CheckComplexRequest = function() { return; }

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
   
   this.ready = true;
}

DABC.GaugeDrawElement.prototype.SetValue = function(val) {
   if (val) this.gauge.refresh(val);
}


// ======== end of GaugeDrawElement ======================


// ======== start of RootDrawElement ======================

DABC.RootDrawElement = function(_clname) {
   DABC.DrawElement.call(this);

   this.clname = _clname; // ROOT class name
   this.obj = 0;          // object itself
   this.sinfo = 0;        // used to refer to the streamer info record
   this.req = 0;          // this is current request
   this.titleid = "";     
   this.drawid = 0;       // numeric id of pad, where ROOT object is drawn 
}

// TODO: check how it works in different older browsers
DABC.RootDrawElement.prototype = Object.create( DABC.DrawElement.prototype );

DABC.RootDrawElement.prototype.CreateFrames = function(topid, id) {
   this.drawid = id;
   this.frameid = "histogram" + this.drawid;
   this.titleid = "uid_accordion_" + this.drawid; 
   var entryInfo = "<h5 id='"+this.titleid+"'><a>" + this.itemname + "</a>&nbsp; </h5>\n";
   entryInfo += "<div id='" + this.frameid + "'>\n";
   $(topid).append(entryInfo);
}

DABC.RootDrawElement.prototype.BinaryCallback = function(arg) {
   // in any case, request pointer will be reseted
   this.req = 0;

   if (!arg) {
      $("#report").append("<br> RootDrawElement get callback " + this.itemname + " error");
      return; 
   }

   // $("#report").append("<br> RootDrawElement get callback " + this.itemname + " sz " + arg.length);

   if (!this.sinfo) {
      // we are doing sreamer infos
      if (gFile) $("#report").append("<br> gFile already exists???"); 
      else gFile = new JSROOTIO.RootFile;

      gFile.fStreamerInfo.ExtractStreamerInfo(arg);

      // we are done with the reconstructing of streamer infos
      this.ready = true;

      // $("#report").append("<br> Streamer infos unpacked!!!");

      // with streamer info one could try to update complex fields
      DABC.mgr.UpdateComplexFields();

      return;
   } 
   // not try to update again
   this.ready = true;

   var was_object = (this.obj != 0);
   
   this.obj = {};
   this.obj['_typename'] = 'JSROOTIO.' + this.clname;

   // $("#report").append("<br>make streaming " + arg.length + " for class "+ this.clname);

   if (!JSROOTIO.GetStreamer(this.clname))
      $("#report").append("<br>!!!!! streamer not found !!!!!!!");

   JSROOTIO.GetStreamer(this.clname).Stream(this.obj, arg, 0);

   JSROOTCore.addMethods(this.obj);

   // $("#report").append("<br>Object name " + this.obj['fName']+ " class "  + this.obj['_typename'] + "  created");

   if (was_object) {
      // here should be redraw of the object 
      JSROOTPainter.drawObject(this.obj, this.drawid);
      // $("#report").append("<br>WE DID IT AGAIN");
   } else {
      JSROOTPainter.drawObject(this.obj, this.drawid);
      addCollapsible("#"+this.titleid);
   }
}

DABC.RootDrawElement.prototype.CheckComplexRequest = function() {
   // in any case streamer info is required before normal request can be submitted

   if (this.sinfo) {
//    $("#report").append("<br> checking sinfo " + (this.sinfo.ready ? "true" : "false"));
   } else {
//    $("#report").append("<br> sinfo itself " + (this.ready ? "true" : "false"));
   }

   // in any case one should wait streamer info
   if (this.sinfo && !this.sinfo.ready) return;

   // when streamer info ready, than ignore request 
   if (!this.sinfo && this.ready) return;

   // if request is created and running, do nothing
   if (this.req) return;

   // new request will be started only when ready flag is not null
   if (this.ready) return;

   var url = "getbinary?" + this.itemname;
   
   if (this.version>0) url += "&v=" + this.version;

   this.req = DABC.mgr.NewBinRequest(url, true, this);

   // $("#report").append("<br> Send request " + url);

   this.req.send();
}

// ======== end of RootDrawElement ======================



// ============= start of DABC.Manager =============== 

DABC.Manager = function() {
   this.cnt = 0;            // counter to create new element 
   this.arr = new Array();  // array of DrawElement
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


DABC.Manager.prototype.NewBinRequest = function(url, async, item) {
   var xhr = new XMLHttpRequest();

   if (typeof ActiveXObject == "function") {
      xhr.onreadystatechange = function() {
         if (this.readyState == 4 && (this.status == 200 || this.status == 206)) {
            var filecontent = new String("");
            var array = new VBArray(this.responseBody).toArray();
            for (var i = 0; i < array.length; i++) {
               filecontent = filecontent + String.fromCharCode(array[i]);
            }
            // DABC.mgr.BinaryCallback(itemname, filecontent);
            item.BinaryCallback(filecontent);
            delete filecontent;
            filecontent = null;
         }
         xhr.open('POST', url, async);  
      }
   } else {
      xhr.onreadystatechange = function() {
         if (this.readyState == 4 && (this.status == 0 || this.status == 200 ||
               this.status == 206)) {
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

            item.BinaryCallback(filecontent);

            // DABC.mgr.BinaryCallback(itemname, filecontent);

            delete filecontent;
            filecontent = null;
         }
      }

      xhr.open('POST', url, async);
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
      if (!this.arr[i].simple())
         this.arr[i].CheckComplexRequest();
}

DABC.Manager.prototype.UpdateAll = function() {
   this.UpdateSimpleFields();
   this.UpdateComplexFields();
}


DABC.Manager.prototype.click = function(item) {
   var itemname = item.getAttribute("fullname");

   var elem = this.FindItem(itemname);

   if (elem) {
      if (!elem.simple()) {
         elem.ready = false; // indicate what we want to update it
         this.UpdateComplexFields();
      }
      return;
   }

   var kind = item.getAttribute("kind");
   
   var check_compl = false;

   if (kind == "rate") {
      elem = new DABC.GaugeDrawElement();
   } else
   if (kind.indexOf("ROOT.") == 0) {

      // one need to find item with StreamerInfo
      // FIXME: later one need to find streamer info without any naming conventions
      var pos = itemname.indexOf("ROOT/");
      if (!pos) {
         alert("String ROOT/ not found in ROOT item name - requiered to find StreamerInfo.");
         return;
      }
      var sinfoname = itemname.substr(0, pos+5) + "StreamerInfo";

      // this element is only required 
      var sinfo = this.FindItem(sinfoname);
      
      if (!sinfo) {
         AssertPrerequisites(function() { $("#report").append("<br>Loading JSRootIO done<br> sinfo item " + sinfoname); });
         sinfo = new DABC.RootDrawElement(kind.substr(5));
         sinfo.itemname = sinfoname;
         this.arr.push(sinfo);
      }

      // if we clicked info, just ensure that it was created (but not drawn)
      if (itemname == sinfoname) {
         this.UpdateComplexFields();
         return;
      }
      
      elem = new DABC.RootDrawElement(kind.substr(5));
      elem.sinfo = sinfo;
      check_compl = true;
   }

   if (elem==0) return;

   elem.itemname = itemname;

   elem.CreateFrames("#report", this.cnt++);

   elem.SetValue(item.getAttribute("value"));
   
   this.arr.push(elem);
   
   if (check_compl) this.UpdateComplexFields();
}

// ============= end of DABC.Manager =============== 
