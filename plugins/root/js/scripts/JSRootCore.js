/// @file JSRootCore.js
/// Core methods of JavaScript ROOT

/// @namespace JSROOT
/// Holder of all JSROOT functions and classes

(function( factory ) {
   if ( typeof define === "function" && define.amd ) {

      var jsroot = factory({});


      var dir = jsroot.source_dir + "scripts/", ext = jsroot.source_min ? ".min" : "";

      var paths = {
            'd3'                   : dir+'d3.v3.min',
            'jquery'               : dir+'jquery.min',
            'jquery-ui'            : dir+'jquery-ui.min',
            'jqueryui-mousewheel'  : dir+'jquery.mousewheel'+ext,
            'jqueryui-touch-punch' : dir+'touch-punch.min',
            'rawinflate'           : dir+'rawinflate'+ext,
            'MathJax'              : 'https://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS-MML_SVG&amp;delayStartupUntil=configured',
            'saveSvgAsPng'         : dir+'saveSvgAsPng'+ext,
            'threejs'              : dir+'three'+ext,
            'threejs_all'          : dir+'three.extra'+ext,
//            'JSRootCore'           : dir+'JSRootCore'+ext,
            'JSRootMath'           : dir+'JSRootMath'+ext,
            'JSRootInterface'      : dir+'JSRootInterface'+ext,
            'JSRootIOEvolution'    : dir+'JSRootIOEvolution'+ext,
            'JSRootPainter'        : dir+'JSRootPainter'+ext,
            'JSRootPainter.more'   : dir+'JSRootPainter.more'+ext,
            'JSRootPainter.jquery' : dir+'JSRootPainter.jquery'+ext,
            'JSRoot3DPainter'      : dir+'JSRoot3DPainter'+ext,
            'ThreeCSG'             : dir+'ThreeCSG'+ext,
            'JSRootGeoPainter'     : dir+'JSRootGeoPainter'+ext
         };

      var cfg_paths;
      if ((requirejs.s!==undefined) && (requirejs.s.contexts !== undefined) && ((requirejs.s.contexts._!==undefined) &&
           requirejs.s.contexts._.config!==undefined)) cfg_paths = requirejs.s.contexts._.config.paths;
                                                 else console.warn("Require.js paths changed - please contact JSROOT developers");

      // check if modules are already loaded
      for (var module in paths)
         if (requirejs.defined(module) || (cfg_paths && (module in cfg_paths)))
            delete paths[module];

      // add mapping if script loaded as bower module 'jsroot'
//      if (cfg_paths  && ('jsroot' in cfg_paths))
//           requirejs.config({ map : { "*": { "JSRootCore": "jsroot" } } });


      // configure all dependencies
      requirejs.config({
        paths: paths,
        shim: {
         'jqueryui-mousewheel': { deps: ['jquery-ui'] },
         'jqueryui-touch-punch': { deps: ['jquery-ui'] },
         'threejs_all': { deps: [ 'threejs'] },
         'MathJax': {
             exports: 'MathJax',
             init: function () {
                MathJax.Hub.Config({ TeX: { extensions: ["color.js"] }});
                MathJax.Hub.Register.StartupHook("SVG Jax Ready",function () {
                   var VARIANT = MathJax.OutputJax.SVG.FONTDATA.VARIANT;
                   VARIANT["normal"].fonts.unshift("MathJax_SansSerif");
                   VARIANT["bold"].fonts.unshift("MathJax_SansSerif-bold");
                   VARIANT["italic"].fonts.unshift("MathJax_SansSerif");
                   VARIANT["-tex-mathit"].fonts.unshift("MathJax_SansSerif");
                });
                MathJax.Hub.Startup.onload();
                return MathJax;
             }
          }
       }
      });


      // AMD. Register as an anonymous module.
      define( jsroot );

      if (!require.specified("JSRootCore"))
          define('JSRootCore', [], jsroot);

      if (!require.specified("jsroot"))
         define('jsroot', [], jsroot);

   } else {

      if (typeof JSROOT != 'undefined')
         throw new Error("JSROOT is already defined", "JSRootCore.js");

      JSROOT = {};

      factory(JSROOT);
   }
} (function(JSROOT) {

   JSROOT.version = "dev 23/03/2016";

   JSROOT.source_dir = "";
   JSROOT.source_min = false;
   JSROOT.source_fullpath = ""; // full name of source script
   JSROOT.bower_dir = ""; // when specified, use standard libs from bower location

   JSROOT.id_counter = 0;

   JSROOT.touches = false;
   JSROOT.browser = { isOpera:false, isFirefox:true, isSafari:false, isChrome:false, isIE:false };

   if ((typeof document !== "undefined") && (typeof window !== "undefined")) {
      var scripts = document.getElementsByTagName('script');
      for (var n = 0; n < scripts.length; ++n) {
         var src = scripts[n].src;
         if ((src===undefined) || (typeof src !== 'string')) continue;

         var pos = src.indexOf("scripts/JSRootCore.");
         if (pos<0) continue;

         JSROOT.source_dir = src.substr(0, pos);
         JSROOT.source_min = src.indexOf("scripts/JSRootCore.min.js") >= 0;

         JSROOT.source_fullpath = src;

         if ((console!==undefined) && (typeof console.log == 'function'))
            console.log("Set JSROOT.source_dir to " + JSROOT.source_dir + ", " + JSROOT.version);
         break;
      }

      JSROOT.touches = ('ontouchend' in document); // identify if touch events are supported
      JSROOT.browser.isOpera = !!window.opera || navigator.userAgent.indexOf(' OPR/') >= 0;
      JSROOT.browser.isFirefox = typeof InstallTrigger !== 'undefined';
      JSROOT.browser.isSafari = Object.prototype.toString.call(window.HTMLElement).indexOf('Constructor') > 0;
      JSROOT.browser.isChrome = !!window.chrome && !JSROOT.browser.isOpera;
      JSROOT.browser.isIE = false || !!document.documentMode;
   }

   JSROOT.browser.isWebKit = JSROOT.browser.isChrome || JSROOT.browser.isSafari;

   // default draw styles, can be changed after loading of JSRootCore.js
   JSROOT.gStyle = {
         Tooltip : 2, // 0 - off, 1-default, 2-advanced (experimental)
         ContextMenu : true,
         Zooming : true,
         MoveResize : true,   // enable move and resize of elements like statbox, title, pave, colz
         DragAndDrop : true,  // enables drag and drop functionality
         ToolBar : true,    // show additional tool buttons on the canvas
         OptimizeDraw : 1, // drawing optimization: 0 - disabled, 1 - only for large (>5000 1d bins, >50 2d bins) histograms, 2 - always
         AutoStat : true,
         OptStat  : 1111,
         OptFit   : 0,
         FrameNDC : { fX1NDC: 0.07, fY1NDC: 0.12, fX2NDC: 0.95, fY2NDC: 0.88 },
         StatNDC  : { fX1NDC: 0.78, fY1NDC: 0.75, fX2NDC: 0.98, fY2NDC: 0.91 },
         StatText : { fTextAngle: 0, fTextSize: 9, fTextAlign: 12, fTextColor: 1, fTextFont: 42 },
         StatFill : { fFillColor: 0, fFillStyle: 1001 },
         TimeOffset : 788918400000, // UTC time at 01/01/95
         StatFormat : "6.4g",
         FitFormat : "5.4g",
         Palette : 57,
         MathJax : 0,  // 0 - never, 1 - only for complex cases, 2 - always
         ProgressBox : true,  // show progress box
         Embed3DinSVG : 2,  // 0 - no embed, 1 - overlay over SVG (IE), 2 - embed into SVG (works only with Firefox and Chrome)
         NoWebGL : false // if true, WebGL will be disabled
      };

   JSROOT.BIT = function(n) { return 1 << (n); }

   // TH1 status bits
   JSROOT.TH1StatusBits = {
         kNoStats       : JSROOT.BIT(9),  // don't draw stats box
         kUserContour   : JSROOT.BIT(10), // user specified contour levels
         kCanRebin      : JSROOT.BIT(11), // can rebin axis
         kLogX          : JSROOT.BIT(15), // X-axis in log scale
         kIsZoomed      : JSROOT.BIT(16), // bit set when zooming on Y axis
         kNoTitle       : JSROOT.BIT(17), // don't draw the histogram title
         kIsAverage     : JSROOT.BIT(18)  // Bin contents are average (used by Add)
   };

   JSROOT.EAxisBits = {
         kTickPlus      : JSROOT.BIT(9),
         kTickMinus     : JSROOT.BIT(10),
         kAxisRange     : JSROOT.BIT(11),
         kCenterTitle   : JSROOT.BIT(12),
         kCenterLabels  : JSROOT.BIT(14),
         kRotateTitle   : JSROOT.BIT(15),
         kPalette       : JSROOT.BIT(16),
         kNoExponent    : JSROOT.BIT(17),
         kLabelsHori    : JSROOT.BIT(18),
         kLabelsVert    : JSROOT.BIT(19),
         kLabelsDown    : JSROOT.BIT(20),
         kLabelsUp      : JSROOT.BIT(21),
         kIsInteger     : JSROOT.BIT(22),
         kMoreLogLabels : JSROOT.BIT(23),
         kDecimals      : JSROOT.BIT(11)
   };

   // wrapper for console.log, avoids missing console in IE
   // if divid specified, provide output to the HTML element
   JSROOT.console = function(value, divid) {
      if ((divid!=null) && (typeof divid=='string') && ((typeof document.getElementById(divid))!='undefined'))
         document.getElementById(divid).innerHTML = value;
      else
      if ((typeof console != 'undefined') && (typeof console.log == 'function'))
         console.log(value);
   }

   // This is part of the JSON-R code, found on
   // https://github.com/graniteds/jsonr
   // Only unref part was used, arrays are not accounted as objects
   // Should be used to reintroduce objects references, produced by TBufferJSON
   JSROOT.JSONR_unref = function(value, dy) {
      var c, i, k, ks;
      if (!dy) dy = [];

      switch (typeof value) {
      case 'string':
          if ((value.length > 5) && (value.substr(0, 5) == "$ref:")) {
             c = parseInt(value.substr(5));
             if (!isNaN(c) && (c < dy.length)) {
                value = dy[c];
             }
          }
          break;

      case 'object':
         if (value !== null) {

            if (Object.prototype.toString.apply(value) === '[object Array]') {
               for (i = 0; i < value.length; ++i) {
                  value[i] = this.JSONR_unref(value[i], dy);
               }
            } else {

               // account only objects in ref table
               if (dy.indexOf(value) === -1) {
                  dy.push(value);
               }

               // add methods to all objects, where _typename is specified
               if ('_typename' in value) this.addMethods(value);

               ks = Object.keys(value);
               for (i = 0; i < ks.length; ++i) {
                  k = ks[i];
                  value[k] = this.JSONR_unref(value[k], dy);
               }
            }
         }
         break;
      }

      return value;
   }

   JSROOT.debug = 0;

   // This is simple replacement of jQuery.extend method
   // Just copy (not clone) all fields from source to the target object
   JSROOT.extend = function(tgt, src, map, deep_copy) {
      if ((src === null) || (typeof src !== 'object')) return src;
      if ((tgt === null) || (typeof tgt !== 'object')) tgt = {};

      for (var k in src)
         tgt[k] = src[k];

      return tgt;
   }

   // Make deep clone of the object, including all sub-objects
   JSROOT.clone = function(src, map) {
      if (src === null) return null;

      if (!map) {
         map = { obj:[], clones:[] };
      } else {
         var i = map.obj.indexOf(src);
         if (i>=0) return map.clones[i];
      }

      var proto = Object.prototype.toString.apply(src);

      // process normal array
      if (proto === '[object Array]') {
         var tgt = [];
         map.obj.push(src);
         map.clones.push(tgt);
         for (var i = 0; i < src.length; ++i)
            tgt.push(JSROOT.clone(src[i], map));

         return tgt;
      }

      // process typed array
      if ((proto.indexOf('[object ') == 0) && (proto.indexOf('Array]') == proto.length-6)) {
         var tgt = [];
         map.obj.push(src);
         map.clones.push(tgt);
         for (var i = 0; i < src.length; ++i)
            tgt.push(src[i]);

         return tgt;
      }

      var tgt = {};
      map.obj.push(src);
      map.clones.push(tgt);

      for (var k in src) {
         if (typeof src[k] === 'object')
            tgt[k] = JSROOT.clone(src[k], map);
         else
            tgt[k] = src[k];
      }

      return tgt;
   }

   // method can be used to delete all functions from objects
   // only such objects can be cloned when transfer to Worker
   JSROOT.clear_func = function(src, map) {
      if (src === null) return;

      var proto = Object.prototype.toString.apply(src);

      if (proto === '[object Array]') {
         for (var n=0;n<src.length;n++)
            if (typeof src[n] === 'object')
               JSROOT.clear_func(src[n], map);
         return;
      }

      if ((proto.indexOf('[object ') == 0) && (proto.indexOf('Array]') == proto.length-6)) return;

      if (!map) map = [];
      var nomap = (map.length == 0);
      if ('__clean_func__' in src) return;

      map.push(src);
      src['__clean_func__'] = true;

      for (var k in src) {
         if (typeof src[k] === 'object')
            JSROOT.clear_func(src[k], map);
         else
         if (typeof src[k] === 'function') delete src[k];
      }

      if (nomap)
         for (var n=0;n<map.length;++n)
            delete map[n]['__clean_func__'];
   }


   JSROOT.parse = function(arg) {
      if ((arg==null) || (arg=="")) return null;
      var obj = JSON.parse(arg);
      if (obj!=null) obj = this.JSONR_unref(obj);
      return obj;
   }

   JSROOT.GetUrlOption = function(opt, url, dflt) {
      // analyzes document.URL and extracts options after '?' mark
      // following options supported ?opt1&opt2=3
      // In case of opt1 empty string will be returned, in case of opt2 '3'
      // If option not found, null is returned (or provided default value)

      if (arguments.length < 3) dflt = null;
      if ((opt==null) || (typeof opt != 'string') || (opt.length==0)) return dflt;

      if (!url) {
         if (typeof document === 'undefined') return dflt;
         url = document.URL;
      }

      var pos = url.indexOf("?");
      if (pos<0) return dflt;
      url = url.slice(pos+1);

      while (url.length>0) {

         if (url==opt) return "";

         pos = url.indexOf("&");
         if (pos < 0) pos = url.length;

         if (url.indexOf(opt) == 0) {
            if (url.charAt(opt.length)=="&") return "";

            // replace several symbols which are known to make a problem
            if (url.charAt(opt.length)=="=")
               return url.slice(opt.length+1, pos).replace(/%27/g, "'").replace(/%22/g, '"').replace(/%20/g, ' ').replace(/%3C/g, '<').replace(/%3E/g, '>').replace(/%5B/g, '[').replace(/%5D/g, ']');
         }

         url = url.slice(pos+1);
      }
      return dflt;
   }

   JSROOT.ParseAsArray = function(val) {
      // parse string value as array.
      // It could be just simple string:  "value"
      //  or array with or without string quotes:  [element], ['eleme1',elem2]

      var res = [];

      if (typeof val != 'string') return res;

      val = val.trim();
      if (val=="") return res;

      // return as array with single element
      if ((val.length<2) || (val[0]!='[') || (val[val.length-1]!=']')) {
         res.push(val); return res;
      }

      // try to parse ourself
      var arr = val.substr(1, val.length-2).split(","); // remove brackets

      for (var i = 0; i < arr.length; ++i) {
         var sub = arr[i].trim();
         if ((sub.length>1) && (sub[0]==sub[sub.length-1]) && ((sub[0]=='"') || (sub[0]=="'")))
            sub = sub.substr(1, sub.length-2);
         res.push(sub);
      }
      return res;
   }

   JSROOT.GetUrlOptionAsArray = function(opt, url) {
      // special handling of URL options to produce array
      // if normal option is specified ...?opt=abc, than array with single element will be created
      // one could specify normal JSON array ...?opt=['item1','item2']
      // but also one could skip quotes ...?opt=[item1,item2]
      // one could collect values from several options, specifying
      // options names via semicolon like opt='item;items'

      var res = [];

      while (opt.length>0) {
         var separ = opt.indexOf(";");
         var part = separ>0 ? opt.substr(0, separ) : opt;
         if (separ>0) opt = opt.substr(separ+1); else opt = "";

         var val = this.GetUrlOption(part, url, null);
         res = res.concat(JSROOT.ParseAsArray(val));
      }
      return res;
   }

   JSROOT.findFunction = function(name) {
      var func = window[name];
      if (typeof func == 'function') return func;
      var separ = name.lastIndexOf(".");
      if (separ<0) return null;
      var namespace = name.slice(0, separ);
      name = name.slice(separ+1);
      if (namespace=="JSROOT") func = this[name]; else
      if (namespace=="JSROOT.Painter") { if ('Painter' in this) func = this['Painter'][name]; } else
      if (window[namespace]) func = window[namespace][name];
      return (typeof func == 'function') ? func : null;
   }

   JSROOT.CallBack = function(func, arg1, arg2) {
      // generic method to invoke callback function
      // func either normal function or container like
      // { obj: object_pointer, func: name of method to call }
      // arg1, arg2 are optional arguments of the callback

      if (typeof func == 'string') func = JSROOT.findFunction(func);

      if (func == null) return;

      if (typeof func == 'function') return func(arg1,arg2);

      if (typeof func != 'object') return;

      if (('obj' in func) && ('func' in func) &&
         (typeof func.obj == 'object') && (typeof func.func == 'string') &&
         (typeof func.obj[func.func] == 'function')) {
         alert('Old-style call-back, change code for ' + func.func);
             return func.obj[func.func](arg1, arg2);
      }
   }

   JSROOT.NewHttpRequest = function(url, kind, user_call_back) {
      // Create asynchronous XMLHttpRequest object.
      // One should call req.send() to submit request
      // kind of the request can be:
      //  "bin" - abstract binary data, result as string (default)
      //  "buf" - abstract binary data, result as BufferArray (if supported)
      //  "text" - returns req.responseText
      //  "object" - returns JSROOT.parse(req.responseText)
      //  "xml" - returns res.responseXML
      //  "head" - returns request itself, uses "HEAD" method
      // Result will be returned to the callback functions
      // Request will be set as this pointer in the callback
      // If failed, request returns null

      var xhr = new XMLHttpRequest();

      function callback(res) {
         // we set pointer on request when calling callback
         if (typeof user_call_back == 'function') user_call_back.call(xhr, res);
      }

      var pthis = this;

      if (window.ActiveXObject) {

         xhr.onreadystatechange = function() {
            if (xhr.readyState != 4) return;

            if (xhr.status != 200 && xhr.status != 206) {
               // error
               return callback(null);
            }

            if (kind == "xml") return callback(xhr.responseXML);
            if (kind == "text") return callback(xhr.responseText);
            if (kind == "object") return callback(pthis.parse(xhr.responseText));
            if (kind == "head") return callback(xhr);

            if ((kind == "buf") && ('responseType' in xhr) &&
                (xhr.responseType == 'arraybuffer') && ('response' in xhr))
               return callback(xhr.response);

            var filecontent = new String("");
            var array = new VBArray(xhr.responseBody).toArray();
            for (var i = 0; i < array.length; ++i)
               filecontent = filecontent + String.fromCharCode(array[i]);
            delete array;
            callback(filecontent);
         }

         xhr.open(kind == 'head' ? 'HEAD' : 'GET', url, true);

         if (kind=="buf") {
            if (('Uint8Array' in window) && ('responseType' in xhr))
              xhr.responseType = 'arraybuffer';
         }

      } else {

         xhr.onreadystatechange = function() {
            if (xhr.readyState != 4) return;

            if (xhr.status != 200 && xhr.status != 206) {
               return callback(null);
            }

            if (kind == "xml") return callback(xhr.responseXML);
            if (kind == "text") return callback(xhr.responseText);
            if (kind == "object") return callback(pthis.parse(xhr.responseText));
            if (kind == "head") return callback(xhr);

            // if no response type is supported, return as text (most probably, will fail)
            if (! ('responseType' in xhr))
               return callback(xhr.responseText);

            if ((kind=="bin") && ('Uint8Array' in window) && ('byteLength' in xhr.response)) {
               // if string representation in requested - provide it
               var filecontent = "";
               var u8Arr = new Uint8Array(xhr.response, 0, xhr.response.byteLength);
               for (var i = 0; i < u8Arr.length; ++i)
                  filecontent = filecontent + String.fromCharCode(u8Arr[i]);
               delete u8Arr;

               return callback(filecontent);
            }

            callback(xhr.response);
         }

         xhr.open(kind == 'head' ? 'HEAD' : 'GET', url, true);

         if ((kind == "bin") || (kind == "buf")) {
            if (('Uint8Array' in window) && ('responseType' in xhr)) {
               xhr.responseType = 'arraybuffer';
            } else {
               //XHR binary charset opt by Marcus Granado 2006 [http://mgran.blogspot.com]
               xhr.overrideMimeType("text/plain; charset=x-user-defined");
            }
         }

      }
      return xhr;
   }

   JSROOT.loadScript = function(urllist, callback, debugout) {
      // dynamic script loader using callback
      // (as loading scripts may be asynchronous)
      // one could specify list of scripts or style files, separated by semicolon ';'
      // one can prepend file name with '$$$' - than file will be loaded from JSROOT location
      // This location can be set by JSROOT.source_dir or it will be detected automatically
      // by the position of JSRootCore.js file, which must be loaded by normal methods:
      // <script type="text/javascript" src="scripts/JSRootCore.js"></script>

      function completeLoad() {
         if (debugout)
            document.getElementById(debugout).innerHTML = "";
         else
            JSROOT.progress();

         if ((urllist!=null) && (urllist.length>0))
            return JSROOT.loadScript(urllist, callback, debugout);

         JSROOT.CallBack(callback);
      }

      if ((urllist==null) || (urllist.length==0))
         return completeLoad();

      var filename = urllist;
      var separ = filename.indexOf(";");
      if (separ>0) {
         filename = filename.substr(0, separ);
         urllist = urllist.slice(separ+1);
      } else {
         urllist = "";
      }

      var isrootjs = false, isbower = false;
      if (filename.indexOf("$$$")===0) {
         isrootjs = true;
         filename = filename.slice(3);
         if ((filename.indexOf("style/")==0) && JSROOT.source_min &&
             (filename.lastIndexOf('.css')==filename.length-3) &&
             (filename.indexOf('.min.css')<0))
            filename = filename.slice(0, filename.length-4) + '.min.css';
      } else
      if (filename.indexOf("###")===0) {
         isbower = true;
         filename = filename.slice(3);
      }
      var isstyle = filename.indexOf('.css') > 0;

      if (isstyle) {
         var styles = document.getElementsByTagName('link');
         for (var n = 0; n < styles.length; ++n) {
            if ((styles[n].type != 'text/css') || (styles[n].rel !== 'stylesheet')) continue;

            var href = styles[n].href;
            if ((href == null) || (href.length == 0)) continue;

            if (href.indexOf(filename)>=0) return completeLoad();
         }

      } else {
         var scripts = document.getElementsByTagName('script');

         for (var n = 0; n < scripts.length; ++n) {
            // if (scripts[n].type != 'text/javascript') continue;

            var src = scripts[n].src;
            if ((src == null) || (src.length == 0)) continue;

            if ((src.indexOf(filename)>=0) && (src.indexOf("load=")<0)) {
               // avoid wrong decision when script name is specified as more argument
               return completeLoad();
            }
         }
      }

      if (isrootjs && (JSROOT.source_dir!=null)) filename = JSROOT.source_dir + filename; else
      if (isbower && (JSROOT.bower_dir.length>0)) filename = JSROOT.bower_dir + filename;

      var element = null;

      if (debugout)
         document.getElementById(debugout).innerHTML = "loading " + filename + " ...";
      else
         JSROOT.progress("loading " + filename + " ...");

      if (isstyle) {
         element = document.createElement("link");
         element.setAttribute("rel", "stylesheet");
         element.setAttribute("type", "text/css");
         element.setAttribute("href", filename);
      } else {
         element = document.createElement("script");
         element.setAttribute('type', "text/javascript");
         element.setAttribute('src', filename);
      }

      if (element.readyState) { // Internet Explorer specific
         element.onreadystatechange = function() {
            if (element.readyState == "loaded" || element.readyState == "complete") {
               element.onreadystatechange = null;
               completeLoad();
            }
         }
      } else { // Other browsers
         element.onload = function() {
            element.onload = null;
            completeLoad();
         }
      }

      document.getElementsByTagName("head")[0].appendChild(element);
   }

   JSROOT.AssertPrerequisites = function(kind, callback, debugout) {
      // one could specify kind of requirements
      // 'io' for I/O functionality (default)
      // '2d' for 2d graphic
      // 'jq' jQuery and jQuery-ui
      // 'jq2d' jQuery-dependend part of 2d graphic
      // '3d' for 3d graphic
      // 'simple' for basic user interface
      // 'load:' list of user-specific scripts at the end of kind string

      var jsroot = JSROOT;

      if (!('doing_assert' in jsroot)) jsroot.doing_assert = [];


      if ((typeof kind != 'string') || (kind == ''))
         return jsroot.CallBack(callback);

      if (kind=='__next__') {
         if (jsroot.doing_assert.length==0) return;
         var req = jsroot.doing_assert[0];
         if ('running' in req) return;
         kind = req._kind;
         callback = req._callback;
         debugout = req._debug;
      } else {
         jsroot.doing_assert.push({_kind:kind, _callback:callback, _debug: debugout});
         if (jsroot.doing_assert.length > 1) return;
      }

      jsroot.doing_assert[0]['running'] = true;

      if (kind.charAt(kind.length-1)!=";") kind+=";";

      var ext = jsroot.source_min ? ".min" : "";

      var need_jquery = false, use_bower = (JSROOT.bower_dir.length>0);

      // file names should be separated with ';'
      var mainfiles = "", extrafiles = ""; // scripts for direct loadin
      var modules = [];  // modules used for require.js

      if (kind.indexOf('io;')>=0) {
         mainfiles += "$$$scripts/rawinflate" + ext + ".js;" +
                      "$$$scripts/JSRootIOEvolution" + ext + ".js;";
         modules.push('JSRootIOEvolution');
      }

      if (kind.indexOf('2d;')>=0) {
         if (!('_test_d3_' in jsroot)) {
            if (typeof d3 != 'undefined') {
               jsroot.console('Reuse existing d3.js ' + d3.version + ", required 3.4.10", debugout);
               jsroot['_test_d3_'] = 1;
            } else {
               mainfiles += use_bower ? '###d3/d3.min.js;' : '$$$scripts/d3.v3.min.js;';
               jsroot['_test_d3_'] = 2;
            }
         }
         modules.push('JSRootPainter');
         mainfiles += '$$$scripts/JSRootPainter' + ext + ".js;";
         extrafiles += '$$$style/JSRootPainter' + ext + '.css;';
      }

      if (kind.indexOf('savepng;')>=0) {
         modules.push('saveSvgAsPng');
         mainfiles += '$$$scripts/saveSvgAsPng' + ext + ".js;";
      }

      if (kind.indexOf('jq;')>=0) need_jquery = true;

      if (kind.indexOf('math;')>=0)  {
         mainfiles += '$$$scripts/JSRootMath' + ext + ".js;";
         modules.push('JSRootMath');
      }

      if (kind.indexOf('more2d;')>=0) {
         mainfiles += '$$$scripts/JSRootPainter.more' + ext + ".js;";
         modules.push('JSRootPainter.more');
      }

      if (kind.indexOf('jq2d;')>=0) {
         mainfiles += '$$$scripts/JSRootPainter.jquery' + ext + ".js;";
         modules.push('JSRootPainter.jquery');
         need_jquery = true;
      }

      if ((kind.indexOf("3d;")>=0) || (kind.indexOf("geom;")>=0)) {
         if (use_bower)
           mainfiles += "###threejs/build/three.min.js;" +
                        "###threejs/examples/js/utils/FontUtils.js;" +
                        "###threejs/examples/js/renderers/Projector.js;" +
                        "###threejs/examples/js/renderers/CanvasRenderer.js;" +
                        "###threejs/examples/js/geometries/TextGeometry.js;" +
                        "###threejs/examples/js/controls/OrbitControls.js;" +
                        "###threejs/examples/js/controls/TransformControls.js;" +
                        "###threejs/examples/fonts/helvetiker_regular.typeface.js";
         else
            mainfiles += "$$$scripts/three" + ext + ".js;" +
                         "$$$scripts/three.extra" + ext + ".js;";
         modules.push("threejs_all");
         mainfiles += "$$$scripts/JSRoot3DPainter" + ext + ".js;";
         modules.push('JSRoot3DPainter');
      }

      if (kind.indexOf("geom;")>=0) {
         mainfiles += "$$$scripts/ThreeCSG" + ext + ".js;" +
                      "$$$scripts/JSRootGeoPainter" + ext + ".js;";
         extrafiles += "$$$style/JSRootGeoPainter" + ext + ".css;";
         modules.push('ThreeCSG', 'JSRootGeoPainter');
      }

      if (kind.indexOf("mathjax;")>=0) {
         if (typeof MathJax == 'undefined') {
            mainfiles += (use_bower ? "###MathJax/MathJax.js" : "https://cdn.mathjax.org/mathjax/latest/MathJax.js") +
                         "?config=TeX-AMS-MML_SVG," + jsroot.source_dir + "scripts/mathjax_config.js;";
         }
         if (JSROOT.gStyle.MathJax == 0) JSROOT.gStyle.MathJax = 1;
         modules.push('MathJax');
      }

      if (kind.indexOf("simple;")>=0) {
         need_jquery = true;
         mainfiles += '$$$scripts/JSRootInterface' + ext + ".js;";
         extrafiles += '$$$style/JSRootInterface' + ext + '.css;';
         modules.push('JSRootInterface');
      }

      if (need_jquery && (JSROOT.load_jquery==null)) {
         var has_jq = (typeof jQuery != 'undefined'), lst_jq = "";

         if (has_jq)
            jsroot.console('Reuse existing jQuery ' + jQuery.fn.jquery + ", required 2.1.4", debugout);
         else
            lst_jq += (use_bower ? "###jquery/dist" : "$$$scripts") + "/jquery.min.js;";
         if (has_jq && typeof $.ui != 'undefined')
            jsroot.console('Reuse existing jQuery-ui ' + $.ui.version + ", required 1.11.4", debugout);
         else {
            lst_jq += (use_bower ? "###jquery-ui" : "$$$scripts") + '/jquery-ui.min.js;';
            extrafiles += '$$$style/jquery-ui' + ext + '.css;';
         }

         if (JSROOT.touches) {
            lst_jq += use_bower ? '###jqueryui-touch-punch/jquery.ui.touch-punch.min.js;' : '$$$scripts/touch-punch.min.js;';
            modules.push('jqueryui-touch-punch');
         }

         modules.splice(0,0, 'jquery', 'jquery-ui', 'jqueryui-mousewheel');
         mainfiles = lst_jq + mainfiles;

         jsroot.load_jquery = true;
      }

      var pos = kind.indexOf("user:");
      if (pos<0) pos = kind.indexOf("load:");
      if (pos>=0) extrafiles += kind.slice(pos+5);

      function load_callback() {
         var req = jsroot.doing_assert.shift();
         jsroot.CallBack(req._callback);
         jsroot.AssertPrerequisites('__next__');
      }

      if ((typeof define === "function") && define.amd && (modules.length>0)) {
         jsroot.console("loading " + JSON.stringify(modules) + " with require.js", debugout);
         require(modules, function() {
            jsroot.loadScript(extrafiles, load_callback, debugout);
         });
      } else {
         jsroot.loadScript(mainfiles + extrafiles, load_callback, debugout);
      }
   }

   // function can be used to open ROOT file, I/O functionality will be loaded when missing
   JSROOT.OpenFile = function(filename, callback) {
      JSROOT.AssertPrerequisites("io", function() {
         new JSROOT.TFile(filename, callback);
      });
   }

   // function can be used to draw supported ROOT classes,
   // required functionality will be loaded automatically
   // if painter pointer required, one should load '2d' functionlity itself
   JSROOT.draw = function(divid, obj, opt) {
      JSROOT.AssertPrerequisites("2d", function() {
         JSROOT.draw(divid, obj, opt);
      });
   }

   JSROOT.redraw = function(divid, obj, opt) {
      JSROOT.AssertPrerequisites("2d", function() {
         JSROOT.redraw(divid, obj, opt);
      });
   }

   JSROOT.BuildSimpleGUI = function(user_scripts, andThen) {
      if (typeof user_scripts == 'function') {
         andThen = user_scripts;
         user_scripts = null;
      }

      var debugout = null;
      var nobrowser = JSROOT.GetUrlOption('nobrowser')!=null;
      var requirements = "io;2d;";

      if (document.getElementById('simpleGUI')) {
         debugout = 'simpleGUI';
         if ((JSROOT.GetUrlOption('json')!=null) &&
             (JSROOT.GetUrlOption('file')==null) &&
             (JSROOT.GetUrlOption('files')==null)) requirements = "2d;";
      } else
      if (document.getElementById('onlineGUI')) { debugout = 'onlineGUI'; requirements = "2d;"; } else
      if (document.getElementById('drawGUI')) { debugout = 'drawGUI'; requirements = "2d;"; nobrowser = true; }

      if (user_scripts == 'check_existing_elements') {
         user_scripts = null;
         if (debugout == null) return;
      }

      if (!nobrowser) requirements += 'jq2d;simple;';

      if (user_scripts == null) user_scripts = JSROOT.GetUrlOption("autoload");
      if (user_scripts == null) user_scripts = JSROOT.GetUrlOption("load");

      if (user_scripts != null)
         requirements += "load:" + user_scripts + ";";

      JSROOT.AssertPrerequisites(requirements, function() {
         JSROOT.CallBack(JSROOT.findFunction(nobrowser ? 'JSROOT.BuildNobrowserGUI' : 'BuildSimpleGUI'));
         JSROOT.CallBack(andThen);
      }, debugout);
   };

   JSROOT.Create = function(typename, target) {
      var obj = target;
      if (obj == null) obj = { _typename: typename };

      switch (typename) {
         case 'TObject':
             JSROOT.extend(obj, { fUniqueID: 0, fBits: 0x3000008 });
             break;
         case 'TNamed':
            JSROOT.extend(obj, { fUniqueID: 0, fBits: 0x3000008, fName: "", fTitle: "" });
            break;
         case 'TList':
         case 'THashList':
            JSROOT.extend(obj, { name: typename, arr : [], opt : [] });
            break;
         case 'TAttAxis':
            JSROOT.extend(obj, { fNdivisions: 510, fAxisColor: 1,
                                 fLabelColor: 1, fLabelFont: 42, fLabelOffset: 0.005, fLabelSize: 0.035, fTickLength: 0.03,
                                 fTitleOffset: 1, fTitleSize: 0.035, fTitleColor: 1, fTitleFont : 42 });
            break;
         case 'TAxis':
            JSROOT.Create("TNamed", obj);
            JSROOT.Create("TAttAxis", obj);
            JSROOT.extend(obj, { fNbins: 0, fXmin: 0, fXmax: 0, fXbins : [], fFirst: 0, fLast: 0,
                                 fBits2: 0, fTimeDisplay: false, fTimeFormat: "", fLabels: null });
            break;
         case 'TAttLine':
            JSROOT.extend(obj, { fLineColor: 1, fLineStyle : 1, fLineWidth : 1 });
            break;
         case 'TAttFill':
            JSROOT.extend(obj, { fFillColor: 0, fFillStyle : 0 } );
            break;
         case 'TAttMarker':
            JSROOT.extend(obj, { fMarkerColor: 1, fMarkerStyle : 1, fMarkerSize : 1. });
            break;
         case 'TLine':
            JSROOT.Create("TObject", obj);
            JSROOT.Create("TAttLine", obj);
            JSROOT.extend(obj, { fX1: 0, fX2: 1, fY1: 0, fY2: 1 });
            break;
         case 'TBox':
            JSROOT.Create("TObject", obj);
            JSROOT.Create("TAttLine", obj);
            JSROOT.Create("TAttFill", obj);
            JSROOT.extend(obj, { fX1: 0, fX2: 1, fY1: 0, fY2: 1 });
            break;
         case 'TPave':
            JSROOT.Create("TBox", obj);
            JSROOT.extend(obj, { fX1NDC : 0., fY1NDC: 0, fX2NDC: 1, fY2NDC: 1,
                                 fBorderSize: 0, fInit: 1, fShadowColor: 1,
                                 fCornerRadius: 0, fOption: "blNDC", fName: "title" });
            break;
         case 'TAttText':
            JSROOT.extend(obj, { fTextAngle: 0, fTextSize: 0, fTextAlign: 22, fTextColor: 1, fTextFont: 42});
            break;
         case 'TPaveText':
            JSROOT.Create("TPave", obj);
            JSROOT.Create("TAttText", obj);
            JSROOT.extend(obj, { fLabel: "", fLongest: 27, fMargin: 0.05, fLines: JSROOT.Create("TList") });
            break;
         case 'TPaveStats':
            JSROOT.Create("TPaveText", obj);
            JSROOT.extend(obj, { fOptFit: 0, fOptStat: 0, fFitFormat: "", fStatFormat: "", fParent: null });
            break;
         case 'TObjString':
            JSROOT.Create("TObject", obj);
            JSROOT.extend(obj, { fString: "" });
            break;
         case 'TH1':
            JSROOT.Create("TNamed", obj);
            JSROOT.Create("TAttLine", obj);
            JSROOT.Create("TAttFill", obj);
            JSROOT.Create("TAttMarker", obj);

            JSROOT.extend(obj, {
               fNcells : 0,
               fXaxis: JSROOT.Create("TAxis"),
               fYaxis: JSROOT.Create("TAxis"),
               fZaxis: JSROOT.Create("TAxis"),
               fBarOffset: 0, fBarWidth: 1000, fEntries: 0.,
               fTsumw: 0., fTsumw2: 0., fTsumwx: 0., fTsumwx2: 0.,
               fMaximum: -1111., fMinimum: -1111, fNormFactor: 0., fContour: [],
               fSumw2: [], fOption: "",
               fFunctions: JSROOT.Create("TList"),
               fBufferSize: 0, fBuffer: [], fBinStatErrOpt: 0 });
            break;
         case 'TH1I':
         case 'TH1F':
         case 'TH1D':
         case 'TH1S':
         case 'TH1C':
            JSROOT.Create("TH1", obj);
            obj.fArray = [];
            break;
         case 'TH2':
            JSROOT.Create("TH1", obj);
            JSROOT.extend(obj, { fScalefactor: 1., fTsumwy: 0.,  fTsumwy2: 0, fTsumwxy : 0});
            break;
         case 'TH2I':
         case 'TH2F':
         case 'TH2D':
         case 'TH2S':
         case 'TH2C':
            JSROOT.Create("TH2", obj);
            obj.fArray = [];
            break;
         case 'TGraph':
            JSROOT.Create("TNamed", obj);
            JSROOT.Create("TAttLine", obj);
            JSROOT.Create("TAttFill", obj);
            JSROOT.Create("TAttMarker", obj);
            JSROOT.extend(obj, { fFunctions: JSROOT.Create("TList"), fHistogram: null,
                                 fMaxSize: 0, fMaximum:-1111, fMinimum:-1111, fNpoints: 0, fX: [], fY: [] });
            break;
         case 'TMultiGraph':
            JSROOT.Create("TNamed", obj);
            JSROOT.extend(obj, { fFunctions: JSROOT.Create("TList"), fGraphs: JSROOT.Create("TList"),
                                 fHistogram: null, fMaximum: -1111, fMinimum: -1111 });
            break;
         case 'TGaxis':
            JSROOT.Create("TLine", obj);
            JSROOT.Create("TAttText", obj);
            JSROOT.extend(obj, { _fChopt: "", fFunctionName: "", fGridLength: 0,
                                  fLabelColor: 1, fLabelFont: 42, fLabelOffset: 0.005, fLabelSize: 0.035,
                                  fName: "", fNdiv: 12, fTickSize: 0.02, fTimeFormat: "",
                                  fTitle: "", fTitleOffset: 1, fTitleSize: 0.035,
                                  fWmax: 100, fWmin: 0 });
            break;

      }

      obj._typename = typename;
      this.addMethods(obj);
      return obj;
   }

   // obsolete functions, can be removed by next JSROOT release
   JSROOT.CreateTList = function() { return JSROOT.Create("TList"); }
   JSROOT.CreateTAxis = function() { return JSROOT.Create("TAxis"); }

   JSROOT.CreateTH1 = function(nbinsx) {

      var histo = JSROOT.extend(JSROOT.Create("TH1I"),
                   { fName: "dummy_histo_" + this.id_counter++, fTitle: "dummytitle" });

      if (nbinsx!==undefined) {
         histo.fNcells = nbinsx+2;
         for (var i=0;i<histo.fNcells;++i) histo.fArray.push(0);
         JSROOT.extend(histo.fXaxis, { fNbins: nbinsx, fXmin: 0,  fXmax: nbinsx });
      }
      return histo;
   }

   JSROOT.CreateTH2 = function(nbinsx, nbinsy) {

      var histo = JSROOT.extend(JSROOT.Create("TH2I"),
                    { fName: "dummy_histo_" + this.id_counter++, fTitle: "dummytitle" });

      if ((nbinsx!==undefined) && (nbinsy!==undefined)) {
         histo.fNcells = (nbinsx+2) * (nbinsy+2);
         for (var i=0;i<histo.fNcells;++i) histo.fArray.push(0);
         JSROOT.extend(histo.fXaxis, { fNbins: nbinsx, fXmin: 0, fXmax: nbinsx });
         JSROOT.extend(histo.fYaxis, { fNbins: nbinsy, fXmin: 0, fXmax: nbinsy });
      }
      return histo;
   }

   JSROOT.CreateTGraph = function(npoints, xpts, ypts) {
      var graph = JSROOT.extend(JSROOT.Create("TGraph"),
              { fBits: 0x3000408, fName: "dummy_graph_" + this.id_counter++, fTitle: "dummytitle" });

      if (npoints>0) {
         graph.fMaxSize = graph.fNpoints = npoints;

         var usex = (typeof xpts == 'object') && (xpts.length === npoints);
         var usey = (typeof ypts == 'object') && (ypts.length === npoints);

         for (var i=0;i<npoints;++i) {
            graph.fX.push(usex ? xpts[i] : i/npoints);
            graph.fY.push(usey ? ypts[i] : i/npoints);
         }
      }

      return graph;
   }

   JSROOT.CreateTMultiGraph = function() {
      var mgraph = JSROOT.Create("TMultiGraph");
      for(var i=0; i<arguments.length; ++i)
          mgraph.fGraphs.Add(arguments[i], "");
      return mgraph;
   }

   JSROOT.methodsCache = {}; // variable used to keep methods for known classes

   JSROOT.getMethods = function(typename, obj) {
      var m = JSROOT.methodsCache[typename];

      if (m !== undefined) return m;

      m = {};

      if ((typename=="TObject") || (typename=="TNamed") || ((obj!==undefined) && ('fBits' in obj))) {
         m.TestBit = function (f) { return (this.fBits & f) != 0; };
         m.InvertBit = function (f) { this.fBits = this.fBits ^ (f & 0xffffff); };
      }

      if ((typename === 'TList') || (typename === 'THashList')) {
         m.Clear = function() {
            this.arr = [];
            this.opt = [];
         }
         m.Add = function(obj,opt) {
            this.arr.push(obj);
            this.opt.push((opt && typeof opt=='string') ? opt : "");
         }
         m.AddFirst = function(obj,opt) {
            this.arr.unshift(obj);
            this.opt.unshift((opt && typeof opt=='string') ? opt : "");
         }
         m.RemoveAt = function(indx) {
            this.arr.splice(indx, 1);
            this.opt.splice(indx, 1);
         }
      }

      if ((typename === "TPaveText") || (typename === "TPaveStats")) {
         m.AddText = function(txt) {
            this.fLines.Add({ fTitle: txt, fTextColor: 1 });
         }
         m.Clear = function() {
            this.fLines.Clear();
         }
      }

      if (typename.indexOf("TF1") == 0) {
         m.addFormula = function(obj) {
            if (obj==null) return;
            if (!('formulas' in this)) this.formulas = [];
            this.formulas.push(obj);
         }

         m.evalPar = function(x) {
            if (! ('_func' in this) || (this._title !== this.fTitle)) {

              var _func = this.fTitle;

              if ('formulas' in this)
                 for (var i=0;i<this.formulas.length;++i)
                    while (_func.indexOf(this.formulas[i].fName) >= 0)
                       _func = _func.replace(this.formulas[i].fName, this.formulas[i].fTitle);
              _func = _func.replace(/\b(abs)\b/g, 'TMath::Abs');
              _func = _func.replace('TMath::Exp(', 'Math.exp(');
              _func = _func.replace('TMath::Abs(', 'Math.abs(');
              if (typeof JSROOT.Math == 'object') {
                 this._math = JSROOT.Math;
                 _func = _func.replace('TMath::Prob(', 'this._math.Prob(');
                 _func = _func.replace('gaus(', 'this._math.gaus(this, x, ');
                 _func = _func.replace('gausn(', 'this._math.gausn(this, x, ');
                 _func = _func.replace('expo(', 'this._math.expo(this, x, ');
                 _func = _func.replace('landau(', 'this._math.landau(this, x, ');
                 _func = _func.replace('landaun(', 'this._math.landaun(this, x, ');
              }
              _func = _func.replace('pi', 'Math.PI');
              for (var i=0;i<this.fNpar;++i)
                 while(_func.indexOf('['+i+']') != -1)
                    _func = _func.replace('['+i+']', this.GetParValue(i));
              _func = _func.replace(/\b(sin)\b/gi, 'Math.sin');
              _func = _func.replace(/\b(cos)\b/gi, 'Math.cos');
              _func = _func.replace(/\b(tan)\b/gi, 'Math.tan');
              _func = _func.replace(/\b(exp)\b/gi, 'Math.exp');

               this._func = new Function("x", "return " + _func).bind(this);
               this._title = this.fTitle;
            }

            return this._func(x);
         }
         m.GetParName = function(n) {
            if (('fFormula' in this) && ('fParams' in this.fFormula)) return this.fFormula.fParams[n].first;
            if ('fNames' in this) return this.fNames[n];
            return "Par"+n;
         }
         m.GetParValue = function(n) {
            if (('fFormula' in this) && ('fClingParameters' in this.fFormula)) return this.fFormula.fClingParameters[n];
            if (('fParams' in this) && (this.fParams!=null))  return this.fParams[n];
            return null;
         }
      }

      if ((typename.indexOf("TGraph") == 0) || (typename == "TCutG")) {
         // check if point inside figure specified by the TGrpah
         m.IsInside = function(xp,yp) {
            var j = this.fNpoints - 1, x = this.fX, y = this.fY;
            var oddNodes = false;

            for (var i=0; i<this.fNpoints; ++i) {
               if ((y[i]<yp && y[j]>=yp) || (y[j]<yp && y[i]>=yp)) {
                  if (x[i]+(yp-y[i])/(y[j]-y[i])*(x[j]-x[i])<xp) {
                     oddNodes = !oddNodes;
                  }
               }
               j=i;
            }

            return oddNodes;
         };
      }

      if (typename.indexOf("TH1") == 0 ||
          typename.indexOf("TH2") == 0 ||
          typename.indexOf("TH3") == 0) {
         m.getBinError = function(bin) {
            //   -*-*-*-*-*Return value of error associated to bin number bin*-*-*-*-*
            //    if the sum of squares of weights has been defined (via Sumw2),
            //    this function returns the sqrt(sum of w2).
            //    otherwise it returns the sqrt(contents) for this bin.
            if (bin >= this.fNcells) bin = this.fNcells - 1;
            if (bin < 0) bin = 0;
            if (bin < this.fSumw2.length)
               return Math.sqrt(this.fSumw2[bin]);
            return Math.sqrt(Math.abs(this.fArray[bin]));
         };
         m.setBinContent = function(bin, content) {
            // Set bin content - only trival case, without expansion
            this.fEntries++;
            this.fTsumw = 0;
            if ((bin>=0) && (bin<this.fArray.length))
               this.fArray[bin] = content;
         };
      }

      if (typename.indexOf("TH1") == 0) {
         m.getBin = function(x) { return x; }
         m.getBinContent = function(bin) { return this.fArray[bin]; }
      }

      if (typename.indexOf("TH2") == 0) {
         m.getBin = function(x, y) { return (x + (this.fXaxis.fNbins+2) * y); }
         m.getBinContent = function(x, y) { return this.fArray[this.getBin(x, y)]; }
      }

      if (typename.indexOf("TH3") == 0) {
         m.getBin = function(x, y, z) { return (x + (this.fXaxis.fNbins+2) * (y + (this.fYaxis.fNbins+2) * z)); }
         m.getBinContent = function(x, y, z) { return this.fArray[this.getBin(x, y, z)]; };
      }

      if (typename.indexOf("TProfile") == 0) {
         m.getBin = function(x) { return x; }
         m.getBinContent = function(bin) {
            if (bin < 0 || bin >= this.fNcells) return 0;
            if (this.fBinEntries[bin] < 1e-300) return 0;
            if (!this.fArray) return 0;
            return this.fArray[bin]/this.fBinEntries[bin];
         };
         m.getBinEffectiveEntries = function(bin) {
            if (bin < 0 || bin >= this.fNcells) return 0;
            var sumOfWeights = this.fBinEntries[bin];
            if ( this.fBinSumw2 == null || this.fBinSumw2.length != this.fNcells) {
               // this can happen  when reading an old file
               return sumOfWeights;
            }
            var sumOfWeightsSquare = this.fSumw2[bin];
            return ( sumOfWeightsSquare > 0 ? sumOfWeights * sumOfWeights / sumOfWeightsSquare : 0 );
         };
         m.getBinError = function(bin) {
            if (bin < 0 || bin >= this.fNcells) return 0;
            var cont = this.fArray[bin],               // sum of bin w *y
                sum  = this.fBinEntries[bin],          // sum of bin weights
                err2 = this.fSumw2[bin],               // sum of bin w * y^2
                neff = this.getBinEffectiveEntries(bin);  // (sum of w)^2 / (sum of w^2)
            if (sum < 1e-300) return 0;                  // for empty bins
            var EErrorType = { kERRORMEAN : 0, kERRORSPREAD : 1, kERRORSPREADI : 2, kERRORSPREADG : 3 };
            // case the values y are gaussian distributed y +/- sigma and w = 1/sigma^2
            if (this.fErrorMode === EErrorType.kERRORSPREADG)
               return 1.0/Math.sqrt(sum);
            // compute variance in y (eprim2) and standard deviation in y (eprim)
            var contsum = cont/sum;
            var eprim2  = Math.abs(err2/sum - contsum*contsum);
            var eprim   = Math.sqrt(eprim2);
            if (this.fErrorMode === EErrorType.kERRORSPREADI) {
               if (eprim != 0) return eprim/Math.sqrt(neff);
               // in case content y is an integer (so each my has an error +/- 1/sqrt(12)
               // when the std(y) is zero
               return 1.0/Math.sqrt(12*neff);
            }
            // if approximate compute the sums (of w, wy and wy2) using all the bins
            //  when the variance in y is zero
            // case option "S" return standard deviation in y
            if (this.fErrorMode === EErrorType.kERRORSPREAD) return eprim;
            // default case : fErrorMode = kERRORMEAN
            // return standard error on the mean of y
            return (eprim/Math.sqrt(neff));
         };
      }

      JSROOT.methodsCache[typename] = m;
      return m;
   };

   JSROOT.addMethods = function(obj) {
      this.extend(obj, JSROOT.getMethods(obj._typename, obj));
   };

   JSROOT.lastFFormat = "";

   JSROOT.FFormat = function(value, fmt) {
      // method used to convert numeric value to string according specified format
      // format can be like 5.4g or 4.2e or 6.4f
      // function saves actual format in JSROOT.lastFFormat variable
      if (!fmt) fmt = "6.4g";

      JSROOT.lastFFormat = "";

      fmt = fmt.trim();
      var len = fmt.length;
      if (len<2) return value.toFixed(4);
      var last = fmt.charAt(len-1);
      fmt = fmt.slice(0,len-1);
      var isexp = null;
      var prec = fmt.indexOf(".");
      if (prec<0) prec = 4; else prec = Number(fmt.slice(prec+1));
      if (isNaN(prec) || (prec<0) || (prec==null)) prec = 4;

      var significance = false;
      if ((last=='e') || (last=='E')) { isexp = true; } else
      if (last=='Q') { isexp = true; significance = true; } else
      if ((last=='f') || (last=='F')) { isexp = false; } else
      if (last=='W') { isexp = false; significance = true; } else
      if ((last=='g') || (last=='G')) {
         var se = JSROOT.FFormat(value, fmt+'Q');
         var _fmt = JSROOT.lastFFormat;
         var sg = JSROOT.FFormat(value, fmt+'W');

         if (se.length < sg.length) {
            JSROOT.lastFFormat = _fmt;
            return se;
         }
         return sg;
      } else {
         isexp = false;
         prec = 4;
      }

      if (isexp) {
         // for exponential representation only one significant digit befor point
         if (significance) prec--;
         if (prec<0) prec = 0;

         JSROOT.lastFFormat = '5.'+prec+'e';

         return value.toExponential(prec);
      }

      var sg = value.toFixed(prec);

      if (significance) {

         // when using fixed representation, one could get 0.0
         if ((value!=0) && (Number(sg)==0.) && (prec>0)) {
            prec = 20; sg = value.toFixed(prec);
         }

         var l = 0;
         while ((l<sg.length) && (sg.charAt(l) == '0' || sg.charAt(l) == '-' || sg.charAt(l) == '.')) l++;

         var diff = sg.length - l - prec;
         if (sg.indexOf(".")>l) diff--;

         if (diff != 0) {
            prec-=diff;
            if (prec<0) prec = 0; else if (prec>20) prec = 20;
            sg = value.toFixed(prec);
         }
      }

      JSROOT.lastFFormat = '5.'+prec+'f';

      return sg;
   }

   JSROOT.log10 = function(n) {
      return Math.log(n) / Math.log(10);
   }

   // dummy function, will be redefined when JSRootPainter is loaded
   JSROOT.progress = function(msg) {
      if ((msg !== undefined) && (typeof msg=="string")) JSROOT.console(msg);
   }

   JSROOT.Initialize = function() {

      if (JSROOT.source_fullpath.length === 0) return this;

      function window_on_load(func) {
         if (func!=null) {
            if (document.attachEvent ? document.readyState === 'complete' : document.readyState !== 'loading')
               func();
            else
               window.onload = func;
         }
         return JSROOT;
      }

      var src = JSROOT.source_fullpath;

//      if (JSROOT.source_min) {
//         if ( typeof define === "function" && define.amd ) {
            // all references are done with 'JSRootCore' name,
            // define it directly, otherwise it will be loaded once again
//            define('JSRootCore', [], JSROOT);
//         }
//      }

      if (JSROOT.GetUrlOption('gui', src) !== null)
         return window_on_load( function() { JSROOT.BuildSimpleGUI(); } );

      if ( typeof define === "function" && define.amd )
         return window_on_load( function() { JSROOT.BuildSimpleGUI('check_existing_elements'); } );

      var prereq = "";
      if (JSROOT.GetUrlOption('io', src)!=null) prereq += "io;";
      if (JSROOT.GetUrlOption('2d', src)!=null) prereq += "2d;";
      if (JSROOT.GetUrlOption('jq2d', src)!=null) prereq += "jq2d;";
      if (JSROOT.GetUrlOption('more2d', src)!=null) prereq += "more2d;";
      if (JSROOT.GetUrlOption('geo', src)!=null) prereq += "geo;";
      if (JSROOT.GetUrlOption('3d', src)!=null) prereq += "3d;";
      if (JSROOT.GetUrlOption('math', src)!=null) prereq += "math;";
      if (JSROOT.GetUrlOption('mathjax', src)!=null) prereq += "mathjax;";
      var user = JSROOT.GetUrlOption('load', src);
      if ((user!=null) && (user.length>0)) prereq += "load:" + user;
      var onload = JSROOT.GetUrlOption('onload', src);
      var bower = JSROOT.GetUrlOption('bower', src);
      if (bower!==null) {
         if (bower.length>0) JSROOT.bower_dir = bower; else
            if (JSROOT.source_dir.indexOf("jsroot/") == JSROOT.source_dir.length - 7)
               JSROOT.bower_dir = JSROOT.source_dir.substr(0, JSROOT.source_dir.length - 7);
         if (JSROOT.bower_dir.length > 0) console.log("Set JSROOT.bower_dir to " + JSROOT.bower_dir);
      }

      if ((prereq.length>0) || (onload!=null))
         window_on_load(function() {
            if (prereq.length>0) JSROOT.AssertPrerequisites(prereq, onload); else
               if (onload!=null) {
                  onload = JSROOT.findFunction(onload);
                  if (typeof onload == 'function') onload();
               }
         });

      return this;
   }

   return JSROOT.Initialize();

}));

/// JSRootCore.js ends

