/** JavaScript ROOT 3D geometry painter
 * @file JSRootGeoPainter.js */

(function( factory ) {
   if ( typeof define === "function" && define.amd ) {
      define( [ 'JSRootPainter', 'd3', 'threejs', 'dat.gui', 'JSRoot3DPainter', 'JSRootGeoBase' ], factory );
   } else
   if (typeof exports === 'object' && typeof module !== 'undefined') {
      var jsroot = require("./JSRootCore.js");
      if (!jsroot.nodejs && (typeof window != 'undefined')) require("./dat.gui.min.js");
      factory(jsroot, require("./d3.min.js"), require("./three.min.js"), require("./JSRoot3DPainter.js"), require("./JSRootGeoBase.js"));
   } else {

      if (typeof JSROOT == 'undefined')
         throw new Error('JSROOT is not defined', 'JSRootGeoPainter.js');

      if (typeof JSROOT.Painter != 'object')
         throw new Error('JSROOT.Painter is not defined', 'JSRootGeoPainter.js');

      if (typeof d3 == 'undefined')
         throw new Error('d3 is not defined', 'JSRootGeoPainter.js');

      if (typeof THREE == 'undefined')
         throw new Error('THREE is not defined', 'JSRootGeoPainter.js');

      if (typeof dat == 'undefined')
         throw new Error('dat.gui is not defined', 'JSRootGeoPainter.js');

      factory( JSROOT, d3, THREE );
   }
} (function( JSROOT, d3, THREE ) {

   "use strict";

   JSROOT.sources.push("geom");

   if ( typeof define === "function" && define.amd )
      JSROOT.loadScript('$$$style/JSRootGeoPainter.css');

   if (typeof JSROOT.GEO !== 'object')
      console.error('JSROOT.GEO namespace is not defined')

   // ============================================================================================

   function Toolbar(container, buttons, bright) {
      this.bright = bright;
      if ((container !== undefined) && (typeof container.append == 'function'))  {
         this.element = container.append("div").attr('class','jsroot');
         this.addButtons(buttons);
      }
   }

   Toolbar.prototype.addButtons = function(buttons) {
      var pthis = this;

      this.buttonsNames = [];
      buttons.forEach(function(buttonGroup) {
         var group = pthis.element.append('div').attr('class', 'toolbar-group');

         buttonGroup.forEach(function(buttonConfig) {
            var buttonName = buttonConfig.name;
            if (!buttonName) {
               throw new Error('must provide button \'name\' in button config');
            }
            if (pthis.buttonsNames.indexOf(buttonName) !== -1) {
               throw new Error('button name \'' + buttonName + '\' is taken');
            }
            pthis.buttonsNames.push(buttonName);

            pthis.createButton(group, buttonConfig);
         });
      });
   };

   Toolbar.prototype.createButton = function(group, config) {

      var title = config.title;
      if (title === undefined) title = config.name;

      if (typeof config.click !== 'function')
         throw new Error('must provide button \'click\' function in button config');

      var button = group.append('a')
                        .attr('class', this.bright ? 'toolbar-btn-bright' : 'toolbar-btn')
                        .attr('rel', 'tooltip')
                        .attr('data-title', title)
                        .on('click', config.click);

      this.createIcon(button, config.icon || JSROOT.ToolbarIcons.question);
   }


   Toolbar.prototype.changeBrightness = function(bright) {
      this.bright = bright;
      if (!this.element) return;

      this.element.selectAll(bright ? '.toolbar-btn' : ".toolbar-btn-bright")
                  .attr("class", !bright ? 'toolbar-btn' : "toolbar-btn-bright");
   }


   Toolbar.prototype.createIcon = function(button, thisIcon) {
      var size = thisIcon.size || 512,
          scale = thisIcon.scale || 1,
          svg = button.append("svg:svg")
                      .attr('height', '1em')
                      .attr('width', '1em')
                      .attr('viewBox', [0, 0, size, size].join(' '));

      if ('recs' in thisIcon) {
          var rec = {};
          for (var n=0;n<thisIcon.recs.length;++n) {
             JSROOT.extend(rec, thisIcon.recs[n]);
             svg.append('rect').attr("x", rec.x).attr("y", rec.y)
                               .attr("width", rec.w).attr("height", rec.h)
                               .attr("fill", rec.f);
          }
       } else {
          var elem = svg.append('svg:path').attr('d',thisIcon.path);
          if (scale !== 1)
             elem.attr('transform', 'scale(' + scale + ' ' + scale +')');
       }
   };

   Toolbar.prototype.removeAllButtons = function() {
      this.element.remove();
   };

   /**
    * @class JSROOT.TGeoPainter Holder of different functions and classes for drawing geometries
    */

   function TGeoPainter(obj) {

      if (obj && (obj._typename === "TGeoManager")) {
         this.geo_manager = obj;
         obj = obj.fMasterVolume;
      }

      if (obj && (obj._typename.indexOf('TGeoVolume') === 0))
         obj = { _typename:"TGeoNode", fVolume: obj, fName: obj.fName, $geoh: obj.$geoh, _proxy: true };

      JSROOT.TObjectPainter.call(this, obj);

      this.no_default_title = true; // do not set title to main DIV
      this.mode3d = true; // indication of 3D mode

      this.Cleanup(true);
   }

   TGeoPainter.prototype = Object.create( JSROOT.TObjectPainter.prototype );

   TGeoPainter.prototype.CreateToolbar = function(args) {
      if (this._toolbar || this._usesvg) return;
      var painter = this;
      var buttonList = [{
         name: 'toImage',
         title: 'Save as PNG',
         icon: JSROOT.ToolbarIcons.camera,
         click: function() {
            painter.Render3D(0);
            var dataUrl = painter._renderer.domElement.toDataURL("image/png");
            dataUrl.replace("image/png", "image/octet-stream");
            var link = document.createElement('a');
            if (typeof link.download === 'string') {
               document.body.appendChild(link); //Firefox requires the link to be in the body
               link.download = "geometry.png";
               link.href = dataUrl;
               link.click();
               document.body.removeChild(link); //remove the link when done
            }
         }
      }];

      buttonList.push({
         name: 'control',
         title: 'Toggle control UI',
         icon: JSROOT.ToolbarIcons.rect,
         click: function() { painter.showControlOptions('toggle'); }
      });

      buttonList.push({
         name: 'enlarge',
         title: 'Enlarge geometry drawing',
         icon: JSROOT.ToolbarIcons.circle,
         click: function() { painter.ToggleEnlarge(); }
      });

      if (JSROOT.gStyle.ContextMenu)
      buttonList.push({
         name: 'menu',
         title: 'Show context menu',
         icon: JSROOT.ToolbarIcons.question,
         click: function() {

            d3.event.preventDefault();
            d3.event.stopPropagation();

            var evnt = d3.event;

            if (!JSROOT.Painter.closeMenu())
               JSROOT.Painter.createMenu(painter, function(menu) {
                  menu.painter.FillContextMenu(menu);
                  menu.show(evnt);
               });
         }
      });

      var bkgr = new THREE.Color(this.options.background);

      this._toolbar = new Toolbar( this.select_main(), [buttonList], (bkgr.r + bkgr.g + bkgr.b) < 1);
   }

   TGeoPainter.prototype.GetGeometry = function() {
      return this.GetObject();
   }

   TGeoPainter.prototype.ModifyVisisbility = function(name, sign) {
      if (JSROOT.GEO.NodeKind(this.GetGeometry()) !== 0) return;

      if (name == "")
         return JSROOT.GEO.SetBit(this.GetGeometry().fVolume, JSROOT.GEO.BITS.kVisThis, (sign === "+"));

      var regexp, exact = false;

      //arg.node.fVolume
      if (name.indexOf("*") < 0) {
         regexp = new RegExp("^"+name+"$");
         exact = true;
      } else {
         regexp = new RegExp("^" + name.split("*").join(".*") + "$");
         exact = false;
      }

      this.FindNodeWithVolume(regexp, function(arg) {
         JSROOT.GEO.InvisibleAll.call(arg.node.fVolume, (sign !== "+"));
         return exact ? arg : null; // continue search if not exact expression provided
      });
   }

   TGeoPainter.prototype.decodeOptions = function(opt) {
      var res = { _grid: false, _bound: false, _debug: false,
                  _full: false, _axis: false, _axis_center: false,
                  _count: false, wireframe: false,
                   scale: new THREE.Vector3(1,1,1), zoom: 1.0,
                   more: 1, maxlimit: 100000, maxnodeslimit: 3000,
                   use_worker: false, update_browser: true, show_controls: false,
                   highlight: false, highlight_scene: false, select_in_view: false,
                   project: '', is_main: false, tracks: false, ortho_camera: false,
                   clipx: false, clipy: false, clipz: false, ssao: false,
                   script_name: "", transparency: 0, autoRotate: false, background: '#FFFFFF',
                   depthMethod: "box" };

      var _opt = JSROOT.GetUrlOption('_grid');
      if (_opt !== null && _opt == "true") res._grid = true;
      var _opt = JSROOT.GetUrlOption('_debug');
      if (_opt !== null && _opt == "true") { res._debug = true; res._grid = true; }
      if (_opt !== null && _opt == "bound") { res._debug = true; res._grid = true; res._bound = true; }
      if (_opt !== null && _opt == "full") { res._debug = true; res._grid = true; res._full = true; res._bound = true; }

      var macro = opt.indexOf("macro:");
      if (macro>=0) {
         var separ = opt.indexOf(";", macro+6);
         if (separ<0) separ = opt.length;
         res.script_name = opt.substr(macro+6,separ-macro-6);
         opt = opt.substr(0, macro) + opt.substr(separ+1);
         console.log('script', res.script_name, 'rest', opt);
      }

      while (true) {
         var pp = opt.indexOf("+"), pm = opt.indexOf("-");
         if ((pp<0) && (pm<0)) break;
         var p1 = pp, sign = "+";
         if ((p1<0) || ((pm>=0) && (pm<pp))) { p1 = pm; sign = "-"; }

         var p2 = p1+1, regexp = new RegExp('[,; .]');
         while ((p2<opt.length) && !regexp.test(opt[p2]) && (opt[p2]!='+') && (opt[p2]!='-')) p2++;

         var name = opt.substring(p1+1, p2);
         opt = opt.substr(0,p1) + opt.substr(p2);
         // console.log("Modify visibility", sign,':',name);

         this.ModifyVisisbility(name, sign);
      }

      var d = new JSROOT.DrawOptions(opt);

      if (d.check("MAIN")) res.is_main = true;

      if (d.check("TRACKS")) res.tracks = true;
      if (d.check("ORTHO_CAMERA")) res.ortho_camera = true;

      if (d.check("DEPTHRAY") || d.check("DRAY")) res.depthMethod = "ray";
      if (d.check("DEPTHBOX") || d.check("DBOX")) res.depthMethod = "box";
      if (d.check("DEPTHPNT") || d.check("DPNT")) res.depthMethod = "pnt";
      if (d.check("DEPTHSIZE") || d.check("DSIZE")) res.depthMethod = "size";
      if (d.check("DEPTHDFLT") || d.check("DDFLT")) res.depthMethod = "dflt";

      if (d.check("ZOOM", true)) res.zoom = d.partAsInt(0, 100) / 100;

      if (d.check('BLACK')) res.background = "#000000";

      if (d.check('BKGR_', true)) {
         var bckgr = null;
         if (d.partAsInt(1)>0) bckgr = JSROOT.Painter.root_colors[d.partAsInt()]; else
         for (var col=0;col<8;++col)
            if (JSROOT.Painter.root_colors[col].toUpperCase() === d.part) bckgr = JSROOT.Painter.root_colors[col];
         if (bckgr) res.background = "#" + new THREE.Color(bckgr).getHexString();
      }

      if (d.check("MORE3")) res.more = 3;
      if (d.check("MORE")) res.more = 2;
      if (d.check("ALL")) res.more = 100;

      if (d.check("CONTROLS") || d.check("CTRL")) res.show_controls = true;

      if (d.check("CLIPXYZ")) res.clipx = res.clipy = res.clipz = true;
      if (d.check("CLIPX")) res.clipx = true;
      if (d.check("CLIPY")) res.clipy = true;
      if (d.check("CLIPZ")) res.clipz = true;
      if (d.check("CLIP")) res.clipx = res.clipy = res.clipz = true;

      if (d.check("PROJX", true)) { res.project = 'x'; if (d.partAsInt(1)>0) res.projectPos = d.partAsInt(); }
      if (d.check("PROJY", true)) { res.project = 'y'; if (d.partAsInt(1)>0) res.projectPos = d.partAsInt(); }
      if (d.check("PROJZ", true)) { res.project = 'z'; if (d.partAsInt(1)>0) res.projectPos = d.partAsInt(); }

      if (d.check("DFLT_COLORS") || d.check("DFLT")) this.SetRootDefaultColors();
      if (d.check("SSAO")) res.ssao = true;

      if (d.check("NOWORKER")) res.use_worker = -1;
      if (d.check("WORKER")) res.use_worker = 1;

      if (d.check("NOHIGHLIGHT") || d.check("NOHIGH")) res.highlight = res.highlight_scene = 0;
      if (d.check("HIGHLIGHT")) res.highlight_scene = res.highlight = true;
      if (d.check("HSCENEONLY")) { res.highlight_scene = true; res.highlight = 0; }
      if (d.check("NOHSCENE")) res.highlight_scene = 0;
      if (d.check("HSCENE")) res.highlight_scene = true;

      if (d.check("WIRE")) res.wireframe = true;
      if (d.check("ROTATE")) res.autoRotate = true;

      if (d.check("INVX") || d.check("INVERTX")) res.scale.x = -1;
      if (d.check("INVY") || d.check("INVERTY")) res.scale.y = -1;
      if (d.check("INVZ") || d.check("INVERTZ")) res.scale.z = -1;

      if (d.check("COUNT")) res._count = true;

      if (d.check('TRANSP',true))
         res.transparency = d.partAsInt(0,100)/100;

      if (d.check('OPACITY',true))
         res.transparency = 1 - d.partAsInt(0,100)/100;

      if (d.check("AXISCENTER") || d.check("AC")) { res._axis = true; res._axis_center = true; }

      if (d.check("AXIS") || d.check("A")) res._axis = true;

      if (d.check("D")) res._debug = true;
      if (d.check("G")) res._grid = true;
      if (d.check("B")) res._bound = true;
      if (d.check("W")) res.wireframe = true;
      if (d.check("F")) res._full = true;
      if (d.check("Y")) res._yup = true;
      if (d.check("Z")) res._yup = false;

      return res;
   }

   TGeoPainter.prototype.ActiavteInBrowser = function(names, force) {
      // if (this.GetItemName() === null) return;

      if (typeof names == 'string') names = [ names ];

      if (this._hpainter) {
         // show browser if it not visible
         this._hpainter.actiavte(names, force);

         // if highlight in the browser disabled, suppress in few seconds
         if (!this.options.update_browser)
            setTimeout(this._hpainter.activate.bind(this._hpainter, []), 2000);
      }
   }

   TGeoPainter.prototype.TestMatrixes = function() {
      // method can be used to check matrix calculations with current three.js model

      var painter = this, errcnt = 0, totalcnt = 0, totalmax = 0;

      var arg = {
            domatrix: true,
            func: function(node) {

               var m2 = this.getmatrix();

               var entry = this.CopyStack();

               var mesh = painter._clones.CreateObject3D(entry.stack, painter._toplevel, 'mesh');

               if (!mesh) return true;

               totalcnt++;

               var m1 = mesh.matrixWorld, flip, origm2;

               if (m1.equals(m2)) return true
               if ((m1.determinant()>0) && (m2.determinant()<-0.9)) {
                  flip = THREE.Vector3(1,1,-1);
                  origm2 = m2;
                  m2 = m2.clone().scale(flip);
                  if (m1.equals(m2)) return true;
               }

               var max = 0;
               for (var k=0;k<16;++k)
                  max = Math.max(max, Math.abs(m1.elements[k] - m2.elements[k]));

               totalmax = Math.max(max, totalmax);

               if (max < 1e-4) return true;

               console.log(painter._clones.ResolveStack(entry.stack).name, 'maxdiff', max, 'determ', m1.determinant(), m2.determinant());

               errcnt++;

               return false;
            }
         };


      tm1 = new Date().getTime();

      var cnt = this._clones.ScanVisible(arg);

      tm2 = new Date().getTime();

      console.log('Compare matrixes total',totalcnt,'errors',errcnt, 'takes', tm2-tm1, 'maxdiff', totalmax);
   }


   TGeoPainter.prototype.FillContextMenu = function(menu) {
      menu.add("header: Draw options");

      menu.addchk(this.options.update_browser, "Browser update", function() {
         this.options.update_browser = !this.options.update_browser;
         if (!this.options.update_browser) this.ActiavteInBrowser([]);
      });
      menu.addchk(this.options.show_controls, "Show Controls", function() {
         this.showControlOptions('toggle');
      });
      menu.addchk(this.TestAxisVisibility, "Show axes", function() {
         this.toggleAxisDraw();
      });
      menu.addchk(this.options.wireframe, "Wire frame", function() {
         this.options.wireframe = !this.options.wireframe;
         this.changeWireFrame(this._scene, this.options.wireframe);
      });
      menu.addchk(this.options.highlight, "Highlight volumes", function() {
         this.options.highlight = !this.options.highlight;
      });
      menu.addchk(this.options.highlight_scene, "Highlight scene", function() {
         this.options.highlight_scene = !this.options.highlight_scene;
      });
      menu.add("Reset camera position", function() {
         this.focusCamera();
         this.Render3D();
      });
      if (!this.options.project)
         menu.addchk(this.options.autoRotate, "Autorotate", function() {
            this.options.autoRotate = !this.options.autoRotate;
            this.autorotate(2.5);
         });
      menu.addchk(this.options.select_in_view, "Select in view", function() {
         this.options.select_in_view = !this.options.select_in_view;
         if (this.options.select_in_view) this.startDrawGeometry();
      });
   }

   TGeoPainter.prototype.changeGlobalTransparency = function(transparency, skip_render) {
      var value = 1 - transparency;
      this._toplevel.traverse( function (node) {
         if (node instanceof THREE.Mesh) {
            if (node.material.alwaysTransparent !== undefined) {
               if (!node.material.alwaysTransparent) {
                  node.material.transparent = value !== 1.0;
               }
               node.material.opacity = Math.min(value, node.material.inherentOpacity);
            }
         }
      });
      if (!skip_render) this.Render3D(0);
   }

   TGeoPainter.prototype.showControlOptions = function(on) {
      if (on==='toggle') on = !this._datgui;

      this.options.show_controls = on;

      if (this._datgui) {
         if (on) return;
         d3.select(this._datgui.domElement).remove();
         this._datgui.destroy();
         delete this._datgui;
         return;
      }
      if (!on) return;

      var painter = this;

      this._datgui = new dat.GUI({ autoPlace: false, width: Math.min(650, painter._renderer.domElement.width / 2) });

      var main = this.select_main();
      if (main.style('position')=='static') main.style('position','relative');

      d3.select(this._datgui.domElement)
               .style('position','absolute')
               .style('top',0).style('right',0);

      main.node().appendChild(this._datgui.domElement);

      if (this.options.project) {

         var bound = this.getGeomBoundingBox(this.getProjectionSource(), 0.01);

         var axis = this.options.project;

         if (this.options.projectPos === undefined)
            this.options.projectPos = (bound.min[axis] + bound.max[axis])/2;

         this._datgui.add(this.options, 'projectPos', bound.min[axis], bound.max[axis])
             .name(axis.toUpperCase() + ' projection')
             .onChange(function (value) {
               painter.startDrawGeometry();
           });

      } else {
         // Clipping Options

         var bound = this.getGeomBoundingBox(this._toplevel, 0.01);

         var clipFolder = this._datgui.addFolder('Clipping');

         for (var naxis=0;naxis<3;++naxis) {
            var axis = !naxis ? "x" : ((naxis===1) ? "y" : "z"),
                  axisC = axis.toUpperCase();

            clipFolder.add(this, 'enable' + axisC).name('Enable '+axisC)
            .listen() // react if option changed outside
            .onChange( function (value) {
               painter._enableSSAO = value ? false : painter._enableSSAO;
               painter.updateClipping();
            });

            var clip = "clip" + axisC;
            if (this[clip] === 0) this[clip] = (bound.min[axis]+bound.max[axis])/2;

            var item = clipFolder.add(this, clip, bound.min[axis], bound.max[axis])
                   .name(axisC + ' Position')
                   .onChange(function (value) {
                     if (painter[this.enbale_flag]) painter.updateClipping();
                    });

            item.enbale_flag = "enable"+axisC;
         }

      }

      // Appearance Options

      var appearance = this._datgui.addFolder('Appearance');

      if (this._webgl) {
         appearance.add(this, '_enableSSAO').name('Smooth Lighting (SSAO)').onChange( function (value) {
            painter._renderer.antialias = !painter._renderer.antialias;
            painter.enableX = value ? false : painter.enableX;
            painter.enableY = value ? false : painter.enableY;
            painter.enableZ = value ? false : painter.enableZ;
            painter.updateClipping();
         }).listen();
      }

      appearance.add(this.options, 'highlight').name('Highlight Selection').listen().onChange( function (value) {
         if (!value) painter.HighlightMesh(null);
     });

      appearance.add(this.options, 'transparency', 0.0, 1.0)
                     .listen().onChange(this.changeGlobalTransparency.bind(this));

      appearance.add(this.options, 'wireframe').name('Wireframe').listen().onChange( function (value) {
         painter.changeWireFrame(painter._scene, painter.options.wireframe);
      });

      appearance.addColor(this.options, 'background').name('Background').onChange( function() {
          painter._renderer.setClearColor(painter.options.background, 1);
          painter.Render3D(0);
          var bkgr = new THREE.Color(painter.options.background);
          painter._toolbar.changeBrightness((bkgr.r + bkgr.g + bkgr.b) < 1);
      });

      appearance.add(this, 'focusCamera').name('Reset camera position');

      // Advanced Options

      if (this._webgl) {
         var advanced = this._datgui.addFolder('Advanced');

         advanced.add( this._advceOptions, 'aoClamp', 0.0, 1.0).listen().onChange( function (value) {
            painter._ssaoPass.uniforms[ 'aoClamp' ].value = value;
            painter._enableSSAO = true;
            painter.Render3D(0);
         });

         advanced.add( this._advceOptions, 'lumInfluence', 0.0, 1.0).listen().onChange( function (value) {
            painter._ssaoPass.uniforms[ 'lumInfluence' ].value = value;
            painter._enableSSAO = true;
            painter.Render3D(0);
         });

         advanced.add( this._advceOptions, 'clipIntersection').listen().onChange( function (value) {
            painter.clipIntersection = value;
            painter.updateClipping();
         });

         advanced.add(this._advceOptions, 'depthTest').onChange( function (value) {
            painter._toplevel.traverse( function (node) {
               if (node instanceof THREE.Mesh) {
                  node.material.depthTest = value;
               }
            });
            painter.Render3D(0);
         }).listen();

         advanced.add(this, 'resetAdvanced').name('Reset');
      }
   }


   TGeoPainter.prototype.OrbitContext = function(evnt, intersects) {

      JSROOT.Painter.createMenu(this, function(menu) {
         var numitems = 0, numnodes = 0, cnt = 0;
         if (intersects)
            for (var n=0;n<intersects.length;++n) {
               if (intersects[n].object.stack) numnodes++;
               if (intersects[n].object.geo_name) numitems++;
            }

         if (numnodes + numitems === 0) {
            menu.painter.FillContextMenu(menu);
         } else {
            var many = (numnodes + numitems) > 1;

            if (many) menu.add("header:" + ((numitems > 0) ? "Items" : "Nodes"));

            for (var n=0;n<intersects.length;++n) {
               var obj = intersects[n].object,
                   name, itemname, hdr;

               if (obj.geo_name) {
                  itemname = obj.geo_name;
                  name = itemname.substr(itemname.lastIndexOf("/")+1);
                  if (!name) name = itemname;
                  hdr = name;
               } else
               if (obj.stack) {
                  name = menu.painter._clones.ResolveStack(obj.stack).name;
                  itemname = menu.painter.GetStackFullName(obj.stack);
                  hdr = menu.painter.GetItemName();
                  if (name.indexOf("Nodes/") === 0) hdr = name.substr(6); else
                  if (name.length > 0) hdr = name; else
                  if (!hdr) hdr = "header";

               } else
                  continue;


               menu.add((many ? "sub:" : "header:") + hdr, itemname, function(arg) { this.ActiavteInBrowser([arg], true); });

               menu.add("Browse", itemname, function(arg) { this.ActiavteInBrowser([arg], true); });

               if (menu.painter._hpainter)
                  menu.add("Inspect", itemname, function(arg) { this._hpainter.display(itemname, "inspect"); });

               if (obj.geo_name) {
                  menu.add("Hide", n, function(indx) {
                     var mesh = intersects[indx].object;
                     mesh.visible = false; // just disable mesh
                     if (mesh.geo_object) mesh.geo_object.$hidden_via_menu = true; // and hide object for further redraw
                     menu.painter.Render3D();
                  });

                  if (many) menu.add("endsub:");

                  continue;
               }

               var wireframe = menu.painter.accessObjectWireFrame(obj);

               if (wireframe!==undefined)
                  menu.addchk(wireframe, "Wireframe", n, function(indx) {
                     var m = intersects[indx].object.material;
                     m.wireframe = !m.wireframe;
                     this.Render3D();
                  });

               if (++cnt>1)
                  menu.add("Manifest", n, function(indx) {

                     if (this._last_manifest)
                        this._last_manifest.wireframe = !this._last_manifest.wireframe;

                     if (this._last_hidden)
                        this._last_hidden.forEach(function(obj) { obj.visible = true; });

                     this._last_hidden = [];

                     for (var i=0;i<indx;++i)
                        this._last_hidden.push(intersects[i].object);

                     this._last_hidden.forEach(function(obj) { obj.visible = false; });

                     this._last_manifest = intersects[indx].object.material;

                     this._last_manifest.wireframe = !this._last_manifest.wireframe;

                     this.Render3D();
                  });


               menu.add("Focus", n, function(indx) {
                  this.focusCamera(intersects[indx].object);
               });

               menu.add("Hide", n, function(indx) {
                  var resolve = menu.painter._clones.ResolveStack(intersects[indx].object.stack);

                  if (resolve.obj && (resolve.node.kind === 0) && resolve.obj.fVolume) {
                     JSROOT.GEO.SetBit(resolve.obj.fVolume, JSROOT.GEO.BITS.kVisThis, false);
                     JSROOT.GEO.updateBrowserIcons(resolve.obj.fVolume, this._hpainter);
                  } else
                  if (resolve.obj && (resolve.node.kind === 1)) {
                     resolve.obj.fRnrSelf = false;
                     JSROOT.GEO.updateBrowserIcons(resolve.obj, this._hpainter);
                  }
                  // intersects[arg].object.visible = false;
                  // this.Render3D();

                  this.testGeomChanges();// while many volumes may disappear, recheck all of them
               });

               if (many) menu.add("endsub:");
            }
         }
         menu.show(evnt);
      });
   }

   TGeoPainter.prototype.FilterIntersects = function(intersects) {

      // remove all elements without stack - indicator that this is geometry object
      for (var n=intersects.length-1; n>=0; --n) {

         var obj = intersects[n].object;

         var unique = (obj.stack !== undefined) || (obj.geo_name !== undefined);

         for (var k=0;(k<n) && unique;++k)
            if (intersects[k].object === obj) unique = false;

         if (!unique) intersects.splice(n,1);
      }

      if (this.enableX || this.enableY || this.enableZ ) {
         var clippedIntersects = [];

         for (var i = 0; i < intersects.length; ++i) {
            var clipped = false;
            var point = intersects[i].point;

            if (this.enableX && this._clipPlanes[0].normal.dot(point) > this._clipPlanes[0].constant ) {
               clipped = true;
            }
            if (this.enableY && this._clipPlanes[1].normal.dot(point) > this._clipPlanes[1].constant ) {
               clipped = true;
            }
            if (this.enableZ && this._clipPlanes[2].normal.dot(point) > this._clipPlanes[2].constant ) {
               clipped = true;
            }

            if (clipped)
               clippedIntersects.push(intersects[i]);
         }

         intersects = clippedIntersects;
      }

      return intersects;
   }

   TGeoPainter.prototype.testCameraPositionChange = function() {
      // function analyzes camera position and start redraw of geometry if
      // objects in view may be changed

      if (!this.options.select_in_view || this._draw_all_nodes) return;


      var matrix = JSROOT.GEO.CreateProjectionMatrix(this._camera);

      var frustum = JSROOT.GEO.CreateFrustum(matrix);

      // check if overall bounding box seen
      if (!frustum.CheckBox(this.getGeomBoundingBox(this._toplevel)))
         this.startDrawGeometry();
   }

   TGeoPainter.prototype.ResolveStack = function(stack) {
      return this._clones && stack ? this._clones.ResolveStack(stack) : null;
   }

   TGeoPainter.prototype.GetStackFullName = function(stack) {
      var mainitemname = this.GetItemName(),
          sub = this.ResolveStack(stack);

      if (!sub || !sub.name) return mainitemname;
      return mainitemname ? (mainitemname + "/" + sub.name) : sub.name;
   }

   TGeoPainter.prototype.HighlightMesh = function(active_mesh, color, geo_object, geo_stack, no_recursive) {

      if (geo_object) {
         active_mesh = [];
         var extras = this.getExtrasContainer();
         if (extras && extras.children)
            for (var k=0;k<extras.children.length;++k)
               if (extras.children[k].geo_object === geo_object) {
                  active_mesh.push(extras.children[k]);
               }
      } else if (geo_stack && this._toplevel) {
         active_mesh = [];
         this._toplevel.traverse(function(mesh) {
            if ((mesh instanceof THREE.Mesh) && (mesh.stack===geo_stack)) active_mesh.push(mesh);
         });
      } else if (active_mesh) {
         active_mesh = [ active_mesh ];
      }

      if (active_mesh && !active_mesh.length) active_mesh = null;

      if (active_mesh) {
         // check if highlight is disabled for correspondent objects kinds
         if (!active_mesh[0]) console.log("GOT", active_mesh)


         if (active_mesh[0].geo_object) {
            if (!this.options.highlight_scene) active_mesh = null;
         } else {
            if (!this.options.highlight) active_mesh = null;
         }
      }

      if (!no_recursive) {
         // check all other painters

         if (active_mesh) {
            if (!geo_object) geo_object = active_mesh[0].geo_object;
            if (!geo_stack) geo_stack = active_mesh[0].stack;
         }

         var lst = this._highlight_handlers || (!this._main_painter ? this._slave_painters : this._main_painter._slave_painters.concat([this._main_painter]));

         for (var k=0;k<lst.length;++k)
            if (lst[k]!==this) lst[k].HighlightMesh(null, color, geo_object, geo_stack, true);
      }

      var curr_mesh = this._selected_mesh;

      // check if selections are the same
      if (!curr_mesh && !active_mesh) return;
      var same = false;
      if (curr_mesh && active_mesh && (curr_mesh.length == active_mesh.length)) {
         same = true;
         for (var k=0;k<curr_mesh.length;++k)
            if (curr_mesh[k] !== active_mesh[k]) same = false;
      }
      if (same) return;

      if (curr_mesh)
         for (var k=0;k<curr_mesh.length;++k) {
            var c = curr_mesh[k];
            if (!c.material) continue;
            c.material.color = c.originalColor;
            delete c.originalColor;
            if (c.normalLineWidth)
               c.material.linewidth = c.normalLineWidth;
            if (c.normalMarkerSize)
               c.material.size = c.normalMarkerSize;
         }

      this._selected_mesh = active_mesh;

      if (active_mesh)
         for (var k=0;k<active_mesh.length;++k) {
            var a = active_mesh[k];
            if (!a.material) continue;
            a.originalColor = a.material.color;
            a.material.color = new THREE.Color( color || 0xffaa33 );

            if (a.hightlightLineWidth)
               a.material.linewidth = a.hightlightLineWidth;
            if (a.highlightMarkerSize)
               a.material.size = a.highlightMarkerSize;
         }

      this.Render3D(0);
   }

   TGeoPainter.prototype.addOrbitControls = function() {

      if (this._controls || this._usesvg) return;

      var painter = this;

      this.tooltip_allowed = (JSROOT.gStyle.Tooltip > 0);

      this._controls = JSROOT.Painter.CreateOrbitControl(this, this._camera, this._scene, this._renderer, this._lookat);

      if (this.options.project || this.options.ortho_camera) this._controls.enableRotate = false;

      this._controls.ContextMenu = this.OrbitContext.bind(this);

      this._controls.ProcessMouseMove = function(intersects) {

         var active_mesh = null, tooltip = null, resolve = null, names = [];

         // try to find mesh from intersections
         for (var k=0;k<intersects.length;++k) {
            var obj = intersects[k].object, info = null;
            if (!obj) continue;
            if (obj.geo_object) info = obj.geo_name; else
            if (obj.stack) info = painter.GetStackFullName(obj.stack);
            if (info===null) continue;

            if (info.indexOf("<prnt>")==0)
               info = painter.GetItemName() + info.substr(6);

            names.push(info);

            if (!active_mesh) {
               active_mesh = obj;
               tooltip = info;
               if (active_mesh.stack) resolve = painter.ResolveStack(active_mesh.stack);
            }
         }

         painter.HighlightMesh(active_mesh);

         if (painter.options.update_browser) {
            if (painter.options.highlight && tooltip) names = [ tooltip ];
            painter.ActiavteInBrowser(names);
         }

         if (!resolve || !resolve.obj) return tooltip;

         var lines = JSROOT.GEO.provideInfo(resolve.obj);
         lines.unshift(tooltip);

         return { name: resolve.obj.fName, title: resolve.obj.fTitle || resolve.obj._typename, lines: lines };
      }

      this._controls.ProcessMouseLeave = function() {
         if (painter.options.update_browser)
            painter.ActiavteInBrowser([]);
      }

      this._controls.ProcessDblClick = function() {
         if (painter._last_manifest) {
            painter._last_manifest.wireframe = !painter._last_manifest.wireframe;
            if (painter._last_hidden)
               painter._last_hidden.forEach(function(obj) { obj.visible = true; });
            delete painter._last_hidden;
            delete painter._last_manifest;
            painter.Render3D();
         } else {
            painter.adjustCameraPosition();
         }
      }
   }

   TGeoPainter.prototype.addTransformControl = function() {
      if (this._tcontrols) return;

      if ( !this.options._debug && !this.options._grid ) return;

      // FIXME: at the moment THREE.TransformControls is bogus in three.js, should be fixed and check again
      //return;

      this._tcontrols = new THREE.TransformControls( this._camera, this._renderer.domElement );
      this._scene.add( this._tcontrols );
      this._tcontrols.attach( this._toplevel );
      //this._tcontrols.setSize( 1.1 );
      var painter = this;

      window.addEventListener( 'keydown', function ( event ) {
         switch ( event.keyCode ) {
         case 81: // Q
            painter._tcontrols.setSpace( painter._tcontrols.space === "local" ? "world" : "local" );
            break;
         case 17: // Ctrl
            painter._tcontrols.setTranslationSnap( Math.ceil( painter._overall_size ) / 50 );
            painter._tcontrols.setRotationSnap( THREE.Math.degToRad( 15 ) );
            break;
         case 84: // T (Translate)
            painter._tcontrols.setMode( "translate" );
            break;
         case 82: // R (Rotate)
            painter._tcontrols.setMode( "rotate" );
            break;
         case 83: // S (Scale)
            painter._tcontrols.setMode( "scale" );
            break;
         case 187:
         case 107: // +, =, num+
            painter._tcontrols.setSize( painter._tcontrols.size + 0.1 );
            break;
         case 189:
         case 109: // -, _, num-
            painter._tcontrols.setSize( Math.max( painter._tcontrols.size - 0.1, 0.1 ) );
            break;
         }
      });
      window.addEventListener( 'keyup', function ( event ) {
         switch ( event.keyCode ) {
         case 17: // Ctrl
            painter._tcontrols.setTranslationSnap( null );
            painter._tcontrols.setRotationSnap( null );
            break;
         }
      });

      this._tcontrols.addEventListener( 'change', function() { painter.Render3D(0); });
   }

   TGeoPainter.prototype.nextDrawAction = function() {
      // return false when nothing todo
      // return true if one could perform next action immediately
      // return 1 when call after short timeout required
      // return 2 when call must be done from processWorkerReply

      if (!this._clones || (this.drawing_stage == 0)) return false;

      if (this.drawing_stage == 1) {

         // wait until worker is really started
         if (this.options.use_worker>0) {
            if (!this._worker) { this.startWorker(); return 1; }
            if (!this._worker_ready) return 1;
         }

         // first copy visibility flags and check how many unique visible nodes exists
         var numvis = this._clones.MarkVisisble(),
             matrix = null, frustum = null;

         if (this.options.select_in_view && !this._first_drawing) {
            // extract camera projection matrix for selection

            matrix = JSROOT.GEO.CreateProjectionMatrix(this._camera);

            frustum = JSROOT.GEO.CreateFrustum(matrix);

            // check if overall bounding box seen
            if (frustum.CheckBox(this.getGeomBoundingBox(this._toplevel))) {
               matrix = null; // not use camera for the moment
               frustum = null;
            }
         }

         this._current_face_limit = this.options.maxlimit;
         this._current_nodes_limit = this.options.maxnodeslimit;
         if (matrix) {
            this._current_face_limit*=1.25;
            this._current_nodes_limit*=1.25;
         }

         // here we decide if we need worker for the drawings
         // main reason - too large geometry and large time to scan all camera positions
         var need_worker = !JSROOT.BatchMode && ((numvis > 10000) || (matrix && (this._clones.ScanVisible() > 1e5)));

         // worker does not work when starting from file system
         if (need_worker && JSROOT.source_dir.indexOf("file://")==0) {
            console.log('disable worker for jsroot from file system');
            need_worker = false;
         }

         if (need_worker && !this._worker && (this.options.use_worker >= 0))
            this.startWorker(); // we starting worker, but it may not be ready so fast

         if (!need_worker || !this._worker_ready) {
            //var tm1 = new Date().getTime();
            var res = this._clones.CollectVisibles(this._current_face_limit, frustum, this._current_nodes_limit);
            this._new_draw_nodes = res.lst;
            this._draw_all_nodes = res.complete;
            //var tm2 = new Date().getTime();
            //console.log('Collect visibles', this._new_draw_nodes.length, 'takes', tm2-tm1);
            this.drawing_stage = 3;
            return true;
         }

         var job = {
               collect: this._current_face_limit,   // indicator for the command
               collect_nodes: this._current_nodes_limit,
               visible: this._clones.GetVisibleFlags(),
               matrix: matrix ? matrix.elements : null
         };

         this.submitToWorker(job);

         this.drawing_stage = 2;

         this.drawing_log = "Worker select visibles";

         return 2; // we now waiting for the worker reply
      }

      if (this.drawing_stage == 2) {
         // do nothing, we are waiting for worker reply

         this.drawing_log = "Worker select visibles";

         return 2;
      }

      if (this.drawing_stage == 3) {
         // here we merge new and old list of nodes for drawing,
         // normally operation is fast and can be implemented with one call

         this.drawing_log = "Analyse visibles";

         if (this._draw_nodes) {
            var del = this._clones.MergeVisibles(this._new_draw_nodes, this._draw_nodes);
            // remove should be fast, do it here
            for (var n=0;n<del.length;++n)
               this._clones.CreateObject3D(del[n].stack, this._toplevel, 'delete_mesh');

            if (del.length > 0)
               this.drawing_log = "Delete " + del.length + " nodes";
         }

         this._draw_nodes = this._new_draw_nodes;
         delete this._new_draw_nodes;
         this.drawing_stage = 4;
         return true;
      }

      if (this.drawing_stage === 4) {

         this.drawing_log = "Collect shapes";

         // collect shapes
         var shapes = this._clones.CollectShapes(this._draw_nodes);

         // merge old and new list with produced shapes
         this._build_shapes = this._clones.MergeShapesLists(this._build_shapes, shapes);

         this.drawing_stage = 5;
         return true;
      }


      if (this.drawing_stage === 5) {
         // this is building of geometries,
         // one can ask worker to build them or do it ourself

         if (this.canSubmitToWorker()) {
            var job = { limit: this._current_face_limit, shapes: [] }, cnt = 0;
            for (var n=0;n<this._build_shapes.length;++n) {
               var clone = null, item = this._build_shapes[n];
               // only submit not-done items
               if (item.ready || item.geom) {
                  // this is place holder for existing geometry
                  clone = { id: item.id, ready: true, nfaces: JSROOT.GEO.numGeometryFaces(item.geom), refcnt: item.refcnt };
               } else {
                  clone = JSROOT.clone(item, null, true);
                  cnt++;
               }

               job.shapes.push(clone);
            }

            if (cnt > 0) {
               /// only if some geom missing, submit job to the worker
               this.submitToWorker(job);
               this.drawing_log = "Worker build shapes";
               this.drawing_stage = 6;
               return 2;
            }
         }

         this.drawing_stage = 7;
      }

      if (this.drawing_stage === 6) {
         // waiting shapes from the worker, worker should activate our code
         return 2;
      }

      if ((this.drawing_stage === 7) || (this.drawing_stage === 8)) {

         if (this.drawing_stage === 7) {
            // building shapes
            var res = this._clones.BuildShapes(this._build_shapes, this._current_face_limit, 500);
            if (res.done) {
               this.drawing_stage = 8;
            } else {
               this.drawing_log = "Creating: " + res.shapes + " / " + this._build_shapes.length + " shapes,  "  + res.faces + " faces";
               if (res.notusedshapes < 30) return true;
            }
         }

         // final stage, create all meshes

         var tm0 = new Date().getTime(), ready = true,
             toplevel = this.options.project ? this._full_geom : this._toplevel;

         for (var n=0; n<this._draw_nodes.length;++n) {
            var entry = this._draw_nodes[n];
            if (entry.done) continue;

            var shape = this._build_shapes[entry.shapeid];
            if (!shape.ready) {
               if (this.drawing_stage === 8) console.warn('shape marked as not ready when should');
               ready = false;
               continue;
            }

            entry.done = true;
            shape.used = true; // indicate that shape was used in building

            if (!shape.geom || (shape.nfaces === 0)) {
               // node is visible, but shape does not created
               this._clones.CreateObject3D(entry.stack, toplevel, 'delete_mesh');
               continue;
            }

            var nodeobj = this._clones.origin[entry.nodeid];
            var clone = this._clones.nodes[entry.nodeid];
            var prop = JSROOT.GEO.getNodeProperties(clone.kind, nodeobj, true);

            this._num_meshes++;
            this._num_faces += shape.nfaces;

            var obj3d = this._clones.CreateObject3D(entry.stack, toplevel, this.options);

            prop.material.wireframe = this.options.wireframe;

            prop.material.side = this.bothSides ? THREE.DoubleSide : THREE.FrontSide;

            var mesh;

            if (obj3d.matrixWorld.determinant() > -0.9) {
               mesh = new THREE.Mesh( shape.geom, prop.material );
            } else {
               mesh = JSROOT.GEO.createFlippedMesh(obj3d, shape, prop.material);
            }

            obj3d.add(mesh);

            // keep full stack of nodes
            mesh.stack = entry.stack;
            mesh.renderOrder = this._clones.maxdepth - entry.stack.length; // order of transparency handling

            // keep hierarchy level
            mesh.$jsroot_order = obj3d.$jsroot_depth;

            // set initial render order, when camera moves, one must refine it
            //mesh.$jsroot_order = mesh.renderOrder =
            //   this._clones.maxdepth - ((obj3d.$jsroot_depth !== undefined) ? obj3d.$jsroot_depth : entry.stack.length);

            if (this.options._debug || this.options._full) {
               var wfg = new THREE.WireframeGeometry( mesh.geometry ),
                   wfm = new THREE.LineBasicMaterial( { color: prop.fillcolor, linewidth: (nodeobj && nodeobj.fVolume) ? nodeobj.fVolume.fLineWidth : 1  } ),
                   helper = new THREE.LineSegments(wfg, wfm);
               obj3d.add(helper);
            }

            if (this.options._bound || this.options._full) {
               var boxHelper = new THREE.BoxHelper( mesh );
               obj3d.add( boxHelper );
            }

            var tm1 = new Date().getTime();
            if (tm1 - tm0 > 500) { ready = false; break; }
         }

         if (ready) {
            if (this.options.project) {
               this.drawing_log = "Build projection";
               this.drawing_stage = 10;
               return true;
            }

            this.drawing_log = "Building done";
            this.drawing_stage = 0;
            return false;
         }

         if (this.drawing_stage > 7)
            this.drawing_log = "Building meshes " + this._num_meshes + " / " + this._num_faces;
         return true;
      }

      if (this.drawing_stage === 9) {
         // wait for main painter to be ready

         if (!this._main_painter) {
            console.warn('MAIN PAINTER DISAPPER');
            this.drawing_stage = 0;
            return false;
         }
         if (!this._main_painter._drawing_ready) return 1;

         this.drawing_stage = 10; // just do projection
      }

      if (this.drawing_stage === 10) {
         this.doProjection();
         this.drawing_log = "Building done";
         this.drawing_stage = 0;
         return false;
      }

      console.error('never come here stage = ' + this.drawing_stage);

      return false;
   }

   TGeoPainter.prototype.getProjectionSource = function() {
      if (this._clones_owner)
         return this._full_geom;
      if (!this._main_painter) {
         console.warn('MAIN PAINTER DISAPPER');
         return null;
      }
      if (!this._main_painter._drawing_ready) {
         console.warn('MAIN PAINTER NOT READY WHEN DO PROJECTION');
         return null;
      }
      return this._main_painter._toplevel;
   }

   TGeoPainter.prototype.getGeomBoundingBox = function(topitem, scalar) {
      var box3 = new THREE.Box3(), check_any = !this._clones;

      if (!topitem) {
         box3.min.x = box3.min.y = box3.min.z = -1;
         box3.max.x = box3.max.y = box3.max.z = 1;
         return box3;
      }

      box3.makeEmpty();

      topitem.traverse(function(mesh) {
         if (check_any || ((mesh instanceof THREE.Mesh) && mesh.stack)) JSROOT.GEO.getBoundingBox(mesh, box3);
      });

      if (scalar !== undefined) box3.expandByVector(box3.getSize().multiplyScalar(scalar));

      return box3;
   }


   TGeoPainter.prototype.doProjection = function() {
      var toplevel = this.getProjectionSource(), pthis = this;

      if (!toplevel) return false;

      JSROOT.Painter.DisposeThreejsObject(this._toplevel, true);

      var axis = this.options.project;

      if (this.options.projectPos === undefined) {

         var bound = this.getGeomBoundingBox(toplevel),
             min = bound.min[this.options.project], max = bound.max[this.options.project],
             mean = (min+max)/2;

         if ((min<0) && (max>0) && (Math.abs(mean) < 0.2*Math.max(-min,max))) mean = 0; // if middle is around 0, use 0

         this.options.projectPos = mean;
      }

      toplevel.traverse(function(mesh) {
         if (!(mesh instanceof THREE.Mesh) || !mesh.stack) return;

         var geom2 = JSROOT.GEO.projectGeometry(mesh.geometry, mesh.parent.matrixWorld, pthis.options.project, pthis.options.projectPos, mesh._flippedMesh);

         if (!geom2) return;

         var mesh2 = new THREE.Mesh( geom2, mesh.material.clone() );

         pthis._toplevel.add(mesh2);

         mesh2.stack = mesh.stack;
      });

      return true;
   }

   TGeoPainter.prototype.SameMaterial = function(node1, node2) {

      if ((node1===null) || (node2===null)) return node1 === node2;

      if (node1.fVolume.fLineColor >= 0)
         return (node1.fVolume.fLineColor === node2.fVolume.fLineColor);

       var m1 = (node1.fVolume.fMedium !== null) ? node1.fVolume.fMedium.fMaterial : null;
       var m2 = (node2.fVolume.fMedium !== null) ? node2.fVolume.fMedium.fMaterial : null;

       if (m1 === m2) return true;

       if ((m1 === null) || (m2 === null)) return false;

       return (m1.fFillStyle === m2.fFillStyle) && (m1.fFillColor === m2.fFillColor);
    }

   TGeoPainter.prototype.createScene = function(usesvg, webgl, w, h) {
      // three.js 3D drawing
      this._scene = new THREE.Scene();
      this._scene.fog = new THREE.Fog(0xffffff, 1, 10000);
      this._scene.overrideMaterial = new THREE.MeshLambertMaterial( { color: 0x7000ff, transparent: true, opacity: 0.2, depthTest: false } );

      this._scene_width = w;
      this._scene_height = h;

      if (this.options.ortho_camera) {
         this._camera =  new THREE.OrthographicCamera(-600, 600, -600, 600, 1, 10000);
      } else {
         this._camera = new THREE.PerspectiveCamera(25, w / h, 1, 10000);
         this._camera.up = this.options._yup ? new THREE.Vector3(0,1,0) : new THREE.Vector3(0,0,1);
      }

      this._scene.add( this._camera );

      this._selected_mesh = null;

      this._overall_size = 10;

      this._toplevel = new THREE.Object3D();

      this._scene.add(this._toplevel);

      if (usesvg) {
         this._renderer = new THREE.SVGRenderer( { precision: 0, astext: true } );
         if (this._renderer.outerHTML !== undefined) {
            // this is indication of new three.js functionality
            if (!JSROOT.svg_workaround) JSROOT.svg_workaround = [];
            var doc = (typeof document === 'undefined') ? JSROOT.nodejs_document : document;
            this._renderer.domElement = doc.createElementNS( 'http://www.w3.org/2000/svg', 'path');
            this._renderer.workaround_id = JSROOT.svg_workaround.length;
            this._renderer.domElement.setAttribute('jsroot_svg_workaround', this._renderer.workaround_id);
            JSROOT.svg_workaround[this._renderer.workaround_id] = "<svg></svg>"; // dummy, need to be replaced
         }
      } else {
         this._renderer = webgl ?
                           new THREE.WebGLRenderer({ antialias: true, logarithmicDepthBuffer: false,
                                                     preserveDrawingBuffer: true }) :
                           new THREE.CanvasRenderer({ antialias: true });
         this._renderer.setPixelRatio(window.devicePixelRatio);
      }
      this._renderer.setClearColor(this.options.background, 1);
      this._renderer.setSize(w, h, !this._fit_main_area);
      this._renderer.localClippingEnabled = true;

      if (this._fit_main_area && !usesvg) {
         this._renderer.domElement.style.width = "100%";
         this._renderer.domElement.style.height = "100%";
         var main = this.select_main();
         if (main.style('position')=='static') main.style('position','relative');
      }

      this._animating = false;

      // Clipping Planes

      this.clipIntersection = true;
      this.bothSides = false; // which material kind should be used
      this.enableX = this.enableY = this.enableZ = false;
      this.clipX = this.clipY = this.clipZ = 0.0;

      this._clipPlanes = [ new THREE.Plane(new THREE.Vector3( 1, 0, 0), this.clipX),
                           new THREE.Plane(new THREE.Vector3( 0, this.options._yup ? -1 : 1, 0), this.clipY),
                           new THREE.Plane(new THREE.Vector3( 0, 0, this.options._yup ? 1 : -1), this.clipZ) ];

       // Lights

      //var light = new THREE.HemisphereLight( 0xffffff, 0x999999, 0.5 );
      //this._scene.add(light);

      /*
      var directionalLight = new THREE.DirectionalLight( 0xffffff, 0.2 );
      directionalLight.position.set( 0, 1, 0 );
      this._scene.add( directionalLight );

      this._lights = new THREE.Object3D();
      var a = new THREE.PointLight(0xefefef, 0.2);
      var b = new THREE.PointLight(0xefefef, 0.2);
      var c = new THREE.PointLight(0xefefef, 0.2);
      var d = new THREE.PointLight(0xefefef, 0.2);
      this._lights.add(a);
      this._lights.add(b);
      this._lights.add(c);
      this._lights.add(d);
      this._camera.add( this._lights );
      a.position.set( 20000, 20000, 20000 );
      b.position.set( -20000, 20000, 20000 );
      c.position.set( 20000, -20000, 20000 );
      d.position.set( -20000, -20000, 20000 );
      */
      this._pointLight = new THREE.PointLight(0xefefef, 1);
      this._camera.add( this._pointLight );
      this._pointLight.position.set(10, 10, 10);

      // Default Settings

      this._defaultAdvanced = { aoClamp: 0.70,
                                lumInfluence: 0.4,
                              //  shininess: 100,
                                clipIntersection: true,
                                depthTest: true
                              };

      // Smooth Lighting Shader (Screen Space Ambient Occlusion)
      // http://threejs.org/examples/webgl_postprocessing_ssao.html

      this._enableSSAO = this.options.ssao;

      if (webgl) {
         var renderPass = new THREE.RenderPass( this._scene, this._camera );
         // Setup depth pass
         this._depthMaterial = new THREE.MeshDepthMaterial( { side: THREE.DoubleSide });
         this._depthMaterial.depthPacking = THREE.RGBADepthPacking;
         this._depthMaterial.blending = THREE.NoBlending;
         var pars = { minFilter: THREE.LinearFilter, magFilter: THREE.LinearFilter };
         this._depthRenderTarget = new THREE.WebGLRenderTarget( w, h, pars );
         // Setup SSAO pass
         this._ssaoPass = new THREE.ShaderPass( THREE.SSAOShader );
         this._ssaoPass.renderToScreen = true;
         this._ssaoPass.uniforms[ "tDepth" ].value = this._depthRenderTarget.texture;
         this._ssaoPass.uniforms[ 'size' ].value.set( w, h );
         this._ssaoPass.uniforms[ 'cameraNear' ].value = this._camera.near;
         this._ssaoPass.uniforms[ 'cameraFar' ].value = this._camera.far;
         this._ssaoPass.uniforms[ 'onlyAO' ].value = false;//( postprocessing.renderMode == 1 );
         this._ssaoPass.uniforms[ 'aoClamp' ].value = this._defaultAdvanced.aoClamp;
         this._ssaoPass.uniforms[ 'lumInfluence' ].value = this._defaultAdvanced.lumInfluence;
         // Add pass to effect composer
         this._effectComposer = new THREE.EffectComposer( this._renderer );
         this._effectComposer.addPass( renderPass );
         this._effectComposer.addPass( this._ssaoPass );
      }

      this._advceOptions = {};
      this.resetAdvanced();
   }


   TGeoPainter.prototype.startDrawGeometry = function(force) {

      if (!force && (this.drawing_stage!==0)) {
         this._draw_nodes_again = true;
         return;
      }

      this._startm = new Date().getTime();
      this._last_render_tm = this._startm;
      this._last_render_meshes = 0;
      this.drawing_stage = 1;
      this._drawing_ready = false;
      this.drawing_log = "collect visible";
      this._num_meshes = 0;
      this._num_faces = 0;
      this._selected_mesh = null;

      if (this.options.project) {
         if (this._clones_owner) {
            if (this._full_geom) {
               this.drawing_stage = 10;
               this.drawing_log = "build projection";
            } else {
               this._full_geom = new THREE.Object3D();
            }

         } else {
            this.drawing_stage = 9;
            this.drawing_log = "wait for main painter";
         }
      }

      delete this._last_manifest;
      delete this._last_hidden; // clear list of hidden objects

      delete this._draw_nodes_again; // forget about such flag

      this.continueDraw();
   }

   TGeoPainter.prototype.resetAdvanced = function() {
      if (this._webgl) {
         this._advceOptions.aoClamp = this._defaultAdvanced.aoClamp;
         this._advceOptions.lumInfluence = this._defaultAdvanced.lumInfluence;

         this._ssaoPass.uniforms[ 'aoClamp' ].value = this._defaultAdvanced.aoClamp;
         this._ssaoPass.uniforms[ 'lumInfluence' ].value = this._defaultAdvanced.lumInfluence;
      }

      this._advceOptions.depthTest = this._defaultAdvanced.depthTest;
      this._advceOptions.clipIntersection = this._defaultAdvanced.clipIntersection;
      this.clipIntersection = this._defaultAdvanced.clipIntersection;

      var painter = this;
      this._toplevel.traverse( function (node) {
         if (node instanceof THREE.Mesh) {
            node.material.depthTest = painter._defaultAdvanced.depthTest;
         }
      });

      this.Render3D(0);
   }

   TGeoPainter.prototype.updateMaterialSide = function(both_sides, force) {
      if ((this.bothSides === both_sides) && !force) return;

      this._scene.traverse( function(obj) {
         if (obj.hasOwnProperty("material") && ('emissive' in obj.material)) {
            obj.material.side = both_sides ? THREE.DoubleSide : THREE.FrontSide;
            obj.material.needsUpdate = true;
        }
      });

      this.bothSides = both_sides;
   }

   TGeoPainter.prototype.updateClipping = function(without_render) {
      this._clipPlanes[0].constant = this.clipX;
      this._clipPlanes[1].constant = -this.clipY;
      this._clipPlanes[2].constant = this.options._yup ? -this.clipZ : this.clipZ;

      var painter = this;
      this._scene.traverse( function (node) {
         if (node instanceof THREE.Mesh) {
            node.material.clipIntersection = painter.clipIntersection;
            node.material.clippingPlanes = [];
            if (painter.enableX) node.material.clippingPlanes.push(painter._clipPlanes[0]);
            if (painter.enableY) node.material.clippingPlanes.push(painter._clipPlanes[1]);
            if (painter.enableZ) node.material.clippingPlanes.push(painter._clipPlanes[2]);
         }
      });

      this.updateMaterialSide(this.enableX || this.enableY || this.enableZ);

      if (!without_render) this.Render3D(0);
   }

   TGeoPainter.prototype.getGeomBox = function() {
      var extras = this.getExtrasContainer('collect');

      var box = this.getGeomBoundingBox(this._toplevel);

      if (extras)
         for (var k=0;k<extras.length;++k) this._toplevel.add(extras[k]);

      return box;
   }

   TGeoPainter.prototype.adjustCameraPosition = function(first_time) {

      if (!this._toplevel) return;

      var box = this.getGeomBoundingBox(this._toplevel);

      var sizex = box.max.x - box.min.x,
          sizey = box.max.y - box.min.y,
          sizez = box.max.z - box.min.z,
          midx = (box.max.x + box.min.x)/2,
          midy = (box.max.y + box.min.y)/2,
          midz = (box.max.z + box.min.z)/2;

      this._overall_size = 2 * Math.max(sizex, sizey, sizez);

      this._scene.fog.near = this._overall_size * 2;
      this._camera.near = this._overall_size / 350;
      this._scene.fog.far = this._overall_size * 12;
      this._camera.far = this._overall_size * 12;


      if (this._webgl) {
         this._ssaoPass.uniforms[ 'cameraNear' ].value = this._camera.near;//*this._nFactor;
         this._ssaoPass.uniforms[ 'cameraFar' ].value = this._camera.far;///this._nFactor;
      }

      if (first_time) {
         this.clipX = midx;
         this.clipY = midy;
         this.clipZ = midz;
      }

      if (this.options.ortho_camera) {
         this._camera.left = box.min.x;
         this._camera.right = box.max.x;
         this._camera.top = box.min.y;
         this._camera.bottom = box.max.y;
      }

      // this._camera.far = 100000000000;

      this._camera.updateProjectionMatrix();

      var k = 2*this.options.zoom;

      if (this.options.ortho_camera) {
         this._camera.position.set(0, 0, Math.max(sizey,sizez));
      } else if (this.options.project) {
         switch (this.options.project) {
            case 'x': this._camera.position.set(k*1.5*Math.max(sizey,sizez), 0, 0); break
            case 'y': this._camera.position.set(0, k*1.5*Math.max(sizex,sizez), 0); break
            case 'z': this._camera.position.set(0, 0, k*1.5*Math.max(sizex,sizey)); break
         }
      } else if (this.options._yup) {
         this._camera.position.set(midx-k*Math.max(sizex,sizez), midy+k*sizey, midz-k*Math.max(sizex,sizez));
      } else {
         this._camera.position.set(midx-k*Math.max(sizex,sizey), midy-k*Math.max(sizex,sizey), midz+k*sizez);
      }

      this._lookat = new THREE.Vector3(midx, midy, midz);
      this._camera.lookAt(this._lookat);

      this._pointLight.position.set(sizex/5, sizey/5, sizez/5);

      if (this._controls) {
         this._controls.target.copy(this._lookat);
         this._controls.update();
      }

      // recheck which elements to draw
      if (this.options.select_in_view)
         this.startDrawGeometry();
   }

   TGeoPainter.prototype.focusOnItem = function(itemname) {

      if (!itemname || !this._clones) return;

      var stack = this._clones.FindStackByName(itemname);

      if (!stack) return;

      var info = this._clones.ResolveStack(stack, true);

      this.focusCamera( info, false );
   }

   TGeoPainter.prototype.focusCamera = function( focus, clip ) {

      if (this.options.project) return this.adjustCameraPosition();

      var autoClip = clip === undefined ? false : clip;

      var box = new THREE.Box3();
      if (focus === undefined) {
         box = this.getGeomBoundingBox(this._toplevel);
      } else if (focus instanceof THREE.Mesh) {
         box.setFromObject(focus);
      } else {
         var center = new THREE.Vector3().setFromMatrixPosition(focus.matrix),
             node = focus.node,
             halfDelta = new THREE.Vector3( node.fDX, node.fDY, node.fDZ ).multiplyScalar(0.5);
         box.min = center.clone().sub(halfDelta);
         box.max = center.clone().add(halfDelta);
      }

      var sizex = box.max.x - box.min.x,
          sizey = box.max.y - box.min.y,
          sizez = box.max.z - box.min.z,
          midx = (box.max.x + box.min.x)/2,
          midy = (box.max.y + box.min.y)/2,
          midz = (box.max.z + box.min.z)/2;

      var position;
      if (this.options._yup)
         position = new THREE.Vector3(midx-2*Math.max(sizex,sizez), midy+2*sizey, midz-2*Math.max(sizex,sizez));
      else
         position = new THREE.Vector3(midx-2*Math.max(sizex,sizey), midy-2*Math.max(sizex,sizey), midz+2*sizez);

      var target = new THREE.Vector3(midx, midy, midz);
      //console.log("Zooming to x: " + target.x + " y: " + target.y + " z: " + target.z );


      // Find to points to animate "lookAt" between
      var dist = this._camera.position.distanceTo(target);
      var oldTarget = this._controls.target;

      var frames = 200;
      var step = 0;
      // Amount to change camera position at each step
      var posIncrement = position.sub(this._camera.position).divideScalar(frames);
      // Amount to change "lookAt" so it will end pointed at target
      var targetIncrement = target.sub(oldTarget).divideScalar(frames);
      // console.log( targetIncrement );

      // Automatic Clipping

      if (autoClip) {

         var topBox = this.getGeomBoundingBox(this._toplevel);

         this.clipX = this.enableX ? this.clipX : topBox.min.x;
         this.clipY = this.enableY ? this.clipY : topBox.min.y;
         this.clipZ = this.enableZ ? this.clipZ : topBox.min.z;

         this.enableX = this.enableY = this.enableZ = true;

         // These should be center of volume, box may not be doing this correctly
         var incrementX  = ((box.max.x + box.min.x) / 2 - this.clipX) / frames,
             incrementY  = ((box.max.y + box.min.y) / 2 - this.clipY) / frames,
             incrementZ  = ((box.max.z + box.min.z) / 2 - this.clipZ) / frames;

         this.updateClipping();
      }

      var painter = this;
      this._animating = true;

      // Interpolate //

      function animate() {
         if (painter._animating === undefined) return;

         if (painter._animating) {
            requestAnimationFrame( animate );
         } else {
            painter.startDrawGeometry();
         }
         var smoothFactor = -Math.cos( ( 2.0 * Math.PI * step ) / frames ) + 1.0;
         painter._camera.position.add( posIncrement.clone().multiplyScalar( smoothFactor ) );
         oldTarget.add( targetIncrement.clone().multiplyScalar( smoothFactor ) );
         painter._lookat = oldTarget;
         painter._camera.lookAt( painter._lookat );
         painter._camera.updateProjectionMatrix();
         if (autoClip) {
            painter.clipX += incrementX * smoothFactor;
            painter.clipY += incrementY * smoothFactor;
            painter.clipZ += incrementZ * smoothFactor;
            painter.updateClipping();
         } else {
            painter.Render3D();
         }
         step++;
         painter._animating = step < frames;
      }
      animate();

   //   this._controls.update();

   }

   TGeoPainter.prototype.autorotate = function(speed) {

      var rotSpeed = (speed === undefined) ? 2.0 : speed,
          painter = this, last = new Date();

      function animate() {
         if (!painter._renderer || !painter.options) return;

         var current = new Date();

         if ( painter.options.autoRotate ) requestAnimationFrame( animate );

         if (painter._controls) {
            painter._controls.autoRotate = painter.options.autoRotate;
            painter._controls.autoRotateSpeed = rotSpeed * ( current.getTime() - last.getTime() ) / 16.6666;
            painter._controls.update();
         }
         last = new Date();
         painter.Render3D(0);
      }

      if (this._webgl) animate();
   }

   TGeoPainter.prototype.completeScene = function() {

      if ( this.options._debug || this.options._grid ) {
         if ( this.options._full ) {
            var boxHelper = new THREE.BoxHelper(this._toplevel);
            this._scene.add( boxHelper );
         }
         this._scene.add( new THREE.AxisHelper( 2 * this._overall_size ) );
         this._scene.add( new THREE.GridHelper( Math.ceil( this._overall_size), Math.ceil( this._overall_size ) / 50 ) );
         this.helpText("<font face='verdana' size='1' color='red'><center>Transform Controls<br>" +
               "'T' translate | 'R' rotate | 'S' scale<br>" +
               "'+' increase size | '-' decrease size<br>" +
               "'W' toggle wireframe/solid display<br>"+
         "keep 'Ctrl' down to snap to grid</center></font>");
      }
   }


   TGeoPainter.prototype.drawCount = function(unqievis, clonetm) {

      var res = 'Unique nodes: ' + this._clones.nodes.length + '<br/>' +
                'Unique visible: ' + unqievis + '<br/>' +
                'Time to clone: ' + clonetm + 'ms <br/>';

      // need to fill cached value line numvischld
      this._clones.ScanVisible();

      var painter = this, nshapes = 0;

      var arg = {
         cnt: [],
         func: function(node) {
            if (this.cnt[this.last]===undefined)
               this.cnt[this.last] = 1;
            else
               this.cnt[this.last]++;

            nshapes += JSROOT.GEO.CountNumShapes(painter._clones.GetNodeShape(node.id));

            return true;
         }
      };

      var tm1 = new Date().getTime();
      var numvis = this._clones.ScanVisible(arg);
      var tm2 = new Date().getTime();

      res += 'Total visible nodes: ' + numvis + '<br/>';
      res += 'Total shapes: ' + nshapes + '<br/>';

      for (var lvl=0;lvl<arg.cnt.length;++lvl) {
         if (arg.cnt[lvl] !== undefined)
            res += ('  lvl' + lvl + ': ' + arg.cnt[lvl] + '<br/>');
      }

      res += "Time to scan: " + (tm2-tm1) + "ms <br/>";

      res += "<br/><br/>Check timing for matrix calculations ...<br/>";

      var elem = this.select_main().style('overflow', 'auto').html(res);

      setTimeout(function() {
         arg.domatrix = true;
         tm1 = new Date().getTime();
         numvis = painter._clones.ScanVisible(arg);
         tm2 = new Date().getTime();
         elem.append("p").text("Time to scan with matrix: " + (tm2-tm1) + "ms");
         painter.DrawingReady();
      }, 100);

      return this;
   }


   TGeoPainter.prototype.PerformDrop = function(obj, itemname, hitem, opt, call_back) {

      console.log('Calling perform drop');

      if (obj && (obj.$kind==='TTree')) {
         // drop tree means function call which must extract tracks from provided tree

         var funcname = "extract_geo_tracks";

         if (opt && opt.indexOf("$")>0) {
            funcname = opt.substr(0, opt.indexOf("$"));
            opt = opt.substr(opt.indexOf("$")+1);
         }

         var func = JSROOT.findFunction(funcname);

         if (!func) return JSROOT.CallBack(call_back);

         var geo_painter = this;

         return func(obj, opt, function(tracks) {
            if (tracks) {
               geo_painter.drawExtras(tracks, "", false); // FIXME: probably tracks should be remembered??
               geo_painter.Render3D(100);
            }
            JSROOT.CallBack(call_back); // finally callback
         });
      }

      if (this.drawExtras(obj, itemname)) {
         if (hitem) hitem._painter = this; // set for the browser item back pointer
      }

      JSROOT.CallBack(call_back);
   }

   TGeoPainter.prototype.MouseOverHierarchy = function(on, itemname, hitem) {
      // function called when mouse is going over the item in the browser

      if (!this.options) return; // protection for cleaned-up painter

      var obj = hitem._obj;
      if (this.options._debug)
         console.log('Mouse over', on, itemname, (obj ? obj._typename : "---"));

      // let's highlight tracks and hits only for the time being
      if (!obj || (obj._typename !== "TEveTrack" && obj._typename !== "TEvePointSet" && obj._typename !== "TPolyMarker3D")) return;

      this.HighlightMesh(null, 0x00ff00, on ? obj : null);
   }

   TGeoPainter.prototype.clearExtras = function() {
      this.getExtrasContainer("delete");
      delete this._extraObjects; // workaround, later will be normal function
      this.Render3D();
   }

   TGeoPainter.prototype.addExtra = function(obj, itemname) {

      // register extra objects like tracks or hits
      // Check if object already exists to prevent duplication

      if (this._extraObjects === undefined)
         this._extraObjects = JSROOT.Create("TList");

      if (this._extraObjects.arr.indexOf(obj)>=0) return false;

      this._extraObjects.Add(obj, itemname);

      delete obj.$hidden_via_menu; // remove previous hidden property

      return true;
   }

   TGeoPainter.prototype.ExtraObjectVisible = function(hpainter, hitem, toggle) {
      if (!this._extraObjects) return;

      var itemname = hpainter.itemFullName(hitem),
          indx = this._extraObjects.opt.indexOf(itemname);

      if ((indx<0) && hitem._obj) {
         indx = this._extraObjects.arr.indexOf(hitem._obj);
         // workaround - if object found, replace its name
         if (indx>=0) this._extraObjects.opt[indx] = itemname;
      }

      if (indx < 0) return;

      var obj = this._extraObjects.arr[indx],
          res = obj.$hidden_via_menu ? false : true;

      if (toggle) {
         obj.$hidden_via_menu = res; res = !res;

         var mesh = null;
         // either found painted object or just draw once again
         this._toplevel.traverse(function(node) { if (node.geo_object === obj) mesh = node; });

         if (mesh) mesh.visible = res; else
         if (res) this.drawExtras(obj, "", false);

         if (mesh || res) this.Render3D();
      }

      return res;
   }


   TGeoPainter.prototype.drawExtras = function(obj, itemname, add_objects) {
      if (!obj || obj._typename===undefined) return false;

      // if object was hidden via menu, do not redraw it with next draw call
      if (!add_objects && obj.$hidden_via_menu) return false;

      var isany = false, do_render = false;
      if (add_objects === undefined) {
         add_objects = true;
         do_render = true;
      }

      if ((obj._typename === "TList") || (obj._typename === "TObjArray")) {
         if (!obj.arr) return false;
         for (var n=0;n<obj.arr.length;++n) {
            var sobj = obj.arr[n];
            var sname = (itemname === undefined) ? obj.opt[n] : (itemname + "/[" + n + "]");
            if (this.drawExtras(sobj, sname, add_objects)) isany = true;
         }
      } else if (obj._typename === 'THREE.Mesh') {
         // adding mesh as is
         this.getExtrasContainer().add(obj);
         isany = true;
      } else if (obj._typename === 'TGeoTrack') {
         if (add_objects && !this.addExtra(obj, itemname)) return false;
         isany = this.drawGeoTrack(obj, itemname);
      } else if ((obj._typename === 'TEveTrack') || (obj._typename === 'ROOT::Experimental::TEveTrack')) {
         if (add_objects && !this.addExtra(obj, itemname)) return false;
         isany = this.drawEveTrack(obj, itemname);
      } else if ((obj._typename === 'TEvePointSet') || (obj._typename === "ROOT::Experimental::TEvePointSet") || (obj._typename === "TPolyMarker3D")) {
         if (add_objects && !this.addExtra(obj, itemname)) return false;
         isany = this.drawHit(obj, itemname);
      } else if ((obj._typename === "TEveGeoShapeExtract") || (obj._typename === "ROOT::Experimental::TEveGeoShapeExtract")) {
         if (add_objects && !this.addExtra(obj, itemname)) return false;
         isany = this.drawExtraShape(obj, itemname);
      }

      if (isany && do_render)
         this.Render3D(100);

      return isany;
   }

   TGeoPainter.prototype.getExtrasContainer = function(action, name) {
      if (!this._toplevel) return null;

      if (!name) name = "tracks";

      var extras = null, lst = [];
      for (var n=0;n<this._toplevel.children.length;++n) {
         var chld = this._toplevel.children[n];
         if (!chld._extras) continue;
         if (action==='collect') { lst.push(chld); continue; }
         if (chld._extras === name) { extras = chld; break; }
      }

      if (action==='collect') {
         for (var k=0;k<lst.length;++k) this._toplevel.remove(lst[k]);
         return lst;
      }

      if (action==="delete") {
         if (extras) this._toplevel.remove(extras);
         JSROOT.Painter.DisposeThreejsObject(extras);
         return null;
      }

      if ((action!=="get") && !extras) {
         extras = new THREE.Object3D();
         extras._extras = name;
         this._toplevel.add(extras);
      }

      return extras;
   }

   TGeoPainter.prototype.drawGeoTrack = function(track, itemname) {
      if (!track || !track.fNpoints) return false;

      var track_width = track.fLineWidth || 1,
          track_color = JSROOT.Painter.root_colors[track.fLineColor] || "rgb(255,0,255)";

      if (JSROOT.browser.isWin) track_width = 1; // not supported on windows

      var npoints = Math.round(track.fNpoints/4),
          buf = new Float32Array((npoints-1)*6),
          pos = 0, projv = this.options.projectPos,
          projx = (this.options.project === "x"),
          projy = (this.options.project === "y"),
          projz = (this.options.project === "z");

      for (var k=0;k<npoints-1;++k) {
         buf[pos]   = projx ? projv : track.fPoints[k*4];
         buf[pos+1] = projy ? projv : track.fPoints[k*4+1];
         buf[pos+2] = projz ? projv : track.fPoints[k*4+2];
         buf[pos+3] = projx ? projv : track.fPoints[k*4+4];
         buf[pos+4] = projy ? projv : track.fPoints[k*4+5];
         buf[pos+5] = projz ? projv : track.fPoints[k*4+6];
         pos+=6;
      }

      var lineMaterial = new THREE.LineBasicMaterial({ color: track_color, linewidth: track_width }),
          line = JSROOT.Painter.createLineSegments(buf, lineMaterial);

      line.geo_name = itemname;
      line.geo_object = track;
      if (!JSROOT.browser.isWin) {
         line.hightlightLineWidth = track_width*3;
         line.normalLineWidth = track_width;
      }

      this.getExtrasContainer().add(line);

      return true;
   }

   TGeoPainter.prototype.drawEveTrack = function(track, itemname) {
      if (!track || (track.fN <= 0)) return false;

      var track_width = track.fLineWidth || 1,
          track_color = JSROOT.Painter.root_colors[track.fLineColor] || "rgb(255,0,255)";

      if (JSROOT.browser.isWin) track_width = 1; // not supported on windows

      var buf = new Float32Array((track.fN-1)*6), pos = 0,
          projv = this.options.projectPos,
          projx = (this.options.project === "x"),
          projy = (this.options.project === "y"),
          projz = (this.options.project === "z");

      for (var k=0;k<track.fN-1;++k) {
         buf[pos]   = projx ? projv : track.fP[k*3];
         buf[pos+1] = projy ? projv : track.fP[k*3+1];
         buf[pos+2] = projz ? projv : track.fP[k*3+2];
         buf[pos+3] = projx ? projv : track.fP[k*3+3];
         buf[pos+4] = projy ? projv : track.fP[k*3+4];
         buf[pos+5] = projz ? projv : track.fP[k*3+5];
         pos+=6;
      }

      var lineMaterial = new THREE.LineBasicMaterial({ color: track_color, linewidth: track_width }),
          line = JSROOT.Painter.createLineSegments(buf, lineMaterial);

      line.geo_name = itemname;
      line.geo_object = track;
      if (!JSROOT.browser.isWin) {
         line.hightlightLineWidth = track_width*3;
         line.normalLineWidth = track_width;
      }

      this.getExtrasContainer().add(line);

      return true;
   }

   TGeoPainter.prototype.drawHit = function(hit, itemname) {
      if (!hit || !hit.fN || (hit.fN < 0)) return false;

      var hit_size = 8*hit.fMarkerSize,
          size = hit.fN,
          projv = this.options.projectPos,
          projx = (this.options.project === "x"),
          projy = (this.options.project === "y"),
          projz = (this.options.project === "z"),
          pnts = new JSROOT.Painter.PointsCreator(size, this._webgl, hit_size);

      for (var i=0;i<size;i++)
         pnts.AddPoint(projx ? projv : hit.fP[i*3],
                       projy ? projv : hit.fP[i*3+1],
                       projz ? projv : hit.fP[i*3+2]);

      var mesh = pnts.CreatePoints(JSROOT.Painter.root_colors[hit.fMarkerColor] || "rgb(0,0,255)");

      mesh.highlightMarkerSize = hit_size*3;
      mesh.normalMarkerSize = hit_size;

      mesh.geo_name = itemname;
      mesh.geo_object = hit;

      this.getExtrasContainer().add(mesh);

      return true;
   }

   TGeoPainter.prototype.drawExtraShape = function(obj, itemname) {
      var toplevel = JSROOT.GEO.build(obj);
      if (!toplevel) return false;

      toplevel.geo_name = itemname;
      toplevel.geo_object = obj;

      this.getExtrasContainer().add(toplevel);
      return true;
   }

   TGeoPainter.prototype.FindNodeWithVolume = function(name, action, prnt, itemname, volumes) {

      var first_level = false, res = null;

      if (!prnt) {
         prnt = this.GetGeometry();
         if (!prnt && (JSROOT.GEO.NodeKind(prnt)!==0)) return null;
         itemname = this.geo_manager ? prnt.fName : "";
         first_level = true;
         volumes = [];
      } else {
         if (itemname.length>0) itemname += "/";
         itemname += prnt.fName;
      }

      if (!prnt.fVolume || prnt.fVolume._searched) return null;

      if (name.test(prnt.fVolume.fName)) {
         res = action({ node: prnt, item: itemname });
         if (res) return res;
      }

      prnt.fVolume._searched = true;
      volumes.push(prnt.fVolume);

      if (prnt.fVolume.fNodes)
         for (var n=0;n<prnt.fVolume.fNodes.arr.length;++n) {
            res = this.FindNodeWithVolume(name, action, prnt.fVolume.fNodes.arr[n], itemname, volumes);
            if (res) break;
         }

      if (first_level)
         for (var n=0, len=volumes.length; n<len; ++n)
            delete volumes[n]._searched;

      return res;
   }

   TGeoPainter.prototype.SetRootDefaultColors = function() {
      // set default colors like TGeoManager::DefaultColors() does

      var dflt = { kWhite:0,   kBlack:1,   kGray:920,
                     kRed:632, kGreen:416, kBlue:600, kYellow:400, kMagenta:616, kCyan:432,
                     kOrange:800, kSpring:820, kTeal:840, kAzure:860, kViolet:880, kPink:900 };

      var nmax = 110, col = [];
      for (var i=0;i<nmax;i++) col.push(dflt.kGray);

      //here we should create a new TColor with the same rgb as in the default
      //ROOT colors used below
      col[ 3] = dflt.kYellow-10;
      col[ 4] = col[ 5] = dflt.kGreen-10;
      col[ 6] = col[ 7] = dflt.kBlue-7;
      col[ 8] = col[ 9] = dflt.kMagenta-3;
      col[10] = col[11] = dflt.kRed-10;
      col[12] = dflt.kGray+1;
      col[13] = dflt.kBlue-10;
      col[14] = dflt.kOrange+7;
      col[16] = dflt.kYellow+1;
      col[20] = dflt.kYellow-10;
      col[24] = col[25] = col[26] = dflt.kBlue-8;
      col[29] = dflt.kOrange+9;
      col[79] = dflt.kOrange-2;

      var name = { test:function() { return true; }} // select all volumes

      this.FindNodeWithVolume(name, function(arg) {
         var vol = arg.node.fVolume;
         var med = vol.fMedium;
         if (!med) return null;
         var mat = med.fMaterial;
         var matZ = Math.round(mat.fZ);
         vol.fLineColor = col[matZ];
         if (mat.fDensity<0.1) mat.fFillStyle = 3000+60; // vol.SetTransparency(60)
      });
   }


   TGeoPainter.prototype.checkScript = function(script_name, call_back) {

      var painter = this, draw_obj = this.GetGeometry(), name_prefix = "";

      if (this.geo_manager) name_prefix = draw_obj.fName;

      if (!script_name || (script_name.length<3) || (JSROOT.GEO.NodeKind(draw_obj)!==0))
         return JSROOT.CallBack(call_back, draw_obj, name_prefix);

      var mgr = {
            GetVolume: function (name) {
               var regexp = new RegExp("^"+name+"$");
               var currnode = painter.FindNodeWithVolume(regexp, function(arg) { return arg; } );

               if (!currnode) console.log('Did not found '+name + ' volume');

               // return proxy object with several methods, typically used in ROOT geom scripts
               return {
                   found: currnode,
                   fVolume: currnode ? currnode.node.fVolume : null,
                   InvisibleAll: function(flag) {
                      JSROOT.GEO.InvisibleAll.call(this.fVolume, flag);
                   },
                   Draw: function() {
                      if (!this.found || !this.fVolume) return;
                      draw_obj = this.found.node;
                      name_prefix = this.found.item;
                      console.log('Select volume for drawing', this.fVolume.fName, name_prefix);
                   },
                   SetTransparency: function(lvl) {
                     if (this.fVolume && this.fVolume.fMedium && this.fVolume.fMedium.fMaterial)
                        this.fVolume.fMedium.fMaterial.fFillStyle = 3000+lvl;
                   },
                   SetLineColor: function(col) {
                      if (this.fVolume) this.fVolume.fLineColor = col;
                   }
                };
            },

            DefaultColors: function() {
               painter.SetRootDefaultColors();
            },

            SetMaxVisNodes: function(limit) {
               console.log('Automatic visible depth for ' + limit + ' nodes');
               if (limit>0) painter.options.maxnodeslimit = limit;
            }
          };

      JSROOT.progress('Loading macro ' + script_name);

      JSROOT.NewHttpRequest(script_name, "text", function(res) {
         if (!res || (res.length==0))
            return JSROOT.CallBack(call_back, draw_obj, name_prefix);

         var lines = res.split('\n');

         ProcessNextLine(0);

         function ProcessNextLine(indx) {

            var first_tm = new Date().getTime();
            while (indx < lines.length) {
               var line = lines[indx++].trim();

               if (line.indexOf('//')==0) continue;

               if (line.indexOf('gGeoManager')<0) continue;
               line = line.replace('->GetVolume','.GetVolume');
               line = line.replace('->InvisibleAll','.InvisibleAll');
               line = line.replace('->SetMaxVisNodes','.SetMaxVisNodes');
               line = line.replace('->DefaultColors','.DefaultColors');
               line = line.replace('->Draw','.Draw');
               line = line.replace('->SetTransparency','.SetTransparency');
               line = line.replace('->SetLineColor','.SetLineColor');
               if (line.indexOf('->')>=0) continue;

               // console.log(line);

               try {
                  var func = new Function('gGeoManager',line);
                  func(mgr);
               } catch(err) {
                  console.error('Problem by processing ' + line);
               }

               var now = new Date().getTime();
               if (now - first_tm > 300) {
                  JSROOT.progress('exec ' + line);
                  return setTimeout(ProcessNextLine.bind(this,indx),1);
               }
            }

            JSROOT.CallBack(call_back, draw_obj, name_prefix);
         }

      }).send();
   }

   TGeoPainter.prototype.prepareObjectDraw = function(draw_obj, name_prefix) {

      // first delete clones (if any)
      delete this._clones;

      if (this._main_painter) {

         this._clones_owner = false;

         this._clones = this._main_painter._clones;

         console.log('Reuse clones', this._clones.nodes.length, 'from main painter');

      } else if (!draw_obj) {

         this._clones_owner = false;

         this._clones = null;

      } else {

         this._start_drawing_time = new Date().getTime();

         this._clones_owner = true;

         this._clones = new JSROOT.GEO.ClonedNodes(draw_obj);

         this._clones.name_prefix = name_prefix;

         var uniquevis = this._clones.MarkVisisble(true);
         if (uniquevis <= 0)
            uniquevis = this._clones.MarkVisisble(false);
         else
            uniquevis = this._clones.MarkVisisble(true, true); // copy bits once and use normal visibility bits

         var spent = new Date().getTime() - this._start_drawing_time;

         console.log('Creating clones', this._clones.nodes.length, 'takes', spent, 'uniquevis', uniquevis);

         if (this.options._count)
            return this.drawCount(uniquevis, spent);
      }

      if (!this._scene) {

         // this is limit for the visible faces, number of volumes does not matter
         this.options.maxlimit = (this._webgl ? 200000 : 100000) * this.options.more;

         this._first_drawing = true;

         // activate worker
         if (this.options.use_worker > 0) this.startWorker();

         var size = this.size_for_3d(this._usesvg ? 3 : undefined);

         this._fit_main_area = (size.can3d === -1);

         this.createScene(this._usesvg, this._webgl, size.width, size.height);

         this.add_3d_canvas(size, this._renderer.domElement);

         // set top painter only when first child exists
         this.AccessTopPainter(true);
      }

      this.CreateToolbar();

      if (this._clones) {
         this.showDrawInfo("Drawing geometry");
         this.startDrawGeometry(true);
      } else {
         this.completeDraw();
      }
   }

   TGeoPainter.prototype.showDrawInfo = function(msg) {
      // methods show info when first geometry drawing is performed

      if (!this._first_drawing || !this._start_drawing_time) return;

      var main = this._renderer.domElement.parentNode,
          info = d3.select(main).select(".geo_info");

      if (!msg) {
         info.remove();
      } else {
         var spent = (new Date().getTime() - this._start_drawing_time)*1e-3;
         if (info.empty()) info = d3.select(main).append("p").attr("class","geo_info");
         info.html(msg + ", " + spent.toFixed(1) + "s");
      }

   }

   TGeoPainter.prototype.continueDraw = function() {

      // nothing to do - exit
      if (this.drawing_stage === 0) return;

      var tm0 = new Date().getTime(),
          interval = this._first_drawing ? 1000 : 200,
          now = tm0;

      while(true) {

         var res = this.nextDrawAction();

         if (!res) break;

         now = new Date().getTime();

         // stop creation after 100 sec, render as is
         if (now - this._startm > 1e5) {
            this.drawing_stage = 0;
            break;
         }

         // if we are that fast, do next action
         if ((res === true) && (now - tm0 < interval)) continue;

         if ((now - tm0 > interval) || (res === 1) || (res === 2)) {

            JSROOT.progress(this.drawing_log);

            this.showDrawInfo(this.drawing_log);

            if (this._first_drawing && this._webgl && (this._num_meshes - this._last_render_meshes > 100) && (now - this._last_render_tm > 2.5*interval)) {
               this.adjustCameraPosition();
               this.Render3D(-1);
               this._last_render_tm = new Date().getTime();
               this._last_render_meshes = this._num_meshes;
            }
            if (res !== 2) setTimeout(this.continueDraw.bind(this), (res === 1) ? 100 : 1);

            return;
         }
      }

      var take_time = now - this._startm;

      if (this._first_drawing)
         JSROOT.console('Create tm = ' + take_time + ' meshes ' + this._num_meshes + ' faces ' + this._num_faces);

      if (take_time > 300) {
         JSROOT.progress('Rendering geometry');
         this.showDrawInfo("Rendering");
         return setTimeout(this.completeDraw.bind(this, true), 10);
      }

      this.completeDraw(true);
   }

   TGeoPainter.prototype.TestCameraPosition = function() {

      this._camera.updateMatrixWorld();
      var origin = this._camera.position.clone();

      if (this._last_camera_position) {
         // if camera position does not changed a lot, ignore such change
         var dist = this._last_camera_position.distanceTo(origin);
         if (dist < (this._overall_size || 1000)/1e4) return;
      }

      this._last_camera_position = origin; // remember current camera position

      if (this._webgl)
         JSROOT.GEO.produceRenderOrder(this._toplevel, origin, this.options.depthMethod, this._clones);
   }

   TGeoPainter.prototype.Render3D = function(tmout, measure) {
      // call 3D rendering of the geometry drawing
      // tmout specifies delay, after which actual rendering will be invoked
      // Timeout used to avoid multiple rendering of the picture when several 3D drawings
      // superimposed with each other.
      // If tmeout<=0, rendering performed immediately
      // Several special values are used:
      //   -2222 - rendering performed only if there were previous calls, which causes timeout activation

      if (!this._renderer) {
         console.warn('renderer object not exists - check code');
         return;
      }

      if (tmout === undefined) tmout = 5; // by default, rendering happens with timeout

      if ((tmout <= 0) || this._usesvg) {
         if ('render_tmout' in this) {
            clearTimeout(this.render_tmout);
         } else {
            if (tmout === -2222) return; // special case to check if rendering timeout was active
         }

         var tm1 = new Date();

         if (typeof this.TestAxisVisibility === 'function')
            this.TestAxisVisibility(this._camera, this._toplevel);

         this.TestCameraPosition();

         // do rendering, most consuming time
         if (this._webgl && this._enableSSAO) {
            this._scene.overrideMaterial = this._depthMaterial;
        //    this._renderer.logarithmicDepthBuffer = false;
            this._renderer.render(this._scene, this._camera, this._depthRenderTarget, true);
            this._scene.overrideMaterial = null;
            this._effectComposer.render();
         } else {
       //     this._renderer.logarithmicDepthBuffer = true;
            this._renderer.render(this._scene, this._camera);
         }

         var tm2 = new Date();

         this.last_render_tm = tm2.getTime() - tm1.getTime();

         delete this.render_tmout;

         if ((this.first_render_tm === 0) && measure) {
            this.first_render_tm = this.last_render_tm;
            JSROOT.console('First render tm = ' + this.first_render_tm);
         }

         // when using SVGrenderer producing text output, provide result
         if (this._renderer.workaround_id !== undefined)
            JSROOT.svg_workaround[this._renderer.workaround_id] = this._renderer.outerHTML;

         return;
      }

      // do not shoot timeout many times
      if (!this.render_tmout)
         this.render_tmout = setTimeout(this.Render3D.bind(this,0,measure), tmout);
   }


   TGeoPainter.prototype.startWorker = function() {

      if (this._worker) return;

      this._worker_ready = false;
      this._worker_jobs = 0; // counter how many requests send to worker

      var pthis = this;

      this._worker = new Worker(JSROOT.source_dir + "scripts/JSRootGeoWorker.js");

      this._worker.onmessage = function(e) {

         if (typeof e.data !== 'object') return;

         if ('log' in e.data)
            return JSROOT.console('geo: ' + e.data.log);

         if ('progress' in e.data)
            return JSROOT.progress(e.data.progress);

         e.data.tm3 = new Date().getTime();

         if ('init' in e.data) {
            pthis._worker_ready = true;
            return JSROOT.console('Worker ready: ' + (e.data.tm3 - e.data.tm0));
         }

         pthis.processWorkerReply(e.data);
      };

      // send initialization message with clones
      this._worker.postMessage( { init: true, browser: JSROOT.browser, tm0: new Date().getTime(), clones: this._clones.nodes, sortmap: this._clones.sortmap  } );
   }

   TGeoPainter.prototype.canSubmitToWorker = function(force) {
      if (!this._worker) return false;

      return this._worker_ready && ((this._worker_jobs == 0) || force);
   }

   TGeoPainter.prototype.submitToWorker = function(job) {
      if (!this._worker) return false;

      this._worker_jobs++;

      job.tm0 = new Date().getTime();

      this._worker.postMessage(job);
   }

   TGeoPainter.prototype.processWorkerReply = function(job) {
      this._worker_jobs--;

      if ('collect' in job) {
         this._new_draw_nodes = job.new_nodes;
         this._draw_all_nodes = job.complete;
         this.drawing_stage = 3;
         // invoke methods immediately
         return this.continueDraw();
      }

      if ('shapes' in job) {

         for (var n=0;n<job.shapes.length;++n) {
            var item = job.shapes[n],
                origin = this._build_shapes[n];

            // var shape = this._clones.GetNodeShape(item.nodeid);

            if (item.buf_pos && item.buf_norm) {
               if (item.buf_pos.length === 0) {
                  origin.geom = null;
               } else if (item.buf_pos.length !== item.buf_norm.length) {
                  console.error('item.buf_pos',item.buf_pos.length, 'item.buf_norm', item.buf_norm.length);
                  origin.geom = null;
               } else {
                  origin.geom = new THREE.BufferGeometry();

                  origin.geom.addAttribute( 'position', new THREE.BufferAttribute( item.buf_pos, 3 ) );
                  origin.geom.addAttribute( 'normal', new THREE.BufferAttribute( item.buf_norm, 3 ) );
               }

               origin.ready = true;
               origin.nfaces = item.nfaces;
            }
         }

         job.tm4 = new Date().getTime();

         // console.log('Get reply from worker', job.tm3-job.tm2, ' decode json in ', job.tm4-job.tm3);

         this.drawing_stage = 7; // first check which shapes are used, than build meshes

         // invoke methods immediately
         return this.continueDraw();
      }
   }

   TGeoPainter.prototype.testGeomChanges = function() {
      if (this._main_painter) {
         console.warn('Get testGeomChanges call for slave painter');
         return this._main_painter.testGeomChanges();
      }
      this.startDrawGeometry();
      for (var k=0;k<this._slave_painters.length;++k)
         this._slave_painters[k].startDrawGeometry();
   }

   TGeoPainter.prototype.drawSimpleAxis = function() {

      var box = this.getGeomBoundingBox(this._toplevel);

      this.getExtrasContainer('delete', 'axis');
      var container = this.getExtrasContainer('create', 'axis');

      var text_size = 0.02 * Math.max( (box.max.x - box.min.x), (box.max.y - box.min.y), (box.max.z - box.min.z)),
          names = ['x','y','z'], center = [0,0,0];

      if (this.options._axis_center)
         for (var naxis=0;naxis<3;++naxis) {
            var name = names[naxis];
            if ((box.min[name]<=0) && (box.max[name]>=0)) continue;
            center[naxis] = (box.min[name] + box.max[name])/2;
         }

      for (var naxis=0;naxis<3;++naxis) {

         var buf = new Float32Array(6), axiscol, name = names[naxis];

         function Convert(value) {
            var range = box.max[name] - box.min[name];
            if (range<2) return value.toFixed(3);
            if (Math.abs(value)>1e5) return value.toExponential(3);
            return Math.round(value).toString();
         }

         var lbl = Convert(box.max[name]);

         buf[0] = box.min.x;
         buf[1] = box.min.y;
         buf[2] = box.min.z;

         buf[3] = box.min.x;
         buf[4] = box.min.y;
         buf[5] = box.min.z;

         switch (naxis) {
           case 0: buf[3] = box.max.x; axiscol = "red"; if (this.options._yup) lbl = "X "+lbl; else lbl+=" X"; break;
           case 1: buf[4] = box.max.y; axiscol = "green"; if (this.options._yup) lbl+=" Y"; else lbl = "Y " + lbl; break;
           case 2: buf[5] = box.max.z; axiscol = "blue"; lbl += " Z"; break;
         }

         if (this.options._axis_center)
            for (var k=0;k<6;++k)
               if ((k % 3) !== naxis) buf[k] = center[k%3];

         var lineMaterial = new THREE.LineBasicMaterial({ color: axiscol }),
             mesh = JSROOT.Painter.createLineSegments(buf, lineMaterial);

         container.add(mesh);

         var textMaterial = new THREE.MeshBasicMaterial({ color: axiscol });

         if ((center[naxis]===0) && (center[naxis]>=box.min[name]) && (center[naxis]<=box.max[name]))
           if (!this.options._axis_center || (naxis===0)) {
               var geom = new THREE.SphereBufferGeometry(text_size*0.25);
               mesh = new THREE.Mesh(geom, textMaterial);
               mesh.translateX((naxis===0) ? center[0] : buf[0]);
               mesh.translateY((naxis===1) ? center[1] : buf[1]);
               mesh.translateZ((naxis===2) ? center[2] : buf[2]);
               container.add(mesh);
           }

         var text3d = new THREE.TextGeometry(lbl, { font: JSROOT.threejs_font_helvetiker_regular, size: text_size, height: 0, curveSegments: 5 });
         mesh = new THREE.Mesh(text3d, textMaterial);
         var textbox = new THREE.Box3().setFromObject(mesh);

         mesh.translateX(buf[3]);
         mesh.translateY(buf[4]);
         mesh.translateZ(buf[5]);

         if (this.options._yup) {
            switch (naxis) {
               case 0: mesh.rotateY(Math.PI); mesh.translateX(-textbox.max.x - text_size*0.5); mesh.translateY(-textbox.max.y/2);  break;
               case 1: mesh.rotateX(-Math.PI/2); mesh.rotateY(-Math.PI/2); mesh.translateX(text_size*0.5); mesh.translateY(-textbox.max.y/2); break;
               case 2: mesh.rotateY(-Math.PI/2); mesh.translateX(text_size*0.5); mesh.translateY(-textbox.max.y/2); break;
           }
         } else {
            switch (naxis) {
               case 0: mesh.rotateX(Math.PI/2); mesh.translateY(-textbox.max.y/2); mesh.translateX(text_size*0.5); break;
               case 1: mesh.rotateX(Math.PI/2); mesh.rotateY(-Math.PI/2); mesh.translateX(-textbox.max.x-text_size*0.5); mesh.translateY(-textbox.max.y/2); break;
               case 2: mesh.rotateX(Math.PI/2); mesh.rotateZ(Math.PI/2); mesh.translateX(text_size*0.5); mesh.translateY(-textbox.max.y/2); break;
            }
         }

         container.add(mesh);

         text3d = new THREE.TextGeometry(Convert(box.min[name]), { font: JSROOT.threejs_font_helvetiker_regular, size: text_size, height: 0, curveSegments: 5 });

         mesh = new THREE.Mesh(text3d, textMaterial);
         textbox = new THREE.Box3().setFromObject(mesh);

         mesh.translateX(buf[0]);
         mesh.translateY(buf[1]);
         mesh.translateZ(buf[2]);

         if (this.options._yup) {
            switch (naxis) {
               case 0: mesh.rotateY(Math.PI); mesh.translateX(text_size*0.5); mesh.translateY(-textbox.max.y/2); break;
               case 1: mesh.rotateX(-Math.PI/2); mesh.rotateY(-Math.PI/2); mesh.translateY(-textbox.max.y/2); mesh.translateX(-textbox.max.x-text_size*0.5); break;
               case 2: mesh.rotateY(-Math.PI/2);  mesh.translateX(-textbox.max.x-text_size*0.5); mesh.translateY(-textbox.max.y/2); break;
            }
         } else {
            switch (naxis) {
               case 0: mesh.rotateX(Math.PI/2); mesh.translateX(-textbox.max.x-text_size*0.5); mesh.translateY(-textbox.max.y/2); break;
               case 1: mesh.rotateX(Math.PI/2); mesh.rotateY(-Math.PI/2); mesh.translateY(-textbox.max.y/2); mesh.translateX(text_size*0.5); break;
               case 2: mesh.rotateX(Math.PI/2); mesh.rotateZ(Math.PI/2);  mesh.translateX(-textbox.max.x-text_size*0.5); mesh.translateY(-textbox.max.y/2); break;
            }
         }

         container.add(mesh);
      }

      this.TestAxisVisibility = function(camera, toplevel) {
         if (!camera) {
            this.getExtrasContainer('delete', 'axis');
            delete this.TestAxisVisibility;
            this.Render3D();
            return;
         }
      }
   }

   TGeoPainter.prototype.toggleAxisDraw = function(force_draw) {
      if (this.TestAxisVisibility) {
         if (!force_draw)
           this.TestAxisVisibility(null, this._toplevel);
      } else {
         this.drawSimpleAxis();
      }
   }

   TGeoPainter.prototype.completeDraw = function(close_progress) {

      var first_time = false, check_extras = true;

      if (!this.options) {
         console.warn('options object does not exist in completeDraw - something went wrong');
         return;
      }

      if (!this._clones) {
         check_extras = false;
         // if extra object where append, redraw them at the end
         this.getExtrasContainer("delete"); // delete old container
         var extras = (this._main_painter ? this._main_painter._extraObjects : null) || this._extraObjects;
         this.drawExtras(extras, "", false);
      }

      if (this._first_drawing) {
         this.adjustCameraPosition(true);
         this.showDrawInfo();
         this._first_drawing = false;
         first_time = true;

         if (this._webgl) {
            this.enableX = this.options.clipx;
            this.enableY = this.options.clipy;
            this.enableZ = this.options.clipz;
            this.updateClipping(true); // only set clip panels, do not render
         }
         if (this.options.tracks && this.geo_manager && this.geo_manager.fTracks)
            this.addExtra(this.geo_manager.fTracks, "<prnt>/Tracks");
      }

      if (this.options.transparency!==0)
         this.changeGlobalTransparency(this.options.transparency, true);

      if (first_time) {
         this.completeScene();
         if (this.options._axis) this.toggleAxisDraw(true);
      }

      this._scene.overrideMaterial = null;

      if (check_extras) {
         // if extra object where append, redraw them at the end
         this.getExtrasContainer("delete"); // delete old container
         var extras = (this._main_painter ? this._main_painter._extraObjects : null) || this._extraObjects;
         this.drawExtras(extras, "", false);
      }

      this.Render3D(0, true);

      if (close_progress) JSROOT.progress();

      this.addOrbitControls();

      this.addTransformControl();

      if (first_time) {

         // after first draw check if highlight can be enabled
         if (this.options.highlight === false)
            this.options.highlight = (this.first_render_tm < 1000);

         // also highlight of scene object can be assigned at the first draw
         if (this.options.highlight_scene === false)
            this.options.highlight_scene = this.options.highlight;

         // if rotation was enabled, do it
         if (this._webgl && this.options.autoRotate && !this.options.project) this.autorotate(2.5);
         if (!this._usesvg && this.options.show_controls) this.showControlOptions(true);
      }

      this.DrawingReady();

      if (this._draw_nodes_again)
         return this.startDrawGeometry(); // relaunch drawing

      this._drawing_ready = true; // indicate that drawing is completed
   }

   TGeoPainter.prototype.Cleanup = function(first_time) {

      if (!first_time) {

         this.AccessTopPainter(false); // remove as pointer

         this.helpText();

         JSROOT.Painter.DisposeThreejsObject(this._scene);

         JSROOT.Painter.DisposeThreejsObject(this._full_geom);

         if (this._tcontrols)
            this._tcontrols.dispose();

         if (this._controls)
            this._controls.Cleanup();

         if (this._context_menu)
            this._renderer.domElement.removeEventListener( 'contextmenu', this._context_menu, false );

         if (this._datgui)
            this._datgui.destroy();

         if (this._worker) this._worker.terminate();

         JSROOT.TObjectPainter.prototype.Cleanup.call(this);

         delete this.options;

         delete this._animating;

         var obj = this.GetGeometry();
         if (obj && this.options.is_main) {
            if (obj.$geo_painter===this) delete obj.$geo_painter; else
            if (obj.fVolume && obj.fVolume.$geo_painter===this) delete obj.fVolume.$geo_painter;
         }

         if (this._main_painter) {
            var pos = this._main_painter._slave_painters.indexOf(this);
            if (pos>=0) this._main_painter._slave_painters.splice(pos,1);
         }

         for (var k=0;k<this._slave_painters.length;++k) {
            var slave = this._slave_painters[k];
            if (slave && (slave._main_painter===this)) slave._main_painter = null;
         }
      }

      for (var k in this._slave_painters) {
         var slave = this._slave_painters[k];
         slave._main_painter = null;
         if (slave._clones === this._clones) slave._clones = null;
      }

      this._main_painter = null;
      this._slave_painters = [];

      if (this._renderer) {
         if (this._renderer.dispose) this._renderer.dispose();
         if (this._renderer.context) delete this._renderer.context;
      }

      delete this._scene;
      this._scene_width = 0;
      this._scene_height = 0;
      this._renderer = null;
      this._toplevel = null;
      this._full_geom = null;
      this._camera = null;
      this._selected_mesh = null;

      if (this._clones && this._clones_owner) this._clones.Cleanup(this._draw_nodes, this._build_shapes);
      delete this._clones;
      delete this._clones_owner;
      delete this._draw_nodes;
      delete this._drawing_ready;
      delete this._build_shapes;
      delete this._new_draw_nodes;

      this.first_render_tm = 0;
      this.last_render_tm = 2000;

      this.drawing_stage = 0;
      delete this.drawing_log;

      delete this._datgui;
      delete this._controls;
      delete this._context_menu;
      delete this._tcontrols;
      delete this._toolbar;

      delete this._worker;
   }

   TGeoPainter.prototype.helpText = function(msg) {
      JSROOT.progress(msg);
   }

   TGeoPainter.prototype.CheckResize = function(size) {
      var pad_painter = this.canv_painter();


      // firefox is the only browser which correctly supports resize of embedded canvas,
      // for others we should force canvas redrawing at every step
      if (pad_painter)
         if (!pad_painter.CheckCanvasResize(size)) return false;

      var sz = this.size_for_3d();

      if ((this._scene_width === sz.width) && (this._scene_height === sz.height)) return false;
      if ((sz.width<10) || (sz.height<10)) return;

      this._scene_width = sz.width;
      this._scene_height = sz.height;

      if ( this._camera.type == "OrthographicCamera")
      {
         this._camera.left = -sz.width;
         this._camera.right = sz.width;
         this._camera.top = -sz.height;
         this._camera.bottom = sz.height;
      }
      else {
         this._camera.aspect = this._scene_width / this._scene_height;
      }
      this._camera.updateProjectionMatrix();   
      this._renderer.setSize( this._scene_width, this._scene_height, !this._fit_main_area );

      this.Render3D();

      return true;
   }

   TGeoPainter.prototype.ToggleEnlarge = function() {

      if (d3.event) {
         d3.event.preventDefault();
         d3.event.stopPropagation();
      }

      if (this.enlarge_main('toggle'))
        this.CheckResize();
   }


   TGeoPainter.prototype.ownedByTransformControls = function(child) {
      var obj = child.parent;
      while (obj && !(obj instanceof THREE.TransformControls) ) {
         obj = obj.parent;
      }
      return (obj && (obj instanceof THREE.TransformControls));
   }

   TGeoPainter.prototype.accessObjectWireFrame = function(obj, on) {
      // either change mesh wireframe or return current value
      // return undefined when wireframe cannot be accessed

      if (!obj.hasOwnProperty("material") || (obj instanceof THREE.GridHelper)) return;

      if (this.ownedByTransformControls(obj)) return;

      if ((on !== undefined) && obj.stack)
         obj.material.wireframe = on;

      return obj.material.wireframe;
   }


   TGeoPainter.prototype.changeWireFrame = function(obj, on) {
      var painter = this;

      obj.traverse(function(obj2) { painter.accessObjectWireFrame(obj2, on); });

      this.Render3D();
   }

   JSROOT.Painter.CreateGeoPainter = function(divid, obj, opt) {
      JSROOT.GEO.GradPerSegm = JSROOT.gStyle.GeoGradPerSegm;
      JSROOT.GEO.CompressComp = JSROOT.gStyle.GeoCompressComp;

      var painter = new TGeoPainter(obj);

      painter.SetDivId(divid, 5);

      painter._usesvg = JSROOT.BatchMode;

      painter._webgl = !painter._usesvg && JSROOT.Painter.TestWebGL();

      painter.options = painter.decodeOptions(opt || "");

      if (painter.options._yup === undefined)
         painter.options._yup = painter.svg_canvas().empty();

      return painter;
   }

   JSROOT.Painter.drawGeoObject = function(divid, obj, opt) {
      if (!obj) return null;

      var shape = null;

      if (('fShapeBits' in obj) && ('fShapeId' in obj)) {
         shape = obj; obj = null;
      } else if ((obj._typename === 'TGeoVolumeAssembly') || (obj._typename === 'TGeoVolume')) {
         shape = obj.fShape;
      } else if ((obj._typename === "TEveGeoShapeExtract") || (obj._typename === "ROOT::Experimental::TEveGeoShapeExtract")) {
         shape = obj.fShape;
      } else if (obj._typename === 'TGeoManager') {
         JSROOT.GEO.SetBit(obj.fMasterVolume, JSROOT.GEO.BITS.kVisThis, false);
         shape = obj.fMasterVolume.fShape;
      } else if ('fVolume' in obj) {
         if (obj.fVolume) shape = obj.fVolume.fShape;
      } else {
         obj = null;
      }

      if (opt && opt.indexOf("comp")==0 && shape && (shape._typename == 'TGeoCompositeShape') && shape.fNode) {
         opt = opt.substr(4);
         obj = JSROOT.GEO.buildCompositeVolume(shape);
      }

      if (!obj && shape)
         obj = JSROOT.extend(JSROOT.Create("TEveGeoShapeExtract"),
                   { fTrans: null, fShape: shape, fRGBA: [0, 1, 0, 1], fElements: null, fRnrSelf: true });

      if (!obj) return null;

      var painter = JSROOT.Painter.CreateGeoPainter(divid, obj, opt);

      if (painter.options.is_main && !obj.$geo_painter)
         obj.$geo_painter = painter;

      if (!painter.options.is_main && painter.options.project && obj.$geo_painter) {
         painter._main_painter = obj.$geo_painter;
         painter._main_painter._slave_painters.push(painter);
      }

      // this.options.script_name = 'https://root.cern/js/files/geom/geomAlice.C'

      painter.checkScript(painter.options.script_name, painter.prepareObjectDraw.bind(painter));

      return painter;
   }

   /// keep for backwards compatibility
   JSROOT.Painter.drawGeometry = JSROOT.Painter.drawGeoObject;

   // ===============================================================================

   JSROOT.GEO.buildCompositeVolume = function(comp, side) {
      // function used to build hierarchy of elements of composite shapes

      var vol = JSROOT.Create("TGeoVolume");
      if (side && (comp._typename!=='TGeoCompositeShape')) {
         vol.fName = side;
         JSROOT.GEO.SetBit(vol, JSROOT.GEO.BITS.kVisThis, true);
         vol.fLineColor = (side=="Left"? 2 : 3);
         vol.fShape = comp;
         return vol;
      }

      JSROOT.GEO.SetBit(vol, JSROOT.GEO.BITS.kVisDaughters, true);
      vol.$geoh = true; // workaround, let know browser that we are in volumes hierarchy
      vol.fName = "";

      var node1 = JSROOT.Create("TGeoNodeMatrix");
      node1.fName = "Left";
      node1.fMatrix = comp.fNode.fLeftMat;
      node1.fVolume = JSROOT.GEO.buildCompositeVolume(comp.fNode.fLeft, "Left");

      var node2 = JSROOT.Create("TGeoNodeMatrix");
      node2.fName = "Right";
      node2.fMatrix = comp.fNode.fRightMat;
      node2.fVolume = JSROOT.GEO.buildCompositeVolume(comp.fNode.fRight, "Right");

      vol.fNodes = JSROOT.Create("TList");
      vol.fNodes.Add(node1);
      vol.fNodes.Add(node2);

      return vol;
   }

   JSROOT.GEO.provideVisStyle = function(obj) {
      if ((obj._typename === 'TEveGeoShapeExtract') || (obj._typename === 'ROOT::Experimental::TEveGeoShapeExtract'))
         return obj.fRnrSelf ? " geovis_this" : "";

      var vis = !JSROOT.GEO.TestBit(obj, JSROOT.GEO.BITS.kVisNone) &&
                JSROOT.GEO.TestBit(obj, JSROOT.GEO.BITS.kVisThis);

      var chld = JSROOT.GEO.TestBit(obj, JSROOT.GEO.BITS.kVisDaughters) ||
                 JSROOT.GEO.TestBit(obj, JSROOT.GEO.BITS.kVisOneLevel);

      if (chld && (!obj.fNodes || (obj.fNodes.arr.length === 0))) chld = false;

      if (vis && chld) return " geovis_all";
      if (vis) return " geovis_this";
      if (chld) return " geovis_daughters";
      return "";
   }


   JSROOT.GEO.getBrowserItem = function(item, itemname, callback) {
      // mark object as belong to the hierarchy, require to
      if (item._geoobj) item._geoobj.$geoh = true;

      JSROOT.CallBack(callback, item, item._geoobj);
   }


   JSROOT.GEO.createItem = function(node, obj, name) {
      var sub = {
         _kind: "ROOT." + obj._typename,
         _name: name ? name : JSROOT.GEO.ObjectName(obj),
         _title: obj.fTitle,
         _parent: node,
         _geoobj: obj,
         _get: JSROOT.GEO.getBrowserItem
      };

      var volume, shape, subnodes, iseve = false;

      if (obj._typename == "TGeoMaterial") sub._icon = "img_geomaterial"; else
      if (obj._typename == "TGeoMedium") sub._icon = "img_geomedium"; else
      if (obj._typename == "TGeoMixture") sub._icon = "img_geomixture"; else
      if ((obj._typename.indexOf("TGeoNode")===0) && obj.fVolume) {
         sub._title = "node:"  + obj._typename;
         if (obj.fTitle.length > 0) sub._title += " " + obj.fTitle;
         volume = obj.fVolume;
      } else
      if (obj._typename.indexOf("TGeoVolume")===0) {
         volume = obj;
      } else
      if ((obj._typename == "TEveGeoShapeExtract") || (obj._typename == "ROOT::Experimental::TEveGeoShapeExtract") ) {
         iseve = true;
         shape = obj.fShape;
         subnodes = obj.fElements ? obj.fElements.arr : null;
      } else
      if ((obj.fShapeBits !== undefined) && (obj.fShapeId !== undefined)) {
         shape = obj;
      }

      if (volume) {
         shape = volume.fShape;
         subnodes = volume.fNodes ? volume.fNodes.arr : null;
      }

      if (volume || shape || subnodes) {
         if (volume) sub._volume = volume;

         if (subnodes) {
            sub._more = true;
            sub._expand = JSROOT.GEO.expandObject;
         } else
         if (shape && (shape._typename === "TGeoCompositeShape") && shape.fNode) {
            sub._more = true;
            sub._shape = shape;
            sub._expand = function(node, obj) {
               JSROOT.GEO.createItem(node, node._shape.fNode.fLeft, 'Left');
               JSROOT.GEO.createItem(node, node._shape.fNode.fRight, 'Right');
               return true;
            }
         }

         if (!sub._title && (obj._typename != "TGeoVolume")) sub._title = obj._typename;

         if (shape) {
            if (sub._title == "")
               sub._title = shape._typename;

            sub._icon = JSROOT.GEO.getShapeIcon(shape);
         } else {
            sub._icon = sub._more ? "img_geocombi" : "img_geobbox";
         }

         if (volume)
            sub._icon += JSROOT.GEO.provideVisStyle(volume);
         else if (iseve)
            sub._icon += JSROOT.GEO.provideVisStyle(obj);

         sub._menu = JSROOT.GEO.provideMenu;
         sub._icon_click  = JSROOT.GEO.browserIconClick;
      }

      if (!node._childs) node._childs = [];

      if (!sub._name)
         if (typeof node._name === 'string') {
            sub._name = node._name;
            if (sub._name.lastIndexOf("s")===sub._name.length-1)
               sub._name = sub._name.substr(0, sub._name.length-1);
            sub._name += "_" + node._childs.length;
         } else {
            sub._name = "item_" + node._childs.length;
         }

      node._childs.push(sub);

      return sub;
   }

   JSROOT.GEO.createList = function(parent, lst, name, title) {

      if (!lst || !('arr' in lst) || (lst.arr.length==0)) return;

      var item = {
          _name: name,
          _kind: "ROOT.TList",
          _title: title,
          _more: true,
          _geoobj: lst,
          _parent: parent,
      }

      item._get = function(item, itemname, callback) {
         if ('_geoobj' in item)
            return JSROOT.CallBack(callback, item, item._geoobj);

         JSROOT.CallBack(callback, item, null);
      }

      item._expand = function(node, lst) {
         // only childs

         if ('fVolume' in lst)
            lst = lst.fVolume.fNodes;

         if (!('arr' in lst)) return false;

         node._childs = [];

         JSROOT.GEO.CheckDuplicates(null, lst.arr);

         for (var n in lst.arr)
            JSROOT.GEO.createItem(node, lst.arr[n]);

         return true;
      }

      if (!parent._childs) parent._childs = [];
      parent._childs.push(item);

   };

   JSROOT.GEO.provideMenu = function(menu, item, hpainter) {

      if (!item._geoobj) return false;

      var obj = item._geoobj, vol = item._volume,
          iseve = ((obj._typename === 'TEveGeoShapeExtract') || (obj._typename === 'ROOT::Experimental::TEveGeoShapeExtract'));

      if (!vol && !iseve) return false;

      menu.add("separator");

      function ScanEveVisible(obj, arg, skip_this) {
         if (!arg) arg = { visible: 0, hidden: 0 };

         if (!skip_this) {
            if (arg.assign!==undefined) obj.fRnrSelf = arg.assign; else
            if (obj.fRnrSelf) arg.vis++; else arg.hidden++;
         }

         if (obj.fElements)
            for (var n=0;n<obj.fElements.arr.length;++n)
               ScanEveVisible(obj.fElements.arr[n], arg, false);

         return arg;
      }

      function ToggleEveVisibility(arg) {
         if (arg === 'self') {
            obj.fRnrSelf = !obj.fRnrSelf;
            item._icon = item._icon.split(" ")[0] + JSROOT.GEO.provideVisStyle(obj);
            hpainter.UpdateTreeNode(item);
         } else {
            ScanEveVisible(obj, { assign: (arg === "true") }, true);
            hpainter.ForEach(function(m) {
               // update all child items
               if (m._geoobj && m._icon) {
                  m._icon = item._icon.split(" ")[0] + JSROOT.GEO.provideVisStyle(m._geoobj);
                  hpainter.UpdateTreeNode(m);
               }
            }, item);
         }

         JSROOT.GEO.findItemWithPainter(item, 'testGeomChanges');
      }

      function ToggleMenuBit(arg) {
         JSROOT.GEO.ToggleBit(vol, arg);
         var newname = item._icon.split(" ")[0] + JSROOT.GEO.provideVisStyle(vol);
         hpainter.ForEach(function(m) {
            // update all items with that volume
            if (item._volume === m._volume) {
               m._icon = newname;
               hpainter.UpdateTreeNode(m);
            }
         });

         hpainter.UpdateTreeNode(item);
         JSROOT.GEO.findItemWithPainter(item, 'testGeomChanges');
      }

      if ((item._geoobj._typename.indexOf("TGeoNode")===0) && JSROOT.GEO.findItemWithPainter(item))
         menu.add("Focus", function() {

           var drawitem = JSROOT.GEO.findItemWithPainter(item);

           if (!drawitem) return;

           var fullname = hpainter.itemFullName(item, drawitem);

           if (drawitem._painter && typeof drawitem._painter.focusOnItem == 'function')
              drawitem._painter.focusOnItem(fullname);
         });

      if (iseve) {
         menu.addchk(obj.fRnrSelf, "Visible", "self", ToggleEveVisibility);
         var res = ScanEveVisible(obj, undefined, true);

         if (res.hidden + res.visible > 0)
            menu.addchk((res.hidden==0), "Daughters", (res.hidden!=0) ? "true" : "false", ToggleEveVisibility);

      } else {
         menu.addchk(JSROOT.GEO.TestBit(vol, JSROOT.GEO.BITS.kVisNone), "Invisible",
               JSROOT.GEO.BITS.kVisNone, ToggleMenuBit);
         menu.addchk(JSROOT.GEO.TestBit(vol, JSROOT.GEO.BITS.kVisThis), "Visible",
               JSROOT.GEO.BITS.kVisThis, ToggleMenuBit);
         menu.addchk(JSROOT.GEO.TestBit(vol, JSROOT.GEO.BITS.kVisDaughters), "Daughters",
               JSROOT.GEO.BITS.kVisDaughters, ToggleMenuBit);
         menu.addchk(JSROOT.GEO.TestBit(vol, JSROOT.GEO.BITS.kVisOneLevel), "1lvl daughters",
               JSROOT.GEO.BITS.kVisOneLevel, ToggleMenuBit);
      }

      return true;
   }

   JSROOT.GEO.findItemWithPainter = function(hitem, funcname) {
      while (hitem) {
         if (hitem._painter && hitem._painter._camera) {
            if (funcname && typeof hitem._painter[funcname] == 'function')
               hitem._painter[funcname]();
            return hitem;
         }
         hitem = hitem._parent;
      }
      return null;
   }

   JSROOT.GEO.updateBrowserIcons = function(obj, hpainter) {
      if (!obj || !hpainter) return;

      hpainter.ForEach(function(m) {
         // update all items with that volume
         if ((obj === m._volume) || (obj === m._geoobj)) {
            m._icon = m._icon.split(" ")[0] + JSROOT.GEO.provideVisStyle(obj);
            hpainter.UpdateTreeNode(m);
         }
      });
   }

   JSROOT.GEO.browserIconClick = function(hitem, hpainter) {
      if (hitem._volume) {
         if (hitem._more && hitem._volume.fNodes && (hitem._volume.fNodes.arr.length>0))
            JSROOT.GEO.ToggleBit(hitem._volume, JSROOT.GEO.BITS.kVisDaughters);
         else
            JSROOT.GEO.ToggleBit(hitem._volume, JSROOT.GEO.BITS.kVisThis);

         JSROOT.GEO.updateBrowserIcons(hitem._volume, hpainter);

         JSROOT.GEO.findItemWithPainter(hitem, 'testGeomChanges');
         return false; // no need to update icon - we did it ourself
      }

      if (hitem._geoobj && (( hitem._geoobj._typename == "TEveGeoShapeExtract") || ( hitem._geoobj._typename == "ROOT::Experimental::TEveGeoShapeExtract"))) {
         hitem._geoobj.fRnrSelf = !hitem._geoobj.fRnrSelf;

         JSROOT.GEO.updateBrowserIcons(hitem._geoobj, hpainter);
         JSROOT.GEO.findItemWithPainter(hitem, 'testGeomChanges');
         return false; // no need to update icon - we did it ourself
      }


      // first check that geo painter assigned with the item
      var drawitem = JSROOT.GEO.findItemWithPainter(hitem);
      if (!drawitem) return false;

      var newstate = drawitem._painter.ExtraObjectVisible(hpainter, hitem, true);

      // return true means browser should update icon for the item
      return (newstate!==undefined) ? true : false;
   }

   JSROOT.GEO.getShapeIcon = function(shape) {
      switch (shape._typename) {
         case "TGeoArb8" : return "img_geoarb8"; break;
         case "TGeoCone" : return "img_geocone"; break;
         case "TGeoConeSeg" : return "img_geoconeseg"; break;
         case "TGeoCompositeShape" : return "img_geocomposite"; break;
         case "TGeoTube" : return "img_geotube"; break;
         case "TGeoTubeSeg" : return "img_geotubeseg"; break;
         case "TGeoPara" : return "img_geopara"; break;
         case "TGeoParaboloid" : return "img_geoparab"; break;
         case "TGeoPcon" : return "img_geopcon"; break;
         case "TGeoPgon" : return "img_geopgon"; break;
         case "TGeoShapeAssembly" : return "img_geoassembly"; break;
         case "TGeoSphere" : return "img_geosphere"; break;
         case "TGeoTorus" : return "img_geotorus"; break;
         case "TGeoTrd1" : return "img_geotrd1"; break;
         case "TGeoTrd2" : return "img_geotrd2"; break;
         case "TGeoXtru" : return "img_geoxtru"; break;
         case "TGeoTrap" : return "img_geotrap"; break;
         case "TGeoGtra" : return "img_geogtra"; break;
         case "TGeoEltu" : return "img_geoeltu"; break;
         case "TGeoHype" : return "img_geohype"; break;
         case "TGeoCtub" : return "img_geoctub"; break;
      }
      return "img_geotube";
   }

   JSROOT.GEO.getBrowserIcon = function(hitem, hpainter) {
      var icon = "";
      if (hitem._kind == 'ROOT.TEveTrack') icon = 'img_evetrack'; else
      if (hitem._kind == 'ROOT.TEvePointSet') icon = 'img_evepoints'; else
      if (hitem._kind == 'ROOT.TPolyMarker3D') icon = 'img_evepoints';
      if (icon.length>0) {
         var drawitem = JSROOT.GEO.findItemWithPainter(hitem);
         if (drawitem)
            if (drawitem._painter.ExtraObjectVisible(hpainter, hitem))
               icon += " geovis_this";
      }
      return icon;
   }

   JSROOT.GEO.expandObject = function(parent, obj) {
      if (!parent || !obj) return false;

      var isnode = (obj._typename.indexOf('TGeoNode') === 0),
          isvolume = (obj._typename.indexOf('TGeoVolume') === 0),
          ismanager = (obj._typename === 'TGeoManager'),
          iseve = ((obj._typename === 'TEveGeoShapeExtract')||(obj._typename === 'ROOT::Experimental::TEveGeoShapeExtract'));

      if (!isnode && !isvolume && !ismanager && !iseve) return false;

      if (parent._childs) return true;

      if (ismanager) {
         JSROOT.GEO.createList(parent, obj.fMaterials, "Materials", "list of materials");
         JSROOT.GEO.createList(parent, obj.fMedia, "Media", "list of media");
         JSROOT.GEO.createList(parent, obj.fTracks, "Tracks", "list of tracks");

         JSROOT.GEO.SetBit(obj.fMasterVolume, JSROOT.GEO.BITS.kVisThis, false);
         JSROOT.GEO.createItem(parent, obj.fMasterVolume);
         return true;
      }

      var volume, subnodes, shape;

      if (iseve) {
         subnodes = obj.fElements ? obj.fElements.arr : null;
         shape = obj.fShape;
      } else {
         volume = (isnode ? obj.fVolume : obj);
         subnodes = volume && volume.fNodes ? volume.fNodes.arr : null;
         shape = volume ? volume.fShape : null;
      }

      if (!subnodes && shape && (shape._typename === "TGeoCompositeShape") && shape.fNode) {
         if (!parent._childs) {
            JSROOT.GEO.createItem(parent, shape.fNode.fLeft, 'Left');
            JSROOT.GEO.createItem(parent, shape.fNode.fRight, 'Right');
         }

         return true;
      }

      if (!subnodes) return false;

      JSROOT.GEO.CheckDuplicates(obj, subnodes);

      for (var i=0;i<subnodes.length;++i)
         JSROOT.GEO.createItem(parent, subnodes[i]);

      return true;
   }

   JSROOT.addDrawFunc({ name: "TGeoVolumeAssembly", icon: 'img_geoassembly', func: JSROOT.Painter.drawGeoObject, expand: JSROOT.GEO.expandObject, opt: ";more;all;count" });
   JSROOT.addDrawFunc({ name: "TEvePointSet", icon_get: JSROOT.GEO.getBrowserIcon, icon_click: JSROOT.GEO.browserIconClick });
   JSROOT.addDrawFunc({ name: "TEveTrack", icon_get: JSROOT.GEO.getBrowserIcon, icon_click: JSROOT.GEO.browserIconClick });

   JSROOT.TGeoPainter = TGeoPainter;

   return JSROOT.Painter;

}));
