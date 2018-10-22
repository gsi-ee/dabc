/// @file JSRootPainter.openui5.js
/// Part of JavaScript ROOT graphics, dependent from openui5 functionality
/// Openui5 loaded directly in the script

(function( factory ) {
   if ( typeof define === "function" && define.amd ) {
      define( ['jquery', 'jquery-ui', 'd3', 'JSRootPainter', 'JSRootPainter.hierarchy', 'JSRootPainter.jquery' ], factory );
   } else {

      if (typeof jQuery == 'undefined')
         throw new Error('jQuery not defined', 'JSRootPainter.openui5.js');

      if (typeof jQuery.ui == 'undefined')
         throw new Error('jQuery-ui not defined','JSRootPainter.openui5.js');

      if (typeof d3 != 'object')
         throw new Error('This extension requires d3.v3.js', 'JSRootPainter.openui5.js');

      if (typeof JSROOT == 'undefined')
         throw new Error('JSROOT is not defined', 'JSRootPainter.openui5.js');

      if (typeof JSROOT.Painter != 'object')
         throw new Error('JSROOT.Painter not defined', 'JSRootPainter.openui5.js');

      factory(jQuery, jQuery.ui, d3, JSROOT);
   }
} (function($, myui, d3, JSROOT) {

   "use strict";

   JSROOT.sources.push("openui5");

   var load_callback = JSROOT.complete_script_load;
   delete JSROOT.complete_script_load; // normal callback is intercepted - we need to instantiate openui5

   JSROOT.completeUI5Loading = function() {
      // console.log('complete ui5 loading', typeof sap);
      JSROOT.sap = sap;

      // var cust_style = document.createElement("link");
      // cust_style.setAttribute("rel", "stylesheet");
      // cust_style.setAttribute("type", "text/css");
      // cust_style.setAttribute("href", JSROOT.source_dir + "openui5/custom.css");
      // document.getElementsByTagName("head")[0].appendChild(cust_style);

      JSROOT.CallBack(load_callback);
      load_callback = null;
   }

   var element = document.createElement("script");
   element.setAttribute('type', "text/javascript");
   element.setAttribute('id', "sap-ui-bootstrap");
   // use nojQuery while we are already load jquery and jquery-ui, later one can use directly sap-ui-core.js

   // where to take openui5 sources
   var src = "https://openui5.hana.ondemand.com/";
   if (JSROOT.openui5src == 'jsroot') src = JSROOT.source_dir + "openui5dist/"; else
   if (typeof JSROOT.openui5src == 'string') src = JSROOT.openui5src;

   console.log('Use openui5 from ', src);

   // this is location of openui5 scripts when working with THttpServer or when scripts are installed inside JSROOT
   element.setAttribute('src', src + "resources/sap-ui-core-nojQuery.js"); // latest openui5 version

   // this was default location
   // element.setAttribute('src', "https://openui5.hana.ondemand.com/resources/sap-ui-core-nojQuery.js"); // latest openui5 version

   // element.setAttribute('src', "/currentdir/openui5/resources/sap-ui-core-nojQuery.js"); // can be used with THttpServer
   // element.setAttribute('src', "https://openui5.hana.ondemand.com/1.38.21/resources/sap-ui-core-nojQuery.js"); // some previous version
//   element.setAttribute('data-sap-ui-trace', "true");

   element.setAttribute('data-sap-ui-libs', JSROOT.openui5libs || "sap.m, sap.ui.layout, sap.ui.unified, sap.ui.commons");

//   element.setAttribute('data-sap-ui-areas', "uiArea1");

   element.setAttribute('data-sap-ui-theme', 'sap_belize');
   element.setAttribute('data-sap-ui-compatVersion', 'edge');
   // element.setAttribute('data-sap-ui-bindingSyntax', 'complex');

   element.setAttribute('data-sap-ui-preload', 'async'); // '' to disable Component-preload.js

   // configure path for openui5 scripts
   element.setAttribute('data-sap-ui-resourceroots', '{ "sap.ui.jsroot": "' + JSROOT.source_dir + 'openui5/" }');

   element.setAttribute('data-sap-ui-evt-oninit', "JSROOT.completeUI5Loading()");

   document.getElementsByTagName("head")[0].appendChild(element);

   // function allows to create menu with openui
   // for the moment deactivated - can be used later
   JSROOT.Painter.createMenuNew = function(painter, maincallback) {

      var menu = { painter: painter,  element: null, cnt: 1, stack: [], items: [], separ: false };

      // this is slightly modified version of original MenuItem.render function.
      // need to be updated with any further changes
      function RenderCustomItem(rm, oItem, oMenu, oInfo) {
         var oSubMenu = oItem.getSubmenu();
         rm.write("<li ");

         var sClass = "sapUiMnuItm";
         if (oInfo.iItemNo == 1) {
            sClass += " sapUiMnuItmFirst";
         } else if (oInfo.iItemNo == oInfo.iTotalItems) {
            sClass += " sapUiMnuItmLast";
         }
         if (!oMenu.checkEnabled(oItem)) {
            sClass += " sapUiMnuItmDsbl";
         }
         if (oItem.getStartsSection()) {
            sClass += " sapUiMnuItmSepBefore";
         }

         rm.writeAttribute("class", sClass);
         if (oItem.getTooltip_AsString()) {
            rm.writeAttributeEscaped("title", oItem.getTooltip_AsString());
         }
         rm.writeElementData(oItem);

         // ARIA
         if (oInfo.bAccessible) {
            rm.writeAccessibilityState(oItem, {
               role: "menuitem",
               disabled: !oMenu.checkEnabled(oItem),
               posinset: oInfo.iItemNo,
               setsize: oInfo.iTotalItems,
               labelledby: {value: /*oMenu.getId() + "-label " + */this.getId() + "-txt " + this.getId() + "-scuttxt", append: true}
            });
            if (oSubMenu) {
               rm.writeAttribute("aria-haspopup", true);
               rm.writeAttribute("aria-owns", oSubMenu.getId());
            }
         }

         // Left border
         rm.write("><div class=\"sapUiMnuItmL\"></div>");

         // icon/check column
         rm.write("<div class=\"sapUiMnuItmIco\">");
         if (oItem.getIcon()) {
            rm.writeIcon(oItem.getIcon(), null, {title: null});
         }
         rm.write("</div>");

         // Text column
         rm.write("<div id=\"" + this.getId() + "-txt\" class=\"sapUiMnuItmTxt\">");
         rm.write(oItem.custom_html);
         rm.write("</div>");

         // Shortcut column
         rm.write("<div id=\"" + this.getId() + "-scuttxt\" class=\"sapUiMnuItmSCut\"></div>");

         // Submenu column
         rm.write("<div class=\"sapUiMnuItmSbMnu\">");
         if (oSubMenu) {
            rm.write("<div class=\"sapUiIconMirrorInRTL\"></div>");
         }
         rm.write("</div>");

         // Right border
         rm.write("<div class=\"sapUiMnuItmR\"></div>");

         rm.write("</li>");
      }

      JSROOT.sap.ui.define([ 'sap/ui/unified/Menu', 'sap/ui/unified/MenuItem', 'sap/ui/unified/MenuItemBase' ],
                            function(sapMenu, sapMenuItem, sapMenuItemBase) {

         menu.add = function(name, arg, func) {
            if (name == "separator") { this.separ = true; return; }

            if (name.indexOf("header:")==0)
               return this.items.push(new sapMenuItem("", { text: name.substr(7), enabled: false }));

            if (name=="endsub:") {
               var last = this.stack.pop();
               last._item.setSubmenu(new sapMenu("", { items: this.items }));
               this.items = last._items;
               return;
            }

            var issub = false, checked = null;
            if (name.indexOf("sub:")==0) {
               name = name.substr(4);
               issub = true;
            }

            if (typeof arg == 'function') { func = arg; arg = name;  }

            if (name.indexOf("chk:")==0) { name = name.substr(4); checked = true; } else
            if (name.indexOf("unk:")==0) { name = name.substr(4); checked = false; }

            var item = new sapMenuItem("", { });

            if (!issub && (name.indexOf("<svg")==0)) {
               item.custom_html = name;
               item.render = RenderCustomItem;
            } else {
               item.setText(name);
               if (this.separ) item.setStartsSection(true);
               this.separ = false;
            }

            if (checked) item.setIcon("sap-icon://accept");

            this.items.push(item);

            if (issub) {
               this.stack.push({ _items: this.items, _item: item });
               this.items = [];
            }

            if (typeof func == 'function') {
               item.menu_func = func; // keep call-back function
               item.menu_arg = arg; // keep call-back argument
            }

            this.cnt++;
         }

         menu.addchk = function(flag, name, arg, func) {
            return this.add((flag ? "chk:" : "unk:") + name, arg, func);
         }

         menu.size = function() { return this.cnt-1; }

         menu.addDrawMenu = function(menu_name, opts, call_back) {
            if (!opts) opts = [];
            if (opts.length==0) opts.push("");

            var without_sub = false;
            if (menu_name.indexOf("nosub:")==0) {
               without_sub = true;
               menu_name = menu_name.substr(6);
            }

            if (opts.length === 1) {
               if (opts[0]==='inspect') menu_name = menu_name.replace("Draw", "Inspect");
               return this.add(menu_name, opts[0], call_back);
            }

            if (!without_sub) this.add("sub:" + menu_name, opts[0], call_back);

            for (var i=0;i<opts.length;++i) {
               var name = opts[i];
               if (name=="") name = '&lt;dflt&gt;';

               var group = i+1;
               if ((opts.length>5) && (name.length>0)) {
                  // check if there are similar options, which can be grouped once again
                  while ((group<opts.length) && (opts[group].indexOf(name)==0)) group++;
               }

               if (without_sub) name = menu_name + " " + name;

               if (group < i+2) {
                  this.add(name, opts[i], call_back);
               } else {
                  this.add("sub:" + name, opts[i], call_back);
                  for (var k=i+1;k<group;++k)
                     this.add(opts[k], opts[k], call_back);
                  this.add("endsub:");
                  i = group-1;
               }
            }
            if (!without_sub) this.add("endsub:");
         }

         menu.remove = function() {
            if (this.remove_bind) {
               document.body.removeEventListener('click', this.remove_bind);
               this.remove_bind = null;
            }
            if (this.element) {
               this.element.destroy();
               if (this.close_callback) this.close_callback();
            }
            this.element = null;
         }

         menu.remove_bind = menu.remove.bind(menu);

         menu.show = function(event, close_callback) {
            this.remove();

            if (typeof close_callback == 'function') this.close_callback = close_callback;

            var old = sap.ui.getCore().byId("root_context_menu");
            if (old) old.destroy();

            document.body.addEventListener('click', this.remove_bind);

            this.element = new sapMenu("root_context_menu", { items: this.items });

            // this.element.attachClosed({}, this.remove, this);

            this.element.attachItemSelect(null, this.menu_item_select, this);

            var eDock = sap.ui.core.Popup.Dock;
            // var oButton = oEvent.getSource();
            this.element.open(false, null, eDock.BeginTop, eDock.BeginTop, null, event.clientX + " " + event.clientY);
         }

         menu.menu_item_select = function(oEvent) {
            var item = oEvent.getParameter("item");
            if (!item || !item.menu_func) return;
            // console.log('select item arg', item.menu_arg);
            // console.log('select item', item.getText());
            if (this.painter)
               item.menu_func.bind(this.painter)(item.menu_arg);
            else
               item.menu_func(item.menu_arg);
         }

         JSROOT.CallBack(maincallback, menu);
      });

      return menu;
   }

   JSROOT.TObjectPainter.prototype.ShowInpsector = function() {
      var handle = {}; // should be controller?
      handle.closeObjectInspector = function() {
         this.dialog.close();
         this.dialog.destroy();
      }
      handle.dialog = JSROOT.sap.ui.xmlfragment("sap.ui.jsroot.view.Inspector", handle);

      // FIXME: global id is used, should find better solution later
      var view = sap.ui.getCore().byId("object_inspector");
      view.getController().setObject(this.GetObject());

      handle.dialog.open();
   }

   // ===================================================================================================

   JSROOT.TCanvasPainter.prototype.ShowGed = function(objpainter) {
      // function used to actiavte GED

      d3.select("#ged_placeholder").text("");

      var panelid = "CanvasGedId";

      var oModel = new JSROOT.sap.ui.model.json.JSONModel({
         handle: null
      });

      sap.ui.getCore().setModel(oModel, panelid);

      var ged = sap.ui.getCore().byId(panelid);

      if (!ged)
         ged = JSROOT.sap.ui.xmlview({
            id: panelid,
            viewName: "sap.ui.jsroot.view.Ged"
            // layoutData: oLd,
            // height: "100%"
         });

      ged.placeAt("ged_placeholder");

      // should be moved into Ged controller - it must be able to detect canvas painter itself
      this.RegisterForPadEvents(ged.getController().padEventsReceiver.bind(ged.getController()));

      this.SelectObjectPainter(objpainter);
   }

   JSROOT.TCanvasPainter.prototype.CleanupGed = function() {

      // dettach pad events receiver
      this.RegisterForPadEvents(null);

      sap.ui.getCore().byId("CanvasGedId").getController().cleanupGed();
   }

   JSROOT.TCanvasPainter.prototype.openuiHasGed = function() {
      var main = JSROOT.sap.ui.getCore().byId("TopCanvasId");
      return main ? main.getController().isGedEditor() : false;
   }

   JSROOT.TCanvasPainter.prototype.openuiActivateGed = function(painter, kind, mode) {
      // function used to actiavte GED in full canvas

      var main = JSROOT.sap.ui.getCore().byId("TopCanvasId");
      if (main) main.getController().showGeEditor(true);

      this.SelectObjectPainter(painter);

      if (typeof this.ProcessChanges == 'function')
         this.ProcessChanges("sbits", this);
   }

   JSROOT.TCanvasPainter.prototype.ActivateFitPanel = function(painter) {
      // function used to actiavte FitPanel

      if (!this.use_openui) return; // not supported in reduced mode
      var main = JSROOT.sap.ui.getCore().byId("TopCanvasId");
      if (main) main.getController().showLeftArea("FitPanel");
   }

   /*
   JSROOT.TCanvasPainter.prototype.SelectObjectPainter = function(objpainter) {
      var ged = null;
      if (this.use_openui) {
         var main = JSROOT.sap.ui.getCore().byId("TopCanvasId")
         ged = main.getController().getLeftController("Ged");
      } else {
         var main = JSROOT.sap.ui.getCore().byId("CanvasGedId");
         if (main) ged = main.getController();
      }
      if (ged) ged.onObjectSelect(this, objpainter);
   }

   JSROOT.TCanvasPainter.prototype.ProcessPadRedraw = function(padpainter) {
      var ged = null;
      if (this.use_openui) {
         var main = JSROOT.sap.ui.getCore().byId("TopCanvasId")
         ged = main.getController().getLeftController("Ged");
      } else {
         var main = JSROOT.sap.ui.getCore().byId("CanvasGedId");
         if (main) ged = main.getController();
      }
      if (ged) ged.onPadRedraw(this, padpainter);
   }
   */

   JSROOT.TCanvasPainter.prototype.openuiHasEventStatus = function() {
      var main = JSROOT.sap.ui.getCore().byId("TopCanvasId");
      return main ? main.getController().isStatusShown() : false;
   }

   JSROOT.TCanvasPainter.prototype.openuiToggleEventStatus = function() {
      var main = JSROOT.sap.ui.getCore().byId("TopCanvasId");
      if (main) main.getController().toggleShowStatus();
   }

   JSROOT.TCanvasPainter.prototype.fullShowStatus = function(lbl1,lbl2,lbl3,lbl4) {
      var main = JSROOT.sap.ui.getCore().byId("TopCanvasId");
      if (main) main.getController().ShowCanvasStatus(lbl1,lbl2,lbl3,lbl4);
   }

   JSROOT.TCanvasPainter.prototype.ShowUI5ProjectionArea = function(kind, call_back) {
      var main = JSROOT.sap.ui.getCore().byId("TopCanvasId");
      if (main) main.getController().showProjectionArea(kind, call_back);
   }

   JSROOT.TCanvasPainter.prototype.DrawInUI5ProjectionArea = function(obj, opt, call_back) {
      var main = JSROOT.sap.ui.getCore().byId("TopCanvasId");
      if (main) main.getController().drawInProjectionArea(obj, opt, call_back);
   }

   JSROOT.TCanvasPainter.prototype.ShowMessage = function(msg) {
      if (!this.use_openui)
         return JSROOT.progress(msg, 7000);
      var main = JSROOT.sap.ui.getCore().byId("TopCanvasId");
      if (main) main.getController().showMessage(msg);
   }

   JSROOT.TCanvasPainter.prototype.fullShowSection = function(that, on) {
      var main = JSROOT.sap.ui.getCore().byId("TopCanvasId");
      if (main) main.getController().showSection(that, on);
   }

   JSROOT.TCanvasPainter.prototype.MethodsDialog = function(painter, method, menu_obj_id) {

      var main = JSROOT.sap.ui.getCore().byId("TopCanvasId");
      if (!main) return;

      var pthis = this;

      method.fClassName = painter.GetClassName();
      // TODO: deliver class name together with menu items
      if ((menu_obj_id.indexOf("#x")>0) || (menu_obj_id.indexOf("#y")>0) || (menu_obj_id.indexOf("#z")>0)) method.fClassName = "TAxis";

      main.getController().showMethodsDialog(method, function(args) {

         if (painter.ExecuteMenuCommand(method, args)) return;

         var exec = method.fExec;
         if (args) exec = exec.substr(0,exec.length-1) + args + ')';

         // invoked only when user press Ok button
         console.log('execute method for object ' + menu_obj_id + ' exec= ' + exec);

         pthis.SendWebsocket('OBJEXEC:' + menu_obj_id + ":" + exec);
      });
   }

   // ====================================================================================

   if (JSROOT.v7 && JSROOT.v7.TCanvasPainter)

   JSROOT.v7.TCanvasPainter.prototype.ActivatePanel = function(name, handle, callback) {
      // function used to actiavte FitPanel

      var main = JSROOT.sap.ui.getCore().byId("TopCanvasId");
      if (!main) return JSROOT.CallBack(callback, false);
      main.getController().showPanelInLeftArea(name, handle, callback);
   }

   return JSROOT;

}));

