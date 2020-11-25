/// @file JSRoot.v7hist.js
/// JavaScript ROOT v7 graphics for histogram classes

JSROOT.define(['d3', 'painter', 'v7gpad'], (d3, jsrp) => {

   "use strict";

   function RHistPainter(histo) {
      JSROOT.ObjectPainter.call(this, histo);
      this.csstype = "hist";
      this.draw_content = true;
      this.nbinsx = 0;
      this.nbinsy = 0;
      this.accept_drops = true; // indicate that one can drop other objects like doing Draw("same")
      this.mode3d = false;

      // initialize histogram methods
      this.GetHisto(true);
   }

   RHistPainter.prototype = Object.create(JSROOT.ObjectPainter.prototype);

   // function ensure that frame is drawn on the canvas
   RHistPainter.prototype.PrepareFrame = function(divid, mode3d) {
      this.SetDivId(divid, -1);

      if (!this.frame_painter())
         JSROOT.v7.drawFrame(divid, null, mode3d ? "3d" : "");

      return this.SetDivId(divid, mode3d ? 4 : 1);
   }

   RHistPainter.prototype.GetHImpl = function(obj) {
      if (obj && obj.fHistImpl)
         return obj.fHistImpl.fIO;
      return null;
   }

   /** Returns true if RHistDisplayItem is used */
   RHistPainter.prototype.IsDisplayItem = function() {
      let obj = this.GetObject();
      return obj && obj.fAxes ? true : false;
   }

   RHistPainter.prototype.GetHisto = function(force) {
      let obj = this.GetObject(), histo = this.GetHImpl(obj);

      if (histo && (!histo.getBinContent || force)) {
         if (histo.fAxes._2) {
            JSROOT.v7.AssignRAxisMethods(histo.fAxes._0);
            JSROOT.v7.AssignRAxisMethods(histo.fAxes._1);
            JSROOT.v7.AssignRAxisMethods(histo.fAxes._2);
            histo.getBin = function(x, y, z) { return (x-1) + this.fAxes._0.GetNumBins()*(y-1) + this.fAxes._0.GetNumBins()*this.fAxes._1.GetNumBins()*(z-1); }
            // FIXME: all normal ROOT methods uses indx+1 logic, but RHist has no underflow/overflow bins now
            histo.getBinContent = function(x, y, z) { return this.fStatistics.fBinContent[this.getBin(x, y, z)]; }
            histo.getBinError = function(x, y, z) {
               let bin = this.getBin(x, y, z);
               if (this.fStatistics.fSumWeightsSquared)
                  return Math.sqrt(this.fStatistics.fSumWeightsSquared[bin]);
               return Math.sqrt(Math.abs(this.fStatistics.fBinContent[bin]));
            }
         } else if (histo.fAxes._1) {
            JSROOT.v7.AssignRAxisMethods(histo.fAxes._0);
            JSROOT.v7.AssignRAxisMethods(histo.fAxes._1);
            histo.getBin = function(x, y) { return (x-1) + this.fAxes._0.GetNumBins()*(y-1); }
            // FIXME: all normal ROOT methods uses indx+1 logic, but RHist has no underflow/overflow bins now
            histo.getBinContent = function(x, y) { return this.fStatistics.fBinContent[this.getBin(x, y)]; }
            histo.getBinError = function(x, y) {
               let bin = this.getBin(x, y);
               if (this.fStatistics.fSumWeightsSquared)
                  return Math.sqrt(this.fStatistics.fSumWeightsSquared[bin]);
               return Math.sqrt(Math.abs(this.fStatistics.fBinContent[bin]));
            }
         } else {
            JSROOT.v7.AssignRAxisMethods(histo.fAxes._0);
            histo.getBin = function(x) { return x-1; }
            // FIXME: all normal ROOT methods uses indx+1 logic, but RHist has no underflow/overflow bins now
            histo.getBinContent = function(x) { return this.fStatistics.fBinContent[x-1]; }
            histo.getBinError = function(x) {
               if (this.fStatistics.fSumWeightsSquared)
                  return Math.sqrt(this.fStatistics.fSumWeightsSquared[x-1]);
               return Math.sqrt(Math.abs(this.fStatistics.fBinContent[x-1]));
            }
         }
      } else if (!histo && obj && obj.fAxes) {
         // case of RHistDisplayItem

         histo = obj;

         if (!histo.getBinContent || force) {
            if (histo.fAxes.length == 3) {
               JSROOT.v7.AssignRAxisMethods(histo.fAxes[0]);
               JSROOT.v7.AssignRAxisMethods(histo.fAxes[1]);
               JSROOT.v7.AssignRAxisMethods(histo.fAxes[2]);

               histo.nx = histo.fIndicies[1] - histo.fIndicies[0];
               histo.dx = histo.fIndicies[0] + 1;
               histo.stepx = histo.fIndicies[2];

               histo.ny = histo.fIndicies[4] - histo.fIndicies[3];
               histo.dy = histo.fIndicies[3] + 1;
               histo.stepy = histo.fIndicies[5];

               histo.nz = histo.fIndicies[7] - histo.fIndicies[6];
               histo.dz = histo.fIndicies[6] + 1;
               histo.stepz = histo.fIndicies[8];

               // this is index in original histogram
               histo.getBin = function(x, y, z) { return (x-1) + this.fAxes[0].GetNumBins()*(y-1) + this.fAxes[0].GetNumBins()*this.fAxes[1].GetNumBins()*(z-1); }

               // this is index in current available data
               if ((histo.stepx > 1) || (histo.stepy > 1) || (histo.stepz > 1))
                  histo.getBin0 = function(x, y, z) { return Math.floor((x-this.dx)/this.stepx) + this.nx/this.stepx*Math.floor((y-this.dy)/this.stepy) + this.nx/this.stepx*this.ny/this.stepy*Math.floor((z-this.dz)/this.stepz); }
               else
                  histo.getBin0 = function(x, y, z) { return (x-this.dx) + this.nx*(y-this.dy) + this.nx*this.ny*(z-dz); }

               histo.getBinContent = function(x, y, z) { return this.fBinContent[this.getBin0(x, y, z)]; }
               histo.getBinError = function(x, y, z) { return Math.sqrt(Math.abs(this.getBinContent(x, y, z))); }


            } else if (histo.fAxes.length == 2) {
               JSROOT.v7.AssignRAxisMethods(histo.fAxes[0]);
               JSROOT.v7.AssignRAxisMethods(histo.fAxes[1]);

               histo.nx = histo.fIndicies[1] - histo.fIndicies[0];
               histo.dx = histo.fIndicies[0] + 1;
               histo.stepx = histo.fIndicies[2];

               histo.ny = histo.fIndicies[4] - histo.fIndicies[3];
               histo.dy = histo.fIndicies[3] + 1;
               histo.stepy = histo.fIndicies[5];

               // this is index in original histogram
               histo.getBin = function(x, y) { return (x-1) + this.fAxes[0].GetNumBins()*(y-1); }

               // this is index in current available data
               if ((histo.stepx > 1) || (histo.stepy > 1))
                  histo.getBin0 = function(x, y) { return Math.floor((x-this.dx)/this.stepx) + this.nx/this.stepx*Math.floor((y-this.dy)/this.stepy); }
               else
                  histo.getBin0 = function(x, y) { return (x-this.dx) + this.nx*(y-this.dy); }

               histo.getBinContent = function(x, y) { return this.fBinContent[this.getBin0(x, y)]; }
               histo.getBinError = function(x, y) { return Math.sqrt(Math.abs(this.getBinContent(x, y))); }
            } else {
               JSROOT.v7.AssignRAxisMethods(histo.fAxes[0]);
               histo.nx = histo.fIndicies[1] - histo.fIndicies[0];
               histo.dx = histo.fIndicies[0] + 1;
               histo.stepx = histo.fIndicies[2];

               histo.getBin = function(x) { return x-1; }
               if (histo.stepx > 1)
                  histo.getBin0 = function(x) { return Math.floor((x-this.dx)/this.stepx); }
               else
                  histo.getBin0 = function(x) { return x-this.dx; }
               histo.getBinContent = function(x) { return this.fBinContent[this.getBin0(x)]; }
               histo.getBinError = function(x) { return Math.sqrt(Math.abs(this.getBinContent(x))); }
            }
         }
      }
      return histo;
   }

   RHistPainter.prototype.IsRProfile = function() {
      return false;
   }

   RHistPainter.prototype.IsTH2Poly = function() {
      return false;
   }

   RHistPainter.prototype.DecodeOptions = function(/*opt*/) {
      if (!this.options) this.options = { Hist : 1 };
   }

   RHistPainter.prototype.Clear3DScene = function() {
      let fp = this.frame_painter();
      if (fp && typeof fp.Create3DScene === 'function')
         fp.Create3DScene(-1);
      this.mode3d = false;
   }

   RHistPainter.prototype.Cleanup = function() {
      // clear all 3D buffers
      this.Clear3DScene();

      delete this.options;

      JSROOT.ObjectPainter.prototype.Cleanup.call(this);
   }

   RHistPainter.prototype.Dimension = function() {
      return 1;
   }

   /** @summary Scan histogram content
     * @abstract */
   RHistPainter.prototype.ScanContent = function(/*when_axis_changed*/) {
      // function will be called once new histogram or
      // new histogram content is assigned
      // one should find min,max,nbins, maxcontent values
      // if when_axis_changed === true specified, content will be scanned after axis zoom changed
   }

   RHistPainter.prototype.DrawAxes = function() {
      // return true when axes was drawn
      let main = this.frame_painter();
      if (!main) return Promise.resolve(false);

      if (this.is_main_painter() && this.draw_content) {
         main.CleanupAxes();
         main.xmin = main.xmax = 0;
         main.ymin = main.ymax = 0;
         main.zmin = main.zmax = 0;
         main.SetAxesRanges(this.GetAxis("x"), this.xmin, this.xmax, this.GetAxis("y"), this.ymin, this.ymax, this.GetAxis("z"), this.zmin, this.zmax);
      }

      return main.DrawAxes();
   }

   RHistPainter.prototype.CheckHistDrawAttributes = function() {
      this.createv7AttFill();
      this.createv7AttLine();
   }

   RHistPainter.prototype.UpdateDisplayItem = function(obj, src) {
      if (!obj || !src) return false;

      obj.fAxes = src.fAxes;
      obj.fIndicies = src.fIndicies;
      obj.fBinContent = src.fBinContent;
      obj.fContMin = src.fContMin;
      obj.fContMinPos = src.fContMinPos;
      obj.fContMax = src.fContMax;

      // update histogram attributes
      this.GetHisto(true);

      return true;
   }

   RHistPainter.prototype.UpdateObject = function(obj /*, opt*/) {

      let origin = this.GetObject();

      if (obj !== origin) {

         if (!this.MatchObjectType(obj)) return false;

         if (this.IsDisplayItem()) {

            this.UpdateDisplayItem(origin, obj);

         } else {

            let horigin = this.GetHImpl(origin),
                hobj = this.GetHImpl(obj);

            if (!horigin || !hobj) return false;

            // make it easy - copy statistics without axes
            horigin.fStatistics = hobj.fStatistics;

            origin.fTitle = obj.fTitle;
         }
      }

      this.ScanContent();

      this.histogram_updated = true; // indicate that object updated

      return true;
   }

   RHistPainter.prototype.GetAxis = function(name) {
      let histo = this.GetHisto(), obj = this.GetObject(), axis = null;

      if (obj && obj.fAxes) {
         switch(name) {
            case "x": axis = obj.fAxes[0]; break;
            case "y": axis = obj.fAxes[1]; break;
            case "z": axis = obj.fAxes[2]; break;
            default: axis = obj.fAxes[0]; break;
         }
      } else if (histo && histo.fAxes) {
         switch(name) {
            case "x": axis = histo.fAxes._0; break;
            case "y": axis = histo.fAxes._1; break;
            case "z": axis = histo.fAxes._2; break;
            default: axis = histo.fAxes._0; break;
         }
      }

      if (axis && !axis.GetBinCoord)
         JSROOT.v7.AssignRAxisMethods(axis);

      return axis;
   }

   /** @summary Extract axes ranges and bins numbers
     * @desc Also here ensured that all axes objects got their necessary methods */
   RHistPainter.prototype.ExtractAxesProperties = function(ndim) {

      let histo = this.GetHisto();
      if (!histo) return;

      this.nbinsx = this.nbinsy = this.nbinsz = 0;

      let axis = this.GetAxis("x");
      this.nbinsx = axis.GetNumBins();
      this.xmin = axis.min;
      this.xmax = axis.max;

      if (ndim < 2) return;
      axis = this.GetAxis("y");
      this.nbinsy = axis.GetNumBins();
      this.ymin = axis.min;
      this.ymax = axis.max;

      if (ndim < 3) return;
      axis = this.GetAxis("z");
      this.nbinsz = axis.GetNumBins();
      this.zmin = axis.min;
      this.zmax = axis.max;
   }

   RHistPainter.prototype.AddInteractive = function() {
      // only first painter in list allowed to add interactive functionality to the frame

      if (this.is_main_painter()) {
         let fp = this.frame_painter();
         if (fp) return fp.AddInteractive();
      }

      return Promise.resolve(false);
   }

   RHistPainter.prototype.ProcessItemReply = function(reply, req) {
      if (!this.IsDisplayItem())
         return console.error('Get item when display normal histogram');

      if (req.reqid === this.current_item_reqid) {

         if (reply !== null) {
            this.UpdateDisplayItem(this.GetObject(), reply.item);
         }

         req.resolveFunc(true);
      }
   }

   /** @summary Special method to request bins from server if existing data insufficient
     * @private */
   RHistPainter.prototype.DrawingBins = function(reason) {

      let is_axes_zoomed = false;
      if (reason && (typeof reason == "string") && (reason.indexOf("zoom") == 0)) {
         if (reason.indexOf("0") > 0) is_axes_zoomed = true;
         if ((this.Dimension() > 1) && (reason.indexOf("1") > 0)) is_axes_zoomed = true;
         if ((this.Dimension() > 2) && (reason.indexOf("2") > 0)) is_axes_zoomed = true;
      }

      if (this.IsDisplayItem() && is_axes_zoomed && (this.v7CommMode() == JSROOT.v7.CommMode.kNormal)) {

         let handle = this.PrepareDraw({ only_indexes: true });

         // submit request if histogram data not enough for display
         if (handle.incomplete)
            return new Promise(resolveFunc => {
               // use empty kind to always submit request
               let req = this.v7SubmitRequest("", { _typename: "ROOT::Experimental::RHistDrawableBase::RRequest" },
                                                  this.ProcessItemReply.bind(this));
               if (req) {
                  this.current_item_reqid = req.reqid; // ignore all previous requests, only this one will be processed
                  req.resolveFunc = resolveFunc;
                  setTimeout(this.ProcessItemReply.bind(this, null, req), 1000); // after 1 s draw something that we can
               } else {
                  resolveFunc(true);
               }
            });
      }

      return Promise.resolve(true);
   }

   RHistPainter.prototype.ToggleTitle = function(/*arg*/) {
      return false;
   }

   RHistPainter.prototype.DrawTitle = function() {
   }

   RHistPainter.prototype.UpdateStatWebCanvas = function() {
   }

   RHistPainter.prototype.ToggleStat = function(/*arg*/) {
   }

   RHistPainter.prototype.GetSelectIndex = function(axis, size, add) {
      // be aware - here indexes starts from 0
      let indx = 0,
          main = this.frame_painter(),
          taxis = this.GetAxis(axis),
          nbins = this['nbins'+axis] || 0,
          min = main ? main['zoom_' + axis + 'min'] : 0,
          max = main ? main['zoom_' + axis + 'max'] : 0;

      if ((min !== max) && taxis) {
         if (size == "left")
            indx = taxis.FindBin(min, add || 0);
         else
            indx = taxis.FindBin(max, (add || 0) + 0.5);
         if (indx<0) indx = 0; else if (indx>nbins) indx = nbins;
      } else {
         indx = (size == "left") ? 0 : nbins;
      }

      return indx;
   }

   RHistPainter.prototype.FindStat = function() {
      return null;
   }

   RHistPainter.prototype.IgnoreStatsFill = function() {
      return true;
   }

   RHistPainter.prototype.CreateStat = function(/*force*/) {
      return null;
   }

   RHistPainter.prototype.ButtonClick = function(funcname) {
      // TODO: move to frame painter
      switch(funcname) {
         case "ToggleZoom":
            if ((this.zoom_xmin !== this.zoom_xmax) || (this.zoom_ymin !== this.zoom_ymax) || (this.zoom_zmin !== this.zoom_zmax)) {
               this.Unzoom();
               this.frame_painter().zoomChangedInteractive('reset');
               return true;
            }
            if (this.draw_content && (typeof this.AutoZoom === 'function')) {
               this.AutoZoom();
               return true;
            }
            break;
         case "ToggleLogX": this.frame_painter().ToggleLog("x"); break;
         case "ToggleLogY": this.frame_painter().ToggleLog("y"); break;
         case "ToggleLogZ": this.frame_painter().ToggleLog("z"); break;
         case "ToggleStatBox": this.ToggleStat(); return true;
      }
      return false;
   }

   RHistPainter.prototype.FillToolbar = function(not_shown) {
      let pp = this.pad_painter();
      if (!pp) return;

      pp.AddButton("auto_zoom", 'Toggle between unzoom and autozoom-in', 'ToggleZoom', "Ctrl *");
      pp.AddButton("arrow_right", "Toggle log x", "ToggleLogX", "PageDown");
      pp.AddButton("arrow_up", "Toggle log y", "ToggleLogY", "PageUp");
      if (this.Dimension() > 1)
         pp.AddButton("arrow_diag", "Toggle log z", "ToggleLogZ");
      if (this.draw_content)
         pp.AddButton("statbox", 'Toggle stat box', "ToggleStatBox");
      if (!not_shown) pp.ShowButtons();
   }

   RHistPainter.prototype.Get3DToolTip = function(indx) {
      let histo = this.GetHisto(),
          tip = { bin: indx, name: histo.fName || "histo", title: histo.fTitle };
      switch (this.Dimension()) {
         case 1:
            tip.ix = indx + 1; tip.iy = 1;
            tip.value = histo.getBinContent(tip.ix);
            tip.error = histo.getBinError(tip.ix);
            tip.lines = this.GetBinTips(indx-1);
            break;
         case 2:
            tip.ix = (indx % this.nbinsx) + 1;
            tip.iy = (indx - (tip.ix - 1)) / this.nbinsx + 1;
            tip.value = histo.getBinContent(tip.ix, tip.iy);
            tip.error = histo.getBinError(tip.ix, tip.iy);
            tip.lines = this.GetBinTips(tip.ix-1, tip.iy-1);
            break;
         case 3:
            tip.ix = indx % this.nbinsx + 1;
            tip.iy = ((indx - (tip.ix - 1)) / this.nbinsx) % this.nbinsy + 1;
            tip.iz = (indx - (tip.ix - 1) - (tip.iy - 1) * this.nbinsx) / this.nbinsx / this.nbinsy + 1;
            tip.value = histo.getBinContent(tip.ix, tip.iy, tip.iz);
            tip.error = histo.getBinError(tip.ix, tip.iy, tip.iz);
            tip.lines = this.GetBinTips(tip.ix-1, tip.iy-1, tip.iz-1);
            break;
      }

      return tip;
   }

   /** @summary Create contour levels for currently selected Z range
     * @private */
   RHistPainter.prototype.CreateContour = function(main, palette, args) {
      if (!main || !palette) return;

      if (!args) args = {};

      let nlevels = JSROOT.gStyle.fNumberContours,
          zmin = this.minbin, zmax = this.maxbin, zminpos = this.minposbin;

      if (args && args.scatter_plot) {
         if (nlevels > 50) nlevels = 50;
         zmin = this.minposbin;
      }

      if (zmin === zmax) { zmin = this.gminbin; zmax = this.gmaxbin; zminpos = this.gminposbin }

      if (this.Dimension() < 3) {
         if (main.zoom_zmin !== main.zoom_zmax) {
            zmin = main.zoom_zmin;
            zmax = main.zoom_zmax;
         } else if (args.full_z_range) {
            zmin = main.zmin;
            zmax = main.zmax;
         }
      }

      palette.CreateContour(main.logz, nlevels, zmin, zmax, zminpos);

      if (this.Dimension() < 3) {
         main.scale_zmin = palette.colzmin;
         main.scale_zmax = palette.colzmax;
      }
   }

   /** @summary Start dialog to modify range of axis where histogram values are displayed
     * @private */
   RHistPainter.prototype.ChangeValuesRange = function(arg) {
      let pmain = this.frame_painter();
      if (!pmain) return;
      let prefix = pmain.IsAxisZoomed(arg) ? "zoom_" + arg : arg;
      let curr = "[" + pmain[prefix+'min'] + "," + pmain[prefix+'max'] + "]";
      let res = prompt("Enter values range for axis " + arg + " like [0,100] or empty string to unzoom", curr);
      res = res ? JSON.parse(res) : [];

      if (!res || (typeof res != "object") || (res.length!=2) || isNaN(res[0]) || isNaN(res[1]))
         pmain.Unzoom(arg);
      else
         pmain.Zoom(arg, res[0], res[1]);
   }

   /** @summary Fill histogram context menu
     * @private */
   RHistPainter.prototype.FillContextMenu = function(menu) {

      menu.add("header:v7histo::anyname");

      if (this.draw_content) {
         menu.addchk(this.ToggleStat('only-check'), "Show statbox", function() { this.ToggleStat(); });

         if (this.Dimension() == 2)
             menu.add("Values range", "z", this.ChangeValuesRange);

         if (typeof this.FillHistContextMenu == 'function')
            this.FillHistContextMenu(menu);
      }

      let fp = this.frame_painter();

      if (this.options.Mode3D) {
         // menu for 3D drawings

         if (menu.size() > 0)
            menu.add("separator");

         let main = this.main_painter() || this;

         menu.addchk(main.IsTooltipAllowed(), 'Show tooltips', function() {
            main.SetTooltipAllowed("toggle");
         });

         menu.addchk(fp.enable_highlight, 'Highlight bins', function() {
            fp.enable_highlight = !fp.enable_highlight;
            if (!fp.enable_highlight && main.BinHighlight3D && main.mode3d) main.BinHighlight3D(null);
         });

         if (fp && fp.Render3D) {
            menu.addchk(main.options.FrontBox, 'Front box', function() {
               main.options.FrontBox = !main.options.FrontBox;
               fp.Render3D();
            });
            menu.addchk(main.options.BackBox, 'Back box', function() {
               main.options.BackBox = !main.options.BackBox;
               fp.Render3D();
            });
         }

         if (this.draw_content) {
            menu.addchk(!this.options.Zero, 'Suppress zeros', function() {
               this.options.Zero = !this.options.Zero;
               this.RedrawPad();
            });

            if ((this.options.Lego==12) || (this.options.Lego==14)) {
               menu.addchk(this.options.Zscale, "Z scale", function() {
                  this.ToggleColz();
               });
               if (this.FillPaletteMenu) this.FillPaletteMenu(menu);
            }
         }

         if (main.control && typeof main.control.reset === 'function')
            menu.add('Reset camera', function() {
               main.control.reset();
            });
      }

      menu.AddAttributesMenu(this);

      if (this.histogram_updated && fp.zoomChangedInteractive())
         menu.add('Let update zoom', function() {
            fp.zoomChangedInteractive('reset');
         });

      return true;
   }

   /** @summary Update palette drawing */
   RHistPainter.prototype.UpdatePaletteDraw = function() {
      if (this.is_main_painter()) {
         let pp = this.FindPainterFor(undefined, undefined, "ROOT::Experimental::RPaletteDrawable");
         if (pp) pp.DrawPalette();
      }
   }

   /** @summary Fill palette menu */
   RHistPainter.prototype.FillPaletteMenu = function(menu) {

      // TODO: rewrite for RPalette functionality
      let curr = this.options.Palette, hpainter = this;
      if ((curr===null) || (curr===0)) curr = JSROOT.settings.Palette;

      function change(arg) {
         hpainter.options.Palette = parseInt(arg);
         hpainter.Redraw(); // redraw histogram
      };

      function add(id, name, more) {
         menu.addchk((id===curr) || more, '<nobr>' + name + '</nobr>', id, change);
      };

      menu.add("sub:Palette");

      add(50, "ROOT 5", (curr>=10) && (curr<51));
      add(51, "Deep Sea");
      add(52, "Grayscale", (curr>0) && (curr<10));
      add(53, "Dark body radiator");
      add(54, "Two-color hue");
      add(55, "Rainbow");
      add(56, "Inverted dark body radiator");
      add(57, "Bird", (curr>112));
      add(58, "Cubehelix");
      add(59, "Green Red Violet");
      add(60, "Blue Red Yellow");
      add(61, "Ocean");
      add(62, "Color Printable On Grey");
      add(63, "Alpine");
      add(64, "Aquamarine");
      add(65, "Army");
      add(66, "Atlantic");

      menu.add("endsub:");
   }

   RHistPainter.prototype.DrawColorPalette = function(/*enabled, postpone_draw, can_move*/) {
      // only when create new palette, one could change frame size

      return null;
   }

   RHistPainter.prototype.ToggleColz = function() {
      let can_toggle = this.options.Mode3D ? (this.options.Lego === 12 || this.options.Lego === 14 || this.options.Surf === 11 || this.options.Surf === 12) :
         this.options.Color || this.options.Contour;

      if (can_toggle) {
         this.options.Zscale = !this.options.Zscale;
         this.DrawColorPalette(this.options.Zscale, false, true);
      }
   }

   RHistPainter.prototype.ToggleMode3D = function() {
      this.options.Mode3D = !this.options.Mode3D;

      if (this.options.Mode3D) {
         if (!this.options.Surf && !this.options.Lego && !this.options.Error) {
            if ((this.nbinsx>=50) || (this.nbinsy>=50))
               this.options.Lego = this.options.Color ? 14 : 13;
            else
               this.options.Lego = this.options.Color ? 12 : 1;

            this.options.Zero = false; // do not show zeros by default
         }
      }

      this.CopyOptionsToOthers();
      this.InteractiveRedraw("pad", "drawopt");
   }

   /** @summary Calculate histogram inidicies and axes values for each visible bin */
   RHistPainter.prototype.PrepareDraw = function(args) {

      if (!args) args = { rounding: true, extra: 0, middle: 0 };

      if (args.extra === undefined) args.extra = 0;
      if (args.right_extra === undefined) args.right_extra = args.extra;
      if (args.middle === undefined) args.middle = 0;

      let histo = this.GetHisto(), xaxis = this.GetAxis("x"), yaxis = this.GetAxis("y"),
          pmain = this.frame_painter(),
          hdim = this.Dimension(),
          i, j, x, y, binz, binarea,
          res = {
             i1: this.GetSelectIndex("x", "left", 0 - args.extra),
             i2: this.GetSelectIndex("x", "right", 1 + args.right_extra),
             j1: (hdim < 2) ? 0 : this.GetSelectIndex("y", "left", 0 - args.extra),
             j2: (hdim < 2) ? 1 : this.GetSelectIndex("y", "right", 1 + args.right_extra),
             k1: (hdim < 3) ? 0 : this.GetSelectIndex("z", "left", 0 - args.extra),
             k2: (hdim < 3) ? 1 : this.GetSelectIndex("z", "right", 1 + args.right_extra),
             stepi: 1, stepj: 1, stepk: 1,
             min: 0, max: 0, sumz: 0, xbar1: 0, xbar2: 1, ybar1: 0, ybar2: 1
          };

      if (this.IsDisplayItem() && histo.fIndicies) {
         if (res.i1 < histo.fIndicies[0]) { res.i1 = histo.fIndicies[0]; res.incomplete = true; }
         if (res.i2 > histo.fIndicies[1]) { res.i2 = histo.fIndicies[1]; res.incomplete = true; }
         res.stepi = histo.fIndicies[2];
         if (res.stepi > 1) res.incomplete = true;
         if ((hdim > 1) && (histo.fIndicies.length > 5)) {
            if (res.j1 < histo.fIndicies[3]) { res.j1 = histo.fIndicies[3]; res.incomplete = true; }
            if (res.j2 > histo.fIndicies[4]) { res.j2 = histo.fIndicies[4]; res.incomplete = true; }
            res.stepj = histo.fIndicies[5];
            if (res.stepj > 1) res.incomplete = true;
         }
         if ((hdim > 2) && (histo.fIndicies.length > 8)) {
            if (res.k1 < histo.fIndicies[6]) { res.k1 = histo.fIndicies[6]; res.incomplete = true; }
            if (res.k2 > histo.fIndicies[7]) { res.k2 = histo.fIndicies[7]; res.incomplete = true; }
            res.stepk = histo.fIndicies[8];
            if (res.stepk > 1) res.incomplete = true;
         }
      }

      if (args.only_indexes) return res;

      // no need for Float32Array, plain Array is 10% faster
      // reserve more places to avoid complex boundary checks

      res.grx = new Array(res.i2+res.stepi+1);
      res.gry = new Array(res.j2+res.stepj+1);

      if (args.original) {
         res.original = true;
         res.origx = new Array(res.i2+1);
         res.origy = new Array(res.j2+1);
      }

      if (args.pixel_density) args.rounding = true;

       // calculate graphical coordinates in advance
      for (i = res.i1; i <= res.i2; ++i) {
         x = xaxis.GetBinCoord(i + args.middle);
         if (pmain.logx && (x <= 0)) { res.i1 = i+1; continue; }
         if (res.origx) res.origx[i] = x;
         res.grx[i] = pmain.grx(x);
         if (args.rounding) res.grx[i] = Math.round(res.grx[i]);

         if (args.use3d) {
            if (res.grx[i] < -pmain.size_xy3d) { res.i1 = i; res.grx[i] = -pmain.size_xy3d; }
            if (res.grx[i] > pmain.size_xy3d) { res.i2 = i; res.grx[i] = pmain.size_xy3d; }
         }
      }

      if (args.use3d) {
         if ((res.i1 < res.i2-2) && (res.grx[res.i1] == res.grx[res.i1+1])) res.i1++;
         if ((res.i1 < res.i2-2) && (res.grx[res.i2-1] == res.grx[res.i2])) res.i2--;
      }

      // copy last valid value to higher indicies
      while (i < res.i2 + res.stepi + 1)
         res.grx[i++] = res.grx[res.i2];

      if (hdim===1) {
         res.gry[0] = pmain.gry(0);
         res.gry[1] = pmain.gry(1);
      } else
      for (j = res.j1; j <= res.j2; ++j) {
         y = yaxis.GetBinCoord(j + args.middle);
         if (pmain.logy && (y <= 0)) { res.j1 = j+1; continue; }
         if (res.origy) res.origy[j] = y;
         res.gry[j] = pmain.gry(y);
         if (args.rounding) res.gry[j] = Math.round(res.gry[j]);

         if (args.use3d) {
            if (res.gry[j] < -pmain.size_xy3d) { res.j1 = j; res.gry[j] = -pmain.size_xy3d; }
            if (res.gry[j] > pmain.size_xy3d) { res.j2 = j; res.gry[j] = pmain.size_xy3d; }
         }
      }

      if (args.use3d && (hdim > 1)) {
         if ((res.j1 < res.j2-2) && (res.gry[res.j1] == res.gry[res.j1+1])) res.j1++;
         if ((res.j1 < res.j2-2) && (res.gry[res.j2-1] == res.gry[res.j2])) res.j2--;
      }

      // copy last valid value to higher indicies
      while ((hdim > 1) && (j < res.j2 + res.stepj + 1))
         res.gry[j++] = res.gry[res.j2];

      //  find min/max values in selected range
      binz = histo.getBinContent(res.i1 + 1, res.j1 + 1);
      this.maxbin = this.minbin = this.minposbin = null;

      for (i = res.i1; i < res.i2; i += res.stepi) {
         for (j = res.j1; j < res.j2; j += res.stepj) {
            binz = histo.getBinContent(i + 1, j + 1);
            if (isNaN(binz)) continue;
            res.sumz += binz;
            if (args.pixel_density) {
               binarea = (res.grx[i+res.stepi]-res.grx[i])*(res.gry[j]-res.gry[j+res.stepj]);
               if (binarea <= 0) continue;
               res.max = Math.max(res.max, binz);
               if ((binz>0) && ((binz<res.min) || (res.min===0))) res.min = binz;
               binz = binz/binarea;
            }
            if (this.maxbin===null) {
               this.maxbin = this.minbin = binz;
            } else {
               this.maxbin = Math.max(this.maxbin, binz);
               this.minbin = Math.min(this.minbin, binz);
            }
            if (binz > 0)
               if ((this.minposbin===null) || (binz<this.minposbin)) this.minposbin = binz;
         }
      }

      res.palette = pmain.GetPalette();

      if (res.palette)
         this.CreateContour(pmain, res.palette, args);

      return res;
   }

   // ======= RH1 painter================================================

   function RH1Painter(histo) {
      RHistPainter.call(this, histo);
      this.wheel_zoomy = false;
   }

   RH1Painter.prototype = Object.create(RHistPainter.prototype);

   RH1Painter.prototype.ScanContent = function(when_axis_changed) {
      // if when_axis_changed === true specified, content will be scanned after axis zoom changed

      let histo = this.GetHisto();
      if (!histo) return;

      if (!this.nbinsx && when_axis_changed) when_axis_changed = false;

      if (!when_axis_changed)
         this.ExtractAxesProperties(1);

      let hmin = 0, hmin_nz = 0, hmax = 0, hsum = 0;

      if (this.IsDisplayItem()) {
         // take min/max values from the display item
         hmin = histo.fContMin;
         hmin_nz = histo.fContMinPos;
         hmax = histo.fContMax;
         hsum = hmax;
      } else {

         let left = this.GetSelectIndex("x", "left"),
             right = this.GetSelectIndex("x", "right");

         if (when_axis_changed) {
            if ((left === this.scan_xleft) && (right === this.scan_xright)) return;
         }

         this.scan_xleft = left;
         this.scan_xright = right;

         let first = true, value, err;

         for (let i = 0; i < this.nbinsx; ++i) {
            value = histo.getBinContent(i+1);
            hsum += value;

            if ((i<left) || (i>=right)) continue;

            if (value > 0)
               if ((hmin_nz == 0) || (value<hmin_nz)) hmin_nz = value;
            if (first) {
               hmin = hmax = value;
               first = false;
            }

            err =  0;

            hmin = Math.min(hmin, value - err);
            hmax = Math.max(hmax, value + err);
         }
      }

      this.stat_entries = hsum;

      this.hmin = hmin;
      this.hmax = hmax;

      this.ymin_nz = hmin_nz; // value can be used to show optimal log scale

      if ((this.nbinsx == 0) || ((Math.abs(hmin) < 1e-300 && Math.abs(hmax) < 1e-300))) {
         this.draw_content = false;
      } else {
         this.draw_content = true;
      }

      if (this.draw_content) {
         if (hmin >= hmax) {
            if (hmin == 0) { this.ymin = 0; this.ymax = 1; }
            else if (hmin < 0) { this.ymin = 2 * hmin; this.ymax = 0; }
            else { this.ymin = 0; this.ymax = hmin * 2; }
         } else {
            let dy = (hmax - hmin) * 0.05;
            this.ymin = hmin - dy;
            if ((this.ymin < 0) && (hmin >= 0)) this.ymin = 0;
            this.ymax = hmax + dy;
         }
      }
   }

   RH1Painter.prototype.CountStat = function(cond) {
      let profile = this.IsRProfile(),
          histo = this.GetHisto(), xaxis = this.GetAxis("x"),
          left = this.GetSelectIndex("x", "left"),
          right = this.GetSelectIndex("x", "right"),
          stat_sumw = 0, stat_sumwx = 0, stat_sumwx2 = 0, stat_sumwy = 0, stat_sumwy2 = 0,
          i, xx = 0, w = 0, xmax = null, wmax = null,
          fp = this.frame_painter(),
          res = { name: "histo", meanx: 0, meany: 0, rmsx: 0, rmsy: 0, integral: 0, entries: this.stat_entries, xmax:0, wmax:0 };

      for (i = left; i < right; ++i) {
         xx = xaxis.GetBinCoord(i+0.5);

         if (cond && !cond(xx)) continue;

         if (profile) {
            w = histo.fBinEntries[i + 1];
            stat_sumwy += histo.fArray[i + 1];
            stat_sumwy2 += histo.fSumw2[i + 1];
         } else {
            w = histo.getBinContent(i + 1);
         }

         if ((xmax===null) || (w>wmax)) { xmax = xx; wmax = w; }

         stat_sumw += w;
         stat_sumwx += w * xx;
         stat_sumwx2 += w * xx * xx;
      }

      // when no range selection done, use original statistic from histogram
      if (!fp.IsAxisZoomed("x") && histo.fTsumw) {
         stat_sumw = histo.fTsumw;
         stat_sumwx = histo.fTsumwx;
         stat_sumwx2 = histo.fTsumwx2;
      }

      res.integral = stat_sumw;

      if (stat_sumw > 0) {
         res.meanx = stat_sumwx / stat_sumw;
         res.meany = stat_sumwy / stat_sumw;
         res.rmsx = Math.sqrt(Math.abs(stat_sumwx2 / stat_sumw - res.meanx * res.meanx));
         res.rmsy = Math.sqrt(Math.abs(stat_sumwy2 / stat_sumw - res.meany * res.meany));
      }

      if (xmax!==null) {
         res.xmax = xmax;
         res.wmax = wmax;
      }

      return res;
   }

   RH1Painter.prototype.FillStatistic = function(stat, dostat/*, dofit*/) {

      // no need to refill statistic if histogram is dummy
      if (this.IgnoreStatsFill()) return false;

      let data = this.CountStat(),
          print_name = dostat % 10,
          print_entries = Math.floor(dostat / 10) % 10,
          print_mean = Math.floor(dostat / 100) % 10,
          print_rms = Math.floor(dostat / 1000) % 10,
          print_under = Math.floor(dostat / 10000) % 10,
          print_over = Math.floor(dostat / 100000) % 10,
          print_integral = Math.floor(dostat / 1000000) % 10,
          print_skew = Math.floor(dostat / 10000000) % 10,
          print_kurt = Math.floor(dostat / 100000000) % 10;

      // make empty at the beginning
      stat.ClearStat();

      if (print_name > 0)
         stat.AddText(data.name);

      if (this.IsRProfile()) {

         if (print_entries > 0)
            stat.AddText("Entries = " + stat.Format(data.entries,"entries"));

         if (print_mean > 0) {
            stat.AddText("Mean = " + stat.Format(data.meanx));
            stat.AddText("Mean y = " + stat.Format(data.meany));
         }

         if (print_rms > 0) {
            stat.AddText("Std Dev = " + stat.Format(data.rmsx));
            stat.AddText("Std Dev y = " + stat.Format(data.rmsy));
         }

      } else {

         if (print_entries > 0)
            stat.AddText("Entries = " + stat.Format(data.entries,"entries"));

         if (print_mean > 0)
            stat.AddText("Mean = " + stat.Format(data.meanx));

         if (print_rms > 0)
            stat.AddText("Std Dev = " + stat.Format(data.rmsx));

         if (print_under > 0)
            stat.AddText("Underflow = " + stat.Format(histo.getBinContent(0), "entries"));

         if (print_over > 0)
            stat.AddText("Overflow = " + stat.Format(histo.getBinContent(this.nbinsx+1), "entries"));

         if (print_integral > 0)
            stat.AddText("Integral = " + stat.Format(data.integral,"entries"));

         if (print_skew > 0)
            stat.AddText("Skew = <not avail>");

         if (print_kurt > 0)
            stat.AddText("Kurt = <not avail>");
      }

      // if (dofit) stat.FillFunctionStat(this.FindFunction('TF1'), dofit);

      return true;
   }

   RH1Painter.prototype.DrawBars = function(handle, width, height) {

      this.CreateG(true);

      let left = handle.i1, right = handle.i2, di = handle.stepi,
          pmain = this.frame_painter(),
          histo = this.GetHisto(), xaxis = this.GetAxis("x"),
          i, x1, x2, grx1, grx2, y, gry1, gry2, w,
          bars = "", barsl = "", barsr = "";

      gry2 = pmain.swap_xy ? 0 : height;
      if ((this.options.BaseLine !== false) && !isNaN(this.options.BaseLine))
         if (this.options.BaseLine >= pmain.scale_ymin)
            gry2 = Math.round(pmain.gry(this.options.BaseLine));

      for (i = left; i < right; i += di) {
         x1 = xaxis.GetBinCoord(i);
         x2 = xaxis.GetBinCoord(i+di);

         if (pmain.logx && (x2 <= 0)) continue;

         grx1 = Math.round(pmain.grx(x1));
         grx2 = Math.round(pmain.grx(x2));

         y = histo.getBinContent(i+1);
         if (pmain.logy && (y < pmain.scale_ymin)) continue;
         gry1 = Math.round(pmain.gry(y));

         w = grx2 - grx1;
         grx1 += Math.round(this.options.BarOffset*w);
         w = Math.round(this.options.BarWidth*w);

         if (pmain.swap_xy)
            bars += "M"+gry2+","+grx1 + "h"+(gry1-gry2) + "v"+w + "h"+(gry2-gry1) + "z";
         else
            bars += "M"+grx1+","+gry1 + "h"+w + "v"+(gry2-gry1) + "h"+(-w)+ "z";

         if (this.options.BarStyle > 0) {
            grx2 = grx1 + w;
            w = Math.round(w / 10);
            if (pmain.swap_xy) {
               barsl += "M"+gry2+","+grx1 + "h"+(gry1-gry2) + "v" + w + "h"+(gry2-gry1) + "z";
               barsr += "M"+gry2+","+grx2 + "h"+(gry1-gry2) + "v" + (-w) + "h"+(gry2-gry1) + "z";
            } else {
               barsl += "M"+grx1+","+gry1 + "h"+w + "v"+(gry2-gry1) + "h"+(-w)+ "z";
               barsr += "M"+grx2+","+gry1 + "h"+(-w) + "v"+(gry2-gry1) + "h"+w + "z";
            }
         }
      }

      if (this.fillatt.empty()) this.fillatt.SetSolidColor("blue");

      if (bars.length > 0)
         this.draw_g.append("svg:path")
                    .attr("d", bars)
                    .call(this.fillatt.func);

      if (barsl.length > 0)
         this.draw_g.append("svg:path")
               .attr("d", barsl)
               .call(this.fillatt.func)
               .style("fill", d3.rgb(this.fillatt.color).brighter(0.5).toString());

      if (barsr.length > 0)
         this.draw_g.append("svg:path")
               .attr("d", barsr)
               .call(this.fillatt.func)
               .style("fill", d3.rgb(this.fillatt.color).darker(0.5).toString());
   }

   RH1Painter.prototype.DrawFilledErrors = function(handle /*, width, height*/) {
      this.CreateG(true);

      let left = handle.i1, right = handle.i2, di = handle.stepi,
          pmain = this.frame_painter(),
          histo = this.GetHisto(), xaxis = this.GetAxis("x"),
          i, x, grx, y, yerr, gry1, gry2,
          bins1 = [], bins2 = [];

      for (i = left; i < right; i += di) {
         x = xaxis.GetBinCoord(i+0.5);
         if (pmain.logx && (x <= 0)) continue;
         grx = Math.round(pmain.grx(x));

         y = histo.getBinContent(i+1);
         yerr = histo.getBinError(i+1);
         if (pmain.logy && (y-yerr < pmain.scale_ymin)) continue;

         gry1 = Math.round(pmain.gry(y + yerr));
         gry2 = Math.round(pmain.gry(y - yerr));

         bins1.push({grx:grx, gry: gry1});
         bins2.unshift({grx:grx, gry: gry2});
      }

      let kind = (this.options.ErrorKind === 4) ? "bezier" : "line",
          path1 = jsrp.BuildSvgPath(kind, bins1),
          path2 = jsrp.BuildSvgPath("L"+kind, bins2);

      if (this.fillatt.empty()) this.fillatt.setSolidColor("blue");

      this.draw_g.append("svg:path")
                 .attr("d", path1.path + path2.path + "Z")
                 .style("stroke", "none")
                 .call(this.fillatt.func);
   }

   /** @summary Draw 1D histogram as SVG */
   RH1Painter.prototype.Draw1DBins = function() {

      let width = this.frame_width(), height = this.frame_height();

      if (!this.draw_content || (width<=0) || (height<=0))
         return this.RemoveDrawG();

      this.CheckHistDrawAttributes();

      let handle = this.PrepareDraw({ extra: 1, only_indexes: true });

      if (this.options.Bar)
         return this.DrawBars(handle, width, height);

      if ((this.options.ErrorKind === 3) || (this.options.ErrorKind === 4))
         return this.DrawFilledErrors(handle, width, height);

      return this.DrawHistBins(handle, width, height);
   }

   RH1Painter.prototype.DrawHistBins = function(handle, width, height) {
      this.CreateG(true);

      let options = this.options,
          left = handle.i1,
          right = handle.i2,
          di = handle.stepi,
          pmain = this.frame_painter(),
          histo = this.GetHisto(), xaxis = this.GetAxis("x"),
          res = "", lastbin = false,
          startx, currx, curry, x, grx, y, gry, curry_min, curry_max, prevy, prevx, i, bestimin, bestimax,
          exclude_zero = !options.Zero,
          show_errors = options.Error,
          show_markers = options.Mark,
          show_line = options.Line,
          show_text = options.Text,
          text_profile = show_text && (this.options.TextKind == "E") && this.IsRProfile(),
          path_fill = null, path_err = null, path_marker = null, path_line = null,
          endx = "", endy = "", dend = 0, my, yerr1, yerr2, bincont, binerr, mx1, mx2, midx,
          text_font;

      if (show_errors && !show_markers && (this.v7EvalAttr("marker_style",1) > 1))
         show_markers = true;

      if (options.ErrorKind === 2) {
         if (this.fillatt.empty()) show_markers = true;
                              else path_fill = "";
      } else if (options.Error) {
         path_err = "";
      }

      if (show_line) path_line = "";

      if (show_markers) {
         // draw markers also when e2 option was specified
         this.createv7AttMarker();
         if (this.markeratt.size > 0) {
            // simply use relative move from point, can optimize in the future
            path_marker = "";
            this.markeratt.reset_pos();
         } else {
            show_markers = false;
         }
      }

      if (show_text) {
         text_font = this.v7EvalFont("text", { size: 20, color: "black", align: 22 });

         if (!text_font.angle && !options.TextKind) {
             let space = width / (right - left + 1);
             if (space < 3 * text_font.size) {
                text_font.setAngle(270);
                text_font.setSize(Math.round(space*0.7));
             }
         }

         this.StartTextDrawing(text_font, 'font');
      }

      // if there are too many points, exclude many vertical drawings at the same X position
      // instead define min and max value and made min-max drawing
      let use_minmax = ((right-left) > 3*width);

      if (options.ErrorKind === 1) {
         let lw = this.lineatt.width + JSROOT.gStyle.fEndErrorSize;
         endx = "m0," + lw + "v-" + 2*lw + "m0," + lw;
         endy = "m" + lw + ",0h-" + 2*lw + "m" + lw + ",0";
         dend = Math.floor((this.lineatt.width-1)/2);
      }

      let draw_markers = show_errors || show_markers;

      if (draw_markers || show_text || show_line) use_minmax = true;

      let draw_bin = besti => {
         bincont = histo.getBinContent(besti+1);
         if (!exclude_zero || (bincont!==0)) {
            mx1 = Math.round(pmain.grx(xaxis.GetBinCoord(besti)));
            mx2 = Math.round(pmain.grx(xaxis.GetBinCoord(besti+di)));
            midx = Math.round((mx1+mx2)/2);
            my = Math.round(pmain.gry(bincont));
            yerr1 = yerr2 = 20;
            if (show_errors) {
               binerr = histo.getBinError(besti+1);
               yerr1 = Math.round(my - pmain.gry(bincont + binerr)); // up
               yerr2 = Math.round(pmain.gry(bincont - binerr) - my); // down
            }

            if (show_text) {
               let cont = text_profile ? histo.fBinEntries[besti+1] : bincont;

               if (cont!==0) {
                  let lbl = (cont === Math.round(cont)) ? cont.toString() : JSROOT.FFormat(cont, JSROOT.gStyle.fPaintTextFormat);

                  if (text_font.angle)
                     this.DrawText({ align: 12, x: midx, y: Math.round(my - 2 - text_font.size/5), width: 0, height: 0, text: lbl, latex: 0 });
                  else
                     this.DrawText({ x: Math.round(mx1 + (mx2-mx1)*0.1), y: Math.round(my-2-text_font.size), width: Math.round((mx2-mx1)*0.8), height: text_font.size, text: lbl, latex: 0 });
               }
            }

            if (show_line && (path_line !== null))
               path_line += ((path_line.length===0) ? "M" : "L") + midx + "," + my;

            if (draw_markers) {
               if ((my >= -yerr1) && (my <= height + yerr2)) {
                  if (path_fill !== null)
                     path_fill += "M" + mx1 +","+(my-yerr1) +
                                  "h" + (mx2-mx1) + "v" + (yerr1+yerr2+1) + "h-" + (mx2-mx1) + "z";
                  if (path_marker !== null)
                     path_marker += this.markeratt.create(midx, my);
                  if (path_err !== null) {
                     if (this.options.errorX > 0) {
                        let mmx1 = Math.round(midx - (mx2-mx1)*this.options.errorX),
                            mmx2 = Math.round(midx + (mx2-mx1)*this.options.errorX);
                        path_err += "M" + (mmx1+dend) +","+ my + endx + "h" + (mmx2-mmx1-2*dend) + endx;
                     }
                     path_err += "M" + midx +"," + (my-yerr1+dend) + endy + "v" + (yerr1+yerr2-2*dend) + endy;
                  }
               }
            }
         }
      }

      for (i = left; i <= right; i += di) {

         x = xaxis.GetBinCoord(i);

         if (pmain.logx && (x <= 0)) continue;

         grx = Math.round(pmain.grx(x));

         lastbin = (i > right - di);

         if (lastbin && (left < right)) {
            gry = curry;
         } else {
            y = histo.getBinContent(i+1);
            gry = Math.round(pmain.gry(y));
         }

         if (res.length === 0) {
            bestimin = bestimax = i;
            prevx = startx = currx = grx;
            prevy = curry_min = curry_max = curry = gry;
            res = "M"+currx+","+curry;
         } else
         if (use_minmax) {
            if ((grx === currx) && !lastbin) {
               if (gry < curry_min) bestimax = i; else
               if (gry > curry_max) bestimin = i;
               curry_min = Math.min(curry_min, gry);
               curry_max = Math.max(curry_max, gry);
               curry = gry;
            } else {

               if (draw_markers || show_text || show_line) {
                  if (bestimin === bestimax) { draw_bin(bestimin); } else
                     if (bestimin < bestimax) { draw_bin(bestimin); draw_bin(bestimax); } else {
                        draw_bin(bestimax); draw_bin(bestimin);
                     }
               }

               // when several points as same X differs, need complete logic
               if (!draw_markers && ((curry_min !== curry_max) || (prevy !== curry_min))) {

                  if (prevx !== currx)
                     res += "h"+(currx-prevx);

                  if (curry === curry_min) {
                     if (curry_max !== prevy)
                        res += "v" + (curry_max - prevy);
                     if (curry_min !== curry_max)
                        res += "v" + (curry_min - curry_max);
                  } else {
                     if (curry_min !== prevy)
                        res += "v" + (curry_min - prevy);
                     if (curry_max !== curry_min)
                        res += "v" + (curry_max - curry_min);
                     if (curry !== curry_max)
                       res += "v" + (curry - curry_max);
                  }

                  prevx = currx;
                  prevy = curry;
               }

               if (lastbin && (prevx !== grx))
                  res += "h"+(grx-prevx);

               bestimin = bestimax = i;
               curry_min = curry_max = curry = gry;
               currx = grx;
            }
         } else
         if ((gry !== curry) || lastbin) {
            if (grx !== currx) res += "h"+(grx-currx);
            if (gry !== curry) res += "v"+(gry-curry);
            curry = gry;
            currx = grx;
         }
      }

      let close_path = "";
      let fill_for_interactive = !JSROOT.BatchMode && this.fillatt.empty() && options.Hist && JSROOT.settings.Tooltip && !draw_markers && !show_line;
      if (!this.fillatt.empty() || fill_for_interactive) {
         let h0 = height + 3;
         if (fill_for_interactive) {
            let gry0 = Math.round(pmain.gry(0));
            if (gry0 <= 0) h0 = -3; else if (gry0 < height) h0 = gry0;
         }
         close_path = "L"+currx+","+h0 + "L"+startx+","+h0 + "Z";
         if (res.length>0) res += close_path;
      }

      if (draw_markers || show_line) {
         if ((path_fill !== null) && (path_fill.length > 0))
            this.draw_g.append("svg:path")
                       .attr("d", path_fill)
                       .call(this.fillatt.func);

         if ((path_err !== null) && (path_err.length > 0))
               this.draw_g.append("svg:path")
                   .attr("d", path_err)
                   .call(this.lineatt.func);

         if ((path_line !== null) && (path_line.length > 0)) {
            if (!this.fillatt.empty())
               this.draw_g.append("svg:path")
                     .attr("d", options.Fill ? (path_line + close_path) : res)
                     .attr("stroke", "none")
                     .call(this.fillatt.func);

            this.draw_g.append("svg:path")
                   .attr("d", path_line)
                   .attr("fill", "none")
                   .call(this.lineatt.func);
         }

         if ((path_marker !== null) && (path_marker.length > 0))
            this.draw_g.append("svg:path")
                .attr("d", path_marker)
                .call(this.markeratt.func);

      } else if (res && options.Hist) {
         this.draw_g.append("svg:path")
                    .attr("d", res)
                    .style("stroke-linejoin","miter")
                    .call(this.lineatt.func)
                    .call(this.fillatt.func);
      }

      if (show_text)
         this.FinishTextDrawing(this.draw_g);
   }

   /** @summary Get tip text for axis bin */
   RHistPainter.prototype.GetAxisBinTip = function(name, bin, step) {
      let pmain = this.frame_painter(),
          handle = pmain[name+"_handle"],
          axis = this.GetAxis(name),
          x1 = axis.GetBinCoord(bin);


      if (handle.kind === 'labels')
         return pmain.AxisAsText(name, x1);

      let x2 = axis.GetBinCoord(bin+(step || 1));

      if (handle.kind === 'time')
         return pmain.AxisAsText(name, (x1+x2)/2);

      return "[" + pmain.AxisAsText(name, x1) + ", " + pmain.AxisAsText(name, x2) + ")";
   }


   RH1Painter.prototype.GetBinTips = function(bin) {
      let tips = [],
          name = this.GetTipName(),
          pmain = this.frame_painter(),
          histo = this.GetHisto(),
          xaxis = this.GetAxis("x"),
          di = this.IsDisplayItem() ? histo.stepx : 1,
          x1 = xaxis.GetBinCoord(bin),
          x2 = xaxis.GetBinCoord(bin+di),
          cont = histo.getBinContent(bin+1),
          xlbl = this.GetAxisBinTip("x", bin, di);

      if (name.length>0) tips.push(name);

      if (this.options.Error || this.options.Mark) {
         tips.push("x = " + xlbl);
         tips.push("y = " + pmain.AxisAsText("y", cont));
         if (this.options.Error) {
            if (xlbl[0] == "[") tips.push("error x = " + ((x2 - x1) / 2).toPrecision(4));
            tips.push("error y = " + histo.getBinError(bin + 1).toPrecision(4));
         }
      } else {
         tips.push("bin = " + bin);
         tips.push("x = " + xlbl);
         if (histo['$baseh']) cont -= histo['$baseh'].getBinContent(bin+1);
         let lbl = "entries = " + (di > 1 ? "~" : "");
         if (cont === Math.round(cont))
            tips.push(lbl + cont);
         else
            tips.push(lbl + JSROOT.FFormat(cont, JSROOT.gStyle.fStatFormat));
      }

      return tips;
   }

   RH1Painter.prototype.ProcessTooltip = function(pnt) {
      if ((pnt === null) || !this.draw_content || this.options.Mode3D || !this.draw_g) {
         if (this.draw_g)
            this.draw_g.select(".tooltip_bin").remove();
         return null;
      }

      let width = this.frame_width(),
          height = this.frame_height(),
          pmain = this.frame_painter(),
          histo = this.GetHisto(), xaxis = this.GetAxis("x"),
          findbin = null, show_rect = true,
          grx1, midx, grx2, gry1, midy, gry2, gapx = 2,
          left = this.GetSelectIndex("x", "left", -1),
          right = this.GetSelectIndex("x", "right", 2),
          l = left, r = right;

      function GetBinGrX(i) {
         let xx = xaxis.GetBinCoord(i);
         return (pmain.logx && (xx<=0)) ? null : pmain.grx(xx);
      }

      function GetBinGrY(i) {
         let yy = histo.getBinContent(i + 1);
         if (pmain.logy && (yy < pmain.scale_ymin))
            return pmain.swap_xy ? -1000 : 10*height;
         return Math.round(pmain.gry(yy));
      }

      let pnt_x = pmain.swap_xy ? pnt.y : pnt.x,
          pnt_y = pmain.swap_xy ? pnt.x : pnt.y;

      while (l < r-1) {
         let m = Math.round((l+r)*0.5),
             xx = GetBinGrX(m);
         if ((xx === null) || (xx < pnt_x - 0.5)) {
            if (pmain.swap_xy) r = m; else l = m;
         } else if (xx > pnt_x + 0.5) {
            if (pmain.swap_xy) l = m; else r = m;
         } else { l++; r--; }
      }

      findbin = r = l;
      grx1 = GetBinGrX(findbin);

      if (pmain.swap_xy) {
         while ((l>left) && (GetBinGrX(l-1) < grx1 + 2)) --l;
         while ((r<right) && (GetBinGrX(r+1) > grx1 - 2)) ++r;
      } else {
         while ((l>left) && (GetBinGrX(l-1) > grx1 - 2)) --l;
         while ((r<right) && (GetBinGrX(r+1) < grx1 + 2)) ++r;
      }

      if (l < r) {
         // many points can be assigned with the same cursor position
         // first try point around mouse y
         let best = height;
         for (let m=l;m<=r;m++) {
            let dist = Math.abs(GetBinGrY(m) - pnt_y);
            if (dist < best) { best = dist; findbin = m; }
         }

         // if best distance still too far from mouse position, just take from between
         if (best > height/10)
            findbin = Math.round(l + (r-l) / height * pnt_y);

         grx1 = GetBinGrX(findbin);
      }

      grx1 = Math.round(grx1);
      grx2 = Math.round(GetBinGrX(findbin+1));

      if (this.options.Bar) {
         let w = grx2 - grx1;
         grx1 += Math.round(this.options.BarOffset*w);
         grx2 = grx1 + Math.round(this.options.BarWidth*w);
      }

      if (grx1 > grx2) { let d = grx1; grx1 = grx2; grx2 = d; }

      midx = Math.round((grx1+grx2)/2);

      midy = gry1 = gry2 = GetBinGrY(findbin);

      if (this.options.Bar) {
         show_rect = true;

         gapx = 0;

         gry1 = Math.round(pmain.gry(((this.options.BaseLine!==false) && (this.options.BaseLine > pmain.scale_ymin)) ? this.options.BaseLine : pmain.scale_ymin));

         if (gry1 > gry2) { let d = gry1; gry1 = gry2; gry2 = d; }

         if (!pnt.touch && (pnt.nproc === 1))
            if ((pnt_y<gry1) || (pnt_y>gry2)) findbin = null;

      } else if (this.options.Error || this.options.Mark) {

         show_rect = true;

         let msize = 3;
         if (this.markeratt) msize = Math.max(msize, this.markeratt.GetFullSize());

         if (this.options.Error) {
            let cont = histo.getBinContent(findbin+1),
                binerr = histo.getBinError(findbin+1);

            gry1 = Math.round(pmain.gry(cont + binerr)); // up
            gry2 = Math.round(pmain.gry(cont - binerr)); // down

            if ((cont==0) && this.IsRProfile()) findbin = null;

            let dx = (grx2-grx1)*this.options.errorX;
            grx1 = Math.round(midx - dx);
            grx2 = Math.round(midx + dx);
         }

         // show at least 6 pixels as tooltip rect
         if (grx2 - grx1 < 2*msize) { grx1 = midx-msize; grx2 = midx+msize; }

         gry1 = Math.min(gry1, midy - msize);
         gry2 = Math.max(gry2, midy + msize);

         if (!pnt.touch && (pnt.nproc === 1))
            if ((pnt_y<gry1) || (pnt_y>gry2)) findbin = null;

      } else if (this.options.Line) {

         show_rect = false;

      } else {

         // if histogram alone, use old-style with rects
         // if there are too many points at pixel, use circle
         show_rect = (pnt.nproc === 1) && (right-left < width);

         if (show_rect) {
            gry2 = height;

            if (!this.fillatt.empty()) {
               gry2 = Math.round(pmain.gry(0));
               if (gry2 < 0) gry2 = 0; else if (gry2 > height) gry2 = height;
               if (gry2 < gry1) { let d = gry1; gry1 = gry2; gry2 = d; }
            }

            // for mouse events pointer should be between y1 and y2
            if (((pnt.y < gry1) || (pnt.y > gry2)) && !pnt.touch) findbin = null;
         }
      }

      if (findbin!==null) {
         // if bin on boundary found, check that x position is ok
         if ((findbin === left) && (grx1 > pnt_x + gapx))  findbin = null; else
         if ((findbin === right-1) && (grx2 < pnt_x - gapx)) findbin = null; else
         // if bars option used check that bar is not match
         if ((pnt_x < grx1 - gapx) || (pnt_x > grx2 + gapx)) findbin = null; else
         // exclude empty bin if empty bins suppressed
         if (!this.options.Zero && (histo.getBinContent(findbin+1)===0)) findbin = null;
      }

      let ttrect = this.draw_g.select(".tooltip_bin");

      if ((findbin === null) || ((gry2 <= 0) || (gry1 >= height))) {
         ttrect.remove();
         return null;
      }

      let res = { name: "histo", title: histo.fTitle,
                  x: midx, y: midy, exact: true,
                  color1: this.lineatt ? this.lineatt.color : 'green',
                  color2: this.fillatt ? this.fillatt.fillcoloralt('blue') : 'blue',
                  lines: this.GetBinTips(findbin) };

      if (pnt.disabled) {
         // case when tooltip should not highlight bin

         ttrect.remove();
         res.changed = true;
      } else if (show_rect) {

         if (ttrect.empty())
            ttrect = this.draw_g.append("svg:rect")
                                .attr("class","tooltip_bin h1bin")
                                .style("pointer-events","none");

         res.changed = ttrect.property("current_bin") !== findbin;

         if (res.changed)
            ttrect.attr("x", pmain.swap_xy ? gry1 : grx1)
                  .attr("width", pmain.swap_xy ? gry2-gry1 : grx2-grx1)
                  .attr("y", pmain.swap_xy ? grx1 : gry1)
                  .attr("height", pmain.swap_xy ? grx2-grx1 : gry2-gry1)
                  .style("opacity", "0.3")
                  .property("current_bin", findbin);

         res.exact = (Math.abs(midy - pnt_y) <= 5) || ((pnt_y>=gry1) && (pnt_y<=gry2));

         res.menu = true; // one could show context menu
         // distance to middle point, use to decide which menu to activate
         res.menu_dist = Math.sqrt((midx-pnt_x)*(midx-pnt_x) + (midy-pnt_y)*(midy-pnt_y));

      } else {
         let radius = this.lineatt.width + 3;

         if (ttrect.empty())
            ttrect = this.draw_g.append("svg:circle")
                                .attr("class","tooltip_bin")
                                .style("pointer-events","none")
                                .attr("r", radius)
                                .call(this.lineatt.func)
                                .call(this.fillatt.func);

         res.exact = (Math.abs(midx - pnt.x) <= radius) && (Math.abs(midy - pnt.y) <= radius);

         res.menu = res.exact; // show menu only when mouse pointer exactly over the histogram
         res.menu_dist = Math.sqrt((midx-pnt.x)*(midx-pnt.x) + (midy-pnt.y)*(midy-pnt.y));

         res.changed = ttrect.property("current_bin") !== findbin;

         if (res.changed)
            ttrect.attr("cx", midx)
                  .attr("cy", midy)
                  .property("current_bin", findbin);
      }

      if (res.changed)
         res.user_info = { obj: histo,  name: "histo",
                           bin: findbin, cont: histo.getBinContent(findbin+1),
                           grx: midx, gry: midy };

      return res;
   }


   RH1Painter.prototype.FillHistContextMenu = function(menu) {

      menu.add("Auto zoom-in", this.AutoZoom);

      let sett = JSROOT.getDrawSettings("ROOT." + this.GetObject()._typename, 'nosame');

      menu.addDrawMenu("Draw with", sett.opts, function(arg) {
         if (arg==='inspect')
            return this.ShowInspector();

         this.DecodeOptions(arg); // obsolete, should be implemented differently

         if (this.options.need_fillcol && this.fillatt && this.fillatt.empty())
            this.fillatt.Change(5,1001);

         // redraw all objects
         this.InteractiveRedraw("pad", "drawopt");
      });
   }

   RH1Painter.prototype.AutoZoom = function() {
      let left = this.GetSelectIndex("x", "left", -1),
          right = this.GetSelectIndex("x", "right", 1),
          dist = right - left, histo = this.GetHisto(), xaxis = this.GetAxis("x");

      if (dist == 0) return;

      // first find minimum
      let min = histo.getBinContent(left + 1);
      for (let indx = left; indx < right; ++indx)
         min = Math.min(min, histo.getBinContent(indx+1));
      if (min > 0) return; // if all points positive, no chance for autoscale

      while ((left < right) && (histo.getBinContent(left+1) <= min)) ++left;
      while ((left < right) && (histo.getBinContent(right) <= min)) --right;

      // if singular bin
      if ((left === right-1) && (left > 2) && (right < this.nbinsx-2)) {
         --left; ++right;
      }

      if ((right - left < dist) && (left < right))
         this.frame_painter().Zoom(xaxis.GetBinCoord(left), xaxis.GetBinCoord(right));
   }

   RH1Painter.prototype.CanZoomIn = function(axis,min,max) {
      let xaxis = this.GetAxis("x");

      if ((axis=="x") && (xaxis.FindBin(max,0.5) - xaxis.FindBin(min,0) > 1)) return true;

      if ((axis=="y") && (Math.abs(max-min) > Math.abs(this.ymax-this.ymin)*1e-6)) return true;

      // check if it makes sense to zoom inside specified axis range
      return false;
   }

   RH1Painter.prototype.CallDrawFunc = function(reason) {
      let main = this.frame_painter();

      if (main && (main.mode3d !== this.options.Mode3D) && !this.is_main_painter())
         this.options.Mode3D = main.mode3d;

      let funcname = this.options.Mode3D ? "Draw3D" : "Draw2D";

      return this[funcname](reason);
   }

   RH1Painter.prototype.Draw2D = function(reason) {
      this.Clear3DScene();
      this.mode3d = false;

      return this.DrawAxes()
                 .then(res => res ? this.DrawingBins(reason) : false)
                 .then(res2 => {
                     if (!res2) return false;
                     // called when bins received from server, must be reentrant
                     this.Draw1DBins();
                     this.UpdateStatWebCanvas();
                     return this.AddInteractive();
                     });
   }

   RH1Painter.prototype.Draw3D = function(reason) {
      this.mode3d = true;
      return JSROOT.require('v7hist3d').then(() => this.Draw3D(reason));
   }

   RH1Painter.prototype.Redraw = function(reason) {
      this.CallDrawFunc(reason);
   }

   let drawHist1 = (divid, histo, opt) => {
      // create painter and add it to canvas
      let painter = new RH1Painter(histo);

      if (!painter.PrepareFrame(divid)) return null;

      painter.options = { Hist: false, Bar: false, BarStyle: 0,
                          Error: false, ErrorKind: -1, errorX: JSROOT.gStyle.fErrorX,
                          Zero: false, Mark: false,
                          Line: false, Fill: false, Lego: 0, Surf: 0,
                          Text: false, TextAngle: 0, TextKind: "", AutoColor: 0,
                          BarOffset: 0., BarWidth: 1., BaseLine: false, Mode3D: false };

      let d = new JSROOT.DrawOptions(opt);
      if (d.check('R3D_', true))
         painter.options.Render3D = JSROOT.constants.Render3D.fromString(d.part.toLowerCase());

      let kind = painter.v7EvalAttr("kind", "hist"),
          sub = painter.v7EvalAttr("sub", 0),
          o = painter.options;

      o.Text = painter.v7EvalAttr("text", false);
      o.BarOffset = painter.v7EvalAttr("bar_offset", 0.);
      o.BarWidth = painter.v7EvalAttr("bar_width", 1.);

      switch(kind) {
         case "bar": o.Bar = true; o.BarStyle = sub; break;
         case "err": o.Error = true; o.ErrorKind = sub; break;
         case "p": o.Mark = true; break;
         case "l": o.Line = true; break;
         case "lego": o.Lego = sub > 0 ? 10+sub : 12; o.Mode3D = true; break;
         default: o.Hist = true;
      }

      painter.ScanContent();

      painter.CallDrawFunc().then(() => {
         // if (!painter.options.Mode3D && painter.options.AutoZoom) painter.AutoZoom();
         // painter.FillToolbar();
         painter.DrawingReady();
      });

      return painter;
   }

   // ==================== painter for TH2 histograms ==============================

   function RH2Painter(histo) {
      RHistPainter.call(this, histo);
      this.wheel_zoomy = true;
   }

   RH2Painter.prototype = Object.create(RHistPainter.prototype);

   RH2Painter.prototype.Cleanup = function() {
      delete this.tt_handle;

      RHistPainter.prototype.Cleanup.call(this);
   }

   RH2Painter.prototype.Dimension = function() {
      return 2;
   }

   RH2Painter.prototype.ToggleProjection = function(kind, width) {

      if (kind=="Projections") kind = "";

      if ((typeof kind == 'string') && (kind.length>1)) {
          width = parseInt(kind.substr(1));
          kind = kind[0];
      }

      if (!width) width = 1;

      if (kind && (this.is_projection==kind)) {
         if (this.projection_width === width) {
            kind = "";
         } else {
            this.projection_width = width;
            return;
         }
      }

      delete this.proj_hist;

      let new_proj = (this.is_projection === kind) ? "" : kind;
      this.is_projection = ""; // disable projection redraw until callback
      this.projection_width = width;

      let canp = this.canv_painter();
      if (canp) canp.ToggleProjection(this.is_projection).then(() => this.RedrawProjection("toggling", new_proj));
   }

   RH2Painter.prototype.RedrawProjection = function(ii1, ii2 /*, jj1, jj2*/) {
      // do nothing for the moment

      if (ii1 === "toggling") {
         this.is_projection = ii2;
         ii1 = ii2 = undefined;
      }

      if (!this.is_projection) return;
   }

   RH2Painter.prototype.ExecuteMenuCommand = function(method, args) {
      if (RHistPainter.prototype.ExecuteMenuCommand.call(this,method, args)) return true;

      if ((method.fName == 'SetShowProjectionX') || (method.fName == 'SetShowProjectionY')) {
         this.ToggleProjection(method.fName[17], args && parseInt(args) ? parseInt(args) : 1);
         return true;
      }

      return false;
   }

   RH2Painter.prototype.FillHistContextMenu = function(menu) {
      // painter automatically bind to menu callbacks

      menu.add("sub:Projections", this.ToggleProjection);
      let kind = this.is_projection || "";
      if (kind) kind += this.projection_width;
      let kinds = ["X1", "X2", "X3", "X5", "X10", "Y1", "Y2", "Y3", "Y5", "Y10"];
      for (let k=0;k<kinds.length;++k)
         menu.addchk(kind==kinds[k], kinds[k], kinds[k], this.ToggleProjection);
      menu.add("endsub:");

      menu.add("Auto zoom-in", this.AutoZoom);

      let sett = JSROOT.getDrawSettings("ROOT." + this.GetObject()._typename, 'nosame');

      menu.addDrawMenu("Draw with", sett.opts, function(arg) {
         if (arg==='inspect')
            return this.ShowInspector();
         this.DecodeOptions(arg);
         this.InteractiveRedraw("pad", "drawopt");
      });

      if (this.options.Color)
         this.FillPaletteMenu(menu);
   }

   RH2Painter.prototype.ButtonClick = function(funcname) {
      if (RHistPainter.prototype.ButtonClick.call(this, funcname)) return true;

      switch(funcname) {
         case "ToggleColor": this.ToggleColor(); break;
         case "ToggleColorZ": this.ToggleColz(); break;
         case "Toggle3D": this.ToggleMode3D(); break;
         default: return false;
      }

      // all methods here should not be processed further
      return true;
   }

   RH2Painter.prototype.FillToolbar = function() {
      RHistPainter.prototype.FillToolbar.call(this, true);

      let pp = this.pad_painter();
      if (!pp) return;

      if (!this.IsTH2Poly())
         pp.AddButton("th2color", "Toggle color", "ToggleColor");
      pp.AddButton("th2colorz", "Toggle color palette", "ToggleColorZ");
      pp.AddButton("th2draw3d", "Toggle 3D mode", "Toggle3D");
      pp.ShowButtons();
   }

   RH2Painter.prototype.ToggleColor = function() {

      if (this.options.Mode3D) {
         this.options.Mode3D = false;
         this.options.Color = true;
      } else {
         this.options.Color = !this.options.Color;
      }

      this._can_move_colz = true; // indicate that next redraw can move Z scale

      this.Redraw();

      // this.DrawColorPalette(this.options.Color && this.options.Zscale);
   }

   RH2Painter.prototype.AutoZoom = function() {
      if (this.IsTH2Poly()) return; // not implemented

      let i1 = this.GetSelectIndex("x", "left", -1),
          i2 = this.GetSelectIndex("x", "right", 1),
          j1 = this.GetSelectIndex("y", "left", -1),
          j2 = this.GetSelectIndex("y", "right", 1),
          i,j, histo = this.GetHisto(), xaxis = this.GetAxis("x"), yaxis = this.GetAxis("y");

      if ((i1 == i2) || (j1 == j2)) return;

      // first find minimum
      let min = histo.getBinContent(i1 + 1, j1 + 1);
      for (i = i1; i < i2; ++i)
         for (j = j1; j < j2; ++j)
            min = Math.min(min, histo.getBinContent(i+1, j+1));
      if (min > 0) return; // if all points positive, no chance for autoscale

      let ileft = i2, iright = i1, jleft = j2, jright = j1;

      for (i = i1; i < i2; ++i)
         for (j = j1; j < j2; ++j)
            if (histo.getBinContent(i + 1, j + 1) > min) {
               if (i < ileft) ileft = i;
               if (i >= iright) iright = i + 1;
               if (j < jleft) jleft = j;
               if (j >= jright) jright = j + 1;
            }

      let xmin, xmax, ymin, ymax, isany = false;

      if ((ileft === iright-1) && (ileft > i1+1) && (iright < i2-1)) { ileft--; iright++; }
      if ((jleft === jright-1) && (jleft > j1+1) && (jright < j2-1)) { jleft--; jright++; }

      if ((ileft > i1 || iright < i2) && (ileft < iright - 1)) {
         xmin = xaxis.GetBinCoord(ileft);
         xmax = xaxis.GetBinCoord(iright);
         isany = true;
      }

      if ((jleft > j1 || jright < j2) && (jleft < jright - 1)) {
         ymin = yaxis.GetBinCoord(jleft);
         ymax = yaxis.GetBinCoord(jright);
         isany = true;
      }

      if (isany) this.frame_painter().Zoom(xmin, xmax, ymin, ymax);
   }

   /** @summary Scan content of 2-dim histogram */
   RH2Painter.prototype.ScanContent = function(when_axis_changed) {

      // no need to rescan histogram while result does not depend from axis selection
      if (when_axis_changed && this.nbinsx && this.nbinsy) return;

      let i, j, histo = this.GetHisto();

      this.ExtractAxesProperties(2);

      if (this.IsTH2Poly()) {
         this.gminposbin = null;
         this.gminbin = this.gmaxbin = 0;

         for (let n=0, len=histo.fBins.arr.length; n<len; ++n) {
            let bin_content = histo.fBins.arr[n].fContent;
            if (n===0) this.gminbin = this.gmaxbin = bin_content;

            if (bin_content < this.gminbin) this.gminbin = bin_content; else
               if (bin_content > this.gmaxbin) this.gmaxbin = bin_content;

            if (bin_content > 0)
               if ((this.gminposbin===null) || (this.gminposbin > bin_content)) this.gminposbin = bin_content;
         }
      } else if (this.IsDisplayItem()) {
         // take min/max values from the display item
         this.gminbin = histo.fContMin;
         this.gminposbin = histo.fContMinPos > 0 ? histo.fContMinPos : null;
         this.gmaxbin = histo.fContMax;
      } else {
         // global min/max, used at the moment in 3D drawing
         this.gminbin = this.gmaxbin = histo.getBinContent(1, 1);
         this.gminposbin = null;
         for (i = 0; i < this.nbinsx; ++i) {
            for (j = 0; j < this.nbinsy; ++j) {
               let bin_content = histo.getBinContent(i+1, j+1);
               if (bin_content < this.gminbin) this.gminbin = bin_content; else
                  if (bin_content > this.gmaxbin) this.gmaxbin = bin_content;
               if (bin_content > 0)
                  if ((this.gminposbin===null) || (this.gminposbin > bin_content)) this.gminposbin = bin_content;
            }
         }
      }

      this.zmin = this.gminbin;
      this.zmax = this.gmaxbin;

      // this value used for logz scale drawing
      if (this.gminposbin === null) this.gminposbin = this.gmaxbin*1e-4;

      if (this.options.Axis > 0) { // Paint histogram axis only
         this.draw_content = false;
      } else {
         this.draw_content = this.gmaxbin > 0;
         if (!this.draw_content  && this.options.Zero && this.IsTH2Poly()) {
            this.draw_content = true;
            this.options.Line = 1;
         }
      }
   }

   RH2Painter.prototype.CountStat = function(cond) {
      let histo = this.GetHisto(),
          stat_sum0 = 0, stat_sumx1 = 0, stat_sumy1 = 0,
          stat_sumx2 = 0, stat_sumy2 = 0,
          xside, yside, xx, yy, zz,
          res = { name: "histo", entries: 0, integral: 0, meanx: 0, meany: 0, rmsx: 0, rmsy: 0, matrix: [0,0,0,0,0,0,0,0,0], xmax: 0, ymax:0, wmax: null };

      let xleft = this.GetSelectIndex("x", "left"),
          xright = this.GetSelectIndex("x", "right"),
          yleft = this.GetSelectIndex("y", "left"),
          yright = this.GetSelectIndex("y", "right"),
          xi, yi, xaxis = this.GetAxis("x"), yaxis = this.GetAxis("y");

      // TODO: account underflow/overflow bins, now stored in different array and only by histogram itself
      for (xi = 1; xi <= this.nbinsx; ++xi) {
         xside = (xi <= xleft+1) ? 0 : (xi > xright+1 ? 2 : 1);
         xx = xaxis.GetBinCoord(xi - 0.5);

         for (yi = 1; yi <= this.nbinsy; ++yi) {
            yside = (yi <= yleft+1) ? 0 : (yi > yright+1 ? 2 : 1);
            yy = yaxis.GetBinCoord(yi - 0.5);

            zz = histo.getBinContent(xi, yi);

            res.entries += zz;

            res.matrix[yside * 3 + xside] += zz;

            if ((xside != 1) || (yside != 1)) continue;

            if ((cond!=null) && !cond(xx,yy)) continue;

            if ((res.wmax==null) || (zz>res.wmax)) { res.wmax = zz; res.xmax = xx; res.ymax = yy; }

            stat_sum0 += zz;
            stat_sumx1 += xx * zz;
            stat_sumy1 += yy * zz;
            stat_sumx2 += xx * xx * zz;
            stat_sumy2 += yy * yy * zz;
         }
      }

      if (stat_sum0 > 0) {
         res.meanx = stat_sumx1 / stat_sum0;
         res.meany = stat_sumy1 / stat_sum0;
         res.rmsx = Math.sqrt(Math.abs(stat_sumx2 / stat_sum0 - res.meanx * res.meanx));
         res.rmsy = Math.sqrt(Math.abs(stat_sumy2 / stat_sum0 - res.meany * res.meany));
      }

      if (res.wmax===null) res.wmax = 0;
      res.integral = stat_sum0;

      // if (histo.fEntries > 1) res.entries = histo.fEntries;

      return res;
   }

   RH2Painter.prototype.FillStatistic = function(stat, dostat /*, dofit*/) {

      // no need to refill statistic if histogram is dummy
      // if (this.IgnoreStatsFill()) return false;

      let data = this.CountStat(),
          print_name = Math.floor(dostat % 10),
          print_entries = Math.floor(dostat / 10) % 10,
          print_mean = Math.floor(dostat / 100) % 10,
          print_rms = Math.floor(dostat / 1000) % 10,
          print_under = Math.floor(dostat / 10000) % 10,
          print_over = Math.floor(dostat / 100000) % 10,
          print_integral = Math.floor(dostat / 1000000) % 10,
          print_skew = Math.floor(dostat / 10000000) % 10,
          print_kurt = Math.floor(dostat / 100000000) % 10;

      stat.ClearStat();

      if (print_name > 0)
         stat.AddText(data.name);

      if (print_entries > 0)
         stat.AddText("Entries = " + stat.Format(data.entries,"entries"));

      if (print_mean > 0) {
         stat.AddText("Mean x = " + stat.Format(data.meanx));
         stat.AddText("Mean y = " + stat.Format(data.meany));
      }

      if (print_rms > 0) {
         stat.AddText("Std Dev x = " + stat.Format(data.rmsx));
         stat.AddText("Std Dev y = " + stat.Format(data.rmsy));
      }

      if (print_integral > 0)
         stat.AddText("Integral = " + stat.Format(data.matrix[4],"entries"));

      if (print_skew > 0) {
         stat.AddText("Skewness x = <undef>");
         stat.AddText("Skewness y = <undef>");
      }

      if (print_kurt > 0)
         stat.AddText("Kurt = <undef>");

      if ((print_under > 0) || (print_over > 0)) {
         let m = data.matrix;

         stat.AddText("" + m[6].toFixed(0) + " | " + m[7].toFixed(0) + " | "  + m[7].toFixed(0));
         stat.AddText("" + m[3].toFixed(0) + " | " + m[4].toFixed(0) + " | "  + m[5].toFixed(0));
         stat.AddText("" + m[0].toFixed(0) + " | " + m[1].toFixed(0) + " | "  + m[2].toFixed(0));
      }

      // if (dofit) stat.FillFunctionStat(this.FindFunction('TF1'), dofit);

      return true;
   }

   /** @summary Draw histogram bins as color */
   RH2Painter.prototype.DrawBinsColor = function() {
      let histo = this.GetHisto(),
          handle = this.PrepareDraw(),
          colPaths = [], currx = [], curry = [],
          colindx, cmd1, cmd2, i, j, binz, di = handle.stepi, dj = handle.stepj, dx, dy;

      // now start build
      for (i = handle.i1; i < handle.i2; i += di) {
         for (j = handle.j1; j < handle.j2; j += dj) {
            binz = histo.getBinContent(i + 1, j + 1);
            colindx = handle.palette.getContourIndex(binz);
            if (binz===0) {
               if (!this.options.Zero) continue;
               if ((colindx === null) && this._show_empty_bins) colindx = 0;
            }
            if (colindx === null) continue;

            cmd1 = "M"+handle.grx[i]+","+handle.gry[j+dj];
            if (colPaths[colindx] === undefined) {
               colPaths[colindx] = cmd1;
            } else{
               cmd2 = "m" + (handle.grx[i]-currx[colindx]) + "," + (handle.gry[j+dj]-curry[colindx]);
               colPaths[colindx] += (cmd2.length < cmd1.length) ? cmd2 : cmd1;
            }

            currx[colindx] = handle.grx[i];
            curry[colindx] = handle.gry[j+dj];

            dx = (handle.grx[i+di] - handle.grx[i]) || 1;
            dy = (handle.gry[j] - handle.gry[j+dj]) || 1;

            colPaths[colindx] += "v"+dy + "h"+dx + "v"+(-dy) + "z";
         }
      }

      for (colindx=0;colindx<colPaths.length;++colindx)
        if (colPaths[colindx] !== undefined)
           this.draw_g
               .append("svg:path")
               .attr("palette-index", colindx)
               .attr("fill", handle.palette.getColor(colindx))
               .attr("d", colPaths[colindx]);

      this.UpdatePaletteDraw();

      return handle;
   }

   RH2Painter.prototype.BuildContour = function(handle, levels, palette, contour_func) {
      let histo = this.GetHisto(),
          kMAXCONTOUR = 2004,
          kMAXCOUNT = 2000,
          // arguments used in the PaintContourLine
          xarr = new Float32Array(2*kMAXCONTOUR),
          yarr = new Float32Array(2*kMAXCONTOUR),
          itarr = new Int32Array(2*kMAXCONTOUR),
          lj = 0, ipoly, poly, polys = [], np, npmax = 0,
          x = [0.,0.,0.,0.], y = [0.,0.,0.,0.], zc = [0.,0.,0.,0.], ir = [0,0,0,0],
          i, j, k, n, m, ix, ljfill, count,
          xsave, ysave, itars, jx,
          di = handle.stepi, dj = handle.stepj;

      function BinarySearch(zc) {
         for (let kk=0;kk<levels.length;++kk)
            if (zc<levels[kk]) return kk-1;
         return levels.length-1;
      }

      function PaintContourLine(elev1, icont1, x1, y1,  elev2, icont2, x2, y2) {
         /* Double_t *xarr, Double_t *yarr, Int_t *itarr, Double_t *levels */
         let vert = (x1 === x2),
             tlen = vert ? (y2 - y1) : (x2 - x1),
             n = icont1 +1,
             tdif = elev2 - elev1,
             ii = lj-1,
             maxii = kMAXCONTOUR/2 -3 + lj,
             icount = 0,
             xlen, pdif, diff, elev;

         while (n <= icont2 && ii <= maxii) {
            elev = levels[n];
            diff = elev - elev1;
            pdif = diff/tdif;
            xlen = tlen*pdif;
            if (vert) {
               xarr[ii] = x1;
               yarr[ii] = y1 + xlen;
            } else {
               xarr[ii] = x1 + xlen;
               yarr[ii] = y1;
            }
            itarr[ii] = n;
            icount++;
            ii +=2;
            n++;
         }
         return icount;
      }

      let arrx = handle.original ? handle.origx : handle.grx,
          arry = handle.original ? handle.origy : handle.gry;

      for (j = handle.j1; j < handle.j2-dj; j += dj) {

         y[1] = y[0] = (arry[j] + arry[j+dj])/2;
         y[3] = y[2] = (arry[j+dj] + arry[j+2*dj])/2;

         for (i = handle.i1; i < handle.i2-di; i += di) {

            zc[0] = histo.getBinContent(i+1, j+1);
            zc[1] = histo.getBinContent(i+1+di, j+1);
            zc[2] = histo.getBinContent(i+1+di, j+1+dj);
            zc[3] = histo.getBinContent(i+1, j+1+dj);

            for (k=0;k<4;k++)
               ir[k] = BinarySearch(zc[k]);

            if ((ir[0] !== ir[1]) || (ir[1] !== ir[2]) || (ir[2] !== ir[3]) || (ir[3] !== ir[0])) {
               x[3] = x[0] = (arrx[i] + arrx[i+1])/2;
               x[2] = x[1] = (arrx[i+1] + arrx[i+2])/2;

               if (zc[0] <= zc[1]) n = 0; else n = 1;
               if (zc[2] <= zc[3]) m = 2; else m = 3;
               if (zc[n] > zc[m]) n = m;
               n++;
               lj=1;
               for (ix=1;ix<=4;ix++) {
                  m = n%4 + 1;
                  ljfill = PaintContourLine(zc[n-1],ir[n-1],x[n-1],y[n-1],
                        zc[m-1],ir[m-1],x[m-1],y[m-1]);
                  lj += 2*ljfill;
                  n = m;
               }

               if (zc[0] <= zc[1]) n = 0; else n = 1;
               if (zc[2] <= zc[3]) m = 2; else m = 3;
               if (zc[n] > zc[m]) n = m;
               n++;
               lj=2;
               for (ix=1;ix<=4;ix++) {
                  if (n == 1) m = 4;
                  else        m = n-1;
                  ljfill = PaintContourLine(zc[n-1],ir[n-1],x[n-1],y[n-1],
                        zc[m-1],ir[m-1],x[m-1],y[m-1]);
                  lj += 2*ljfill;
                  n = m;
               }
               //     Re-order endpoints

               count = 0;
               for (ix=1; ix<=lj-5; ix +=2) {
                  //count = 0;
                  while (itarr[ix-1] != itarr[ix]) {
                     xsave = xarr[ix];
                     ysave = yarr[ix];
                     itars = itarr[ix];
                     for (jx=ix; jx<=lj-5; jx +=2) {
                        xarr[jx]  = xarr[jx+2];
                        yarr[jx]  = yarr[jx+2];
                        itarr[jx] = itarr[jx+2];
                     }
                     xarr[lj-3]  = xsave;
                     yarr[lj-3]  = ysave;
                     itarr[lj-3] = itars;
                     if (count > kMAXCOUNT) break;
                     count++;
                  }
               }

               if (count > kMAXCOUNT) continue;

               for (ix=1; ix<=lj-2; ix +=2) {

                  ipoly = itarr[ix-1];

                  if ((ipoly >= 0) && (ipoly < levels.length)) {
                     poly = polys[ipoly];
                     if (!poly)
                        poly = polys[ipoly] = JSROOT.CreateTPolyLine(kMAXCONTOUR*4, true);

                     np = poly.fLastPoint;
                     if (np < poly.fN-2) {
                        poly.fX[np+1] = Math.round(xarr[ix-1]); poly.fY[np+1] = Math.round(yarr[ix-1]);
                        poly.fX[np+2] = Math.round(xarr[ix]); poly.fY[np+2] = Math.round(yarr[ix]);
                        poly.fLastPoint = np+2;
                        npmax = Math.max(npmax, poly.fLastPoint+1);
                     } else {
                        // console.log('reject point??', poly.fLastPoint);
                     }
                  }
               }
            } // end of if (ir[0]
         } // end of j
      } // end of i

      let polysort = new Int32Array(levels.length), first = 0;
      //find first positive contour
      for (ipoly=0;ipoly<levels.length;ipoly++) {
         if (levels[ipoly] >= 0) { first = ipoly; break; }
      }
      //store negative contours from 0 to minimum, then all positive contours
      k = 0;
      for (ipoly=first-1;ipoly>=0;ipoly--) {polysort[k] = ipoly; k++;}
      for (ipoly=first;ipoly<levels.length;ipoly++) { polysort[k] = ipoly; k++;}

      let xp = new Float32Array(2*npmax),
          yp = new Float32Array(2*npmax);

      for (k=0;k<levels.length;++k) {

         ipoly = polysort[k];
         poly = polys[ipoly];
         if (!poly) continue;

         let colindx = ipoly,
             xx = poly.fX, yy = poly.fY, np = poly.fLastPoint+1,
             istart = 0, iminus, iplus, xmin = 0, ymin = 0, nadd;

         while (true) {

            iminus = npmax;
            iplus  = iminus+1;
            xp[iminus]= xx[istart];   yp[iminus] = yy[istart];
            xp[iplus] = xx[istart+1]; yp[iplus]  = yy[istart+1];
            xx[istart] = xx[istart+1] = xmin;
            yy[istart] = yy[istart+1] = ymin;
            while (true) {
               nadd = 0;
               for (i=2;i<np;i+=2) {
                  if ((iplus < 2*npmax-1) && (xx[i] === xp[iplus]) && (yy[i] === yp[iplus])) {
                     iplus++;
                     xp[iplus] = xx[i+1]; yp[iplus] = yy[i+1];
                     xx[i] = xx[i+1] = xmin;
                     yy[i] = yy[i+1] = ymin;
                     nadd++;
                  }
                  if ((iminus > 0) && (xx[i+1] === xp[iminus]) && (yy[i+1] === yp[iminus])) {
                     iminus--;
                     xp[iminus] = xx[i]; yp[iminus] = yy[i];
                     xx[i] = xx[i+1] = xmin;
                     yy[i] = yy[i+1] = ymin;
                     nadd++;
                  }
               }
               if (nadd == 0) break;
            }

            if ((iminus+1 < iplus) && (iminus>=0))
               contour_func(colindx, xp, yp, iminus, iplus, ipoly);

            istart = 0;
            for (i=2;i<np;i+=2) {
               if (xx[i] !== xmin && yy[i] !== ymin) {
                  istart = i;
                  break;
               }
            }

            if (istart === 0) break;
         }
      }
   }

   RH2Painter.prototype.DrawBinsContour = function(frame_w,frame_h) {
      let handle = this.PrepareDraw({ rounding: false, extra: 100, original: this.options.Proj != 0 }),
          main = this.frame_painter(),
          palette = main.GetPalette(),
          levels = palette.GetContour(),
          func = main.GetProjectionFunc();

      let BuildPath = (xp,yp,iminus,iplus,do_close) => {
         let cmd = "", last, pnt, fisrt, isany;
         for (let i = iminus; i <= iplus; ++i) {
            if (func) {
               pnt = func(xp[i], yp[i]);
               pnt.x = Math.round(main.grx(pnt.x));
               pnt.y = Math.round(main.gry(pnt.y));
            } else {
               pnt = { x: Math.round(xp[i]), y: Math.round(yp[i]) };
            }
            if (!cmd) {
               cmd = "M" + pnt.x + "," + pnt.y; first = pnt;
            } else if ((i == iplus) && first && (pnt.x == first.x) && (pnt.y == first.y)) {
               if (!isany) return ""; // all same points
               cmd += "z"; do_close = false;
            } else if ((pnt.x != last.x) && (pnt.y != last.y)) {
               cmd +=  "l" + (pnt.x - last.x) + "," + (pnt.y - last.y); isany = true;
            } else if (pnt.x != last.x) {
               cmd +=  "h" + (pnt.x - last.x); isany = true;
            } else if (pnt.y != last.y) {
               cmd +=  "v" + (pnt.y - last.y); isany = true;
            }
            last = pnt;
         }
         if (do_close) cmd += "z";
         return cmd;
      }

      if (this.options.Contour===14) {
         let dd = "M0,0h"+frame_w+"v"+frame_h+"h-"+frame_w+"z";
         if (this.options.Proj) {
            let dj = handle.stepj, sz = parseInt((handle.j2 - handle.j1)/dj),
                xd = new Float32Array(sz*2), yd = new Float32Array(sz*2);
            for (let i=0;i<sz;++i) {
               xd[i] = handle.origx[handle.i1];
               yd[i] = (handle.origy[handle.j1]*(i*dj+0.5) + handle.origy[handle.j2]*(sz-0.5-i*dj))/sz;
               xd[i+sz] = handle.origx[handle.i2];
               yd[i+sz] = (handle.origy[handle.j2]*(i*dj+0.5) + handle.origy[handle.j1]*(sz-0.5-i*dj))/sz;
            }
            dd = BuildPath(xd,yd,0,2*sz-1,true);
         }

         this.draw_g
             .append("svg:path")
             .attr("d", dd)
             .style('stroke','none')
             .style("fill", palette.getColor(0));
      }

      this.BuildContour(handle, levels, palette,
         (colindx,xp,yp,iminus,iplus) => {
            let icol = palette.getColor(colindx),
                fillcolor = icol, lineatt;

            switch (this.options.Contour) {
               case 1: break;
               case 11: fillcolor = 'none'; lineatt = new JSROOT.TAttLineHandler({ color: icol }); break;
               case 12: fillcolor = 'none'; lineatt = new JSROOT.TAttLineHandler({ color:1, style: (colindx%5 + 1), width: 1 }); break;
               case 13: fillcolor = 'none'; lineatt = this.lineatt; break;
               case 14: break;
            }

            let dd = BuildPath(xp, yp, iminus, iplus, fillcolor != 'none');
            if (!dd) return;

            let elem = this.draw_g
                          .append("svg:path")
                          .attr("class","th2_contour")
                          .attr("d", dd)
                          .style("fill", fillcolor);

            if (lineatt)
               elem.call(lineatt.func);
            else
               elem.style('stroke','none');
         }
      );

      handle.hide_only_zeros = true; // text drawing suppress only zeros

      return handle;
   }

   RH2Painter.prototype.CreatePolyBin = function(pmain, bin, text_pos) {
      let cmd = "", ngr, ngraphs = 1, gr = null;

      if (bin.fPoly._typename=='TMultiGraph')
         ngraphs = bin.fPoly.fGraphs.arr.length;
      else
         gr = bin.fPoly;

      if (text_pos)
         bin._sumx = bin._sumy = bin._suml = 0;

      function AddPoint(x1,y1,x2,y2) {
         let len = Math.sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
         bin._sumx += (x1+x2)*len/2;
         bin._sumy += (y1+y2)*len/2;
         bin._suml += len;
      }

      for (ngr = 0; ngr < ngraphs; ++ ngr) {
         if (!gr || (ngr>0)) gr = bin.fPoly.fGraphs.arr[ngr];

         let npnts = gr.fNpoints, n,
             x = gr.fX, y = gr.fY,
             grx = Math.round(pmain.grx(x[0])),
             gry = Math.round(pmain.gry(y[0])),
             nextx, nexty;

         if ((npnts>2) && (x[0]==x[npnts-1]) && (y[0]==y[npnts-1])) npnts--;

         cmd += "M"+grx+","+gry;

         for (n=1;n<npnts;++n) {
            nextx = Math.round(pmain.grx(x[n]));
            nexty = Math.round(pmain.gry(y[n]));
            if (text_pos) AddPoint(grx,gry, nextx, nexty);
            if ((grx!==nextx) || (gry!==nexty)) {
               if (grx===nextx) cmd += "v" + (nexty - gry); else
                  if (gry===nexty) cmd += "h" + (nextx - grx); else
                     cmd += "l" + (nextx - grx) + "," + (nexty - gry);
            }
            grx = nextx; gry = nexty;
         }

         if (text_pos) AddPoint(grx, gry, Math.round(pmain.grx(x[0])), Math.round(pmain.gry(y[0])));
         cmd += "z";
      }

      if (text_pos) {
         if (bin._suml > 0) {
            bin._midx = Math.round(bin._sumx / bin._suml);
            bin._midy = Math.round(bin._sumy / bin._suml);
         } else {
            bin._midx = Math.round(pmain.grx((bin.fXmin + bin.fXmax)/2));
            bin._midy = Math.round(pmain.gry((bin.fYmin + bin.fYmax)/2));
         }
      }

      return cmd;
   }

   RH2Painter.prototype.DrawPolyBinsColor = function() {
      let histo = this.GetHisto(),
          pmain = this.frame_painter(),
          colPaths = [], textbins = [],
          colindx, cmd, bin, item,
          i, len = histo.fBins.arr.length,
          palette = pmain.GetPalette();

      // force recalculations of contours
      // use global coordinates
      this.maxbin = this.gmaxbin;
      this.minbin = this.gminbin;
      this.minposbin = this.gminposbin;

      this.CreateContour(pmain, palette);

      for (i = 0; i < len; ++ i) {
         bin = histo.fBins.arr[i];
         colindx = palette.getContourIndex(bin.fContent);
         if (colindx === null) continue;
         if (bin.fContent === 0) {
            if (!this.options.Zero || !this.options.Line) continue;
            colindx = 0;
         }

         // check if bin outside visible range
         if ((bin.fXmin > pmain.scale_xmax) || (bin.fXmax < pmain.scale_xmin) ||
             (bin.fYmin > pmain.scale_ymax) || (bin.fYmax < pmain.scale_ymin)) continue;

         cmd = this.CreatePolyBin(pmain, bin, this.options.Text && bin.fContent);

         if (colPaths[colindx] === undefined)
            colPaths[colindx] = cmd;
         else
            colPaths[colindx] += cmd;

         if (this.options.Text) textbins.push(bin);
      }

      for (colindx=0;colindx<colPaths.length;++colindx)
         if (colPaths[colindx]) {
            item = this.draw_g
                     .append("svg:path")
                     .attr("palette-index", colindx)
                     .attr("fill", colindx ? palette.getColor(colindx) : "none")
                     .attr("d", colPaths[colindx]);
            if (this.options.Line)
               item.call(this.lineatt.func);
         }

      if (textbins.length > 0) {
         let textFont  = this.v7EvalFont("text", { size: 12, color: "black", align: 22 }),
             text_g = this.draw_g.append("svg:g").attr("class","th2poly_text");

         this.StartTextDrawing(textFont, 'font', text_g);

         for (i = 0; i < textbins.length; ++ i) {
            bin = textbins[i];

            let lbl = "";

            if (!this.options.TextKind) {
               lbl = (Math.round(bin.fContent) === bin.fContent) ? bin.fContent.toString() :
                          JSROOT.FFormat(bin.fContent, JSROOT.gStyle.fPaintTextFormat);
            } else {
               if (bin.fPoly) lbl = bin.fPoly.fName;
               if (lbl === "Graph") lbl = "";
               if (!lbl) lbl = bin.fNumber;
            }

            this.DrawText({ x: bin._midx, y: bin._midy, text: lbl, latex: 0, draw_g: text_g });
         }

         this.FinishTextDrawing(text_g);
      }

      return { poly: true };
   }

   RH2Painter.prototype.DrawBinsText = function(handle) {
      let histo = this.GetHisto(),
          i,j,binz,binw,binh,lbl,posx,posy,sizex,sizey;

      if (handle===null) handle = this.PrepareDraw({ rounding: false });

      let textFont  = this.v7EvalFont("text", { size: 20, color: "black", align: 22 }),
          text_offset = 0,
          text_g = this.draw_g.append("svg:g").attr("class","th2_text"),
          di = handle.stepi, dj = handle.stepj,
          profile2d = (this.options.TextKind == "E") &&
                      this.MatchObjectType('TProfile2D') && (typeof histo.getBinEntries=='function');

      if (this.options.BarOffset) text_offset = this.options.BarOffset;

      this.StartTextDrawing(textFont, 'font', text_g);

      for (i = handle.i1; i < handle.i2; i += di)
         for (j = handle.j1; j < handle.j2; j += dj) {
            binz = histo.getBinContent(i+1, j+1);
            if ((binz === 0) && !this._show_empty_bins) continue;

            binw = handle.grx[i+di] - handle.grx[i];
            binh = handle.gry[j] - handle.gry[j+dj];

            if (profile2d)
               binz = histo.getBinEntries(i+1, j+1);

            lbl = (binz === Math.round(binz)) ? binz.toString() :
                      JSROOT.FFormat(binz, JSROOT.gStyle.fPaintTextFormat);

            if (textFont.angle) {
               posx = Math.round(handle.grx[i] + binw*0.5);
               posy = Math.round(handle.gry[j+dj] + binh*(0.5 + text_offset));
               sizex = 0;
               sizey = 0;
            } else {
               posx = Math.round(handle.grx[i] + binw*0.1);
               posy = Math.round(handle.gry[j+dj] + binh*(0.1 + text_offset));
               sizex = Math.round(binw*0.8);
               sizey = Math.round(binh*0.8);
            }

            this.DrawText({ align: 22, x: posx, y: posy, width: sizex, height: sizey, text: lbl, latex: 0, draw_g: text_g });
         }

      this.FinishTextDrawing(text_g);

      handle.hide_only_zeros = true; // text drawing suppress only zeros

      return handle;
   }

   RH2Painter.prototype.DrawBinsArrow = function() {
      let histo = this.GetHisto(), cmd = "",
          i,j, dn = 1e-30, dx, dy, xc,yc,
          dxn,dyn,x1,x2,y1,y2, anr,si,co,
          handle = this.PrepareDraw({ rounding: false }),
          scale_x = (handle.grx[handle.i2] - handle.grx[handle.i1])/(handle.i2 - handle.i1 + 1-0.03)/2,
          scale_y = (handle.gry[handle.j2] - handle.gry[handle.j1])/(handle.j2 - handle.j1 + 1-0.03)/2,
          di = handle.stepi, dj = handle.stepj;

      for (let loop=0;loop<2;++loop)
         for (i = handle.i1; i < handle.i2; i += di)
            for (j = handle.j1; j < handle.j2; j += dj) {

               if (i === handle.i1) {
                  dx = histo.getBinContent(i+1+di, j+1) - histo.getBinContent(i+1, j+1);
               } else if (i >= handle.i2-di) {
                  dx = histo.getBinContent(i+1, j+1) - histo.getBinContent(i+1-di, j+1);
               } else {
                  dx = 0.5*(histo.getBinContent(i+1+di, j+1) - histo.getBinContent(i+1-di, j+1));
               }
               if (j === handle.j1) {
                  dy = histo.getBinContent(i+1, j+1+dj) - histo.getBinContent(i+1, j+1);
               } else if (j >= handle.j2-dj) {
                  dy = histo.getBinContent(i+1, j+1) - histo.getBinContent(i+1, j+1-dj);
               } else {
                  dy = 0.5*(histo.getBinContent(i+1, j+1+dj) - histo.getBinContent(i+1, j+1-dj));
               }

               if (loop===0) {
                  dn = Math.max(dn, Math.abs(dx), Math.abs(dy));
               } else {
                  xc = (handle.grx[i] + handle.grx[i+di])/2;
                  yc = (handle.gry[j] + handle.gry[j+dj])/2;
                  dxn = scale_x*dx/dn;
                  dyn = scale_y*dy/dn;
                  x1  = xc - dxn;
                  x2  = xc + dxn;
                  y1  = yc - dyn;
                  y2  = yc + dyn;
                  dx = Math.round(x2-x1);
                  dy = Math.round(y2-y1);

                  if ((dx!==0) || (dy!==0)) {
                     cmd += "M"+Math.round(x1)+","+Math.round(y1)+"l"+dx+","+dy;

                     if (Math.abs(dx) > 5 || Math.abs(dy) > 5) {
                        anr = Math.sqrt(2/(dx*dx + dy*dy));
                        si  = Math.round(anr*(dx + dy));
                        co  = Math.round(anr*(dx - dy));
                        if ((si!==0) && (co!==0))
                           cmd+="l"+(-si)+","+co + "m"+si+","+(-co) + "l"+(-co)+","+(-si);
                     }
                  }
               }
            }

      this.draw_g
         .append("svg:path")
         .attr("class","th2_arrows")
         .attr("d", cmd)
         .style("fill", "none")
         .call(this.lineatt.func);

      return handle;
   }


   RH2Painter.prototype.DrawBinsBox = function() {

      let histo = this.GetHisto(),
          handle = this.PrepareDraw({ rounding: false }),
          main = this.frame_painter();

      if (main.maxbin === main.minbin) {
         main.maxbin = this.gmaxbin;
         main.minbin = this.gminbin;
         main.minposbin = this.gminposbin;
      }
      if (main.maxbin === main.minbin)
         main.minbin = Math.min(0, main.maxbin-1);

      let absmax = Math.max(Math.abs(main.maxbin), Math.abs(main.minbin)),
          absmin = Math.max(0, main.minbin),
          i, j, binz, absz, res = "", cross = "", btn1 = "", btn2 = "",
          zdiff, dgrx, dgry, xx, yy, ww, hh,
          xyfactor = 1, uselogz = false, logmin = 0, logmax = 1,
          di = handle.stepi, dj = handle.stepj;

      if (main.logz && (absmax>0)) {
         uselogz = true;
         logmax = Math.log(absmax);
         if (absmin>0) logmin = Math.log(absmin); else
         if ((main.minposbin>=1) && (main.minposbin<100)) logmin = Math.log(0.7); else
            logmin = (main.minposbin > 0) ? Math.log(0.7*main.minposbin) : logmax - 10;
         if (logmin >= logmax) logmin = logmax - 10;
         xyfactor = 1. / (logmax - logmin);
      } else {
         xyfactor = 1. / (absmax - absmin);
      }

      // now start build
      for (i = handle.i1; i < handle.i2; i += di) {
         for (j = handle.j1; j < handle.j2; j += dj) {
            binz = histo.getBinContent(i + 1, j + 1);
            absz = Math.abs(binz);
            if ((absz === 0) || (absz < absmin)) continue;

            zdiff = uselogz ? ((absz>0) ? Math.log(absz) - logmin : 0) : (absz - absmin);
            // area of the box should be proportional to absolute bin content
            zdiff = 0.5 * ((zdiff < 0) ? 1 : (1 - Math.sqrt(zdiff * xyfactor)));
            // avoid oversized bins
            if (zdiff < 0) zdiff = 0;

            ww = handle.grx[i+di] - handle.grx[i];
            hh = handle.gry[j] - handle.gry[j+dj];

            dgrx = zdiff * ww;
            dgry = zdiff * hh;

            xx = Math.round(handle.grx[i] + dgrx);
            yy = Math.round(handle.gry[j+dj] + dgry);

            ww = Math.max(Math.round(ww - 2*dgrx), 1);
            hh = Math.max(Math.round(hh - 2*dgry), 1);

            res += "M"+xx+","+yy + "v"+hh + "h"+ww + "v-"+hh + "z";

            if ((binz<0) && (this.options.BoxStyle === 10))
               cross += "M"+xx+","+yy + "l"+ww+","+hh + "M"+(xx+ww)+","+yy + "l-"+ww+","+hh;

            if ((this.options.BoxStyle === 11) && (ww>5) && (hh>5)) {
               let pww = Math.round(ww*0.1),
                   phh = Math.round(hh*0.1),
                   side1 = "M"+xx+","+yy + "h"+ww + "l"+(-pww)+","+phh + "h"+(2*pww-ww) +
                           "v"+(hh-2*phh)+ "l"+(-pww)+","+phh + "z",
                   side2 = "M"+(xx+ww)+","+(yy+hh) + "v"+(-hh) + "l"+(-pww)+","+phh + "v"+(hh-2*phh)+
                           "h"+(2*pww-ww) + "l"+(-pww)+","+phh + "z";
               if (binz<0) { btn2+=side1; btn1+=side2; }
                      else { btn1+=side1; btn2+=side2; }
            }
         }
      }

      if (res.length > 0) {
         let elem = this.draw_g.append("svg:path")
                               .attr("d", res)
                               .call(this.fillatt.func);
         if ((this.options.BoxStyle === 11) || !this.fillatt.empty())
            elem.style('stroke','none');
         else
            elem.call(this.lineatt.func);
      }

      if ((btn1.length>0) && (this.fillatt.color !== 'none'))
         this.draw_g.append("svg:path")
                    .attr("d", btn1)
                    .style("stroke","none")
                    .call(this.fillatt.func)
                    .style("fill", d3.rgb(this.fillatt.color).brighter(0.5).toString());

      if (btn2.length>0)
         this.draw_g.append("svg:path")
                    .attr("d", btn2)
                    .style("stroke","none")
                    .call(this.fillatt.func)
                    .style("fill", this.fillatt.color === 'none' ? 'red' : d3.rgb(this.fillatt.color).darker(0.5).toString());

      if (cross.length > 0) {
         let elem = this.draw_g.append("svg:path")
                               .attr("d", cross)
                               .style("fill", "none");
         if (this.lineatt.color !== 'none')
            elem.call(this.lineatt.func);
         else
            elem.style('stroke','black');
      }

      return handle;
   }

   RH2Painter.prototype.DrawCandle = function(w) {
      let histo = this.GetHisto(), yaxis = this.GetAxis("y"),
          handle = this.PrepareDraw(),
          pmain = this.frame_painter(), // used for axis values conversions
          i, j, y, sum1, cont, center, counter, integral, pnt,
          bars = "", markers = "", posy;

      // create attribute only when necessary
      this.createv7AttMarker();

      // reset absolution position for markers
      this.markeratt.reset_pos();

      handle.candle = []; // array of drawn points

      // loop over visible x-bins
      for (i = handle.i1; i < handle.i2; ++i) {
         sum1 = 0;
         //estimate integral
         integral = 0;
         counter = 0;
         for (j = 0; j < this.nbinsy; ++j) {
            integral += histo.getBinContent(i+1,j+1);
         }
         pnt = { bin:i, meany:0, m25y:0, p25y:0, median:0, iqr:0, whiskerp:0, whiskerm:0};
         //estimate quantiles... simple function... not so nice as GetQuantiles
         for (j = 0; j < this.nbinsy; ++j) {
            cont = histo.getBinContent(i+1,j+1);
            posy = yaxis.GetBinCoord(j + 0.5);
            if (counter/integral < 0.001 && (counter + cont)/integral >=0.001) pnt.whiskerm = posy; // Lower whisker
            if (counter/integral < 0.25 && (counter + cont)/integral >=0.25) pnt.m25y = posy; // Lower edge of box
            if (counter/integral < 0.5 && (counter + cont)/integral >=0.5) pnt.median = posy; //Median
            if (counter/integral < 0.75 && (counter + cont)/integral >=0.75) pnt.p25y = posy; //Upper edge of box
            if (counter/integral < 0.999 && (counter + cont)/integral >=0.999) pnt.whiskerp = posy; // Upper whisker
            counter += cont;
            y = posy; // center of y bin coordinate
            sum1 += cont*y;
         }
         if (counter > 0) {
            pnt.meany = sum1/counter;
         }
         pnt.iqr = pnt.p25y-pnt.m25y;

         //Whiskers cannot exceed 1.5*iqr from box
         if ((pnt.m25y-1.5*pnt.iqr) > pnt.whsikerm)  {
            pnt.whiskerm = pnt.m25y-1.5*pnt.iqr;
         }
         if ((pnt.p25y+1.5*pnt.iqr) < pnt.whiskerp) {
            pnt.whiskerp = pnt.p25y+1.5*pnt.iqr;
         }

         // exclude points with negative y when log scale is specified
         if (pmain.logy && (pnt.whiskerm<=0)) continue;

         w = handle.grx[i+1] - handle.grx[i];
         w *= 0.66;
         center = (handle.grx[i+1] + handle.grx[i]) / 2 + this.options.BarOffset*w;
         if (this.options.BarWidth > 0) w = w * this.options.BarWidth;

         pnt.x1 = Math.round(center - w/2);
         pnt.x2 = Math.round(center + w/2);
         center = Math.round(center);

         pnt.y0 = Math.round(pmain.gry(pnt.median));
         // mean line
         bars += "M" + pnt.x1 + "," + pnt.y0 + "h" + (pnt.x2-pnt.x1);

         pnt.y1 = Math.round(pmain.gry(pnt.p25y));
         pnt.y2 = Math.round(pmain.gry(pnt.m25y));

         // rectangle
         bars += "M" + pnt.x1 + "," + pnt.y1 +
         "v" + (pnt.y2-pnt.y1) + "h" + (pnt.x2-pnt.x1) + "v-" + (pnt.y2-pnt.y1) + "z";

         pnt.yy1 = Math.round(pmain.gry(pnt.whiskerp));
         pnt.yy2 = Math.round(pmain.gry(pnt.whiskerm));

         // upper part
         bars += "M" + center + "," + pnt.y1 + "v" + (pnt.yy1-pnt.y1);
         bars += "M" + pnt.x1 + "," + pnt.yy1 + "h" + (pnt.x2-pnt.x1);

         // lower part
         bars += "M" + center + "," + pnt.y2 + "v" + (pnt.yy2-pnt.y2);
         bars += "M" + pnt.x1 + "," + pnt.yy2 + "h" + (pnt.x2-pnt.x1);

         //estimate outliers
         for (j = 0; j < this.nbinsy; ++j) {
            cont = histo.getBinContent(i+1,j+1);
            posy = yaxis.GetBinCoord(j + 0.5);
            if (cont > 0 && posy < pnt.whiskerm) markers += this.markeratt.create(center, posy);
            if (cont > 0 && posy > pnt.whiskerp) markers += this.markeratt.create(center, posy);         }

         handle.candle.push(pnt); // keep point for the tooltip
      }

      if (bars.length > 0)
         this.draw_g.append("svg:path")
             .attr("d", bars)
             .call(this.lineatt.func)
             .call(this.fillatt.func);

      if (markers.length > 0)
         this.draw_g.append("svg:path")
             .attr("d", markers)
             .call(this.markeratt.func);

      return handle;
   }

   RH2Painter.prototype.DrawBinsScatter = function() {
      let histo = this.GetHisto(),
          handle = this.PrepareDraw({ rounding: true, pixel_density: true, scatter_plot: true }),
          colPaths = [], currx = [], curry = [], cell_w = [], cell_h = [],
          colindx, cmd1, cmd2, i, j, binz, cw, ch, factor = 1.,
          scale = this.options.ScatCoef * ((this.gmaxbin) > 2000 ? 2000. / this.gmaxbin : 1.),
          di = handle.stepi, dj = handle.stepj;

      JSROOT.seed(handle.sumz);

      if (scale*handle.sumz < 1e5) {
         // one can use direct drawing of scatter plot without any patterns

         this.createv7AttMarker();

         this.markeratt.reset_pos();

         let path = "", k, npix;
         for (i = handle.i1; i < handle.i2; i += di) {
            cw = handle.grx[i+di] - handle.grx[i];
            for (j = handle.j1; j < handle.j2; j += dj) {
               ch = handle.gry[j] - handle.gry[j+dj];
               binz = histo.getBinContent(i + 1, j + 1);

               npix = Math.round(scale*binz);
               if (npix <= 0) continue;

               for (k = 0; k < npix; ++k)
                  path += this.markeratt.create(
                            Math.round(handle.grx[i] + cw * JSROOT.random()),
                            Math.round(handle.gry[j+1] + ch * JSROOT.random()));
            }
         }

         this.draw_g
              .append("svg:path")
              .attr("d", path)
              .call(this.markeratt.func);

         return handle;
      }

      // limit filling factor, do not try to produce as many points as filled area;
      if (this.maxbin > 0.7) factor = 0.7/this.maxbin;

      // let nlevels = Math.round(handle.max - handle.min);

      // now start build
      for (i = handle.i1; i < handle.i2; i += di) {
         for (j = handle.j1; j < handle.j2; j += dj) {
            binz = histo.getBinContent(i + 1, j + 1);
            if ((binz <= 0) || (binz < this.minbin)) continue;

            cw = handle.grx[i+di] - handle.grx[i];
            ch = handle.gry[j] - handle.gry[j+dj];
            if (cw*ch <= 0) continue;

            colindx = handle.palette.getContourIndex(binz/cw/ch);
            if (colindx < 0) continue;

            cmd1 = "M"+handle.grx[i]+","+handle.gry[j+dj];
            if (colPaths[colindx] === undefined) {
               colPaths[colindx] = cmd1;
               cell_w[colindx] = cw;
               cell_h[colindx] = ch;
            } else{
               cmd2 = "m" + (handle.grx[i]-currx[colindx]) + "," + (handle.gry[j+dj] - curry[colindx]);
               colPaths[colindx] += (cmd2.length < cmd1.length) ? cmd2 : cmd1;
               cell_w[colindx] = Math.max(cell_w[colindx], cw);
               cell_h[colindx] = Math.max(cell_h[colindx], ch);
            }

            currx[colindx] = handle.grx[i];
            curry[colindx] = handle.gry[j+dj];

            colPaths[colindx] += "v"+ch+"h"+cw+"v-"+ch+"z";
         }
      }

      let layer = this.svg_frame().select('.main_layer'),
          defs = layer.select("defs");
      if (defs.empty() && (colPaths.length>0))
         defs = layer.insert("svg:defs",":first-child");

      this.createv7AttMarker();

      let cntr = handle.palette.GetContour();

      for (colindx=0;colindx<colPaths.length;++colindx)
        if ((colPaths[colindx] !== undefined) && (colindx<cntr.length)) {
           let pattern_class = "scatter_" + colindx,
               pattern = defs.select('.' + pattern_class);
           if (pattern.empty())
              pattern = defs.append('svg:pattern')
                            .attr("class", pattern_class)
                            .attr("id", "jsroot_scatter_pattern_" + JSROOT._.id_counter++)
                            .attr("patternUnits","userSpaceOnUse");
           else
              pattern.selectAll("*").remove();

           let npix = Math.round(factor*cntr[colindx]*cell_w[colindx]*cell_h[colindx]);
           if (npix<1) npix = 1;

           let arrx = new Float32Array(npix), arry = new Float32Array(npix);

           if (npix===1) {
              arrx[0] = arry[0] = 0.5;
           } else {
              for (let n=0;n<npix;++n) {
                 arrx[n] = JSROOT.random();
                 arry[n] = JSROOT.random();
              }
           }

           // arrx.sort();

           this.markeratt.reset_pos();

           let path = "";

           for (let n=0;n<npix;++n)
              path += this.markeratt.create(arrx[n] * cell_w[colindx], arry[n] * cell_h[colindx]);

           pattern.attr("width", cell_w[colindx])
                  .attr("height", cell_h[colindx])
                  .append("svg:path")
                  .attr("d",path)
                  .call(this.markeratt.func);

           this.draw_g
               .append("svg:path")
               .attr("scatter-index", colindx)
               .attr("fill", 'url(#' + pattern.attr("id") + ')')
               .attr("d", colPaths[colindx]);
        }

      return handle;
   }

   /** @summary Draw RH2 bins in 2D mode */
   RH2Painter.prototype.Draw2DBins = function() {

      if (!this.draw_content)
         return this.RemoveDrawG();

      this.CheckHistDrawAttributes();

      this.CreateG(true);

      let w = this.frame_width(),
          h = this.frame_height(),
          handle = null;

      // if (this.lineatt.color == 'none') this.lineatt.color = 'cyan';

      if (this.IsTH2Poly()) {
         handle = this.DrawPolyBinsColor();
      } else {
         if (this.options.Scat)
            handle = this.DrawBinsScatter();
         else if (this.options.Color)
            handle = this.DrawBinsColor();
         else if (this.options.Box)
            handle = this.DrawBinsBox();
         else if (this.options.Arrow)
            handle = this.DrawBinsArrow();
         else if (this.options.Contour > 0)
            handle = this.DrawBinsContour(w, h);
         else if (this.options.Candle)
            handle = this.DrawCandle(w);

         if (this.options.Text)
            handle = this.DrawBinsText(handle);

         if (!handle)
            handle = this.DrawBinsColor();
      }

      this.tt_handle = handle;
   }

   RH2Painter.prototype.GetBinTips = function (i, j) {
      let lines = [],
           histo = this.GetHisto(),
           binz = histo.getBinContent(i+1,j+1),
           di = 1, dj = 1;

      if (this.IsDisplayItem()) {
         di = histo.stepx || 1;
         dj = histo.stepy || 1;
      }

      lines.push(this.GetTipName() || "histo<2>");
      lines.push("x = " + this.GetAxisBinTip("x", i, di));
      lines.push("y = " + this.GetAxisBinTip("y", j, dj));

      lines.push("bin = " + i + ", " + j);

      if (histo.$baseh) binz -= histo.$baseh.getBinContent(i+1,j+1);

      let lbl = "entries = " + ((di>1) || (dj>1) ? "~" : "");

      if (binz === Math.round(binz))
         lines.push(lbl + binz);
      else
         lines.push(lbl + JSROOT.FFormat(binz, JSROOT.gStyle.fStatFormat));

      return lines;
   }

   RH2Painter.prototype.GetCandleTips = function(p) {
      let lines = [], main = this.frame_painter(), xaxis = this.GetAxis("y");

      lines.push(this.GetTipName() || "histo");

      lines.push("x = " + main.AxisAsText("x", xaxis.GetBinCoord(p.bin)));

      lines.push('mean y = ' + JSROOT.FFormat(p.meany, JSROOT.gStyle.fStatFormat))
      lines.push('m25 = ' + JSROOT.FFormat(p.m25y, JSROOT.gStyle.fStatFormat))
      lines.push('p25 = ' + JSROOT.FFormat(p.p25y, JSROOT.gStyle.fStatFormat))

      return lines;
   }

   RH2Painter.prototype.ProvidePolyBinHints = function(binindx, realx, realy) {

      let histo = this.GetHisto(),
          bin = histo.fBins.arr[binindx],
          pmain = this.frame_painter(),
          binname = bin.fPoly.fName,
          lines = [], numpoints = 0;

      if (binname === "Graph") binname = "";
      if (binname.length === 0) binname = bin.fNumber;

      if ((realx===undefined) && (realy===undefined)) {
         realx = realy = 0;
         let gr = bin.fPoly, numgraphs = 1;
         if (gr._typename === 'TMultiGraph') { numgraphs = bin.fPoly.fGraphs.arr.length; gr = null; }

         for (let ngr=0;ngr<numgraphs;++ngr) {
            if (!gr || (ngr>0)) gr = bin.fPoly.fGraphs.arr[ngr];

            for (let n=0;n<gr.fNpoints;++n) {
               ++numpoints;
               realx += gr.fX[n];
               realy += gr.fY[n];
            }
         }

         if (numpoints > 1) {
            realx = realx / numpoints;
            realy = realy / numpoints;
         }
      }

      lines.push(this.GetTipName() || "histo");
      lines.push("x = " + pmain.AxisAsText("x", realx));
      lines.push("y = " + pmain.AxisAsText("y", realy));
      if (numpoints > 0) lines.push("npnts = " + numpoints);
      lines.push("bin = " + binname);
      if (bin.fContent === Math.round(bin.fContent))
         lines.push("content = " + bin.fContent);
      else
         lines.push("content = " + JSROOT.FFormat(bin.fContent, JSROOT.gStyle.fStatFormat));
      return lines;
   }

   RH2Painter.prototype.ProcessTooltip = function(pnt) {
      if (!pnt || !this.draw_content || !this.draw_g || !this.tt_handle || this.options.Proj) {
         if (this.draw_g !== null)
            this.draw_g.select(".tooltip_bin").remove();
         return null;
      }

      let histo = this.GetHisto(),
          h = this.tt_handle,
          ttrect = this.draw_g.select(".tooltip_bin");

      if (h.poly) {
         // process tooltips from TH2Poly

         let pmain = this.frame_painter(), foundindx = -1, bin;
         const realx = (pmain.grx === pmain.x) ? pmain.x.invert(pnt.x) : undefined,
               realy = (pmain.gry === pmain.y) ? pmain.y.invert(pnt.y) : undefined;

         if ((realx!==undefined) && (realy!==undefined)) {
            const len = histo.fBins.arr.length;

            for (let i = 0; (i < len) && (foundindx < 0); ++ i) {
               bin = histo.fBins.arr[i];

               // found potential bins candidate
               if ((realx < bin.fXmin) || (realx > bin.fXmax) ||
                    (realy < bin.fYmin) || (realy > bin.fYmax)) continue;

               // ignore empty bins with col0 option
               if ((bin.fContent === 0) && !this.options.Zero) continue;

               let gr = bin.fPoly, numgraphs = 1;
               if (gr._typename === 'TMultiGraph') { numgraphs = bin.fPoly.fGraphs.arr.length; gr = null; }

               for (let ngr=0;ngr<numgraphs;++ngr) {
                  if (!gr || (ngr>0)) gr = bin.fPoly.fGraphs.arr[ngr];
                  if (gr.IsInside(realx,realy)) {
                     foundindx = i;
                     break;
                  }
               }
            }
         }

         if (foundindx < 0) {
            ttrect.remove();
            return null;
         }

         let res = { name: "histo", title: histo.fTitle || "title",
                     x: pnt.x, y: pnt.y,
                     color1: this.lineatt ? this.lineatt.color : 'green',
                     color2: this.fillatt ? this.fillatt.fillcoloralt('blue') : 'blue',
                     exact: true, menu: true,
                     lines: this.ProvidePolyBinHints(foundindx, realx, realy) };

         if (pnt.disabled) {
            ttrect.remove();
            res.changed = true;
         } else {

            if (ttrect.empty())
               ttrect = this.draw_g.append("svg:path")
                            .attr("class","tooltip_bin h1bin")
                            .style("pointer-events","none");

            res.changed = ttrect.property("current_bin") !== foundindx;

            if (res.changed)
                  ttrect.attr("d", this.CreatePolyBin(pmain, bin))
                        .style("opacity", "0.7")
                        .property("current_bin", foundindx);
         }

         if (res.changed)
            res.user_info = { obj: histo,  name: "histo",
                              bin: foundindx,
                              cont: bin.fContent,
                              grx: pnt.x, gry: pnt.y };

         return res;

      } else

      if (h.candle) {
         // process tooltips for candle

         let p, i;

         for (i=0;i<h.candle.length;++i) {
            p = h.candle[i];
            if ((p.x1 <= pnt.x) && (pnt.x <= p.x2) && (p.yy1 <= pnt.y) && (pnt.y <= p.yy2)) break;
         }

         if (i>=h.candle.length) {
            ttrect.remove();
            return null;
         }

         let res = { name: "histo", title: histo.fTitle || "title",
                     x: pnt.x, y: pnt.y,
                     color1: this.lineatt ? this.lineatt.color : 'green',
                     color2: this.fillatt ? this.fillatt.fillcoloralt('blue') : 'blue',
                     lines: this.GetCandleTips(p), exact: true, menu: true };

         if (pnt.disabled) {
            ttrect.remove();
            res.changed = true;
         } else {

            if (ttrect.empty())
               ttrect = this.draw_g.append("svg:rect")
                                   .attr("class","tooltip_bin h1bin")
                                   .style("pointer-events","none");

            res.changed = ttrect.property("current_bin") !== i;

            if (res.changed)
               ttrect.attr("x", p.x1)
                     .attr("width", p.x2-p.x1)
                     .attr("y", p.yy1)
                     .attr("height", p.yy2- p.yy1)
                     .style("opacity", "0.7")
                     .property("current_bin", i);
         }

         if (res.changed)
            res.user_info = { obj: histo,  name: "histo",
                              bin: i+1, cont: p.median, binx: i+1, biny: 1,
                              grx: pnt.x, gry: pnt.y };

         return res;
      }

      let i, j, binz = 0, colindx = null;

      // search bins position
      for (i = h.i1; i < h.i2; ++i)
         if ((pnt.x>=h.grx[i]) && (pnt.x<=h.grx[i+1])) break;

      for (j = h.j1; j < h.j2; ++j)
         if ((pnt.y>=h.gry[j+1]) && (pnt.y<=h.gry[j])) break;

      if ((i < h.i2) && (j < h.j2)) {
         binz = histo.getBinContent(i+1,j+1);
         if (this.is_projection) {
            colindx = 0; // just to avoid hide
         } else if (h.hide_only_zeros) {
            colindx = (binz === 0) && !this._show_empty_bins ? null : 0;
         } else {
            colindx = h.palette.getContourIndex(binz);
            if ((colindx === null) && (binz === 0) && this._show_empty_bins) colindx = 0;
         }
      }

      if (colindx === null) {
         ttrect.remove();
         return null;
      }

      let res = { name: "histo", title: histo.fTitle || "title",
                  x: pnt.x, y: pnt.y,
                  color1: this.lineatt ? this.lineatt.color : 'green',
                  color2: this.fillatt ? this.fillatt.fillcoloralt('blue') : 'blue',
                  lines: this.GetBinTips(i, j), exact: true, menu: true };

      if (this.options.Color) res.color2 = h.palette.getColor(colindx);

      if (pnt.disabled && !this.is_projection) {
         ttrect.remove();
         res.changed = true;
      } else {
         if (ttrect.empty())
            ttrect = this.draw_g.append("svg:rect")
                                .attr("class","tooltip_bin h1bin")
                                .style("pointer-events","none");

         let i1 = i, i2 = i+1,
             j1 = j, j2 = j+1,
             x1 = h.grx[i1], x2 = h.grx[i2],
             y1 = h.gry[j2], y2 = h.gry[j1],
             binid = i*10000 + j;

         if (this.is_projection == "X") {
            x1 = 0; x2 = this.frame_width();
            if (this.projection_width > 1) {
               let dd = (this.projection_width-1)/2;
               if (j2+dd >= h.j2) { j2 = Math.min(Math.round(j2+dd), h.j2); j1 = Math.max(j2 - this.projection_width, h.j1); }
                             else { j1 = Math.max(Math.round(j1-dd), h.j1); j2 = Math.min(j1 + this.projection_width, h.j2); }
            }
            y1 = h.gry[j2]; y2 = h.gry[j1];
            binid = j1*777 + j2*333;
         } else if (this.is_projection == "Y") {
            y1 = 0; y2 = this.frame_height();
            if (this.projection_width > 1) {
               let dd = (this.projection_width-1)/2;
               if (i2+dd >= h.i2) { i2 = Math.min(Math.round(i2+dd), h.i2); i1 = Math.max(i2 - this.projection_width, h.i1); }
                             else { i1 = Math.max(Math.round(i1-dd), h.i1); i2 = Math.min(i1 + this.projection_width, h.i2); }
            }
            x1 = h.grx[i1], x2 = h.grx[i2],
            binid = i1*777 + i2*333;
         }

         res.changed = ttrect.property("current_bin") !== binid;

         if (res.changed)
            ttrect.attr("x", x1)
                  .attr("width", x2 - x1)
                  .attr("y", y1)
                  .attr("height", y2 - y1)
                  .style("opacity", "0.7")
                  .property("current_bin", binid);

         if (this.is_projection && res.changed)
            this.RedrawProjection(i1, i2, j1, j2);
      }

      if (res.changed)
         res.user_info = { obj: histo, name: "histo",
                           bin: histo.getBin(i+1, j+1), cont: binz, binx: i+1, biny: j+1,
                           grx: pnt.x, gry: pnt.y };

      return res;
   }

   RH2Painter.prototype.CanZoomIn = function(axis,min,max) {
      // check if it makes sense to zoom inside specified axis range

      if (axis=="z") return true;

      let obj = this.GetAxis(axis);

      return (obj.FindBin(max,0.5) - obj.FindBin(min,0) > 1);
   }

   RH2Painter.prototype.Draw2D = function(reason) {

      this.mode3d = false;
      this.Clear3DScene();

      // draw new palette, resize frame if required
      // let pp = this.DrawColorPalette(this.options.Zscale && (this.options.Color || this.options.Contour), true);

      return this.DrawAxes()
                 .then(res => res ? this.DrawingBins(reason) : false)
                 .then(res2 => {
                    // called when bins received from server, must be reentrant
                    if (!res2) return false;
                    this.Draw2DBins();
                    this.UpdateStatWebCanvas();
                    return this.AddInteractive();
                 });
   }

   RH2Painter.prototype.Draw3D = function(reason) {
      this.mode3d = true;
      return JSROOT.require('v7hist3d').then(() => this.Draw3D(reason));
   }

   RH2Painter.prototype.CallDrawFunc = function(reason) {
      let main = this.frame_painter();

      if (main && (main.mode3d !== this.options.Mode3D) && !this.is_main_painter())
         this.options.Mode3D = main.mode3d;

      let funcname = this.options.Mode3D ? "Draw3D" : "Draw2D";

      return this[funcname](reason);
   }

   RH2Painter.prototype.Redraw = function(reason) {
      this.CallDrawFunc(reason);
   }

   let drawHist2 = (divid, obj, opt) => {
      // create painter and add it to canvas
      let painter = new RH2Painter(obj);

      if (!painter.PrepareFrame(divid)) return null;

      painter.options = { Hist: false, Error: false, Zero: false, Mark: false,
                          Line: false, Fill: false, Lego: 0, Surf: 0,
                          Text: true, TextAngle: 0, TextKind: "",
                          BaseLine: false, Mode3D: false, AutoColor: 0,
                          Color: false, Scat: false, ScatCoef: 1, Candle: "", Box: false, BoxStyle: 0, Arrow: false, Contour: 0, Proj: 0,
                          BarOffset: 0., BarWidth: 1., minimum: -1111, maximum: -1111 };

      let kind = painter.v7EvalAttr("kind", ""),
          sub = painter.v7EvalAttr("sub", 0),
          o = painter.options;

      o.Text = painter.v7EvalAttr("text", false);

      switch(kind) {
         case "lego": o.Lego = sub > 0 ? 10+sub : 12; o.Mode3D = true; break;
         case "surf": o.Surf = sub > 0 ? 10+sub : 1; o.Mode3D = true; break;
         case "box": o.Box = true; o.BoxStyle = 10 + sub; break;
         case "err": o.Error = true; o.Mode3D = true; break;
         case "cont": o.Contour = sub > 0 ? 10+sub : 1; break;
         case "arr": o.Arrow = true; break;
         case "scat": o.Scat = true; break;
         case "col": o.Color = true; break;
         default: if (!o.Text) o.Color = true;
      }

      // here we deciding how histogram will look like and how will be shown
      // painter.DecodeOptions(opt);

      if (painter.IsTH2Poly()) {
         if (o.Mode3D) o.Lego = 12;
                  else o.Color = true;
      }

      painter._show_empty_bins = false;

      painter._can_move_colz = true;

      painter.ScanContent();

      // painter.CreateStat(); // only when required

      painter.CallDrawFunc().then(() => {
         //if (!this.options.Mode3D && this.options.AutoZoom) this.AutoZoom();
         // this.FillToolbar();
         //if (this.options.Project && !this.mode3d)
         //   this.ToggleProjection(this.options.Project);
         painter.DrawingReady();
      });

      return painter;
   }

   // =================================================================================


   // place basic declaration here to be able use it with RHistDisplayItem
   function RH3Painter(histo) {
      JSROOT.v7.RHistPainter.call(this, histo);

      this.mode3d = true;
   }

   RH3Painter.prototype = Object.create(RHistPainter.prototype);

   RH3Painter.prototype.Dimension = function() {
      return 3;
   }

   let drawHist3 = (divid, histo /*, opt*/) => {
      // create painter and add it to canvas
      let painter = new RH3Painter(histo);

      painter.PrepareFrame(divid, true); // create if necessary frame in 3d mode

      painter.options = { Box: 0, Scatter: false, Sphere: 0, Color: false, minimum: -1111, maximum: -1111 };

      let kind = painter.v7EvalAttr("kind", ""),
          sub = painter.v7EvalAttr("sub", 0),
          o = painter.options;

      switch(kind) {
         case "box": o.Box = 10 + sub; break;
         case "sphere": o.Sphere = 10 + sub; break;
         case "col": o.Color = true; break;
         case "scat": o.Scatter = true;  break;
         default: o.Box = 10;
      }

      JSROOT.require('v7hist3d').then(() => {
         painter.ScanContent();
         painter.Redraw();
         painter.DrawingReady();
      });

      return painter;
   }

   // =================================================================================

   let drawHistDisplayItem = (divid, obj, opt) => {
      if (!obj)
         return null;

      if (obj.fAxes.length == 1)
         return drawHist1(divid, obj, opt);

      if (obj.fAxes.length == 2)
         return drawHist2(divid, obj, opt);

      if (obj.fAxes.length == 3)
         return drawHist3(divid, obj, opt);

      return null;
   }

   // =============================================================


   function RHistStatsPainter(palette, opt) {
      JSROOT.v7.RPavePainter.call(this, palette, opt, "stats");
   }

   RHistStatsPainter.prototype = Object.create(JSROOT.v7.RPavePainter.prototype);

   RHistStatsPainter.prototype.ClearStat = function() {
      this.stats_lines = [];
   }

   RHistStatsPainter.prototype.AddText = function(line) {
      this.stats_lines.push(line);
   }

   RHistStatsPainter.prototype.UpdateStatistic = function(reply) {
      this.stats_lines = reply.lines;
      this.DrawStatistic(this.stats_lines);
   }

   RHistStatsPainter.prototype.FillStatistic = function() {
      let pp = this.pad_painter();
      if (pp && pp._fast_drawing) return false;

      let obj = this.GetObject();
      if (obj.fLines !== undefined) {
         this.stats_lines = obj.fLines;
         delete obj.fLines;
         return true;
      }

      if (this.v7CommMode() == JSROOT.v7.CommMode.kOffline) {
         let main = this.main_painter();
         if (!main || (typeof main.FillStatistic !== 'function')) return false;
         // we take statistic from main painter
         return main.FillStatistic(this, JSROOT.gStyle.fOptStat, JSROOT.gStyle.fOptFit);
      }

      // show lines which are exists, maybe server request will be recieved later
      return (this.stats_lines !== undefined);
   }

   RHistStatsPainter.prototype.Format = function(value, fmt) {
      if (!fmt) fmt = "stat";

      switch(fmt) {
         case "stat" : fmt = JSROOT.gStyle.fStatFormat; break;
         case "fit": fmt = JSROOT.gStyle.fFitFormat; break;
         case "entries": if ((Math.abs(value) < 1e9) && (Math.round(value) == value)) return value.toFixed(0); fmt = "14.7g"; break;
         case "last": fmt = this.lastformat; break;
      }

      let res = JSROOT.FFormat(value, fmt || "6.4g", true);

      this.lastformat = res[1];

      return res[0];
   }

   RHistStatsPainter.prototype.DrawContent = function() {
      if (this.FillStatistic())
         this.DrawStatistic(this.stats_lines);
   }

   RHistStatsPainter.prototype.ChangeMask = function(nbit) {
      let obj = this.GetObject(), mask = (1<<nbit);
      if (obj.fShowMask & mask)
         obj.fShowMask = obj.fShowMask & ~mask;
      else
         obj.fShowMask = obj.fShowMask | mask;

      if (this.FillStatistic())
         this.DrawStatistic(this.stats_lines);
   }

   RHistStatsPainter.prototype.StatsContextMenu = function(evnt) {
      evnt.preventDefault();
      evnt.stopPropagation(); // disable main context menu

      JSROOT.require('interactive')
            .then(() => jsrp.createMenu(this, evnt))
            .then(menu =>
         {
            let obj = this.GetObject(),
                action = this.ChangeMask.bind(this);

            menu.add("header: StatBox");

            for (let n=0;n<obj.fEntries.length; ++n)
               menu.addchk((obj.fShowMask & (1<<n)), obj.fEntries[n], n, action);

            this.FillObjectExecMenu(menu, "", () => menu.show());
         });
   }

   RHistStatsPainter.prototype.DrawStatistic = function(lines) {

      let textFont = this.v7EvalFont("stats_text", { size: 12, color: "black", align: 22 }),
          first_stat = 0, num_cols = 0, maxlen = 0,
          width = this.pave_width,
          height = this.pave_height;

      if (!lines) return;

      let nlines = lines.length;
      // adjust font size
      for (let j = 0; j < nlines; ++j) {
         let line = lines[j];
         if (j>0) maxlen = Math.max(maxlen, line.length);
         if ((j == 0) || (line.indexOf('|') < 0)) continue;
         if (first_stat === 0) first_stat = j;
         let parts = line.split("|");
         if (parts.length > num_cols)
            num_cols = parts.length;
      }

      // for characters like 'p' or 'y' several more pixels required to stay in the box when drawn in last line
      let stepy = height / nlines, has_head = false, margin_x = 0.02 * width;

      let text_g = this.draw_g.select(".statlines");
      if (text_g.empty())
         text_g = this.draw_g.append("svg:g").attr("class", "statlines");
      else
         text_g.selectAll("*").remove();

      textFont.setSize(height/(nlines * 1.2))
      this.StartTextDrawing(textFont, 'font' , text_g);

      if (nlines == 1) {
         this.DrawText({ width: width, height: height, text: lines[0], latex: 1, draw_g: text_g });
      } else
      for (let j = 0; j < nlines; ++j) {
         let posy = j*stepy;

         if (first_stat && (j >= first_stat)) {
            let parts = lines[j].split("|");
            for (let n = 0; n < parts.length; ++n)
               this.DrawText({ align: "middle", x: width * n / num_cols, y: posy, latex: 0,
                               width: width/num_cols, height: stepy, text: parts[n], draw_g: text_g });
         } else if (lines[j].indexOf('=') < 0) {
            if (j==0) {
               has_head = true;
               if (lines[j].length > maxlen + 5)
                  lines[j] = lines[j].substr(0,maxlen+2) + "...";
            }
            this.DrawText({ align: (j == 0) ? "middle" : "start", x: margin_x, y: posy,
                            width: width-2*margin_x, height: stepy, text: lines[j], draw_g: text_g });
         } else {
            let parts = lines[j].split("="), args = [];

            for (let n = 0; n < 2; ++n) {
               let arg = {
                  align: (n == 0) ? "start" : "end", x: margin_x, y: posy,
                  width: width-2*margin_x, height: stepy, text: parts[n], draw_g: text_g,
                  _expected_width: width-2*margin_x, _args: args,
                  post_process: function(painter) {
                    if (this._args[0].ready && this._args[1].ready)
                       painter.TextScaleFactor(1.05*(this._args[0].result_width && this._args[1].result_width)/this.__expected_width, this.draw_g);
                  }
               };
               args.push(arg);
            }

            for (let n = 0; n < 2; ++n)
               this.DrawText(args[n]);
         }
      }

      this.FinishTextDrawing(text_g);

      let lpath = "";

      if (has_head)
         lpath += "M0," + Math.round(stepy) + "h" + width;

      if ((first_stat > 0) && (num_cols > 1)) {
         for (let nrow = first_stat; nrow < nlines; ++nrow)
            lpath += "M0," + Math.round(nrow * stepy) + "h" + width;
         for (let ncol = 0; ncol < num_cols - 1; ++ncol)
            lpath += "M" + Math.round(width / num_cols * (ncol + 1)) + "," + Math.round(first_stat * stepy) + "V" + height;
      }

      if (lpath) this.draw_g.append("svg:path").attr("d",lpath) /*.call(this.lineatt.func)*/;
   }

   RHistStatsPainter.prototype.Redraw = function(reason) {
      if (reason && (typeof reason == "string") && (reason.indexOf("zoom") == 0) &&
          (this.v7CommMode() == JSROOT.v7.CommMode.kNormal)) {
         let req = {
            _typename: "ROOT::Experimental::RHistStatBoxBase::RRequest",
            mask: this.GetObject().fShowMask // lines to show in stat box
         };

         this.v7SubmitRequest("stat", req, this.UpdateStatistic.bind(this));
      }

      this.DrawPave();
   }

   function drawHistStats(divid, stats, opt) {
      let painter = new RHistStatsPainter(stats, opt);

      painter.SetDivId(divid);

      painter.DrawPave();

      return painter.DrawingReady();
   }

   JSROOT.v7.RHistPainter = RHistPainter;
   JSROOT.v7.RH1Painter = RH1Painter;
   JSROOT.v7.RH2Painter = RH2Painter;
   JSROOT.v7.RH3Painter = RH3Painter;

   JSROOT.v7.drawHist1 = drawHist1;
   JSROOT.v7.drawHist2 = drawHist2;
   JSROOT.v7.drawHist3 = drawHist3;

   JSROOT.v7.drawHistDisplayItem = drawHistDisplayItem;
   JSROOT.v7.drawHistStats = drawHistStats;

   return JSROOT;

});
