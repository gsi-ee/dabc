
const create = JSROOT.create,
      httpRequest = JSROOT.httpRequest,
      addDrawFunc = JSROOT.addDrawFunc,
      BasePainter = JSROOT.BasePainter,
      TH1Painter = JSROOT.TH1Painter,
      TH2Painter = JSROOT.TH2Painter,
      getHPainter = JSROOT.getHPainter;

if (typeof d3_select == 'undefined')
   globalThis.d3_select = JSROOT?.d3_select ?? d3?.select; // provided by bundle

JSROOT.settings.DragAndDrop = true;

// const jsrp = arr[0];

const DABC = {};

DABC.version = "2.12.0 6/04/2022";

DABC.source_dir = "";

DABC.id_counter = 1;

const src = document.currentScript.src;
if (src && (typeof src == "string")) {
   const pos = src.indexOf("scripts/dabc.js");
   if (pos >= 0) {
      DABC.source_dir = src.slice(0, pos);
      console.log(`Set DABC.source_dir to ${DABC.source_dir}, ${DABC.version}`);
   }
}

DABC.invokeCommand = function(itemname, args) {
   let url = itemname + "/execute";
   if (args && (typeof args == 'string')) url += "?" + args;

   httpRequest(url,"object");
}

/** @summary add button style as first child */
DABC.addDabcStyle = function(dom) {
   dom.append("style").html(
      `.dabc_btn {
          display: inline-block;
          padding: .4em 1em;
          margin-right: .1em;
          vertical-align: middle;
          text-align: center;
          font-family: Arial,Helvetica,sans-serif;
          font-size: 1em;
          border: 1px solid #c5c5c5;
          background: #f6f6f6;
          font-weight: normal;
          color: #454545;
          border-radius: 3px;
          cursor: pointer;
       }
       .dabc_btn:hover {
          background: #e6e6e6;
       }
       .dabc_btn:active {
          border: 1px solid blue;
          color: white;
       }`).lower();
}

// method for custom HADAQ-specific GUI, later could be moved into hadaq.js script

DABC.HadaqDAQControl = function(hpainter, itemname) {
   let mdi = hpainter.getDisplay();
   if (!mdi) return null;

   let frame = mdi.findFrame(itemname, true);
   if (!frame) return null;

   let item = hpainter.findItem(itemname), calarr = [];

   if (item) {
      for (let n in item._parent._childs) {
         let name = item._parent._childs[n]._name;

         if ((name.indexOf("TRB")==0) && (name.indexOf("TdcCal")>0)) {
            let fullname = hpainter.itemFullName(item._parent._childs[n]);
            calarr.push(fullname);
         }
      }
   }

   let html = "<fieldset>" +
              "<legend>DAQ</legend>" +
              "<button class='hadaq_startfile dabc_btn'>Start file</button>" +
              "<button class='hadaq_stopfile dabc_btn'>Stop file</button>" +
              "<button class='hadaq_restartfile dabc_btn'>Restart file</button>" +
              '<input class="hadaq_filename" type="text" name="filename" value="file.hld" style="margin-top:5px;"/><br/>' +
              "<label class='hadaq_rate'>Rate: __undefind__</label><br/>"+
              "<label class='hadaq_info'>Info: __undefind__</label>"+
              "</fieldset>" +
              "<fieldset>" +
              "<legend>Calibration</legend>";
   for (let n in calarr)
      html += "<div class='hadaq_calibr' style='padding:2px;margin:2px'>" + calarr[n] + "</div>";
   html+="</fieldset>";

   let dom = d3_select(frame);

   dom.html(html);

   DABC.addDabcStyle(dom);

   dom.select(".hadaq_startfile").on("click", () => {
      DABC.invokeCommand(itemname+"/StartHldFile", "filename="+dom.select('.hadaq_filename').property("value")+"&maxsize=2000");
   });
   dom.select(".hadaq_stopfile").on("click", () => {
      DABC.invokeCommand(itemname+"/StopHldFile");
   });
   dom.select(".hadaq_restartfile").on("click", () => {
      DABC.invokeCommand(itemname+"/RestartHldFile");
   });

   let inforeq = false;

   function UpdateDaqStatus(res) {
      if (res==null) return;
      let rate = "";
      for (let n in res._childs) {
         let item = res._childs[n], lbl = "", units = "";

         if (item._name == 'HadaqInfo')
            dom.select('.hadaq_info').text("Info: " + item.value);
         if (item._name=='HadaqData') { lbl = "Rate: "; units = " " + item.units+"/s"; }
         if (item._name=='HadaqEvents') { lbl = "Ev: "; units = " Hz"; }
         if (item._name=='HadaqLostEvents') { lbl = "Lost:"; units = " Hz"; }
         if (lbl) {
            if (rate.length>0) rate += " ";
            rate += lbl + item.value + units;
         }
      }

      dom.select('.hadaq_rate').style("font-size","120%").text(rate);
   }

   let handler = setInterval(function() {
      if (d3_select(frame).select(".hadaq_info").empty()) {
         // if main element disapper (reset), stop handler
         clearInterval(handler);
         return;
      }

      if (inforeq) return;

      let url = "multiget.json?items=[";
      url+="'" + itemname + "'";
      for (let n in calarr)
         url+= ",'" + calarr[n]+"/Status" + "'";
      url+="]";
      //url+="]&compact=3";

      inforeq = true;
      httpRequest(url, "object").then(res => {
         if (!res) return;
         UpdateDaqStatus(res[0].result);
         res.shift();
         DABC.updateTrbStatus(d3_select(frame).select('.hadaq_calibr'), res, hpainter, true);
      }).finally(() => { inforeq = false; });
   }, 2000);
}

DABC.updateTrbStatus = function(holder, res, hpainter, multiget) {

   function makehname(prefix, code, name) {
      let str = code.toString(16).toUpperCase();
      while (str.length < 4) str = "0"+str;
      str = prefix+"_"+str;
      if (name) str += "_"+name;
      let hitem = null;
      hpainter.forEachItem(item => { if ((item._name == str) && !hitem) hitem = item; });
      if (hitem) return hpainter.itemFullName(hitem);
      console.log("did not found histogram " + str);
      return str;
   }

   function makecalname(code) {
      let str = code.toString(16);
      while (str.length < 4) str = "0"+str;
      str = "TRB" + str + "_TdcCal";
      let hitem = null;
      hpainter.forEachItem(item => { if ((item._name == str) && !hitem) hitem = item; });
      if (hitem) return hpainter.itemFullName(hitem);
      console.log("did not found element " + str);
      return str;
   }

   function get_status_color(status) {
      if (status.indexOf('Ready')==0) return 'green';
      if (status.indexOf('File')==0) return 'blue';
      if (status.indexOf('Accum')==0) return 'lightblue';
      return 'red';
   }

   holder.each(function(_data, index) {
      if (!res) return;
      let dom = d3_select(this),
          info = multiget ? res[index] : res;
      // when doing multiget, return object stored as result field
      if (('result' in info) && multiget) info = info.result;
      if (!info) return;

      if (dom.selectAll("*").empty()) {
         let code = "<div style='display:flex;flex-direction:row;width:100%;'>";
         code += "<button title='clear all TRB histograms' hist='" + makehname("TRB", info.trb) + "'>Clr</button>";
         if (!info.tdc) {
            code += "<button hist='" + makehname("TRB", info.trb, "ErrorBits") + "'>"+info.trb.toString(16)+"_ErrorBits</button>";
            code += "<button hist='" + makehname("TRB", info.trb, "TrigType") + "'>"+info.trb.toString(16)+"_TrigType</button>";
            code += "<button hist='" + makehname("TRB", info.trb, "SubevSize") + "'>"+info.trb.toString(16)+"_SubevSize</button>";
         } else {
            code += "<button title='show calibration log' hist='" + makehname("TRB", info.trb) + "'>Log</button>";
            code += "<button title='acknowledge calibration quality' hist='" + makecalname(info.trb) + "'>Ackn</button>";
            code += "<button hist='" + makehname("TRB", info.trb, "TrigType") + "'>TrigType</button>";
            code += "<button hist='" + makehname("TRB", info.trb, "MsgPerTDC") + "'>MsgPerTDC</button>";
            code += "<button hist='" + makehname("TRB", info.trb, "ToTPerTDC") + "'>ToTPerTDC</button>";
            for (let j in info.tdc)
               code+="<button class='tdc_btn' tdc='" + info.tdc[j] + "' hist='" + makehname("TDC", info.tdc[j], "Channels") + "'>"+info.tdc[j].toString(16)+"</button>";
         }
         if (info.tdc)
            code += `<progress class="hadaq_progress" max="100" style="flex-grow:1">0%</progress>`;
         code += "</div>";
         dom.html(code);
         dom.selectAll("button").classed("dabc_btn", true).on("click", function() {
            let histname = d3_select(this).attr('hist');
            if (d3_select(this).text() == "Clr")
               return httpRequest(histname+"/cmd.json?command=ClearHistos", "object");
            if (d3_select(this).text() == "Ackn")
               return httpRequest(histname+"/cmd.json?command=AcknowledgeQuality", "object");
            if (d3_select(this).text() == "Log") {
               // log item is one level higher then histograms, excluding trb module name
               histname = histname.substr(0, histname.length-8) + "CalibrLog";
            }

            let frame = hpainter.getDisplay().findFrame("dabc_drawing");
            if (frame) hpainter.getDisplay().cleanupFrame(frame);

            hpainter.displayItems([histname],["frameid:dabc_drawing"]);
         });
         if (info.tdc)
            dom.select(".hadaq_progress").property("value", info.progress);
      }

      dom.style('background-color', get_status_color(info.value));
      dom.attr('title',"TRB:" + info.trb.toString(16) + " State: " + info.value + " Time:" + info.time);

      dom.select(".hadaq_progress").attr("title", "progress: " + info.progress + "%")
         .property("value", info.progress);

      for (let j in info.tdc) {
         dom.select(`.tdc_btn[tdc='${info.tdc[j]}']`)
            .style('color', get_status_color(info.tdc_status[j]))
            .style('background', 'white')
            .attr("title", `TDC ${info.tdc[j].toString(16)} ${info.tdc_status[j]}  Progress: ${info.tdc_progr[j]}`);
      }
   });
}


DABC.StreamControl = function(hpainter, itemname) {
   let mdi = hpainter.getDisplay();
   if (!mdi) return null;

   let frame = mdi.findFrame(itemname, true);
   if (!frame) return null;

   let dom = d3_select(frame),
       ffid = dom.attr('id'),
       item = hpainter.findItem(itemname + "/Status"),
       calarr = [];
   if (!ffid) {
      ffid = "dabc_stream_control";
      dom.attr('id', "dabc_stream_control");
   }

   if (item) {
      for (let n in item._childs)
         calarr.push(hpainter.itemFullName(item._childs[n]));
   }

   let html = "<fieldset>" +
              "<legend>Stream</legend>" +
              "<button class='store_startfile dabc_btn' title='Start storage of ROOT file'>Start file</button>" +
              "<button class='store_stopfile dabc_btn' title='Stop storage of ROOT file'>Stop file</button>" +
              '<input class="store_filename" type="text" name="filename" value="file.root" style="margin-top:5px;"/><br/>' +
              "<label class='stream_rate'>Rate: __undefind__</label><br/>" +
              "<label class='stream_info'>Info: __undefind__</label>" +
              "</fieldset>" +
              "<fieldset>" +
              "<legend>Calibration</legend>";
   for (let n in calarr)
      html += "<div class='stream_tdc_calibr' style='padding:2px;margin:2px'>" + calarr[n] + "</div>";
   html+="</fieldset>";

   dom.html(html);

   DABC.addDabcStyle(dom);

   dom.select(".store_filename").attr("title", "Name of ROOT file to store.\nOne can specify store kind and maxsize with args like:\n file.root&kind=2&maxsize=2000");

   dom.select(".store_startfile").on("click", () => DABC.invokeCommand(itemname+"/Control/StartRootFile", "fname="+dom.select('.store_filename').property("value")));
   dom.select(".store_stopfile").on("click", () => DABC.invokeCommand(itemname+"/Control/StopRootFile"));

   let inforeq = false;

   function UpdateStreamStatus(res) {
      if (!res) return;
      dom.select('.stream_rate').style("font-size","120%").text("Rate: " + res.EventsRate + " ev/s");
      dom.select('.stream_info').text("Info: " + res.StoreInfo);
   }

   let handler = setInterval(function() {
      if (d3_select("#"+ffid+" .stream_info").empty()) {
         // if main element disapper (reset), stop handler
         clearInterval(handler);
         return;
      }

      if (inforeq) return;
      inforeq = true;

      httpRequest(itemname + "/Status/get.json", "object").then(res => {
         if (!res) return;
         UpdateStreamStatus(res);
         DABC.updateTrbStatus(d3_select(frame).select('.stream_tdc_calibr'), res._childs, hpainter, false);
      }).finally(() => { inforeq = false; });

   }, 2000);
}

// =========================================== BNET ============================================

DABC.compareArrays = function(arr1, arr2) {
   if (!arr1 || !arr2) return arr1 == arr2;
   if (arr1.length != arr2.length) return false;
   for (let k = 0; k < arr1.length; ++k)
      if (arr1[k] != arr2[k]) return false;
   return true;
}

class BnetPainter extends BasePainter {

constructor(hpainter, itemname) {

   let frame = hpainter.getDisplay().findFrame(itemname, true);

   super(frame);

   this.hpainter = hpainter;
   this.itemname = itemname;
   this.frame = frame;
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

   cleanup(arg) {
      super.cleanup(arg);

      if (this.main_timer) {
         clearInterval(this.main_timer);
         delete this.main_timer;
      }

      delete this.hpainter;
      delete this.frame;
   }

   active() {
      return this.hpainter && this.frame;
   }

   makeLabel(attr, txt, sz) {
      let lbl = "<label";
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

   refreshHtml(lastprefix) {

      let html = "<div style='overflow:hidden;max-height:100%;max-width:100%'>";

      html += "<fieldset style='margin:5px'>" +
              "<legend class='bnet_state' style='font-size:200%'>Run control</legend>" +
              "<button class='bnet_startrun dabc_btn' title='Start run, write files on all event builders'>Start</button>" +
              "<select class='bnet_selectrun dabc_btn' style='width: 17em'>" +
              "<option>NO_FILE</option>" +
              "<option value='be'>Beam file</option>" +
              "<option value='te'>Test file</option>" +
              "<option value='co'>Cosmics file</option>" +
              "<option value='md'>MDC file</option>" +
              "<option value='ri'>RICH file</option>" +
              "<option value='ec'>ECAL file</option>" +
              "<option value='st'>Start file</option>" +
              "<option value='rp'>RPC file</option>" +
              "<option value='to'>TOF file</option>" +
              "<option value='fw'>FW file</option>" +
              "<option value='pt'>Pion Tracker file</option>" +
              "<option value='fd'>Forward detector (all) file</option>" +
              "<option value='fs'>Forward detector (sts only) file</option>" +
              "<option value='fr'>Forward detector (rpc only) file</option>" +
              "<option value='it'>Inner TOF file</option>" +
              "<option value='tc'>TDC Calibration file</option>" +
              "<option value='ct'>TDC Calibration test file</option>" +
              "</select>" +
              "<button class='bnet_stoprun dabc_btn' title='Stops run, close all opened files'>Stop</button>" +
              "<button class='bnet_lastcalibr dabc_btn' title='Status of last calibration'>CALIBR</button>" +
              "<button class='bnet_resetdaq dabc_btn' title='Drop all DAQ buffers on all nodes'>Reset</button>" +
              "<button class='bnet_totalrate dabc_btn' title='Total data rate'>0.00 MB/s</button>" +
              "<button class='bnet_totalevents dabc_btn' title='Total build events'>0.0 Ev/s</button>" +
              "<button class='bnet_lostevents dabc_btn' title='Total lost events'>0.0 Ev/s</button>" +
              "<button class='bnet_frameclear dabc_btn' title='Clear drawings'>Clr</button>" +
              "<input style='vertical-align:middle;' title='regular update of histograms' type='checkbox' class='bnet_monitoring'/>" +
              "<button class='bnet_histclear dabc_btn' title='Clear all histograms'>Hist</button>" +
              "<label class='bnet_runid_lbl' title='Current RUNID'>Runid: </label>" +
              "<label class='bnet_runprefix_lbl' title='Current Run Prefix'>Prefix: </label>" +
              "</fieldset>";

      html += "<fieldset style='margin:5px'>" +
              "<legend>Builder nodes</legend>" +
              "<div style='display:flex;flex-direction:column;font-family:monospace'>";
      html += "<div style='float:left' class='jsroot bnet_builders_header'>"
      html += "<pre style='margin:0'>";
      html += this.makeLabel("class='bnet_item_clear h_item' title='clear drawings'", "Node", 20) + "| " +
              this.makeLabel("class='bnet_item_label h_item' title='display all data rates' itemname='__bld__/HadaqData'", "Data", 8) + "| " +
              this.makeLabel("class='bnet_item_label h_item' title='display all event rates' itemname='__bld__/HadaqEvents'", "Events", 8) + "| " +
              this.makeLabel("", "File local", 24) +  "| " +
              this.makeLabel("class='bnet_item_label h_item' title='display local file sizes' itemname='__bld__/RunFileSize'", "Size", 8) + "| " +
              this.makeLabel("", "File LTSM", 24) +  "| " +
              this.makeLabel("class='bnet_item_label h_item' title='display LTSM file sizes' itemname='__bld__/LtsmFileSize'", "Size", 8);
      html += "</pre>";
      html += "</div>";
      for (let node in this.BuilderItems) {
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
      html += this.makeLabel("class='bnet_item_clear h_item' title='clear drawings'", "Node", 20) + "| " +
              this.makeLabel("class='bnet_item_label h_item' title='display all data rates' itemname='__inp__/HadaqData'", "Data", 8) + "| " +
              this.makeLabel("class='bnet_item_label h_item' title='display all events rates' itemname='__inp__/HadaqEvents'", "Events", 8) + "| " +
              this.makeLabel("class='bnet_trb_clear h_item' title='remove hubs display'", "HUBs", 4);
      html += "</pre>";
      html += "</div>";
      for (let node in this.InputItems) {
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

      let painter = this, main = d3_select(this.frame).html(html),
          ctrl_visible = JSROOT.decodeUrl().has("browser") ? "" : "none";

      DABC.addDabcStyle(main);

      main.classed("jsroot_fixed_frame", true);
      main.selectAll(".bnet_trb_clear").on("click", () => this.displayCalItem(0, ""));

      main.selectAll(".bnet_item_clear").on("click", () => this.clearDisplay());

      main.select(".bnet_monitoring").on("click", () => {
         let on = main.select(".bnet_monitoring").property('checked');
         painter.hpainter.enableMonitoring(on);
         painter.hpainter.updateItems();
      });

      main.selectAll(".bnet_item_label").on("click", function() {
         painter.displayItem(d3_select(this).attr("itemname"));
      });

      let itemname = this.itemname;

      main.select(".bnet_startrun").style("display", ctrl_visible).on("click", () => {
         DABC.invokeCommand(itemname + "/StartRun", "tmout=60&prefix=" + main.select(".bnet_selectrun").property("value"));
      });

      main.select(".bnet_selectrun").style("display", ctrl_visible).property("value", lastprefix || undefined);

      main.select(".bnet_stoprun").style("display", ctrl_visible).on("click", () => DABC.invokeCommand(itemname+"/StopRun", "tmout=60" ));

      main.select(".bnet_lastcalibr").on("click", () => DABC.invokeCommand(itemname+"/RefreshRun", "tmout=60"));

      main.select(".bnet_resetdaq").style("display", ctrl_visible).on("click", () => {
         if (confirm("Really drop buffers on all BNET nodes"))
            DABC.invokeCommand(itemname+"/ResetDAQ");
      });

      main.select(".bnet_totalrate").on("click", () => {
         painter.displayItem("/"+itemname+"/DataRate");
      });

      main.select(".bnet_totalevents").on("click", () => {
         painter.displayItem("/"+itemname+"/EventsRate");
      });

      main.select(".bnet_lostevents").on("click", () => {
         painter.displayItem("/"+itemname+"/LostRate");
      });

      main.select(".bnet_frameclear").on("click", () => {
         painter.displayCalItem(0, "");
         painter.clearDisplay();
      });

      main.select(".bnet_histclear").style("display", ctrl_visible).on("click", () => {
         painter.clearAllHistograms();
      });

      // set top painter after drawing
      this.setTopPainter();
   }

   clearDisplay() {
      let frame = this.hpainter.getDisplay().findFrame("dabc_drawing");
      if (frame) this.hpainter.getDisplay().cleanupFrame(frame);
   }

   displayItem(itemname) {
      let items = null, opt = "";

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

         let subitem = itemname.substr(7);
         itemname = "";
         for (let k=0;k<items.length;++k) {
            itemname += (k>0 ? "," : "[") + items[k].substr(1) + subitem;
            opt += (k>0 ? "," : "[") + "plc";
            if (k==0) opt += "frameid:dabc_drawing";
         }

         itemname += ",$legend]";
         opt += ",any]";
      }

      this.clearDisplay();
      this.hpainter.displayItems([itemname], [opt]);
   }

   displayCalItem(hubid, itemname) {
      this.CalibrHub = hubid;
      this.CalibrItem = itemname;

      d3_select(this.frame).select('.bnet_trb_info')
                           .style("display", (itemname || hubid) ? null : "none")
                           .select("legend").html("HUB: 0x" + hubid.toString(16));

      d3_select(this.frame).select('.bnet_tdc_calibr').html(""); // clear
   }

   clearAllHistograms() {
       for(let indx=0;indx<this.InputItems.length;++indx) {
          let itemname = this.InputItems[indx],
              info = this.InputInfo[indx];

          if (!itemname || !info || !info.calibr) continue;

          itemname = itemname.substr(1, itemname.lastIndexOf("/"));

          for (let k=0;k<info.calibr.length;++k)
             if (info.calibr[k])
                httpRequest(itemname + info.calibr[k] + "/cmd.json?command=ClearHistos", "object");
       }

       setTimeout(() => this.hpainter.updateItems(false), 1000);
   }

   getQualityColor(quality) {
      if (quality <= 0.3) return "red";
      if (quality < 0.7) return "yellow";
      if (quality < 0.8) return "lightblue";
      if (quality <= 0.9) return "green";
      return "lightgreen";
   }

   processReq(isbuild, indx, res) {
      if (!res) return;

      let frame = d3_select(this.frame), elem,
          html = "", itemname = "", hadaqinfo = null, hadaqdata = null, hadaqevents = null, hadaqstate = null;

      for (let n=0;n<res._childs.length;++n) {
         let chld = res._childs[n];
         if (chld._name == "HadaqData") hadaqdata = chld; else
         if (chld._name == "HadaqEvents") hadaqevents = chld; else
         if (chld._name == "HadaqInfo") hadaqinfo = chld; else
         if (chld._name == "State") hadaqstate = chld;
      }

      html += "<pre style='margin:0'>";

      if (isbuild) {
         this.BuilderInfo[indx] = res;
         elem = frame.select(".bnet_builder" + indx);
         let col = this.getQualityColor(res.quality);
         itemname = this.BuilderItems[indx];
         let pos = itemname.lastIndexOf("/");
         html += this.makeLabel("class='bnet_item_label h_item' itemname='" + itemname.substr(0,pos) + "/Terminal/Output' style='background-color:" + col +
                                "' title='Item: " + itemname + "  State: " + hadaqstate.value + "  " + (res.mbsinfo || "") + " canrecv:[" + (res.queues || "-") + "]'", this.BuilderNodes[indx].substr(7), 20);
      } else {
         this.InputInfo[indx] = res;
         elem = frame.select(".bnet_input" + indx);
         let col = this.getQualityColor(res.quality);
         itemname = this.InputItems[indx];
         let title = "Item: " + itemname + "  State: " + hadaqstate.value;
         if (res.progress) title += " progress:" + res.progress;
         title += " cansend:[" + (res.queues || "-") + "]";

         let pos = itemname.lastIndexOf("/");
         html += this.makeLabel("class='bnet_item_label h_item' itemname='" + itemname.substr(0,pos) + "/Terminal/Output' style='background-color:" + col +
                                "' title='" + title + "'", this.InputNodes[indx].substr(7), 20);
      }

      let prefix = "class='bnet_item_label h_item' itemname='" + itemname + "/";

      html += "| " + this.makeLabel(prefix + hadaqdata._name + "'", hadaqdata.value, 8);
      html += "| " + this.makeLabel(prefix + hadaqevents._name + "'", hadaqevents.value, 8);

      if (isbuild) {
         let fname = res.runname || "";
         if (fname && (fname.lastIndexOf("/")>0))
            fname = fname.substr(fname.lastIndexOf("/")+1);

         html += "| " + this.makeLabel("title='Full name: " + res.runname + "'", fname, 24);
         html += "| " + this.makeLabel(prefix + "RunFileSize'", ((res.runsize || 0)/1024/1024).toFixed(2), 8);

         fname = res.ltsmname || "";
         if (fname && (fname.lastIndexOf("/")>0))
            fname = fname.substr(fname.lastIndexOf("/")+1);

         html += "| " + this.makeLabel("title='Full name: " + res.ltsmname + "'", fname, 24);
         html += "| " + this.makeLabel(prefix + "LtsmFileSize'", ((res.ltsmsize || 0)/1024/1024).toFixed(2), 8);

         // if (hadaqinfo) html += "| " + this.makeLabel(prefix + hadaqinfo._name + "'", hadaqinfo.value, 30);
      } else {
         // info with HUBs and port numbers
         let totallen = 0;
         html += "|";
         if (res.ports && res.hubs && (res.ports.length == res.hubs.length)) {
            for (let k=0;k<res.ports.length;++k) {
               if (this.CalibrHub == res.hubs[k])
                  frame.select(".bnet_hub_info").html("<pre>" + res.hubs_info[k] + "</pre>");
               let txt = "0x"+res.hubs[k].toString(16);
               totallen += txt.length;
               let title = "state:" + res.hubs_state[k] +
                           " quality:" + res.hubs_quality[k] +
                           " progr:" + res.hubs_progress[k] +
                           " " + res.hubs_info[k];
               let style = "background-color:" + this.getQualityColor(res.hubs_quality[k]);
               let calitem = "";
               if (res.calibr[k])
                  calitem = itemname.substr(0, itemname.lastIndexOf("/")+1) + res.calibr[k];
               let attr = "hubid='" + res.hubs[k] + "' itemname='" + calitem + "' class='h_item bnet_trb_label'";

               html += " " + this.makeLabel("title='" + title + "' style='" + style + "' " + attr, txt, txt.length);
               // if (totallen>40) break;
            }
         }

         if (!totallen) html += " " + this.makeLabel("", "failure", 40);
      }

      html += "</pre>";

      let painter = this,
          main = elem.html(html);

      main.selectAll(".bnet_item_label").on("click", function() {
         painter.displayItem(d3_select(this).attr("itemname"));
      });
      main.selectAll(".bnet_trb_label").on("click", function() {
         painter.displayCalItem(parseInt(d3_select(this).attr("hubid")), d3_select(this).attr("itemname"));
      });
   }

   processCalibrReq(res) {
      if (!res) return;

      this.CalibrInfo = res;

      DABC.updateTrbStatus(d3_select(this.frame).select('.bnet_tdc_calibr'), res, this.hpainter, false);
   }

   sendInfoRequests() {
      for (let n in this.InputItems)
         httpRequest(this.InputItems[n].substr(1) + "/get.json", "object").then(this.processReq.bind(this, false, n));
      for (let n in this.BuilderItems)
         httpRequest(this.BuilderItems[n].substr(1) + "/get.json", "object").then(this.processReq.bind(this, true, n));
      if (this.CalibrItem)
         httpRequest(this.CalibrItem.substr(1) + "/Status/get.json", "object").then(res => this.processCalibrReq(res));
   }

   processMainRequest(res) {

      let dom = d3_select(this.frame);

      dom.style('background-color', res ? null : "grey");

      let chkbox = dom.select(".bnet_monitoring");
      if (!chkbox.empty() && (chkbox.property('checked') != this.hpainter.isMonitoring()))
         chkbox.property('checked', this.hpainter.isMonitoring());

      if (!res) return;

      let inp = null, bld = null, state = null, quality = null, drate, erate, lrate, ninp = [],
          nbld = [], runid = "", runprefix = "", changed = false, lastprefix = "", lastcalibr = null;
      for (let k in res._childs) {
         let elem = res._childs[k];
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
         let col = this.getQualityColor(quality || 0);
         // col = "red";
         //if (state=="Ready") col = "lightgreen"; else
         //if (state=="Accumulating") col = "lightblue"; else
         //if (state=="NoFile") col = "yellow";
         dom.select(".bnet_state").text("Run control: " + state).style('background-color',col);
      }

      if (typeof drate == 'number')
         dom.select(".bnet_totalrate").text(drate.toFixed(2) + " MB/s").style('background-color', (drate > 0) ? "lightgreen" : "yellow");

      if (typeof erate == 'number')
         dom.select(".bnet_totalevents").text(erate.toFixed(1) + " Ev/s").style('background-color', (erate > 0) ? "lightgreen" : "yellow");

      if (typeof lrate == 'number')
         dom.select(".bnet_lostevents").text(lrate.toFixed(1) + " Ev/s").style('background-color', (lrate > 0) ? "yellow" : "lightgreen");

      if (lastcalibr && (typeof lastcalibr.quality == 'number') && (typeof lastcalibr.time == 'string')) {
         let quality = lastcalibr.quality || 1,
             dt = new Date(lastcalibr.time), now = new Date(),
             diff = (now.getTime() - dt.getTime())*1e-3, // seconds
             info = "CALIBR", title = "Calibration ";
         if (diff < 0) { info = "CHECK CALIBR"; if (quality>0.6) quality = 0.6; } else {
            let hv = Math.floor(diff/3600),
                h = hv.toString(),
                m = Math.round((diff - hv)/60).toString();
            if (m.length == 1) m = "0"+m;
            info = h+"h"+m+"m";
            if (h>720) { if (quality>0.1) quality = 0.1; title = "To long time without calibration, "; } else
            if (h>240) { if (quality>0.6) quality = 0.6; title = "Consider to peform calibration, "; }
         }
         title += "last: " + lastcalibr.value;
         dom.select(".bnet_lastcalibr").style('background-color', this.getQualityColor(quality)).attr("title", title).text(info);
      }

      dom.select(".bnet_runid_lbl").text(" RunId: " + runid);
      dom.select(".bnet_runprefix_lbl").text(" Prefix: " + runprefix);

      // run prefix should not be overwritten
      //let sm = dom.select(".bnet_selectrun");
      //if (runprefix && (sm.property("value") != runprefix)) {
      //   sm.property("value", runprefix);
      //}

      if (!DABC.compareArrays(this.InputItems,inp)) {
         this.InputItems = inp;
         this.InputNodes = ninp;
         this.InputInfo = [];
         changed = true;
      }

      if (!DABC.compareArrays(this.BuilderItems,bld)) {
         this.BuilderItems = bld;
         this.BuilderNodes = nbld;
         this.BuilderInfo = [];
         changed = true;
      }

      if (changed) {
         this.displayCalItem(0, "");
         this.refreshHtml(lastprefix);
         this.hpainter.reload(); // also refresh hpainter - most probably items are changed
      }

      this.sendInfoRequests();
   }

   sendMainRequest() {
      if (this.mainreq) return;
      this.mainreq = true;
      httpRequest(this.itemname + "/get.json", "object")
         .then(res => this.processMainRequest(res))
         .catch(() => this.processMainRequest(null))
         .finally(() => { this.mainreq = false; });
   }
}

DABC.BnetControl = function(hpainter, itemname) {
   return hpainter.createCustomDisplay(itemname, "vert2").then(() => {
      let painter = new BnetPainter(hpainter, itemname);
      if (painter.active()) {
         painter.refreshHtml();
         painter.main_timer = setInterval(() => painter.sendMainRequest(), 2000);
         painter.displayItem("/"+painter.itemname+"/EventsRate");
      }
   });
}

// ================================== NEW CODE ========================================================


DABC.ReqH = function(hpainter, item, url) {
   return 'get.json.gz?compact=3';
}

DABC.ConvertH = function(hpainter, item, obj) {
   if (!obj) return;

   let res = {};
   for (let k in obj) res[k] = obj[k]; // first copy all keys
   for (let k in res) delete obj[k];   // then delete them from source

   if (res._kind == 'ROOT.TH1D') {
      create("TH1D", obj);
      Object.assign(obj, { fName: res._name, fTitle: res._title, fFillStyle: 1001 });

      Object.assign(obj.fXaxis, { fNbins: res.nbins, fXmin: res.left,  fXmax: res.right });
      res.bins.splice(0,3); // 3 first items in array are not bins
      obj.fArray = res.bins;
      obj.fNcells = res.nbins + 2;
   } else if (res._kind == 'ROOT.TH2D') {
      create("TH2D", obj);
      Object.assign(obj, { fName: res._name, fTitle: res._title });

      Object.assign(obj.fXaxis, { fNbins: res.nbins1, fXmin: res.left1,  fXmax: res.right1 });
      Object.assign(obj.fYaxis, { fNbins: res.nbins2, fXmin: res.left2,  fXmax: res.right2 });
      res.bins.splice(0,6); // 6 first items in array are not bins
      obj.fNcells = (res.nbins1+2) * (res.nbins2+2);
      obj.fArray = res.bins;
   } else if (res._kind == 'ROOT.TH2Poly') {
      create("TH2D", obj);
      Object.assign(obj, { fName: res._name, fTitle: res._title });
      Object.assign(obj.fXaxis, { fNbins: res.nbins1, fXmin: res.left1,  fXmax: res.right1 });
      Object.assign(obj.fYaxis, { fNbins: res.nbins2, fXmin: res.left2,  fXmax: res.right2 });

      res.bins.splice(0,6); // 6 first items in array are not bins
      obj.fNcells = (res.nbins1+2) * (res.nbins2+2);
      obj.fArray = res.bins;

      obj.fBins = create("TList");

      let npoly = 0, h2poly = JSON.parse(res.h2poly);
      for (let n2 = 0; n2 < res.nbins2; ++n2)
         for (let n1 = 0; n1 < res.nbins1; ++n1) {
            if (npoly >= h2poly.length) break;

            let poly = h2poly[npoly++];
            if (!poly || (poly.length !== 2)) continue;

            let xx = poly[0], yy = poly[1];

            let bin = {
               _typename: "TH2PolyBin",
               fNumber: 1,
               fContent: obj.getBinContent(n1+1, n2+1),
               fPoly: JSROOT.createTGraph(xx.length, xx, yy),
               fXmin: Math.min.apply(null, xx),
               fXmax: Math.max.apply(null, xx),
               fYmin: Math.min.apply(null, yy),
               fYmax: Math.max.apply(null, yy)
            };
            // used of text position only, for triangle workaround
            /* if (xx.length == 4) {
               let xsum = 0, ysum = 0;
               for (let k=0;k<xx.length;++k) {
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
      obj.fXaxis.fLabels = create('THashList');
      let lbls = res.xlabels.split(",");
      for (let n in lbls) {
         let lbl = create('TObjString');
         lbl.fUniqueID = parseInt(n)+1;
         lbl.fString = lbls[n];
         obj.fXaxis.fLabels.Add(lbl);
      }
   }
   if ('ylabels' in res) {
      obj.fYaxis.fLabels = create('THashList');
      let lbls = res.ylabels.split(",");
      for (let n in lbls) {
         let lbl = create('TObjString');
         lbl.fUniqueID = parseInt(n)+1;
         lbl.fString = lbls[n];
         obj.fYaxis.fLabels.Add(lbl);
      }
   }
}


DABC.ExtractSeries = function(name, kind, obj, history) {

   const extractField = node => {
      if (!node || !(name in node)) return null;

      if (kind=="number") return Number(node[name]);
      if (kind=="time") {
         let d  = new Date(node[name]);
         return d.getTime() / 1000.;
      }
      return node[name];
   };

   // xml node must have attribute, which will be extracted
   let val = extractField(obj);
   if (val==null) return null;

   let arr = [];
   arr.push(val);

   if (history!=null)
      for (let n = history.length-1; n >= 0; n--) {
         // in any case stop iterating when see property delete
         if ("dabc:del" in history[n]) break;
         let newval = extractField(history[n]);
         if (newval!=null) val = newval;
         arr.push(val);
      }

   arr.reverse();
   return arr;
}

function makeItemRequest(h, item, fullpath, option) {
   item.fullitemname = fullpath;
   if (!('_history' in item) || (option=="gauge") || (option=='last')) return "get.json?compact=0";
   if (!('hlimit' in item)) item.hlimit = 100;
   let url = "get.json?compact=0&history=" + item.hlimit;
   if (('request_version' in item) && (item.request_version>0)) url += "&version=" + item.request_version;
   item.request_version = 0;
   return url;
}

function afterItemRequest(h, item, obj, option) {
   if (!obj) return;

   if (!('_history' in item) || (option=="gauge") || (option=='last')) {
      obj.fullitemname = item.fullitemname;
      // for gauge output special scripts should be loaded, use unique class name for it
      if (obj._kind == 'rate') obj._typename = "DABC_RateGauge";
      return;
   }

   let new_version = Number(obj._version);

   // let modified = (item.request_version != new_version);

   item.request_version = new_version;

   // this is array with history entries
   let arr = obj.history;

   if (arr!=null) {
      // gap indicates that we could not get full history relative to provided version number
      let gap = obj.history_gap;

      // join both arrays with history entries
      if ((item.history == null) || (arr.length >= item['hlimit']) || gap) {
         item.history = arr;
      } else if (arr.length > 0) {
         // modified = true;
         let total = item.history.length + arr.length;
         if (total > item['hlimit'])
            item.history.splice(0, total - item['hlimit']);

         item.history = item.history.concat(arr);
      }
   }

   if (obj._kind == "log") {
      obj.log = DABC.ExtractSeries("value", "string", obj, item.history);
      return;
   }

   // now we should produce TGraph from the object

   let x = DABC.ExtractSeries("time", "time", obj, item.history),
       y = DABC.ExtractSeries("value", "number", obj, item.history);

   for (let k in obj) delete obj[k];  // delete all object keys

   if (!x) x = [];
   if (!y) y = [];

   obj._typename = 'TGraph';
   create('TGraph', obj);
   obj.fHistogram = JSROOT.createHistogram('TH1F', x.length);

   let _title = item._title || item.fullitemname;

   Object.assign(obj, { fName: item._name, fTitle: _title,
                        fX: x, fY: y, fNpoints: x.length,
                        fLineColor: 2, fLineWidth: 2 });
   const kNotEditable = JSROOT.BIT(18);   // bit set if graph is non editable
   obj.InvertBit(kNotEditable);

   let xrange = x[x.length-1] - x[0];
   obj.fHistogram.fTitle = obj.fTitle;
   obj.fHistogram.fXaxis.fXmin = x[0] - xrange*0.03;
   obj.fHistogram.fXaxis.fXmax = x[x.length-1] + xrange*0.03;

   let ymin = Math.min.apply(null,y), ymax = Math.max.apply(null,y);

   if (!ymin) ymin = 0; if (!ymax) ymax = 0;

   obj.fHistogram.fYaxis.fXmin = ymin - 0.05 * (ymax-ymin);
   obj.fHistogram.fYaxis.fXmax = ymax + 0.05 * (ymax-ymin);

   obj.fHistogram.fXaxis.fTimeDisplay = true;
   obj.fHistogram.fXaxis.fTimeFormat = "%H:%M:%S%F0"; // %FJanuary 1, 1970 00:00:00
}

class GaugePainter extends BasePainter {

   constructor(dom) {
      super(dom);
      this.gauge = null;
      this.min = 0;
      this.max = 1;
      this.lastval = 0;
      this.lastsz = 0;
      this._title = "";
      this._units = "";
   }

   getDomId() {
      let elem = this.selectDom();
      if (elem.empty()) return "";
      let id = elem.attr("id");
      if (!id) {
         id = "dabc_element_" + DABC.id_counter++;
         elem.attr("id", id);
      }
      return id;
   }

   drawGauge(obj) {
      if (!obj) return;

      let val = (obj.value === undefined) ? 0 : Number(obj.value);

      if ('low' in obj) {
         let min = Number(obj.low);
         if (Number.isFinite(min) && (min < this.min)) this.min = min;
      }

      if ('up' in obj) {
         let max = Number(obj.up);
         if (Number.isFinite(max) && (max > this.max)) this.max = max;
      }

      let redo = false;

      if (val > this.max) {
         this.max = 1;
         let cnt = 0;
         while (val > this.max)
            this.max *= (((cnt++ % 3) == 1) ? 2.5 : 2);

         redo = true;
      }

      this._title = obj._name || "gauge";
      this._units = obj.units || "";
      if ((obj.value === undefined) || (val == 0))
         val = "0";
      else if ((Math.round(val) == val) && (Math.abs(val) < 100000))
         val = val.toString();
      else
         val = JSROOT.floatToString(val, "5.3g");
      this.drawValue(val, redo);
   }

   drawValue(val, force) {

      let rect = this.selectDom().node().getBoundingClientRect(),
          sz = Math.min(rect.height, rect.width);

      if ((sz > this.lastsz*1.2) || (sz < this.lastsz*0.9)) force = true;

      if (force) {
         this.selectDom().selectAll("*").remove();
         this.gauge = null;
      }

      this.lastval = val;

      if (!this.gauge) {
         this.lastsz = sz;

         let config =  {
            size: sz,
            label: this._title,
            min: this.min,
            max: this.max,
            majorTicks : 6,
            minorTicks: 5
         };

         if (this._units) config['units'] = this._units + "/s";

         if (!this.gauge)
            this.gauge = new Gauge(this.getDomId(), config);
         else
            this.gauge.configure(config);

         this.gauge.render(this.lastval);

      } else {
         this.gauge.redraw(this.lastval);
      }

      // always set painter
      this.setTopPainter();
   }

   checkResize() {
      this.drawValue(this.lastval);
   }

   redrawObject(obj) {
      this.drawGauge(obj);
      return true;
   }
}

DABC.DrawGauage = function(dom, obj, opt) {

   // at this momemnt gauge should be loaded
   if (typeof Gauge == 'undefined') {
      alert('gauge.js not loaded');
      return null;
   }

   let painter = new GaugePainter(dom);

   painter.drawGauge(obj);

   return Promise.resolve(painter);
}

// ==========================================================================================

class DabcLogPainter extends BasePainter {

   constructor(dom, obj, opt) {
      super(dom);
      this.obj = obj;
      this.history = (opt!="last") && ('log' in obj); // by default draw complete history

      if (this.history) {
         this.selectDom().html("<div style='overflow:auto; max-height: 100%; max-width: 100%; font-family:monospace'></div>");
      } else {
         this.selectDom().html("<div></div>");
      }
      // set top painter after HTML element created
      this.setTopPainter();
   }

   redrawObject(obj) {
      this.obj = obj;
      this.drawLog();
      return true;
   }

   drawLog() {
      let html = "";

      if (this.history && ('log' in this.obj)) {
         for (let i in this.obj.log) {
            html+="<pre style='margin-top:2px;margin-bottom:0px'>"+this.obj.log[i]+"</pre>";
         }
      } else {
         html += obj['fullitemname'] + "<br/>";
         html += "<h5>"+ this.obj.value +"</h5>";
      }
      this.selectDom().select("div").html(html);
   }

}

DABC.DrawLog = function(dom, obj, opt) {
   let painter = new DabcLogPainter(dom, obj, opt);
   painter.drawLog();
   return Promise.resolve(painter);
}

class DabcCommandPainter extends BasePainter {

   constructor(dom, obj) {
      super(dom);
      this.jsonnode = obj;
   }

   numArgs() { return this.jsonnode ? this.jsonnode["numargs"] : 0; }

   argName(n) { return (n < this.numArgs()) ? this.jsonnode["arg"+n] : ""; }

   findArg(name) {
      let num = this.numArgs();
      for (let i = 0; i < num; ++i)
         if (this.argName(i) == name)
            return i;
      return -1;
   }

   argKind(n) { return (n < this.numArgs()) ? this.jsonnode["arg"+n+"_kind"] : ""; }

   argDflt(n) { return (n < this.numArgs()) ? this.jsonnode["arg"+n+"_dflt"] : ""; }

   argMin(n) { return (n < this.numArgs()) ? this.jsonnode["arg"+n+"_min"] : null; }

   argMax(n) { return (n < this.numArgs()) ? this.jsonnode["arg"+n+"_max"] : null; }

   showCommand() {

      let dom = this.selectDom();

      dom.html("");

      if (!this.jsonnode)
         return dom.html("cannot access command definition...<br/>");

      dom.append("h3").text(this.jsonnode.fullitemname);

      dom.append("button")
         .attr("title","Execute command " + this.jsonnode.fullitemname)
         .text("Exectute")
         .on("click", () => this.invokeCommand());

      for (let cnt = 0; cnt < this.numArgs(); cnt++) {
         let argname = this.argName(cnt),
             argkind = this.argKind(cnt);

         dom.append("div").html(`Arg: ${argname} <input class="dabccmd_arg${cnt}">`);

         let elem = dom.select(`.dabccmd_arg${cnt}`)
            .attr("type", argkind=="int" ? "number" : "text")
            .style("width", (argkind=="int") ? "80px" : "170px")
            .property("value", this.argDflt(cnt));

         if (argkind == "int")
            elem.attr("min", this.argMin(cnt)).attr("max", this.argMax(cnt));
      }

      dom.append("div").classed("dabccmd_res", true);
   }

   invokeCommand() {
      if (this.req) return;

      let dom = this.selectDom(),
          resdiv = dom.select('.dabccmd_res'),
          url = this.jsonnode.fullitemname + '/execute';

      resdiv.html("<h5>Send command to server</h5>");

      for (let cnt = 0; cnt < this.numArgs(); cnt++) {
         url += (cnt==0) ? '?' : '&';
         url += this.argName(cnt);
         url += '=';
         url += dom.select(`.dabccmd_arg${cnt}`).property('value');
      }

      this.req = true;

      httpRequest(url, 'object')
          .then(res => {
             let code = `<h5>Get command ${res._name} reply res = ${res._Result_}</h5>`;
             for (let key in res) {
                // do not show internal args
                if (key[0] == '_') continue;

                let indx = this.findArg(key);
                // do not show input args
                if ((indx >= 0) && (dom.select(`.dabccmd_arg${indx}`).property('value') == res[key])) continue;

                code += `<pre>${key} = ${JSON.stringify(res[key])}</pre>`;
              }
              resdiv.html(code);
           })
          .catch(() => resdiv.html("<h5>missing reply from server</h5>"))
          .finally(() => { this.req = false; });
   }
}

DABC.DrawCommand = function(dom, obj, opt) {

   let painter = new DabcCommandPainter(dom, obj, opt);

   painter.showCommand();

   painter.setTopPainter();

   return Promise.resolve(painter);
}

// ==================================================================

TH1Painter.prototype.oldFillHistContextMenu = TH1Painter.prototype.fillHistContextMenu;

TH1Painter.prototype.fillHistContextMenu = function(menu) {
   let itemname = this.getItemName() || "", match_name;
   ['HLD_HitsPerTDC', 'HLD_ErrPerTDC', 'HLD_ExpectedToT'].forEach(name => {
       if (itemname.indexOf(name) > 0) match_name = true;
   });

   if (match_name) {
      let tip = menu.painter.getToolTip(menu.getEventPosition());

      let histo = menu.painter.getHisto(),
          binlbl = menu.painter.getAxisBinTip("x", histo.fXaxis, tip.bin-1);

      let focusOnTdc = lbl => {
         let tdc_name = "TDC_" + lbl.substr(2), tdc_item;

         let hpainter = getHPainter();

         hpainter.forEachItem(item => {
            if (item._name == tdc_name)
               tdc_item = item;
         });
         if (tdc_item)
            hpainter.focusOnItem(tdc_item);
      }

      if (binlbl && (typeof binlbl == "string") && (binlbl.indexOf("0x")==0))
         menu.add(`Focus on TDC ${binlbl}`, () => focusOnTdc(binlbl));

      menu.add("Find TDC", () => menu.input("TDC id", binlbl, "TDC id").then(id => {
         if (!id) return;
         id = id.substr(0,2) + id.substr(2).toUpperCase();

         let nbins = histo.fXaxis.fNbins;
         for (let bin = 0; bin < nbins; ++bin) {
            let lbl = menu.painter.getAxisBinTip("x", histo.fXaxis, bin);
            if (lbl == id) {
               console.log(`Find id ${id} with bin ${bin}`);
               focusOnTdc(id);
               let fp = menu.painter.getFramePainter();
               fp.zoomSingle("x", Math.max(1, bin - 10), Math.min(nbins-1, bin+10));

               fp.zoomChangedInteractive("x", true);
               break;
            }
         }
      }));

   }

   return this.oldFillHistContextMenu(menu);
}

TH2Painter.prototype.oldFillHistContextMenu = TH2Painter.prototype.fillHistContextMenu;

TH2Painter.prototype.fillHistContextMenu = function(menu) {
   let itemname = this.getItemName() || "", match_name;
   ['HLD_HitsPerChannel', 'HLD_ErrPerChannel', 'HLD_CorrPerChannel', 'HLD_ToTPerChannel', 'HLD_ShiftPerChannel', 'HLD_DevPerChannel',
    'HLD_QaFinePerChannel', 'HLD_QAToTPerChannel', 'HLD_QaEdgesPerChannel', 'HLD_QaErrorsPerChannel'].forEach(name => {
       if (itemname.indexOf(name) > 0) match_name = true;
   });

   if (match_name) {
      let tip = menu.painter.getToolTip(menu.getEventPosition());

      // example how to get label from bin index
      // if (tip.binx !== undefined) console.log('binx as text', menu.painter.getFramePainter().axisAsText("x", tip.binx));
      // if (tip.biny !== undefined) console.log('biny as text', menu.painter.getFramePainter().axisAsText("y", tip.biny));

      let histo = menu.painter.getHisto(),
          binlbl = menu.painter.getAxisBinTip("x", histo.fXaxis, tip.binx-1);

      const focusOnTdc = lbl => {
         let tdc_name = "TDC_" + lbl.substr(2), tdc_item;

         let hpainter = getHPainter();

         hpainter.forEachItem(item => {
            if (item._name == tdc_name)
               tdc_item = item;
         });
         if (tdc_item)
            hpainter.focusOnItem(tdc_item);
      };

      if (binlbl && (typeof binlbl == "string") && (binlbl.indexOf("0x")==0))
         menu.add(`Focus on TDC ${binlbl}`, () => focusOnTdc(binlbl));

      menu.add("Find TDC", () => menu.input("TDC id", binlbl).then(id => {
         if (!id) return;
         id = id.substr(0,2) + id.substr(2).toUpperCase();

         let nbins = histo.fXaxis.fNbins;
         for (let bin = 0; bin < nbins; ++bin) {
            let lbl = menu.painter.getAxisBinTip("x", histo.fXaxis, bin);
            if (lbl == id) {
               console.log(`Find id ${id} with bin ${bin}`);
               focusOnTdc(id);
               let fp = menu.painter.getFramePainter();
               fp.zoomSingle("x", Math.max(1, bin - 10), Math.min(nbins-1, bin+10));

               fp.zoomChangedInteractive("x", true);
               break;
            }
         }
      }));
   }

   return this.oldFillHistContextMenu(menu);
};

// ===================================================================

addDrawFunc({
   name: "kind:rate",
   icon: DABC.source_dir + "img/gauge.png",
   func: "<dummy>",
   opt: "line;gauge",
   monitor: 'always',
   make_request: makeItemRequest,
   after_request: afterItemRequest
});

addDrawFunc({
   name: "kind:log",
   icon: "img_text",
   func: DABC.DrawLog,
   opt: "log;last",
   monitor: 'always',
   make_request: makeItemRequest,
   after_request: afterItemRequest
});

addDrawFunc({
   name: "kind:DABC.Command",
   icon: DABC.source_dir + "img/dabc.png",
   make_request: makeItemRequest,
   after_request: afterItemRequest,
   opt: "command",
   prereq: 'jq',
   monitor: 'never',
   func: 'DABC.DrawCommand'
});

// only indicate that item with such kind can be opened as direct link
addDrawFunc({
   name: "kind:DABC.HTML",
   icon: DABC.source_dir + "img/dabc.png",
   aslink: true
});

// example of external scripts loading
addDrawFunc({
   name: "DABC_RateGauge",
   func: "DABC.DrawGauage",
   opt: "gauge",
   script: DABC.source_dir + 'scripts/gauge.js'
});

globalThis.DABC = DABC; // maybe used outside

