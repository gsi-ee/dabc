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

   "use strict";

   if (JSROOT.settings)
      JSROOT.settings.DragAndDrop = true;
   else
      JSROOT.gStyle.DragAndDrop = true;

   DABC = {};

   DABC.version = "2.10.1 23/05/2019";

   DABC.source_dir = function(){
      var scripts = document.getElementsByTagName('script');

      for (var n in scripts) {
         if (scripts[n]['type'] != 'text/javascript') continue;

         var src = scripts[n]['src'];
         if ((src == null) || (src.length == 0)) continue;

         var pos = src.indexOf("dabc.js");
         if (pos<0) continue;
         if (src.indexOf("JSRootCore.")>0) continue;

         console.log("Set DABC.source_dir to " + src.substr(0, pos) + ", " + DABC.version);
         return src.substr(0, pos);
      }
      return "";
   }();

   if (typeof JSROOT.httpRequest == 'function')
      DABC.httpRequest = JSROOT.httpRequest;
   else
      DABC.httpRequest = function(url, kind, post_data) {
         return new Promise((resolveFunc,rejectFunc) => {
            let req = JSROOT.NewHttpRequest(url,kind, (res) => {
               if (res === null)
                  rejectFunc(Error(`Fail to request ${url}`));
               else
                  resolveFunc(res);
            });

            req.send(post_data || null);
         });
      }

   DABC.hasUrlOption = function(name) {
      if (typeof JSROOT.decodeUrl == 'function')
        return JSROOT.decodeUrl().has(name);
      return JSROOT.GetUrlOption(name) !== null;
   }

   let BasePainter = JSROOT.BasePainter || JSROOT.TBasePainter;
   let ObjectPainter = JSROOT.ObjectPainter || JSROOT.TObjectPainter;


   DABC.InvokeCommand = function(itemname, args) {
      var url = itemname + "/execute";
      if (args && (typeof args == 'string')) url += "?" + args;

      DABC.httpRequest(url,"object");
   }


   // method for custom HADAQ-specific GUI, later could be moved into hadaq.js script

   DABC.HadaqDAQControl = function(hpainter, itemname) {
      var mdi = hpainter.GetDisplay();
      if (!mdi) return null;

      var frame = mdi.FindFrame(itemname, true);
      if (!frame) return null;

      var frameid = d3.select(frame).attr('id');

      var item = hpainter.Find(itemname);
      var calarr = [];
      if (item) {
         for (var n in item._parent._childs) {
            var name = item._parent._childs[n]._name;

            if ((name.indexOf("TRB")==0) && (name.indexOf("TdcCal")>0)) {
               var fullname = hpainter.itemFullName(item._parent._childs[n]);
               calarr.push(fullname);
            }
         }
      }

      var html = "<fieldset>" +
                 "<legend>DAQ</legend>" +
                 "<button class='hadaq_startfile'>Start file</button>" +
                 "<button class='hadaq_stopfile'>Stop file</button>" +
                 "<button class='hadaq_restartfile'>Restart file</button>" +
                 '<input class="hadaq_filename" type="text" name="filename" value="file.hld" style="margin-top:5px;"/><br/>' +
                 "<label class='hadaq_rate'>Rate: __undefind__</label><br/>"+
                 "<label class='hadaq_info'>Info: __undefind__</label>"+
                 "</fieldset>" +
                 "<fieldset>" +
                 "<legend>Calibration</legend>";
      for (var n in calarr)
         html += "<div class='hadaq_calibr' style='padding:2px;margin:2px'>" + calarr[n] + "</div>";
      html+="</fieldset>";

      d3.select(frame).html(html);

      $(frame).find(".hadaq_startfile").button().click(function() {
         DABC.InvokeCommand(itemname+"/StartHldFile", "filename="+$(frame).find('.hadaq_filename').val()+"&maxsize=2000");
      });
      $(frame).find(".hadaq_stopfile").button().click(function() {
         DABC.InvokeCommand(itemname+"/StopHldFile");
      });
      $(frame).find(".hadaq_restartfile").button().click(function() {
         DABC.InvokeCommand(itemname+"/RestartHldFile");
      });

      var inforeq = false;

      function UpdateDaqStatus(res) {
         if (res==null) return;
         var rate = "";
         for (var n in res._childs) {
            var item = res._childs[n], lbl = "", units = "";

            if (item._name=='HadaqInfo')
               $(frame).find('.hadaq_info').text("Info: " + item.value);
            if (item._name=='HadaqData') { lbl = "Rate: "; units = " " + item.units+"/s"; }
            if (item._name=='HadaqEvents') { lbl = "Ev: "; units = " Hz"; }
            if (item._name=='HadaqLostEvents') { lbl = "Lost:"; units = " Hz"; }
            if (lbl) {
               if (rate.length>0) rate += " ";
               rate += lbl + item.value + units;
            }
         }

         $(frame).find('.hadaq_rate').css("font-size","120%").text(rate);
      }

      var handler = setInterval(function() {
         if ($("#" + frameid + " .hadaq_info").length==0) {
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

         inforeq = true;
         DABC.httpRequest(url, "object").then(res => {
            if (!res) return;
            UpdateDaqStatus(res[0].result);
            res.shift();
            DABC.UpdateTRBStatus($(frame).find('.hadaq_calibr'), res, hpainter, true);
         }).finally(() => { inforeq = false; });
      }, 2000);
   }

   DABC.UpdateTRBStatus = function(holder, res, hpainter, multiget) {

      function makehname(prefix, code, name) {
         var str = code.toString(16).toUpperCase();
         while (str.length<4) str = "0"+str;
         str = prefix+"_"+str;
         if (name) str += "_"+name;
         var hitem = null;
         hpainter.ForEach(function(item) { if ((item._name == str) && (hitem==null)) hitem = item; });
         if (hitem) return hpainter.itemFullName(hitem);
         console.log("did not found histogram " + str);
         return str;
      }

      function get_status_color(status) {
         if (status.indexOf('Ready')==0) return 'green';
         if (status.indexOf('File')==0) return 'blue';
         if (status.indexOf('Accum')==0) return 'lightblue';
         return 'red';
      }

      holder.each(function(index) {
         if (!res) return;
         var info = multiget ? res[index] : res;
         // when doing multiget, return object stored as result field
         if (('result' in info) && multiget) info = info.result;
         if (!info) return;

         if ($(this).children().length == 0) {
            var code = "<div style='float:left'>";
            code += "<button title='clear all TRB histograms' hist='" + makehname("TRB", info.trb) + "'>Clr</button>";
            if (!info.tdc) {
               code += "<button hist='" + makehname("TRB", info.trb, "ErrorBits") + "'>"+info.trb.toString(16)+"_ErrorBits</button>";
               code += "<button hist='" + makehname("TRB", info.trb, "TrigType") + "'>"+info.trb.toString(16)+"_TrigType</button>";
               code += "<button hist='" + makehname("TRB", info.trb, "SubevSize") + "'>"+info.trb.toString(16)+"_SubevSize</button>";
            } else {
               code += "<button hist='" + makehname("TRB", info.trb) + "'>Log</button>";
               code += "<button hist='" + makehname("TRB", info.trb, "TrigType") + "'>TrigType</button>";
               code += "<button hist='" + makehname("TRB", info.trb, "MsgPerTDC") + "'>MsgPerTDC</button>";
               for (var j in info.tdc)
                  code+="<button class='tdc_btn' tdc='" + info.tdc[j] + "' hist='" + makehname("TDC", info.tdc[j], "Channels") + "'>"+info.tdc[j].toString(16)+"</button>";
            }
            code += "</div>";
            if (info.tdc)
               code += "<div class='hadaq_progress'></div>";
            $(this).html(code);
            $(this).find("button").button().click(function(){
               var histname = $(this).attr('hist');
               if ($(this).text() == "Clr") {
                  DABC.httpRequest(histname+"/cmd.json?command=ClearHistos", "object");
                  return;
               }
               if ($(this).text() == "Log") {
                  histname = histname.substr(0, histname.length-8) + "CalibrLog";
                  console.log('Try to request log item', histname);
               }

               var frame = hpainter.GetDisplay().FindFrame("dabc_drawing");
               if (frame) hpainter.GetDisplay().CleanupFrame(frame);

               hpainter.displayAll([histname],["frameid:dabc_drawing"]);
            });
            if (info.tdc)
               $(this).find(".hadaq_progress").progressbar({ value: info.progress });
         }

         $(this).css('background-color', get_status_color(info.value));
         $(this).attr('title',"TRB:" + info.trb.toString(16) + " State: " + info.value + " Time:" + info.time);

         $(this).find(".hadaq_progress")
            .attr("title", "progress: " + info.progress + "%")
            .progressbar("option", "value", info.progress);

         for (var j in info.tdc) {
            $(this).find(".tdc_btn[tdc='"+info.tdc[j]+"']")
                   .css('color', get_status_color(info.tdc_status[j]))
                   .css('background', 'white')
                   .attr("title", "TDC" + info.tdc[j].toString(16) + " " + info.tdc_status[j] + " Progress:" + info.tdc_progr[j]);
         }
      });
   }


   DABC.StreamControl = function(hpainter, itemname) {
      var mdi = hpainter.GetDisplay();
      if (!mdi) return null;

      var frame = mdi.FindFrame(itemname, true);
      if (!frame) return null;

      var ffid = d3.select(frame).attr('id');

      var item = hpainter.Find(itemname + "/Status");
      var calarr = [];
      if (item) {
         for (var n in item._childs)
            calarr.push(hpainter.itemFullName(item._childs[n]));
      }

      var html = "<fieldset>" +
                 "<legend>Stream</legend>" +
                 "<button class='store_startfile' title='Start storage of ROOT file'>Start file</button>" +
                 "<button class='store_stopfile' title='Stop storage of ROOT file'>Stop file</button>" +
                 '<input class="store_filename" type="text" name="filename" value="file.root" style="margin-top:5px;"/><br/>' +
                 "<label class='stream_rate'>Rate: __undefind__</label><br/>" +
                 "<label class='stream_info'>Info: __undefind__</label>" +
                 "</fieldset>" +
                 "<fieldset>" +
                 "<legend>Calibration</legend>";
      for (var n in calarr)
         html += "<div class='stream_tdc_calibr' style='padding:2px;margin:2px'>" + calarr[n] + "</div>";
      html+="</fieldset>";

      d3.select(frame).html(html);

      $(frame).find(".store_filename").prop("title", "Name of ROOT file to store.\nOne can specify store kind and maxsize with args like:\n file.root&kind=2&maxsize=2000");

      $(frame).find(".store_startfile")
         .button()
         .click(function() {
            DABC.InvokeCommand(itemname+"/Control/StartRootFile", "fname="+$(frame).find('.store_filename').val());
         });
      $(frame).find(".store_stopfile")
         .button()
         .click(function() { DABC.InvokeCommand(itemname+"/Control/StopRootFile"); });

      var inforeq = false;

      function UpdateStreamStatus(res) {
         if (res==null) return;
         $(frame).find('.stream_rate').css("font-size","120%").text("Rate: " + res.EventsRate + " ev/s");
         $(frame).find('.stream_info').text("Info: " + res.StoreInfo);
      }

      var handler = setInterval(function() {
         if ($("#"+ffid+" .stream_info").length==0) {
            // if main element disapper (reset), stop handler
            clearInterval(handler);
            return;
         }

         if (inforeq) return;
         inforeq = true;

         DABC.httpRequest(itemname + "/Status/get.json", "object").then(res => {
            if (!res) return;
            UpdateStreamStatus(res);
            DABC.UpdateTRBStatus($(frame).find('.stream_tdc_calibr'), res._childs, hpainter, false);
         }).finally(() => { inforeq = false; });

      }, 2000);
   }


   // =========================================== BNET ============================================

   DABC.CompareArrays = function(arr1, arr2) {
      if (!arr1 || !arr2) return arr1 == arr2;
      if (arr1.length != arr2.length) return false;
      for (var k=0;k<arr1.length;++k)
         if (arr1[k] != arr2[k]) return false;
      return true;
   }

   DABC.BnetPainter = function(hpainter, itemname) {
      BasePainter.call(this);

      this.hpainter = hpainter;
      this.itemname = itemname;

      this.frame = hpainter.GetDisplay().FindFrame(itemname, true);
      if (!this.frame) return;

      this.InputItems = [];
      this.BuilderItems = [];
      this.CalibrHub = 0;
      this.CalibrItem = "";
      this.InputNodes = [];
      this.BuilderNodes = [];
      this.InputInfo = [];
      this.BuilderInfo = [];
      this.CalibrInfo = null;
      this.inforeq = null;
   }

   DABC.BnetPainter.prototype = Object.create(BasePainter.prototype);

   DABC.BnetPainter.prototype.Cleanup = function(arg) {
      BasePainter.prototype.Cleanup.call(this, arg);

      if (this.main_timer) {
         clearInterval(this.main_timer);
         delete this.main_timer;
      }

      delete this.hpainter;
      delete this.frame;
   }

   DABC.BnetPainter.prototype.active = function() {
      return this.hpainter && this.frame;
   }

   DABC.BnetPainter.prototype.MakeLabel = function(attr, txt, sz) {
      var lbl = "<label";
      if (attr) lbl += " " + attr;
      lbl +=">";
      if (txt === undefined) txt = "<undef>"; else
      if (txt === null) txt = "null"; else
      if (typeof txt != 'string') txt = txt.toString();
      if (txt.length > sz) txt = txt.substr(0, sz);
      lbl += txt;
      lbl += "</label>";

      if (txt.length<sz) {
         lbl += "<label>";
         while (txt.length<sz) { txt += " "; lbl += " "; }
         lbl += "</label>";
      }

      return lbl;
   }

   DABC.BnetPainter.prototype.RefreshHTML = function(lastprefix) {

      var html = "<div style='overflow:hidden;max-height:100%;max-width:100%'>";

      html += "<fieldset style='margin:5px'>" +
              "<legend class='bnet_state' style='font-size:200%'>Run control</legend>" +
              "<button class='bnet_startrun' title='Start run, write files on all event builders'>Start</button>" +
              "<select class='bnet_selectrun'>" +
              "<option>NO_FILE</option>" +
              "<option value='be'>Beam file</option>" +
              "<option value='te'>Test file</option>" +
              "<option value='co'>Cosmics file</option>" +
              "<option value='md'>MDC file</option>" +
              "<option value='ri'>RICH file</option>" +
              "<option value='ec'>ECAL file</option>" +
              "<option value='st'>Start file</option>" +
              "<option value='rp'>RPC file</option>" +
              "<option value='fw'>FW file</option>" +
              "<option value='pt'>Pion Tracker file</option>" +
              "<option value='tc'>TDC Calibration file</option>" +
              "</select>" +
              "<button class='bnet_stoprun' title='Stops run, close all opened files'>Stop</button>" +
              "<button class='bnet_lastcalibr' title='Status of last calibration'>CALIBR</button>" +
              "<button class='bnet_resetdaq' title='Drop all DAQ buffers on all nodes'>Reset</button>" +
              "<button class='bnet_totalrate' title='Total data rate'>0.00 MB/s</button>" +
              "<button class='bnet_totalevents' title='Total build events'>0.0 Ev/s</button>" +
              "<button class='bnet_lostevents' title='Total lost events'>0.0 Ev/s</button>" +
              "<button class='bnet_frameclear' title='Clear drawings'>Clr</button>" +
              "<input style='vertical-align:middle;' title='regular update of histograms' type='checkbox' class='bnet_monitoring'/>" +
              "<button class='bnet_histclear' title='Clear all histograms'>Hist</button>" +
              "<label class='bnet_runid_lbl' title='Current RUNID'>Runid: </label>" +
              "<label class='bnet_runprefix_lbl' title='Current Run Prefix'>Prefix: </label>" +
              "</fieldset>";

      html += "<fieldset style='margin:5px'>" +
              "<legend>Builder nodes</legend>" +
              "<div style='display:flex;flex-direction:column;font-family:monospace'>";
      html += "<div style='float:left' class='jsroot bnet_builders_header'>"
      html += "<pre style='margin:0'>";
      html += this.MakeLabel("class='bnet_item_clear h_item' title='clear drawings'", "Node", 20) + "| " +
              this.MakeLabel("class='bnet_item_label h_item' title='display all data rates' itemname='__bld__/HadaqData'", "Data", 8) + "| " +
              this.MakeLabel("class='bnet_item_label h_item' title='display all event rates' itemname='__bld__/HadaqEvents'", "Events", 8) + "| " +
              this.MakeLabel("", "File local", 24) +  "| " +
              this.MakeLabel("class='bnet_item_label h_item' title='display local file sizes' itemname='__bld__/RunFileSize'", "Size", 8) + "| " +
              this.MakeLabel("", "File LTSM", 24) +  "| " +
              this.MakeLabel("class='bnet_item_label h_item' title='display LTSM file sizes' itemname='__bld__/LtsmFileSize'", "Size", 8);
      html += "</pre>";
      html += "</div>";
      for (var node in this.BuilderItems) {
         html += "<div style='float:left' class='jsroot bnet_builder" + node + "'>";
         html += "<pre style='margin:0'>";
         html += "<label>" + this.BuilderItems[node] + "</label>";
         html += "</pre>";
         html += "</div>";
      }
      html += "</div>" +
              "</fieldset>";

      html += "<fieldset style='margin:5px'>" +
              "<legend>Input nodes</legend>" +
              "<div style='display:flex;flex-direction:column;font-family:monospace'>";
      html += "<div style='float:left' class='jsroot bnet_inputs_header'>"
      html += "<pre style='margin:0'>";
      html += this.MakeLabel("class='bnet_item_clear h_item' title='clear drawings'", "Node", 20) + "| " +
              this.MakeLabel("class='bnet_item_label h_item' title='display all data rates' itemname='__inp__/HadaqData'", "Data", 8) + "| " +
              this.MakeLabel("class='bnet_item_label h_item' title='display all events rates' itemname='__inp__/HadaqEvents'", "Events", 8) + "| " +
              this.MakeLabel("class='bnet_trb_clear h_item' title='remove hubs display'", "HUBs", 4);
      html += "</pre>";
      html += "</div>";
      for (var node in this.InputItems) {
         html += "<div style='float:left' class='jsroot bnet_input" + node + "'>";
         html += "<pre style='margin:0'>";
         html += "<label>" + this.InputItems[node] + "</label>";
         html += "</pre>";
         html += "</div>";
      }
      html += "</div>" +
              "</fieldset>";

      html += "<fieldset class='bnet_trb_info' style='margin:5px;display:none'>" +
              "<legend>Calibration</legend>" +
              "<div class='bnet_hub_info'></div>" +
              "<div class='bnet_tdc_calibr'></div>" +
              "</fieldset>";

      html += "</div>";

      var painter = this, main = d3.select(this.frame).html(html),
          ctrl_visible = DABC.hasUrlOption("browser") ? "" : "none";

      main.classed("jsroot_fixed_frame", true);
      main.selectAll(".bnet_trb_clear").on("click", this.DisplayCalItem.bind(this,0,""));

      main.selectAll(".bnet_item_clear").on("click", this.ClearDisplay.bind(this));

      main.select(".bnet_monitoring").on("click", function() {
         var on = d3.select(this).property('checked');
         painter.hpainter.EnableMonitoring(on);
         painter.hpainter.updateAll(!on);
      });

      main.selectAll(".bnet_item_label").on("click", function() {
         painter.DisplayItem(d3.select(this).attr("itemname"));
      });

      var itemname = this.itemname;

      var jnode = $(main.node());

      jnode.find(".bnet_startrun").button().css("display", ctrl_visible).click(function() {
         DABC.InvokeCommand(itemname+"/StartRun", "tmout=60&prefix=" + $(main.node()).find(".bnet_selectrun").selectmenu().val());
      });
      var sm = jnode.find(".bnet_selectrun");
      sm.selectmenu({ width: 150 });
      if (lastprefix) { sm.val(lastprefix); sm.selectmenu("refresh"); }
      if (ctrl_visible == "none") sm.next('.ui-selectmenu-button').hide();
      jnode.find(".bnet_stoprun").button().css("display", ctrl_visible).click(function() {
         DABC.InvokeCommand(itemname+"/StopRun", "tmout=60" );
      });

      jnode.find(".bnet_lastcalibr").button().click(function() {
      });

      jnode.find(".bnet_resetdaq").button().css("display", ctrl_visible).click(function() {
         if (confirm("Really drop buffers on all BNET nodes"))
            DABC.InvokeCommand(itemname+"/ResetDAQ");
      });

      jnode.find(".bnet_totalrate").button().click(function() {
         painter.DisplayItem("/"+itemname+"/DataRate");
      });

      jnode.find(".bnet_totalevents").button().click(function() {
         painter.DisplayItem("/"+itemname+"/EventsRate");
      });

      jnode.find(".bnet_lostevents").button().click(function() {
         painter.DisplayItem("/"+itemname+"/LostRate");
      });

      jnode.find(".bnet_frameclear").button().click(function() {
         painter.DisplayCalItem(0, "");
         painter.ClearDisplay();
      });

      jnode.find(".bnet_histclear").button().css("display", ctrl_visible).click(function() {
         painter.ClearAllHistograms();
      });


      // set DivId after drawing
      this.SetDivId(this.frame);
   }

   DABC.BnetPainter.prototype.ClearDisplay = function() {
      var frame = this.hpainter.GetDisplay().FindFrame("dabc_drawing");
      if (frame) this.hpainter.GetDisplay().CleanupFrame(frame);
   }

   DABC.BnetPainter.prototype.DisplayItem = function(itemname) {
      var items = null, opt = "";

      if (itemname.indexOf("__inp__")==0) {
         items = this.InputItems;
      } else if (itemname.indexOf("__bld__")==0) {
         items = this.BuilderItems;
      } else {
         itemname = itemname.substr(1);
         opt = "frameid:dabc_drawing";
      }

      if (items !== null) {
         if (items.length<2) return;

         var subitem = itemname.substr(7);
         itemname = "";
         for (var k=0;k<items.length;++k) {
            itemname += (k>0 ? "," : "[") + items[k].substr(1) + subitem;
            opt += (k>0 ? "," : "[") + "plc";
            if (k==0) opt += "frameid:dabc_drawing";
         }

         itemname += ",$legend]";
         opt += ",any]";
      }

      this.ClearDisplay();
      this.hpainter.displayAll([itemname], [opt]);
   }

   DABC.BnetPainter.prototype.DisplayCalItem = function(hubid, itemname) {
      this.CalibrHub = hubid;
      this.CalibrItem = itemname;

      d3.select(this.frame).select('.bnet_trb_info')
                           .style("display", (itemname || hubid) ? null : "none")
                           .select("legend").html("HUB: 0x" + hubid.toString(16));

      d3.select(this.frame).select('.bnet_tdc_calibr').html(""); // clear
   }

   DABC.BnetPainter.prototype.ClearAllHistograms = function() {
       for(var indx=0;indx<this.InputItems.length;++indx) {
          var itemname = this.InputItems[indx],
              info = this.InputInfo[indx];

          if (!itemname || !info || !info.calibr) continue;

          itemname = itemname.substr(1, itemname.lastIndexOf("/"));

          for (var k=0;k<info.calibr.length;++k)
             if (info.calibr[k])
                DABC.httpRequest(itemname + info.calibr[k] + "/cmd.json?command=ClearHistos", "object");
       }

       setTimeout(this.hpainter.updateAll.bind(this.hpainter, false), 1000);
   }

   DABC.BnetPainter.prototype.GetQualityColor = function(quality) {
      if (quality <= 0.3) return "red";
      if (quality < 0.7) return "yellow";
      if (quality < 0.8) return "lightblue";
      return "lightgreen";
   }

   DABC.BnetPainter.prototype.ProcessReq = function(isbuild, indx, res) {
      if (!res) return;

      var frame = d3.select(this.frame), elem,
          html = "", itemname = "", hadaqinfo = null, hadaqdata = null, hadaqevents = null, hadaqstate = null;

      for (var n=0;n<res._childs.length;++n) {
         var chld = res._childs[n];
         if (chld._name == "HadaqData") hadaqdata = chld; else
         if (chld._name == "HadaqEvents") hadaqevents = chld; else
         if (chld._name == "HadaqInfo") hadaqinfo = chld; else
         if (chld._name == "State") hadaqstate = chld;
      }

      html += "<pre style='margin:0'>";

      if (isbuild) {
         this.BuilderInfo[indx] = res;
         elem = frame.select(".bnet_builder" + indx);
         var col = this.GetQualityColor(res.quality);
         itemname = this.BuilderItems[indx];
         var pos = itemname.lastIndexOf("/");
         html += this.MakeLabel("class='bnet_item_label h_item' itemname='" + itemname.substr(0,pos) + "/Terminal/Output' style='background-color:" + col +
                                "' title='Item: " + itemname + "  State: " + hadaqstate.value + "  " + (res.mbsinfo || "") + " canrecv:[" + (res.queues || "-") + "]'", this.BuilderNodes[indx].substr(7), 20);
      } else {
         this.InputInfo[indx] = res;
         elem = frame.select(".bnet_input" + indx);
         var col = this.GetQualityColor(res.quality);
         itemname = this.InputItems[indx];
         var title = "Item: " + itemname + "  State: " + hadaqstate.value;
         if (res.progress) title += " progress:" + res.progress;
         title += " cansend:[" + (res.queues || "-") + "]";

         var pos = itemname.lastIndexOf("/");
         html += this.MakeLabel("class='bnet_item_label h_item' itemname='" + itemname.substr(0,pos) + "/Terminal/Output' style='background-color:" + col +
                                "' title='" + title + "'", this.InputNodes[indx].substr(7), 20);
      }

      var prefix = "class='bnet_item_label h_item' itemname='" + itemname + "/";

      html += "| " + this.MakeLabel(prefix + hadaqdata._name + "'", hadaqdata.value, 8);
      html += "| " + this.MakeLabel(prefix + hadaqevents._name + "'", hadaqevents.value, 8);

      if (isbuild) {
         var fname = res.runname || "";
         if (fname && (fname.lastIndexOf("/")>0))
            fname = fname.substr(fname.lastIndexOf("/")+1);

         html += "| " + this.MakeLabel("title='Full name: " + res.runname + "'", fname, 24);
         html += "| " + this.MakeLabel(prefix + "RunFileSize'", ((res.runsize || 0)/1024/1024).toFixed(2), 8);

         fname = res.ltsmname || "";;
         if (fname && (fname.lastIndexOf("/")>0))
            fname = fname.substr(fname.lastIndexOf("/")+1);

         html += "| " + this.MakeLabel("title='Full name: " + res.ltsmname + "'", fname, 24);
         html += "| " + this.MakeLabel(prefix + "LtsmFileSize'", ((res.ltsmsize || 0)/1024/1024).toFixed(2), 8);

         // if (hadaqinfo) html += "| " + this.MakeLabel(prefix + hadaqinfo._name + "'", hadaqinfo.value, 30);
      } else {
         // info with HUBs and port numbers
         var totallen = 0;
         html += "|";
         if (res.ports && res.hubs && (res.ports.length == res.hubs.length)) {
            for (var k=0;k<res.ports.length;++k) {
               if (this.CalibrHub == res.hubs[k])
                  frame.select(".bnet_hub_info").html("<pre>" + res.hubs_info[k] + "</pre>");
               var txt = "0x"+res.hubs[k].toString(16);
               totallen += txt.length;
               var title = "state:" + res.hubs_state[k] +
                           " quality:" + res.hubs_quality[k] +
                           " progr:" + res.hubs_progress[k] +
                           " " + res.hubs_info[k];
               var style = "background-color:" + this.GetQualityColor(res.hubs_quality[k]);
               var calitem = "";
               if (res.calibr[k])
                  calitem = itemname.substr(0, itemname.lastIndexOf("/")+1) + res.calibr[k];
               var attr = "hubid='" + res.hubs[k] + "' itemname='" + calitem + "' class='h_item bnet_trb_label'";

               html += " " + this.MakeLabel("title='" + title + "' style='" + style + "' " + attr, txt, txt.length);
               // if (totallen>40) break;
            }
         }

         if (!totallen) html += " " + this.MakeLabel("", "failure", 40);
      }

      html += "</pre>";

      var painter = this,
          main = elem.html(html);

      main.selectAll(".bnet_item_label").on("click", function() {
         painter.DisplayItem(d3.select(this).attr("itemname"));
      });
      main.selectAll(".bnet_trb_label").on("click", function() {
         painter.DisplayCalItem(parseInt(d3.select(this).attr("hubid")), d3.select(this).attr("itemname"));
      });
   }

   DABC.BnetPainter.prototype.ProcessCalibrReq = function(res) {
      if (!res) return;

      this.CalibrInfo = res;

      DABC.UpdateTRBStatus($(this.frame).find('.bnet_tdc_calibr'), res, this.hpainter, false);
   }

   DABC.BnetPainter.prototype.SendInfoRequests = function() {
      for (var n in this.InputItems)
         DABC.httpRequest(this.InputItems[n].substr(1) + "/get.json", "object").then(this.ProcessReq.bind(this, false, n));
      for (var n in this.BuilderItems)
         DABC.httpRequest(this.BuilderItems[n].substr(1) + "/get.json", "object").then(this.ProcessReq.bind(this, true, n));
      if (this.CalibrItem)
         DABC.httpRequest(this.CalibrItem.substr(1) + "/Status/get.json", "object").then(this.ProcessCalibrReq.bind(this));
   }

   DABC.BnetPainter.prototype.ProcessMainRequest = function(res) {

      d3.select(this.frame).style('background-color', res ? null : "grey");

      var chkbox = d3.select(this.frame).select(".bnet_monitoring");
      if (!chkbox.empty() && (chkbox.property('checked') !== this.hpainter.IsMonitoring()))
         chkbox.property('checked', this.hpainter.IsMonitoring());

      if (!res) return;

      var inp = null, bld = null, state = null, quality = null, drate, erate, lrate, ninp = [],
          nbld = [], runid = "", runprefix = "", changed = false, lastprefix = "", lastcalibr = null;
      for (var k in res._childs) {
         var elem = res._childs[k];
         switch (elem._name) {
            case "Inputs": inp = elem.value; ninp = elem.nodes; break;
            case "Builders": bld = elem.value; nbld = elem.nodes; break;
            case "LastPrefix": lastprefix = elem.value; break;
            case "LastCalibr": lastcalibr = elem; break;
            case "State": state = elem.value; break;
            case "Quality": quality = elem.value; break;
            case "DataRate": drate = elem.value; break;
            case "EventsRate":  erate = elem.value; break;
            case "LostRate":  lrate = elem.value; break;
            case "RunIdStr": runid = elem.value; break;
            case "RunPrefix": runprefix = elem.value; break;
         }
      }

      if (state) {
         var col = this.GetQualityColor(quality || 0);
         // col = "red";
         //if (state=="Ready") col = "lightgreen"; else
         //if (state=="Accumulating") col = "lightblue"; else
         //if (state=="NoFile") col = "yellow";
         d3.select(this.frame).select(".bnet_state").text("Run control: " + state).style('background-color',col);
      }

      if (typeof drate == 'number')
         $(this.frame).find(".bnet_totalrate").text(drate.toFixed(2) + " MB/s").css('background-color', (drate>0) ? "lightgreen" : "yellow");

      if (typeof erate == 'number')
         $(this.frame).find(".bnet_totalevents").text(erate.toFixed(1) + " Ev/s").css('background-color', (erate>0) ? "lightgreen" : "yellow");

      if (typeof lrate == 'number')
         $(this.frame).find(".bnet_lostevents").text(lrate.toFixed(1) + " Ev/s").css('background-color', (lrate > 0) ? "yellow" : "lightgreen");

      if (lastcalibr && (typeof lastcalibr.quality == 'number') && (typeof lastcalibr.time == 'string')) {
         var quality = lastcalibr.quality || 1,
             dt = new Date(lastcalibr.time), now = new Date(),
             diff = (now.getTime() - dt.getTime())*1e-3, // seconds
             info = "CALIBR", title = "Calibration ";
         if (diff < 0) { info = "CHECK CALIBR"; if (quality>0.6) quality = 0.6; } else {
            var h = Math.floor(diff/3600).toString(), m = Math.round((diff - h*3600)/60).toString();
            if (m.length==1) m = "0"+m;
            info = h+"h"+m+"m";
            if (h>720) { if (quality>0.1) quality = 0.1; title = "To long time without calibration, "; } else
            if (h>240) { if (quality>0.6) quality = 0.6; title = "Consider to peform calibration, "; }
         }
         title += "last: " + lastcalibr.value;
         $(this.frame).find(".bnet_lastcalibr").css('background-color', this.GetQualityColor(quality)).attr("title", title).text(info);
      }

      $(this.frame).find(".bnet_runid_lbl").text(" RunId: " + runid);
      $(this.frame).find(".bnet_runprefix_lbl").text(" Prefix: " + runprefix);

      // run prefix should not be overwritten
      //var sm = $(this.frame).find(".bnet_selectrun");
      //if (runprefix && (sm.val() != runprefix)) {
      //   sm.val(runprefix);
      //   sm.selectmenu("refresh");
      //}

      if (!DABC.CompareArrays(this.InputItems,inp)) {
         this.InputItems = inp;
         this.InputNodes = ninp;
         this.InputInfo = [];
         changed = true;
      }

      if (!DABC.CompareArrays(this.BuilderItems,bld)) {
         this.BuilderItems = bld;
         this.BuilderNodes = nbld;
         this.BuilderInfo = [];
         changed = true;
      }

      if (changed) {
         this.DisplayCalItem(0, "");
         this.RefreshHTML(lastprefix);
         this.hpainter.reload(); // also refresh hpainter - most probably items are changed
      }

      this.SendInfoRequests();
   }

   DABC.BnetPainter.prototype.SendMainRequest = function() {
      if (this.mainreq) return;
      this.mainreq = true;
      DABC.httpRequest(this.itemname + "/get.json", "object")
         .then(res => this.ProcessMainRequest(res))
         .catch(() => this.ProcessMainRequest(null))
         .finally(() => { this.mainreq = false; });
   }

   DABC.BnetControl = function(hpainter, itemname) {
      hpainter.CreateCustomDisplay(itemname, "vert2", function() {
         var painter = new DABC.BnetPainter(hpainter, itemname);
         if (painter.active()) {
            painter.RefreshHTML();
            painter.main_timer = setInterval(painter.SendMainRequest.bind(painter), 2000);
            painter.DisplayItem("/"+painter.itemname+"/EventsRate");
         }
      });
   }

   // ================================== NEW CODE ========================================================


   DABC.ReqH = function(hpainter, item, url) {
      return 'get.json.gz?compact=3';
   }

   DABC.ConvertH = function(hpainter, item, obj) {
      if (!obj) return;

      var res = {};
      for (var k in obj) res[k] = obj[k]; // first copy all keys
      for (var k in res) delete obj[k];   // then delete them from source

      if (res._kind == 'ROOT.TH1D') {
         JSROOT.Create("TH1D", obj);
         JSROOT.extend(obj, { fName: res._name, fTitle: res._title, fFillStyle: 1001 });

         JSROOT.extend(obj.fXaxis, { fNbins: res.nbins, fXmin: res.left,  fXmax: res.right });
         res.bins.splice(0,3); // 3 first items in array are not bins
         obj.fArray = res.bins;
         obj.fNcells = res.nbins + 2;
      } else if (res._kind == 'ROOT.TH2D') {
         JSROOT.Create("TH2D", obj);
         JSROOT.extend(obj, { fName: res._name, fTitle: res._title });

         JSROOT.extend(obj.fXaxis, { fNbins: res.nbins1, fXmin: res.left1,  fXmax: res.right1 });
         JSROOT.extend(obj.fYaxis, { fNbins: res.nbins2, fXmin: res.left2,  fXmax: res.right2 });
         res.bins.splice(0,6); // 6 first items in array are not bins
         obj.fNcells = (res.nbins1+2) * (res.nbins2+2);
         obj.fArray = res.bins;
      } else if (res._kind == 'ROOT.TH2Poly') {
         JSROOT.Create("TH2D", obj);
         JSROOT.extend(obj, { fName: res._name, fTitle: res._title });
         JSROOT.extend(obj.fXaxis, { fNbins: res.nbins1, fXmin: res.left1,  fXmax: res.right1 });
         JSROOT.extend(obj.fYaxis, { fNbins: res.nbins2, fXmin: res.left2,  fXmax: res.right2 });

         res.bins.splice(0,6); // 6 first items in array are not bins
         obj.fNcells = (res.nbins1+2) * (res.nbins2+2);
         obj.fArray = res.bins;

         obj.fBins = JSROOT.Create("TList");

         var npoly = 0, h2poly = JSON.parse(res.h2poly);
         for (var n2 = 0; n2 < res.nbins2; ++n2)
            for (var n1 = 0; n1 < res.nbins1; ++n1) {
               if (npoly >= h2poly.length) break;

               var poly = h2poly[npoly++];
               if (!poly || (poly.length !== 2)) continue;

               var xx = poly[0], yy = poly[1];

               var bin = {
                  _typename: "TH2PolyBin",
                  fNumber: 1,
                  fContent: obj.getBinContent(n1+1, n2+1),
                  fPoly: JSROOT.CreateTGraph(xx.length, xx, yy),
                  fXmin: Math.min.apply(null, xx),
                  fXmax: Math.max.apply(null, xx),
                  fYmin: Math.min.apply(null, yy),
                  fYmax: Math.max.apply(null, yy)
               };
               // used of text position only, for triangle workaround
               /* if (xx.length == 4) {
                  var xsum = 0, ysum = 0;
                  for (var k=0;k<xx.length;++k) {
                     xsum += xx[k]; ysum += yy[k];
                  }
                  bin.fXmin = bin.fXmax = xsum / xx.length;
                  bin.fYmin = bin.fYmax = ysum / xx.length;
               }
               */

               obj.fBins.Add(bin);
            }

         obj.fMarkerColor = 1;
         obj.fMarkerSize = 0.5;

         // set at the end - we use getbincontent
         obj._typename = "TH2Poly";

      } else {
         return;
      }

      if (res.xtitle) obj.fXaxis.fTitle = res.xtitle;
      if (res.ytitle) obj.fYaxis.fTitle = res.ytitle;
      if (res.fillcolor) obj.fFillColor = res.fillcolor;
      if (res.drawopt) obj.fOption = res.drawopt;
      if (res.hmin !== undefined) obj.fMinimum = res.hmin;
      if (res.hmax !== undefined) obj.fMaximum = res.hmax;
      if ('xlabels' in res) {
         obj.fXaxis.fLabels = JSROOT.Create('THashList');
         var lbls = res.xlabels.split(",");
         for (var n in lbls) {
            var lbl = JSROOT.Create('TObjString');
            lbl.fUniqueID = parseInt(n)+1;
            lbl.fString = lbls[n];
            obj.fXaxis.fLabels.Add(lbl);
         }
      }
      if ('ylabels' in res) {
         obj.fYaxis.fLabels = JSROOT.Create('THashList');
         var lbls = res.ylabels.split(",");
         for (var n in lbls) {
            var lbl = JSROOT.Create('TObjString');
            lbl.fUniqueID = parseInt(n)+1;
            lbl.fString = lbls[n];
            obj.fYaxis.fLabels.Add(lbl);
         }
      }
   }


   DABC.ExtractSeries = function(name, kind, obj, history) {

      var ExtractField = function(node) {
         if (!node || !(name in node)) return null;

         if (kind=="number") return Number(node[name]);
         if (kind=="time") {
            var d  = new Date(node[name]);
            return d.getTime() / 1000.;
         }
         return node[name];
      }

      // xml node must have attribute, which will be extracted
      var val = ExtractField(obj);
      if (val==null) return null;

      var arr = new Array();
      arr.push(val);

      if (history!=null)
         for (var n=history.length-1;n>=0;n--) {
            // in any case stop iterating when see property delete
            if ("dabc:del" in history[n]) break;
            var newval = ExtractField(history[n]);
            if (newval!=null) val = newval;
            arr.push(val);
         }

      arr.reverse();
      return arr;
   }

   DABC.MakeItemRequest = function(h, item, fullpath, option) {
      item.fullitemname = fullpath;
      if (!('_history' in item) || (option=="gauge") || (option=='last')) return "get.json?compact=0";
      if (!('hlimit' in item)) item.hlimit = 100;
      var url = "get.json?compact=0&history=" + item.hlimit;
      if (('request_version' in item) && (item.request_version>0)) url += "&version=" + item.request_version;
      item.request_version = 0;
      return url;
   }

   DABC.AfterItemRequest = function(h, item, obj, option) {
      if (!obj) return;

      if (!('_history' in item) || (option=="gauge") || (option=='last')) {
         obj.fullitemname = item.fullitemname;
         // for gauge output special scripts should be loaded, use unique class name for it
         if (obj._kind == 'rate') obj._typename = "DABC_RateGauge";
         return;
      }

      var new_version = Number(obj._version);

      var modified = (item.request_version != new_version);

      this.request_version = new_version;

      // this is array with history entries
      var arr = obj.history;

      if (arr!=null) {
         // gap indicates that we could not get full history relative to provided version number
         var gap = obj.history_gap;

         // join both arrays with history entries
         if ((item.history == null) || (arr.length >= item['hlimit']) || gap) {
            item.history = arr;
         } else
            if (arr.length>0) {
               modified = true;
               var total = item.history.length + arr.length;
               if (total > item['hlimit'])
                  this.history.splice(0, total - item['hlimit']);

               item.history = item.history.concat(arr);
            }
      }

      if (obj._kind == "log") {
         obj.log = DABC.ExtractSeries("value", "string", obj, item.history);
         return;
      }

      // now we should produce TGraph from the object

      var x = DABC.ExtractSeries("time", "time", obj, item.history);
      var y = DABC.ExtractSeries("value", "number", obj, item.history);

      for (var k in obj) delete obj[k];  // delete all object keys

      if (!x) x = [];
      if (!y) y = [];

      obj._typename = 'TGraph';
      JSROOT.Create('TGraph', obj);
      obj.fHistogram = JSROOT.CreateHistogram('TH1F', x.length);

      var _title = item._title || item.fullitemname;

      JSROOT.extend(obj, { fBits: 0x3000408, fName: item._name, fTitle: _title,
                           fX: x, fY: y, fNpoints: x.length,
                           fLineColor: 2, fLineWidth: 2 });

      var xrange = x[x.length-1] - x[0];
      obj.fHistogram.fTitle = obj.fTitle;
      obj.fHistogram.fXaxis.fXmin = x[0] - xrange*0.03;
      obj.fHistogram.fXaxis.fXmax = x[x.length-1] + xrange*0.03;

      var ymin = Math.min.apply(null,y), ymax = Math.max.apply(null,y);

      if (!ymin) ymin = 0; if (!ymax) ymax = 0;

      obj.fHistogram.fYaxis.fXmin = ymin - 0.05 * (ymax-ymin);
      obj.fHistogram.fYaxis.fXmax = ymax + 0.05 * (ymax-ymin);

      obj.fHistogram.fXaxis.fTimeDisplay = true;
      obj.fHistogram.fXaxis.fTimeFormat = "%H:%M:%S%F0"; // %FJanuary 1, 1970 00:00:00
   }

   DABC.DrawGauage = function(divid, obj, opt) {

      // at this momemnt justgauge should be loaded

      if (typeof Gauge == 'undefined') {
         alert('gauge.js not loaded');
         return null;
      }

      var painter = new ObjectPainter(obj);

      painter.gauge = null;
      painter.min = 0;
      painter.max = 1;
      painter.lastval = 0;
      painter.lastsz = 0;
      painter._title = "";

      painter.Draw = function(obj) {
         if (!obj) return;

         var val = Number(obj["value"]);

         if ('low' in obj) {
            var min = Number(obj["low"]);
            if (!isNaN(min) && (min<this.min)) this.min = min;
         }

         if ('up' in obj) {
            var max = Number(obj["up"]);
            if (!isNaN(max) && (max>this.max)) this.max = max;
         }

         var redo = false;

         if (val > this.max) {
            this.max = 1;
            var cnt = 0;
            while (val > this.max)
               this.max *= (((cnt++ % 3) == 1) ? 2.5 : 2);

            redo = true;
         }

         this._title = obj._name || "gauge"
         val = JSROOT.FFormat(val,"5.3g");
         this.DrawValue(val, redo);
      }

      painter.DrawValue = function(val, force) {
         var rect = this.select_main().node().getBoundingClientRect();
         var sz = Math.min(rect.height, rect.width);

         if ((sz > this.lastsz*1.2) || (sz < this.lastsz*0.9)) force = true;

         if (force) {
            this.select_main().selectAll("*").remove();
            this.gauge = null;
         }

         this.lastval = val;

         if (!this.gauge) {
            this.lastsz = sz;

            var config =  {
               size: sz,
               label: this._title,
               min: this.min,
               max: this.max,
               majorTicks : 6,
               minorTicks: 5
            };

            if ('units' in obj) config['units'] = obj['units'] + "/s";

            if (!this.gauge)
               this.gauge = new Gauge(this.select_main().attr('id'), config);
            else
               this.gauge.configure(config);

            this.gauge.render(val);

         } else {
            this.gauge.redraw(val);
         }

         // always set painter
         this.SetDivId(this.divid);
      }

      painter.CheckResize = function() {
         this.DrawValue(this.lastval);
      }

      painter.RedrawObject = function(obj) {
         this.Draw(obj);
         return true;
      }

      painter.SetDivId(divid, -1);

      painter.Draw(obj);

      return painter.DrawingReady();
   }

   // ==========================================================================================

   DABC.DrawLog = function(divid, obj, opt) {
      var painter = new BasePainter();
      painter.obj = obj;
      painter.history = (opt!="last") && ('log' in obj); // by default draw complete history

      painter.SetDivId(divid, 1);

      if (painter.history) {
         painter.select_main().html("<div style='overflow:auto; max-height: 100%; max-width: 100%; font-family:monospace'></div>");
      } else {
         painter.select_main().html("<div></div>");
      }
      // set divid after child element created - only then we could set painter
      painter.SetDivId(divid);

      painter.RedrawObject = function(obj) {
         this.obj = obj;
         this.Draw();
         return true;
      }

      painter.Draw = function() {
         var html = "";

         if (this.history && ('log' in this.obj)) {
            for (var i in this.obj.log) {
               html+="<pre style='margin-top:2px;margin-bottom:0px'>"+this.obj.log[i]+"</pre>";
            }
         } else {
            html += obj['fullitemname'] + "<br/>";
            html += "<h5>"+ this.obj.value +"</h5>";
         }
         this.select_main().select("div").html(html);
      }

      painter.Draw();

      return painter.DrawingReady();
   }


   DABC.DrawCommand = function(divid, obj, opt) {

      painter = new BasePainter;

      painter.SetDivId(divid, -1);
      painter.jsonnode = obj;
      painter.req = null;

      painter.NumArgs = function() {
         if (this.jsonnode==null) return 0;
         return this.jsonnode["numargs"];
      }

      painter.ArgName = function(n) {
         return (n<this.NumArgs()) ? this.jsonnode["arg"+n] : "";
      }

      painter.ArgKind = function(n) {
         return (n<this.NumArgs()) ? this.jsonnode["arg"+n+"_kind"] : "";
      }

      painter.ArgDflt = function(n) {
         return (n<this.NumArgs()) ? this.jsonnode["arg"+n+"_dflt"] : "";
      }

      painter.ArgMin = function(n) {
         return (n<this.NumArgs()) ? this.jsonnode["arg"+n+"_min"] : null;
      }

      painter.ArgMax = function(n) {
         return (n<this.NumArgs()) ? this.jsonnode["arg"+n+"_max"] : null;
      }

      painter.ShowCommand = function() {

         var cmdelemid = this.select_main().attr('id');

         var frame = $("#" + cmdelemid);

         frame.empty();

         if (this.jsonnode==null) {
            frame.append("cannotr access command definition...<br/>");
            return;
         }

         frame.append("<h3>" + this.jsonnode.fullitemname + "</h3>");

         var entryInfo = "<input id='" + cmdelemid + "_button' type='button' title='Execute' value='Execute'/><br/>";

         for (var cnt=0;cnt<this.NumArgs();cnt++) {
            var argname = this.ArgName(cnt);
            var argkind = this.ArgKind(cnt);
            var argdflt = this.ArgDflt(cnt);

            var argid = cmdelemid + "_arg" + cnt;
            var argwidth = (argkind=="int") ? "80px" : "170px";

            entryInfo += "Arg: " + argname + " ";
            entryInfo += "<input id='" + argid + "' style='width:" + argwidth + "' value='"+argdflt+"' argname = '" + argname + "'/>";
            entryInfo += "<br/>";
         }

         entryInfo += "<div id='" + cmdelemid + "_res'/>";

         frame.append(entryInfo);

         var pthis = this;

         $("#"+ cmdelemid + "_button").click(function() { pthis.InvokeCommand(); });

         for (var cnt=0;cnt<this.NumArgs();cnt++) {
            var argid = cmdelemid + "_arg" + cnt;
            var argkind = this.ArgKind(cnt);
            var argmin = this.ArgMin(cnt);
            var argmax = this.ArgMax(cnt);

            if ((argkind=="int") && (argmin!=null) && (argmax!=null))
               $("#"+argid).spinner({ min:argmin, max:argmax});
         }
      }

      painter.InvokeCommand = function() {
         if (this.req) return;

         var cmdelemid = this.select_main().attr('id');

         var resdiv = $("#" + cmdelemid + "_res");
         resdiv.html("<h5>Send command to server</h5>");

         var url = this.jsonnode.fullitemname + "/execute";

         for (var cnt=0;cnt<this.NumArgs();cnt++) {
            var argid = cmdelemid + "_arg" + cnt;
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

         this.req = true;

         DABC.httpRequest(url, "object")
             .then(res => resdiv.html("<h5>Get reply res=" + res['_Result_'] + "</h5>"))
             .catch(() => resdiv.html("<h5>missing reply from server</h5>"))
             .finally(() => { this.req = false; });

      }

      painter.ShowCommand();

      painter.SetDivId(divid);

      return painter.DrawingReady();
   }

   // =============================================================

   JSROOT.addDrawFunc({
      name: "kind:rate",
      icon: DABC.source_dir + "../img/gauge.png",
      func: "<dummy>",
      opt: "line;gauge",
      monitor: true,
      make_request: DABC.MakeItemRequest,
      after_request: DABC.AfterItemRequest
   });

   JSROOT.addDrawFunc({
      name: "kind:log",
      icon: "img_text",
      func: DABC.DrawLog,
      opt: "log;last",
      monitor: 'always',
      make_request: DABC.MakeItemRequest,
      after_request: DABC.AfterItemRequest
   });

   JSROOT.addDrawFunc({
      name: "kind:DABC.Command",
      icon: DABC.source_dir + "../img/dabc.png",
      make_request: DABC.MakeItemRequest,
      after_request: DABC.AfterItemRequest,
      opt: "command",
      prereq: 'jq',
      monitor: 'never',
      func: 'DABC.DrawCommand'
   });

   // only indicate that item with such kind can be opened as direct link
   JSROOT.addDrawFunc({
      name: "kind:DABC.HTML",
      icon: DABC.source_dir + "../img/dabc.png",
      aslink: true
   });

   // example of external scripts loading
   JSROOT.addDrawFunc({
      name: "DABC_RateGauge",
      func: "DABC.DrawGauage",
      opt: "gauge",
      script: DABC.source_dir + 'gauge.js'
   });


})();
