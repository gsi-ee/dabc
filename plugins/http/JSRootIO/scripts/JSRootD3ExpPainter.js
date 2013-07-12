// JSROOTD3Painter.js
//
// core methods for Javascript ROOT Graphics, using d3.js.
//

// The "source_dir" variable is defined in JSRootInterface.js

var d_tree, key_tree, defs;

var kWhite = 0, kBlack = 1, kGray = 920, kRed = 632, kGreen = 416, kBlue = 600,
    kYellow = 400, kMagenta = 616, kCyan = 432, kOrange = 800, kSpring = 820,
    kTeal = 840, kAzure = 860, kViolet = 880, kPink = 900;

var symbols_map = {
   // greek letters
   '#alpha' : '\u03B1',
   '#beta' : '\u03B2',
   '#chi' : '\u03C7',
   '#delta' : '\u03B4',
   '#varepsilon' : '\u03B5',
   '#phi' : '\u03C6',
   '#gamma' : '\u03B3',
   '#eta' : '\u03B7',
   '#iota' : '\u03B9',
   '#varphi' : '\u03C6',
   '#kappa' : '\u03BA',
   '#lambda' : '\u03BB',
   '#mu' : '\u03BC',
   '#nu' : '\u03BD',
   '#omicron' : '\u03BF',
   '#pi' : '\u03C0',
   '#theta' : '\u03B8',
   '#rho' : '\u03C1',
   '#sigma' : '\u03C3',
   '#tau' : '\u03C4',
   '#upsilon' : '\u03C5',
   '#varomega' : '\u03D6',
   '#omega' : '\u03C9',
   '#xi' : '\u03BE',
   '#psi' : '\u03C8',
   '#zeta' : '\u03B6',
   '#Alpha' : '\u0391',
   '#Beta' : '\u0392',
   '#Chi' : '\u03A7',
   '#Delta' : '\u0394',
   '#Epsilon' : '\u0395',
   '#Phi' : '\u03A6',
   '#Gamma' : '\u0393',
   '#Eta' : '\u0397',
   '#Iota' : '\u0399',
   '#vartheta' : '\u03D1',
   '#Kappa' : '\u039A',
   '#Lambda' : '\u039B',
   '#Mu' : '\u039C',
   '#Nu' : '\u039D',
   '#Omicron' : '\u039F',
   '#Pi' : '\u03A0',
   '#Theta' : '\u0398',
   '#Rho' : '\u03A1',
   '#Sigma' : '\u03A3',
   '#Tau' : '\u03A4',
   '#Upsilon' : '\u03A5',
   '#varsigma' : '\u03C2',
   '#Omega' : '\u03A9',
   '#Xi' : '\u039E',
   '#Psi' : '\u03A8',
   '#Zeta' : '\u0396',
   '#varUpsilon' : '\u03D2',
   '#epsilon' : '\u03B5',
   // math symbols

   '#sqrt' : '\u221A',

   // from TLatex tables #2 & #3
   '#leq' : '\u2264',
   '#/' : '\u2044',
   '#infty' : '\u221E',
   '#voidb' : '\u0192',
   '#club' : '\u2663',
   '#diamond' : '\u2666',
   '#heart' : '\u2665',
   '#spade' : '\u2660',
   '#leftrightarrow' : '\u2194',
   '#leftarrow' : '\u2190',
   '#uparrow' : '\u2191',
   '#rightarrow' : '\u2192',
   '#downarrow' : '\u2193',
   '#circ' : '\u02C6', // ^
   '#pm' : '\xB1',
   '#doublequote' : '\u2033',
   '#geq' : '\u2265',
   '#times' : '\xD7',
   '#propto' : '\u221D',
   '#partial' : '\u2202',
   '#bullet' : '\u2022',
   '#divide' : '\xF7',
   '#neq' : '\u2260',
   '#equiv' : '\u2261',
   '#approx' : '\u2248', // should be \u2245 ?
   '#3dots' : '\u2026',
   '#cbar' : '\u007C',
   '#topbar' : '\xAF',
   '#downleftarrow' : '\u21B5',
   '#aleph' : '\u2135',
   '#Jgothic' : '\u2111',
   '#Rgothic' : '\u211C',
   '#voidn' : '\u2118',
   '#otimes' : '\u2297',
   '#oplus' : '\u2295',
   '#oslash' : '\u2205',
   '#cap' : '\u2229',
   '#cup' : '\u222A',
   '#supseteq' : '\u2287',
   '#supset' : '\u2283',
   '#notsubset' : '\u2284',
   '#subseteq' : '\u2286',
   '#subset' : '\u2282',
   '#int' : '\u222B',
   '#in' : '\u2208',
   '#notin' : '\u2209',
   '#angle' : '\u2220',
   '#nabla' : '\u2207',
   '#oright' : '\xAE',
   '#ocopyright' : '\xA9',
   '#trademark' : '\u2122',
   '#prod' : '\u220F',
   '#surd' : '\u221A',
   '#upoint' : '\u22C5',
   '#corner' : '\xAC',
   '#wedge' : '\u2227',
   '#vee' : '\u2228',
   '#Leftrightarrow' : '\u21D4',
   '#Leftarrow' : '\u21D0',
   '#Uparrow' : '\u21D1',
   '#Rightarrow' : '\u21D2',
   '#Downarrow' : '\u21D3',
   '#LT' : '\x3C',
   '#void1' : '\xAE',
   '#copyright' : '\xA9',
   '#void3' : '\u2122',
   '#sum' : '\u2211',
   '#arctop' : '',
   '#lbar' : '',
   '#arcbottom' : '',
   '#void8' : '',
   '#bottombar' : '\u230A',
   '#arcbar' : '',
   '#ltbar' : '',
   '#AA' : '\u212B',
   '#aa' : '\u00E5',
   '#void06' : '',
   '#GT' : '\x3E',
   '#forall' : '\u2200',
   '#exists' : '\u2203',
   '#bar' : '',
   '#vec' : '',
   '#dot' : '\u22C5',
   '#hat' : '\xB7',
   '#ddot' : '',
   '#acute' : '\acute',
   '#grave' : '',
   '#check' : '\u2713',
   '#tilde' : '\u02DC',
   '#slash' : '\u2044',
   '#hbar' : '\u0127',
   '#box' : '',
   '#Box' : '',
   '#parallel' : '',
   '#perp' : '\u22A5',
   '#odot' : ''
};

var tooltip = function() {
   var id = 'tt';
   var top = 3;
   var left = 3;
   var maxw = 150;
   var speed = 10;
   var timer = 20;
   var endalpha = 95;
   var alpha = 0;
   var tt,t,c,b,h;
   var ie = document.all ? true : false;
   return {
      show: function(v, w) {
         if (tt == null) {
            tt = document.createElement('div');
            tt.setAttribute('id',id);
            t = document.createElement('div');
            t.setAttribute('id',id + 'top');
            c = document.createElement('div');
            c.setAttribute('id',id + 'cont');
            b = document.createElement('div');
            b.setAttribute('id',id + 'bot');
            tt.appendChild(t);
            tt.appendChild(c);
            tt.appendChild(b);
            document.body.appendChild(tt);
            tt.style.opacity = 0;
            tt.style.filter = 'alpha(opacity=0)';
            document.onmousemove = this.pos;
         }
         tt.style.display = 'block';
         c.innerHTML = v;
         tt.style.width = w ? w + 'px' : 'auto';
         tt.style.width = 'auto'; // let it be automatically resizing...
         if (!w && ie) {
            t.style.display = 'none';
            b.style.display = 'none';
            tt.style.width = tt.offsetWidth;
            t.style.display = 'block';
            b.style.display = 'block';
         }
         //if (tt.offsetWidth > maxw) { tt.style.width = maxw + 'px'; }
         h = parseInt(tt.offsetHeight) + top;
         clearInterval(tt.timer);
         tt.timer = setInterval( function() { tooltip.fade(1) }, timer );
      },
      pos: function(e) {
         var u = ie ? event.clientY + document.documentElement.scrollTop : e.pageY;
         var l = ie ? event.clientX + document.documentElement.scrollLeft : e.pageX;
         tt.style.top = u + 15 + 'px';//(u - h) + 'px';
         tt.style.left = (l + left) + 'px';
      },
      fade: function(d) {
         var a = alpha;
         if ((a != endalpha && d == 1) || (a != 0 && d == -1)) {
            var i = speed;
            if (endalpha - a < speed && d == 1) {
               i = endalpha - a;
            }
            else if (alpha < speed && d == -1) {
               i = a;
            }
            alpha = a + (i * d);
            tt.style.opacity = alpha * .01;
            tt.style.filter = 'alpha(opacity=' + alpha + ')';
         }
         else {
            clearInterval(tt.timer);
            if (d == -1) { tt.style.display = 'none'; }
         }
      },
      hide: function() {
         if (tt == null) return;
         clearInterval(tt.timer);
         tt.timer = setInterval( function() { tooltip.fade(-1) }, timer );
      }
   };
}();

/**
 * @author alteredq / http://alteredqualia.com/
 * @author mr.doob / http://mrdoob.com/
 */
var Detector = {
   canvas: !! window.CanvasRenderingContext2D,
   webgl: ( function () { try { return !! window.WebGLRenderingContext && !! document.createElement( 'canvas' ).getContext( 'experimental-webgl' ); } catch( e ) { return false; } } )(),
   workers: !! window.Worker, fileapi: window.File && window.FileReader && window.FileList && window.Blob
};

/*
 * Function that generates all root colors
 */
function generateAllColors () {
   var colorMap = new Array(
      'rgb(255, 255, 255)',
      'rgb(0, 0, 0)',
      'rgb(255, 0, 0)',
      'rgb(0, 255, 0)',
      'rgb(0, 0, 255)',
      'rgb(255, 255, 0)',
      'rgb(255, 0, 255)',
      'rgb(0, 255, 255)',
      'rgb(89, 211, 84)',
      'rgb(89, 84, 216)',
      'rgb(254, 254, 254)',
      'rgb(191, 181, 173)',
      'rgb(76, 76, 76)',
      'rgb(102, 102, 102)',
      'rgb(127, 127, 127)',
      'rgb(153, 153, 153)',
      'rgb(178, 178, 178)',
      'rgb(204, 204, 204)',
      'rgb(229, 229, 229)',
      'rgb(242, 242, 242)',
      'rgb(204, 198, 170)',
      'rgb(204, 198, 170)',
      'rgb(193, 191, 168)',
      'rgb(186, 181, 163)',
      'rgb(178, 165, 150)',
      'rgb(183, 163, 155)',
      'rgb(173, 153, 140)',
      'rgb(155, 142, 130)',
      'rgb(135, 102, 86)',
      'rgb(175, 206, 198)',
      'rgb(132, 193, 163)',
      'rgb(137, 168, 160)',
      'rgb(130, 158, 140)',
      'rgb(173, 188, 198)',
      'rgb(122, 142, 153)',
      'rgb(117, 137, 145)',
      'rgb(104, 130, 150)',
      'rgb(109, 122, 132)',
      'rgb(124, 153, 209)',
      'rgb(127, 127, 155)',
      'rgb(170, 165, 191)',
      'rgb(211, 206, 135)',
      'rgb(221, 186, 135)',
      'rgb(188, 158, 130)',
      'rgb(198, 153, 124)',
      'rgb(191, 130, 119)',
      'rgb(206, 94, 96)',
      'rgb(170, 142, 147)',
      'rgb(165, 119, 122)',
      'rgb(147, 104, 112)',
      'rgb(211, 89, 84)');

   var circleColors = [632, 416, 600, 400, 616, 432];

   var rectangleColors = [800, 820, 840, 860, 880, 900];

   var set1 = [ 255,204,204, 255,153,153, 204,153,153, 255,102,102, 204,102,102
               ,153,102,102, 255, 51, 51, 204, 51, 51, 153, 51, 51, 102, 51, 51
               ,255,  0,  0, 204,  0,  0, 153,  0,  0, 102,  0,  0,  51,  0,  0];
   var set2 = [ 204,255,204, 153,255,153, 153,204,153, 102,255,102, 102,204,102
               ,102,153,102, 51,255, 51,  51,204, 51,  51,153, 51,  51,102, 51
               ,  0,255,  0,   0,204,  0,   0,153,  0,   0,102,  0,  0, 51,  0];
   var set3 = [ 204,204,255, 153,153,255, 153,153,204, 102,102,255, 102,102,204
               ,102,102,153,  51, 51,255,  51, 51,204,  51, 51,153,  51, 51,102
               ,  0,  0,255,   0,  0,204,   0,  0,153,   0,  0,102,   0,  0, 51];
   var set4 = [ 255,255,204, 255,255,153, 204,204,153, 255,255,102, 204,204,102
               ,153,153,102, 255,255, 51, 204,204, 51, 153,153, 51, 102,102, 51
               ,255,255,  0, 204,204,  0, 153,153,  0, 102,102,  0,  51, 51,  0];
   var set5 = [ 255,204,255, 255,153,255, 204,153,204, 255,102,255, 204,102,204
               ,153,102,153, 255, 51,255, 204, 51,204, 153, 51,153, 102, 51,102
               ,255,  0,255, 204,  0,204, 153,  0,153, 102,  0,102,  51,  0, 51];
   var set6 = [ 204,255,255, 153,255,255, 153,204,204, 102,255,255, 102,204,204
               ,102,153,153,  51,255,255,  51,204,204,  51,153,153,  51,102,102
               ,  0,255,255,   0,204,204,   0,153,153,   0,102,102,   0, 51,  51];

   var circleSets = new Array(set1, set2, set3, set4, set5, set6);

   var set7 = [ 255,204,153,  204,153,102,  153,102, 51,  153,102,  0,  204,153, 51
                ,255,204,102,  255,153,  0,  255,204, 51,  204,153,  0,  255,204,  0
                ,255,153, 51,  204,102,  0,  102, 51,  0,  153, 51,  0,  204,102, 51
                ,255,153,102,  255,102,  0,  255,102, 51,  204, 51,  0,  255, 51,  0];
   var set8 = [ 153,255, 51,  102,204,  0,   51,102,  0,   51,153,  0,  102,204, 51
               ,153,255,102,  102,255,  0,  102,255, 51,   51,204,  0,   51,255, 0
               ,204,255,153,  153,204,102,  102,153, 51,  102,153,  0,  153,204, 51
               ,204,255,102,  153,255,  0,  204,255, 51,  153,204,  0,  204,255,  0];
   var set9 = [ 153,255,204,  102,204,153,   51,153,102,    0,153,102,   51,204,153
               ,102,255,204,    0,255,102,   51,255,204,    0,204,153,    0,255,204
               , 51,255,153,    0,204,102,    0,102, 51,    0,153, 51,   51,204,102
               ,102,255,153,    0,255,153,   51,255,102,    0,204, 51,    0,255, 51];
   var set10 = [153,204,255,  102,153,204,   51,102,153,    0, 51,153,   51,102,204
               ,102,153,255,    0,102,255,   51,102,255,    0, 51,204,    0, 51,255
               , 51,153,255,    0,102,204,    0, 51,102,    0,102,153,   51,153,204
               ,102,204,255,    0,153,255,   51,204,255,    0,153,204,    0,204,255];
   var set11 = [204,153,255,  153,102,204,  102, 51,153,  102,  0,153,  153, 51,204
               ,204,102,255,  153,  0,255,  204, 51,255,  153,  0,204,  204,  0,255
               ,153, 51,255,  102,  0,204,   51,  0,102,   51,  0,153,  102, 51,204
               ,153,102,255,  102,  0,255,  102, 51,255,   51,  0,204,   51,  0,255];
   var set12 = [255, 51,153,  204,  0,102,  102,  0, 51,  153,  0, 51,  204, 51,102
               ,255,102,153,  255,  0,102,  255, 51,102,  204,  0, 51,  255,  0, 51
               ,255,153,204,  204,102,153,  153, 51,102,  153,  0,102,  204, 51,153
               ,255,102,204,  255,  0,153,  204,  0,153,  255, 51,204,  255,  0,153];

   var rectSets = new Array(set7, set8, set9, set10, set11, set12);

   /*
    * Define circle colors
    */
   for(var i = 0; i < 6; i++) {
      for(var j = 0; j < 15; j++) {
         var colorn = circleColors[i] + j - 10;
         colorMap[colorn] = 'rgb(' + circleSets[i][3*j] + ', ' + circleSets[i][3*j+1] + ', ' + circleSets[i][3*j+2] + ')';
         colorn = rectangleColors[i] + j - 9;
         colorMap[colorn] = 'rgb('+ rectSets[i][3*j] + ', ' + rectSets[i][3*j+1] + ', ' + rectSets[i][3*j+2] + ')';
      }
    }
    return colorMap;
};

function getFontDetails(fontName) {
   var weight = "";
   var style = "";
   var name = "Arial";

   if (fontName.indexOf("bold") != -1) {
      weight = "bold";
      //The first 5 characters are removed because "bold " is always first when it occurs
      fontName = fontName.substring(5, fontName.length);
   }
   if (fontName.charAt(0) == 'i') {
      style = "italic";
      fontName = fontName.substring(7, fontName.length);
   }
   else if (fontName.charAt(0) == 'o') {
      style = "oblique";
      fontName = fontName.substring(8, fontName.length);
   }
   if (name == 'Symbol') {
      weight = "";
      style = "";
   }
   return {
      'weight' : weight,
      'style'  : style,
      'name'   : fontName
   };
};

/*
 * Function that returns the SVG symbol type identifier for a given root matker
 * The result is an array with 3 elements:
 *    the first is the identifier of the root marker in the SVG symbols
 *    the second is true if the shape is filled and false if it is open
 *    the third is true if the shape should be rotated
 * The identifier will be 6 if the shape is a star or 7 if it is '*'
 */
function getRootMarker(markers, i) {
   var marker = markers[i];
   var shape = 0;
   var toFill = true;
   var toRotate = false;

   if (typeof(marker) != 'undefined') {
      var fst = marker.charAt(0);
      switch (fst) {
         case 'd':
            shape = 7;
            return {'shape' : shape};
         case 'o':
            toFill = false;
            break;
         case 'g':
            toRotate = true;
      }

      var type = marker.substr(1, marker.length);
      switch (type) {
         case "circle":
            shape = 0;
            break;
         case "cross":
            shape = 1;
            break;
         case "diamond":
            shape = 2;
            break;
         case "square":
            shape = 3;
            break;
         case "triangle-up":
            shape = 4;
            break;
         case "triangle-down":
            shape = 5;
            break;
         case "star":
            shape = 6;
            break;
      };
   }
   return {
      'shape'    : shape,
      'toFill'   : toFill,
      'toRotate' : toRotate
   };
};

function stringWidth(svg, line, font_name, font_weight, font_size, font_style) {
   /* compute the bounding box of a string by using temporary svg:text */
   var text = svg.append("svg:text")
       .attr("class", "temp_text")
       .attr("font-family", font_name)
       .attr("font-weight", font_weight)
       .attr("font-style", font_style)
       .attr("font-size", font_size)
       .style("opacity", 0)
       .text(line);
   var w = text.node().getBBox().width;
   d3.select(text).remove();
   text = null;
   return w;
}

function format_id(id) {
   /* format the string id to remove specials characters
      (that cannot be used in id strings) */
   var g_id = id;
   if (g_id == "") g_id = "random_histo_" + random_id++;
   while (g_id.indexOf(' ') != -1)
      g_id = g_id.replace(' ', '_');
   while (g_id.indexOf(':') != -1)
      g_id = g_id.replace(':', '_');
   while (g_id.indexOf('.') != -1)
      g_id = g_id.replace('.', '_');
   while (g_id.indexOf('>') != -1)
      g_id = g_id.replace('>', 'gt');
   while (g_id.indexOf('<') != -1)
      g_id = g_id.replace('<', 'lt');
   while (g_id.indexOf('\\') != -1)
      g_id = g_id.replace('\\', '');
   while (g_id.indexOf('\'') != -1)
      g_id = g_id.replace('\'', '');
   while (g_id.indexOf('(') != -1)
      g_id = g_id.replace('(', '_');
   while (g_id.indexOf(')') != -1)
      g_id = g_id.replace(')', '_');
   while (g_id.indexOf('/') != -1)
      g_id = g_id.replace('/', '_');
   while (g_id.indexOf('-') != -1)
      g_id = g_id.replace('-', '_');
   while (g_id.indexOf('[') != -1)
      g_id = g_id.replace('[', '_');
   while (g_id.indexOf(']') != -1)
      g_id = g_id.replace(']', '_');
   return g_id;
};

/*
    Polyfill for touch dblclick
    http://mckamey.mit-license.org
*/
function doubleTap(elem, speed, distance) {
   if (!('ontouchstart' in elem)) {
      // non-touch has native dblclick and no need for polyfill
      return;
   }
   // default dblclick speed to half sec
   speed = Math.abs(+speed) || 500;//ms
   // default dblclick distance to within 40x40 area
   distance = Math.abs(+distance) || 40;//px

   var taps, x, y;
   var reset = function() {
      // reset state
      taps = 0;
      x = NaN;
      y = NaN;
   };
   reset();

   elem.addEventListener('touchstart', function(e) {
      var touch = e.changedTouches[0] || {}, oldX = x, oldY = y;

      taps++;
      x = +touch.pageX || +touch.clientX || +touch.screenX;
      y = +touch.pageY || +touch.clientY || +touch.screenY;

      // NaN will always be false
      if (Math.abs(oldX-x) < distance && Math.abs(oldY-y) < distance) {
         // fire dblclick event
         var e2 = document.createEvent('MouseEvents');
         if (e2.initMouseEvent) {
             e2.initMouseEvent(
                 'dblclick',
                 true,                   // dblclick bubbles
                 true,                   // dblclick cancelable
                 e.view,                 // copy view
                 taps,                   // click count
                 touch.screenX,          // copy coordinates
                 touch.screenY,
                 touch.clientX,
                 touch.clientY,
                 e.ctrlKey,              // copy key modifiers
                 e.altKey,
                 e.shiftKey,
                 e.metaKey,
                 e.button,               // copy button 0: left, 1: middle, 2: right
                 touch.target);          // copy target
         }
         elem.dispatchEvent(e2);
      }
      setTimeout(reset, speed);
   }, false);

   elem.addEventListener('touchmove', function(e) {
      reset();
   }, false);
};

function createFillPatterns(svg, id, color) {
   // create fill patterns - only if they don't exists yet
   var line_color = JSROOTPainter.getRootColor(color);
   for (var i=0; i<defs[0][0]['childNodes'].length;++i) {
      if (defs[0][0]['childNodes'][i]['id'] == "pat" + id + "_" + color &&
          defs[0][0]['childNodes'][i]['style']['stroke'] == line_color)
         return;
   }
   switch (id) {
      case 3001:
         defs.append('svg:pattern')
            .attr("id", "pat3001_"+color)
            .attr("patternUnits", "userSpaceOnUse")
            .attr("width", "3px")
            .attr("height", "2px")
            .style("stroke", line_color)
            .append('svg:rect')
            .attr("x", 0)
            .attr("y", 0)
            .attr("width", 1)
            .attr("height", 1)
            .style("stroke", line_color)
            .append('svg:rect')
            .attr("x", 2)
            .attr("y", 0)
            .attr("width", 1)
            .attr("height", 1)
            .style("stroke", line_color)
            .append('svg:rect')
            .attr("x", 1)
            .attr("y", 1)
            .attr("width", 1)
            .attr("height", 1)
            .style("stroke", line_color);
         break;
      case 3002:
         defs.append('svg:pattern')
            .attr("id", "pat3002_"+color)
            .attr("patternUnits", "userSpaceOnUse")
            .attr("width", "4px")
            .attr("height", "2px")
            .style("stroke", line_color)
            .append('svg:rect')
            .attr("x", 1)
            .attr("y", 0)
            .attr("width", 1)
            .attr("height", 1)
            .style("stroke", line_color)
            .append('svg:rect')
            .attr("x", 3)
            .attr("y", 1)
            .attr("width", 1)
            .attr("height", 1)
            .style("stroke", line_color);
         break;
      case 3003:
         defs.append('svg:pattern')
            .attr("id", "pat3003_"+color)
            .attr("patternUnits", "userSpaceOnUse")
            .attr("width", "4px")
            .attr("height", "4px")
            .style("stroke", line_color)
            .append('svg:rect')
            .attr("x", 2)
            .attr("y", 1)
            .attr("width", 1)
            .attr("height", 1)
            .style("stroke", line_color)
            .append('svg:rect')
            .attr("x", 0)
            .attr("y", 3)
            .attr("width", 1)
            .attr("height", 1)
            .style("stroke", line_color);
         break;
      case 3004:
         defs.append('svg:pattern')
            .attr("id", "pat3004_"+color)
            .attr("patternUnits", "userSpaceOnUse")
            .attr("width", "8px")
            .attr("height", "8px")
            .style("stroke", line_color)
            .append("svg:line")
            .attr("x1", 8)
            .attr("y1", 0)
            .attr("x2", 0)
            .attr("y2", 8)
            .style("stroke", line_color)
            .style("stroke-width", 1);
         break;
      case 3005:
         defs.append('svg:pattern')
            .attr("id", "pat3005_"+color)
            .attr("patternUnits", "userSpaceOnUse")
            .attr("width", "8px")
            .attr("height", "8px")
            .style("stroke", line_color)
            .append("svg:line")
            .attr("x1", 0)
            .attr("y1", 0)
            .attr("x2", 8)
            .attr("y2", 8)
            .style("stroke", line_color)
            .style("stroke-width", 1);
         break;
      case 3006:
         defs.append('svg:pattern')
            .attr("id", "pat3006_"+color)
            .attr("patternUnits", "userSpaceOnUse")
            .attr("width", "4px")
            .attr("height", "4px")
            .style("stroke", line_color)
            .append("svg:line")
            .attr("x1", 1)
            .attr("y1", 0)
            .attr("x2", 1)
            .attr("y2", 3)
            .style("stroke", line_color)
            .style("stroke-width", 1);
         break;
      case 3007:
         defs.append('svg:pattern')
            .attr("id", "pat3007_"+color)
            .attr("patternUnits", "userSpaceOnUse")
            .attr("width", "4px")
            .attr("height", "4px")
            .style("stroke", line_color)
            .append("svg:line")
            .attr("x1", 0)
            .attr("y1", 1)
            .attr("x2", 3)
            .attr("y2", 1)
            .style("stroke", line_color)
            .style("stroke-width", 1);
         break;
      default: /* == 3004 */
         defs.append('svg:pattern')
            .attr("id", "pat"+id+"_"+color)
            .attr("patternUnits", "userSpaceOnUse")
            .attr("width", "8px")
            .attr("height", "8px")
            .style("stroke", line_color)
            .append("svg:line")
            .attr("x1", 8)
            .attr("y1", 0)
            .attr("x2", 0)
            .attr("y2", 8)
            .style("stroke", line_color)
            .style("stroke-width", 1);
         break;
   }
};

var gStyle = {
      'Tooltip'       : 2,    // 0 - off, 1 - event info, 2 - full but may be slow   
      'AutoStat'      : true,
      'OptStat'       : 1111,
      'StatX'         : 0.7, 
      'StatY'         : 0.7, 
      'StatW'         : 0.28, 
      'StatH'         : 0.28,
      'StatColor'     : 0,
      'StatStyle'     : 1001,
      'StatFont'      : 42,
      'StatFontSize'  : 0,
      'StatTextColor' : 1,
   };



(function(){

   if (typeof JSROOTPainter == 'object'){
      var e1 = new Error('JSROOTPainter is already defined');
      e1.source = 'JSROOTD3ExpPainter.js';
      throw e1;
   }

   if (typeof d3 != 'object') {
      var e1 = new Error('This extension requires d3.v2.js');
      e1.source = 'JSROOTD3ExpPainter.js';
      throw e1;
   }

   // Initialize Custom colors
   var root_colors = generateAllColors();

   // Initialize colors of the default palette
   var default_palette = new Array();

   //Initialize ROOT markers
   var root_markers = new Array('fcircle','fcircle', 'fcross', 'dcross', 'ocircle',
      'gcross', 'fcircle', 'fcircle', 'fcircle', 'fcircle', 'fcircle',
      'fcircle', 'fcircle', 'fcircle', 'fcircle', 'fcircle', 'fcircle',
      'fcircle', 'fcircle', 'fcircle', 'fcircle', 'fsquare', 'ftriangle-up',
      'ftriangle-down', 'ocircle', 'osquare', 'otriangle-up', 'odiamond',
      'ocross', 'fstar', 'ostar', 'dcross', 'otriangle-down', 'fdiamond',
      'fcross');

   var root_fonts = new Array('Arial', 'Times New Roman',
      'bold Times New Roman', 'bold italic Times New Roman',
      'Arial', 'oblique Arial', 'bold Arial', 'bold oblique Arial',
      'Courier New', 'oblique Courier New', 'bold Courier New',
      'bold oblique Courier New', 'Symbol', 'Times New Roman',
      'Wingdings', 'Symbol');

   var root_line_styles = new Array("", "", "3, 3", "1, 2", "3, 4, 1, 4",
         "5, 3, 1, 3", "5, 3, 1, 3, 1, 3, 1, 3", "5, 5",
         "5, 3, 1, 3, 1, 3", "20, 5", "20, 10, 1, 10", "1, 2");

   JSROOTPainter = {};

   JSROOTPainter.version = '3.0 2013/07/8';
   
   /*
    * Helper functions
    */

   JSROOTPainter.clearCuts = function(chopt) {
      /* decode string "chopt" and remove graphical cuts */
      var left = chopt.indexOf('[');
      if (left == -1) return chopt;
      var right = chopt.indexOf(']');
      if (right == -1) return chopt;
      var nch = right-left;
      if (nch < 2) return chopt;
      for (i=0;i<=nch;i++) chopt[left+i] = ' ';
      return chopt;
   };

   JSROOTPainter.decodeOptions = function(opt, histo, pad) {
      /* decode string 'opt' and fill the option structure */
      var hdim = 1; // histo['fDimension'];
      if (histo['_typename'].match(/\bJSROOTIO.TH2/)) hdim = 2;
      if (histo['_typename'].match(/\bJSROOTIO.TH3/)) hdim = 3;
      var nch = opt.length;
      var option = { 'Axis': 0, 'Bar': 0, 'Curve': 0, 'Error': 0, 'Hist': 0,
         'Line': 0, 'Mark': 0, 'Fill': 0, 'Same': 0, 'Func': 0, 'Scat': 0,
         'Star': 0, 'Arrow': 0, 'Box': 0, 'Text': 0, 'Char': 0, 'Color': 0,
         'Contour': 0, 'Logx': 0, 'Logy': 0, 'Logz': 0, 'Lego': 0, 'Surf': 0,
         'Off': 0, 'Tri': 0, 'Proj': 0, 'AxisPos': 0, 'Spec': 0, 'Pie': 0,
         'List': 0, 'Zscale': 0, 'FrontBox': 1, 'BackBox': 1, 'System': kCARTESIAN,
         'HighRes': 0, 'Zero': 0
      };
      //check for graphical cuts
      var chopt = opt.toUpperCase();
      chopt = JSROOTPainter.clearCuts(chopt);
      if (hdim > 1) option.Scat = 1;
      if (!nch) option.Hist = 1;
      if ('fFunctions' in histo && histo['fFunctions'].length > 0) option.Func = 2;
      if ('fSumw2' in histo && histo['fSumw2'].length > 0 && hdim == 1) option.Error = 2;
      var l = chopt.indexOf('SPEC');
      if (l != -1) {
         option.Scat = 0;
         chopt = chopt.replace('SPEC', '    ');
         var bs = 0;
         l = chopt.indexOf('BF(');
         if (l != -1) {
            bs = parseInt(chopt)
         }
         option.Spec = Math.max(1600, bs);
         return option;
      }
      l = chopt.indexOf('GL');
      if (l != -1) {
         chopt = chopt.replace('GL', '  ');
      }
      l = chopt.indexOf('X+');
      if (l != -1) {
         option.AxisPos = 10;
         chopt = chopt.replace('X+', '  ');
      }
      l = chopt.indexOf('Y+');
      if (l != -1) {
         option.AxisPos += 1;
         chopt = chopt.replace('Y+', '  ');
      }
      if ((option.AxisPos == 10 || option.AxisPos == 1) && (nch == 2)) option.Hist = 1;
      if (option.AxisPos == 11 && nch == 4) option.Hist = 1;
      l = chopt.indexOf('SAMES');
      if (l != -1) {
         if (nch == 5) option.Hist = 1;
         option.Same = 2;
         chopt = chopt.replace('SAMES', '     ');
      }
      l = chopt.indexOf('SAME');
      if (l != -1) {
         if (nch == 4) option.Hist = 1;
         option.Same = 1;
         chopt = chopt.replace('SAME', '    ');
      }
      l = chopt.indexOf('PIE');
      if (l != -1) {
         option.Pie = 1;
         chopt = chopt.replace('PIE', '   ');
      }
      l = chopt.indexOf('LEGO');
      if (l != -1) {
         option.Scat = 0;
         option.Lego = 1;
         chopt = chopt.replace('LEGO', '    ');
         if (chopt[l+4] == '1') { option.Lego = 11; chopt[l+4] = ' '; }
         if (chopt[l+4] == '2') { option.Lego = 12; chopt[l+4] = ' '; }
         if (chopt[l+4] == '3') { option.Lego = 13; chopt[l+4] = ' '; }
         l = chopt.indexOf('FB'); if (l != -1) { option.FrontBox = 0; chopt = chopt.replace('FB', '  '); }
         l = chopt.indexOf('BB'); if (l != -1) { option.BackBox = 0;  chopt = chopt.replace('BB', '  '); }
         l = chopt.indexOf('0'); if (l != -1) { option.Zero = 1;  chopt = chopt.replace('0', ' '); }
      }
      l = chopt.indexOf('SURF');
      if (l != -1) {
         option.Scat = 0;
         option.Surf = 1; chopt = chopt.replace('SURF', '    ');
         if (chopt[l+4] == '1') { option.Surf = 11; chopt[l+4] = ' '; }
         if (chopt[l+4] == '2') { option.Surf = 12; chopt[l+4] = ' '; }
         if (chopt[l+4] == '3') { option.Surf = 13; chopt[l+4] = ' '; }
         if (chopt[l+4] == '4') { option.Surf = 14; chopt[l+4] = ' '; }
         if (chopt[l+4] == '5') { option.Surf = 15; chopt[l+4] = ' '; }
         if (chopt[l+4] == '6') { option.Surf = 16; chopt[l+4] = ' '; }
         if (chopt[l+4] == '7') { option.Surf = 17; chopt[l+4] = ' '; }
         l = chopt.indexOf('FB'); if (l != -1) { option.FrontBox = 0; chopt = chopt.replace('FB', '  '); }
         l = chopt.indexOf('BB'); if (l != -1) { option.BackBox = 0; chopt = chopt.replace('BB', '  '); }
      }
      l = chopt.indexOf('TF3');
      if (l != -1) {
         l = chopt.indexOf('FB'); if (l != -1) { option.FrontBox = 0; chopt = chopt.replace('FB', '  '); }
         l = chopt.indexOf('BB'); if (l != -1) { option.BackBox = 0; chopt = chopt.replace('BB', '  '); }
      }
      l = chopt.indexOf('ISO');
      if (l != -1) {
         l = chopt.indexOf('FB'); if (l != -1) { option.FrontBox = 0; chopt = chopt.replace('FB', '  '); }
         l = chopt.indexOf('BB'); if (l != -1) { option.BackBox = 0; chopt = chopt.replace('BB', '  '); }
      }
      l = chopt.indexOf('LIST'); if (l != -1) { option.List = 1; chopt = chopt.replace('LIST', '  '); }
      l = chopt.indexOf('CONT');
      if (l != -1) {
         chopt = chopt.replace('CONT', '    ');
         if (hdim > 1) {
            option.Scat = 0;
            option.Contour = 1;
            if (chopt[l+4] == '1') { option.Contour = 11; chopt[l+4] = ' '; }
            if (chopt[l+4] == '2') { option.Contour = 12; chopt[l+4] = ' '; }
            if (chopt[l+4] == '3') { option.Contour = 13; chopt[l+4] = ' '; }
            if (chopt[l+4] == '4') { option.Contour = 14; chopt[l+4] = ' '; }
            if (chopt[l+4] == '5') { option.Contour = 15; chopt[l+4] = ' '; }
         } else {
            option.Hist = 1;
         }
      }
      l = chopt.indexOf('HBAR');
      if (l != -1) {
         option.Hist = 0;
         option.Bar = 20; chopt = chopt.replace('HBAR', '    ');
         if (chopt[l+4] == '1') { option.Bar = 21; chopt[l+4] = ' '; }
         if (chopt[l+4] == '2') { option.Bar = 22; chopt[l+4] = ' '; }
         if (chopt[l+4] == '3') { option.Bar = 23; chopt[l+4] = ' '; }
         if (chopt[l+4] == '4') { option.Bar = 24; chopt[l+4] = ' '; }
      }
      l = chopt.indexOf('BAR');
      if (l != -1) {
         option.Hist = 0;
         option.Bar = 10; chopt = chopt.replace('BAR', '   ');
         if (chopt[l+3] == '1') { option.Bar = 11; chopt[l+3] = ' '; }
         if (chopt[l+3] == '2') { option.Bar = 12; chopt[l+3] = ' '; }
         if (chopt[l+3] == '3') { option.Bar = 13; chopt[l+3] = ' '; }
         if (chopt[l+3] == '4') { option.Bar = 14; chopt[l+3] = ' '; }
      }
      l = chopt.indexOf('ARR' );
      if (l != -1) {
         chopt = chopt.replace('ARR', '   ');
         if (hdim > 1) {
            option.Arrow  = 1;
            option.Scat = 0;
         } else {
            option.Hist = 1;
         }
      }
      l = chopt.indexOf('BOX' );
      if (l != -1) {
         chopt = chopt.replace('BOX', '   ');
         if (hdim>1) {
            Hoption.Scat = 0;
            Hoption.Box  = 1;
            if (chopt[l+3] == '1') { option.Box = 11; chopt[l+3] = ' '; }
         } else {
            option.Hist = 1;
         }
      }
      l = chopt.indexOf('COLZ');
      if (l != -1) {
         chopt = chopt.replace('COLZ', '');
         if (hdim>1) {
            option.Color  = 2;
            option.Scat   = 0;
            option.Zscale = 1;
         } else {
            option.Hist = 1;
         }
      }
      l = chopt.indexOf('COL' );
      if (l != -1) {
         chopt = chopt.replace('COL', '   ');
         if (hdim>1) {
            option.Color = 1;
            option.Scat  = 0;
         } else {
            option.Hist = 1;
         }
      }
      l = chopt.indexOf('CHAR'); if (l != -1) { option.Char = 1; chopt = chopt.replace('CHAR', '    '); option.Scat = 0; }
      l = chopt.indexOf('FUNC'); if (l != -1) { option.Func = 2; chopt = chopt.replace('FUNC', '    '); option.Hist = 0; }
      l = chopt.indexOf('HIST'); if (l != -1) { option.Hist = 2; chopt = chopt.replace('HIST', '    '); option.Func = 0; option.Error = 0; }
      l = chopt.indexOf('AXIS'); if (l != -1) { option.Axis = 1; chopt = chopt.replace('AXIS', '    '); }
      l = chopt.indexOf('AXIG'); if (l != -1) { option.Axis = 2; chopt = chopt.replace('AXIG', '    '); }
      l = chopt.indexOf('SCAT'); if (l != -1) { option.Scat = 1; chopt = chopt.replace('SCAT', '    '); }
      l = chopt.indexOf('TEXT');
      if (l != -1) {
         var angle = parseInt(chopt);
         if (angle != NaN) {
            if (angle < 0)  angle = 0;
            if (angle > 90) angle = 90;
            option.Text = 1000 + angle;
         }
         else {
            option.Text = 1;
         }
         chopt = chopt.replace('TEXT', '    ');
         l = chopt.indexOf('N');
         if (l != -1 && histo['_typename'].match(/\bJSROOTIO.TH2Poly/)) option.Text += 3000;
         option.Scat = 0;
      }
      l = chopt.indexOf('POL');  if (l != -1) { option.System = kPOLAR;       chopt = chopt.replace('POL', '   '); }
      l = chopt.indexOf('CYL');  if (l != -1) { option.System = kCYLINDRICAL; chopt = chopt.replace('CYL', '   '); }
      l = chopt.indexOf('SPH');  if (l != -1) { option.System = kSPHERICAL;   chopt = chopt.replace('SPH', '   '); }
      l = chopt.indexOf('PSR');  if (l != -1) { option.System = kRAPIDITY;    chopt = chopt.replace('PSR', '   '); }
      l = chopt.indexOf('TRI');
      if (l != -1) {
         option.Scat = 0;
         option.Color  = 0;
         option.Tri = 1; chopt = chopt.replace('TRI', '   ');
         l = chopt.indexOf('FB');   if (l != -1) { option.FrontBox = 0; chopt = chopt.replace('FB', '  '); }
         l = chopt.indexOf('BB');   if (l != -1) { option.BackBox = 0;  chopt = chopt.replace('BB', '  '); }
         l = chopt.indexOf('ERR');  if (l != -1) chopt = chopt.replace('ERR', '   ');
      }
      l = chopt.indexOf('AITOFF');
      if (l != -1) {
         Hoption.Proj = 1; chopt = chopt.replace('AITOFF', '      ');  // Aitoff projection
      }
      l = chopt.indexOf('MERCATOR');
      if (l != -1) {
         option.Proj = 2; chopt = chopt.replace('MERCATOR', '        ');  // Mercator projection
      }
      l = chopt.indexOf('SINUSOIDAL');
      if (l != -1) {
         option.Proj = 3; chopt = chopt.replace('SINUSOIDAL', '         ');  // Sinusoidal projection
      }
      l = chopt.indexOf('PARABOLIC');
      if (l != -1) {
         option.Proj = 4; chopt = chopt.replace('PARABOLIC', '         ');  // Parabolic projection
      }
      if (option.Proj > 0) {
         option.Scat = 0;
         option.Contour = 14;
      }
      if (chopt.indexOf('A') != -1)    option.Axis = -1;
      if (chopt.indexOf('B') != -1)    option.Bar  = 1;
      if (chopt.indexOf('C') != -1)  { option.Curve =1; option.Hist = -1; }
      if (chopt.indexOf('F') != -1)    option.Fill =1;
      if (chopt.indexOf('][') != -1) { option.Off  =1; option.Hist =1; }
      if (chopt.indexOf('F2') != -1)   option.Fill =2;
      if (chopt.indexOf('L') != -1)  { option.Line =1; option.Hist = -1; }
      if (chopt.indexOf('P') != -1)  { option.Mark =1; option.Hist = -1; }
      if (chopt.indexOf('Z') != -1)    option.Zscale =1;
      if (chopt.indexOf('*') != -1)    option.Star =1;
      if (chopt.indexOf('H') != -1)    option.Hist =2;
      if (chopt.indexOf('P0') != -1)   option.Mark =10;
      if (histo['_typename'].match(/\bJSROOTIO.TH2Poly/)) {
         if (option.Fill + option.Line + option.Mark != 0 ) option.Scat = 0;
      }
      if (chopt.indexOf('E') != -1) {
         if (hdim == 1) {
            option.Error = 1;
            if (chopt.indexOf('E0') != -1)  option.Error = 10;
            if (chopt.indexOf('E1') != -1)  option.Error = 11;
            if (chopt.indexOf('E2') != -1)  option.Error = 12;
            if (chopt.indexOf('E3') != -1)  option.Error = 13;
            if (chopt.indexOf('E4') != -1)  option.Error = 14;
            if (chopt.indexOf('E5') != -1)  option.Error = 15;
            if (chopt.indexOf('E6') != -1)  option.Error = 16;
            if (chopt.indexOf('X0') != -1) {
               if (option.Error == 1)  option.Error += 20;
               option.Error += 10;
            }
            if (option.Text && histo['_typename'].match(/\bJSROOTIO.TProfile/)) {
               option.Text += 2000;
               option.Error = 0;
            }
         } else {
            if (option.Error == 0) {
               option.Error = 100;
               option.Scat  = 0;
            }
            if (option.Text) {
               option.Text += 2000;
               option.Error = 0;
            }
         }
      }
      if (chopt.indexOf('9') != -1)  option.HighRes = 1;
      if (option.Surf == 15) {
         if (option.System == kPOLAR || option.System == kCARTESIAN) {
            option.Surf = 13;
            //Warning('MakeChopt','option SURF5 is not supported in Cartesian and Polar modes');
         }
      }
      if (pad && typeof(pad) != 'undefined') {
         // Copy options from current pad
         option.Logx = pad['fLogx'];
         option.Logy = pad['fLogy'];
         option.Logz = pad['fLogz'];
      } else {
         option.Logx = false;
         option.Logy = false;
         option.Logz = false;
      }
      //  Check options incompatibilities
      if (option.Bar == 1) option.Hist = -1;
      return option;
   };

   JSROOTPainter.getRootColor = function(color) {
      return root_colors[color];
   };

   JSROOTPainter.padtoX = function(pad, x) {
      // Convert x from pad to X.
      if (pad['fLogx'] && x < 50) return Math.exp(2.302585092994 * x);
      return x;
   };

   JSROOTPainter.padtoY = function(pad, y) {
      // Convert y from pad to Y.
      if (pad['fLogy'] && y < 50) return Math.exp(2.302585092994 * y);
      return y;
   };

   JSROOTPainter.xtoPad = function(x, pad) {
      if (pad['fLogx']) {
         if (x > 0)
            x = JSROOTMath.log10(x);
         else
            x = pad['fUxmin'];
      }
      return x;
   };

   JSROOTPainter.ytoPad = function(y, pad) {
      if (pad['fLogy']) {
         if (y > 0)
            y = JSROOTMath.log10(y);
         else
            y = pad['fUymin'];
      }
      return y;
   };

   /**
    * Converts an HSL color value to RGB. Conversion formula
    * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
    * Assumes h, s, and l are contained in the set [0, 1] and
    * returns r, g, and b in the set [0, 255].
    *
    * @param   Number  h       The hue
    * @param   Number  s       The saturation
    * @param   Number  l       The lightness
    * @return  Array           The RGB representation
    */
   JSROOTPainter.HLStoRGB = function(h, l, s) {
      var r, g, b;
      if (s < 1e-300) {
         r = g = b = l; // achromatic
      } else {
         function hue2rgb(p, q, t){
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t < 1/6) return p + (q - p) * 6 * t;
            if (t < 1/2) return q;
            if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
            return p;
         }
         var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
         var p = 2 * l - q;
         r = hue2rgb(p, q, h + 1/3);
         g = hue2rgb(p, q, h);
         b = hue2rgb(p, q, h - 1/3);
      }
      return 'rgb('+Math.round(r * 255)+', '+Math.round(g * 255)+', '+Math.round(b * 255)+')';
   };

   JSROOTPainter.getMinMax = function(hist, what) {
      if (what == 'max' && hist['fMaximum'] != -1111) return hist['fMaximum'];
      if (what == 'min' && hist['fMinimum'] != -1111) return hist['fMinimum'];
      var bin, binx, biny, binz;
      var xfirst  = 1;;
      var xlast   = hist['fXaxis']['fNbins'];
      var yfirst  = 1;
      var ylast   = hist['fYaxis']['fNbins'];
      var zfirst  = 1;
      var zlast   = hist['fZaxis']['fNbins'];
      var maximum = Number.NEGATIVE_INFINITY;
      var minimum = Number.POSITIVE_INFINITY;
      var tmp_value;
      for (binz=zfirst;binz<=zlast;binz++) {
         for (biny=yfirst;biny<=ylast;biny++) {
            for (binx=xfirst;binx<=xlast;binx++) {
               //bin = hist.getBin(binx,biny,binz);
               //tmp_value = hist.getBinContent(bin);
               tmp_value = hist.getBinContent(binx, biny);
               if (tmp_value > maximum) maximum = tmp_value;
               if (tmp_value < minimum) minimum = tmp_value;
            }
         }
      }
      hist['fMaximum'] = maximum;
      hist['fMinimum'] = minimum;
      if (what == 'max') return maximum;
      if (what == 'min') return minimum;
   }

   JSROOTPainter.getTimeOffset = function(axis) {

      var timeFormat = axis['fTimeFormat'];
      var i, timeoffset = 0;
      var idF = timeFormat.indexOf('%F');

      if (idF >= 0) {
         timeformat = timeFormat.substr(0, idF);
      } else {
         timeformat = timeFormat;
      }

      if (idF >= 0) {
         var lnF = timeFormat.length;
         var stringtimeoffset = timeFormat.substr(idF+2, lnF);
         for (i=0;i<3;++i) stringtimeoffset = stringtimeoffset.replace('-', '/');
         var stimeoffset = new Date(stringtimeoffset);
         timeoffset = stimeoffset.getTime();
         var ids = stringtimeoffset.indexOf('s');
         if (ids >= 0) {
            var lns = stringtimeoffset.length;
            var sdp = stringtimeoffset.substr(ids+1, lns);
            var dp = parseFloat(sdp);
            timeoffset += dp;
         }
      } else {
         timeoffset = 788918400000; // UTC time at 01/01/95
      }
      return timeoffset;
   };

   JSROOTPainter.formatExp = function(label) {
      var str = label;
      if (parseFloat(str) == 1.0) return '1';
      if (parseFloat(str) == 10.0) return '10';
      var str = str.replace('e+', 'x10@');
      var str = str.replace('e-', 'x10@-');
      var _val = str.substring(0, str.indexOf('@'));
      var _exp = str.substr(str.indexOf('@'));
      _val = _val.replace('@', '');
      _exp = _exp.replace('@', '');
      var u, size = _exp.length;
      for (j=0;j<size;++j) {
         var c = _exp.charAt(j);
         if (c == '+') u = '\u207A';
         else if (c == '-') u = '\u207B'
         else {
            var e = parseInt(c);
            if (e == 1) u = String.fromCharCode(0xB9);
            else if (e > 1 && e < 4) u = String.fromCharCode(0xB0+e);
            else u = String.fromCharCode(0x2070+e);
         }
         _exp = _exp.replace(c, u);
      }
      _val = _val.replace('1x', '');
      return _val+_exp;
   };

   JSROOTPainter.translateExp = function(str) {
      var i, j, lstr = str.match(/\^{[0-9]*}/gi);
      if (lstr != null) {
         var symbol = '';
         for (i=0;i<lstr.length;++i) {
            symbol = lstr[i].replace(' ', '');
            symbol = symbol.replace('^{', ''); // &sup
            symbol = symbol.replace('}', ''); // ;
            var size = symbol.length;
            for (j=0;j<size;++j) {
               var c = symbol.charAt(j);
               var e = parseInt(c);
               if (e == 1) u = String.fromCharCode(0xB9);
               else if (e > 1 && e < 4) u = String.fromCharCode(0xB0+e);
               else u = String.fromCharCode(0x2070+e);
               symbol = symbol.replace(c, u);
            }
            str = str.replace(lstr[i], symbol);
         }
      }
      return str;
   };

   JSROOTPainter.translateLaTeX = function(string) {
      var str = string;
      str = this.translateExp(str);
      while (str.indexOf('^{o}') != -1)
         str = str.replace('^{o}', '\xBA');
      var lstr = str.match(/\#sqrt{(.*?)}/gi);
      if (lstr != null) {
         var symbol;
         for (i=0;i<lstr.length;++i) {
            symbol = lstr[i].replace(' ', '');
            symbol = symbol.replace('#sqrt{', '#sqrt');
            symbol = symbol.replace('}', '');
            str = str.replace(lstr[i], symbol);
         }
      }
      var lstr = str.match(/\_{(.*?)}/gi);
      if (lstr != null) {
         var symbol;
         for (i=0;i<lstr.length;++i) {
            symbol = lstr[i].replace(' ', '');
            symbol = symbol.replace('_{', ''); // &sub
            symbol = symbol.replace('}', ''); // ;
            str = str.replace(lstr[i], symbol);
         }
      }
      var lstr = str.match(/\^{(.*?)}/gi);
      if (lstr != null) {
         var symbol;
         for (i=0;i<lstr.length;++i) {
            symbol = lstr[i].replace(' ', '');
            symbol = symbol.replace('^{', ''); // &sup
            symbol = symbol.replace('}', ''); // ;
            str = str.replace(lstr[i], symbol);
         }
      }
      while (str.indexOf('#/') != -1)
         str = str.replace('#/', symbols_map['#/']);
      for (x in symbols_map) {
         while (str.indexOf(x) != -1)
            str = str.replace(x, symbols_map[x]);
      }
      return str;
   };

   
   
   
   // try to write new drawHistogram1D, where recreation of all graphical objects
   // can be done in redraw methdod by switching one argument in the histogram

   
   JSROOTPainter.histoDialog = function(item) {
      // $("#report").append("<br> Actiavte object in dialog " + obj['fName']);
      
      var painter = $("#dialog").data("Painter");

      $("#dialog").dialog("close");
      $("#dialog").empty();

      painter.ExeContextMenu(item);
   }
   
   JSROOTPainter.ObjectPainter = function()
   {
      this.vis   = null;  // canvas where object is drawn
      this.first = null;  // pointer on first painter
   }
   
   JSROOTPainter.ObjectPainter.prototype.IsObject = function(obj) {
      return false;
   }
   
   JSROOTPainter.ObjectPainter.prototype.SetVis = function(vis) {
      if (!('painters' in vis)) {
         vis['painters'] = new Array();
         doubleTap(vis[0][0]); // ????
         
         // only first object in list can have zoom selection
         // create dummy here and not need to check if it exists or not
         this['zoom_xmin'] = 0;
         this['zoom_xmax'] = 0;
         this['zoom_ymin'] = 0;
         this['zoom_ymax'] = 0;
         
      } else {
         this.first = vis['painters'][0];
      }

      vis['painters'].push(this);
      this.vis = vis;    // remember main pad 
   }
   
   JSROOTPainter.ObjectPainter.prototype.RedrawFrame = function(selobj) {
      // call Redraw methods for each painter in the frame
      // if selobj specifief, painter with selected object will be redrawn
      
      if (!this.vis) return;
      
      for (var n=0;n<this.vis['painters'].length;n++) {
         
         painter = this.vis['painters'][n];
         
         if ((selobj!=null) && !painter.IsObject(selobj)) continue;
            
         painter.Redraw();
      }
   }
   
   JSROOTPainter.ObjectPainter.prototype.CollectTooltips = function(tip) {
      if (!this.vis) return false;
      
      tip['empty'] = true;
      
      for (var n=0;n<this.vis['painters'].length;n++)
         this.vis['painters'][n].ProvideTooltip(tip);
      
      return !tip.empty;
   }

   
   JSROOTPainter.ObjectPainter.prototype.AddMenuItem = function(menu, text, cmd)
   {
      menu.append("<a href='javascript: JSROOTPainter.histoDialog(\"" + cmd + "\")'>" + text + "</a><br>");
   }

   JSROOTPainter.ObjectPainter.prototype.GetStat = function(name) {
      // basic method, should be reimplemented in derived objects
      // could be used to recieve any statistic information
      return 0;
   }
   
   JSROOTPainter.ObjectPainter.prototype.Redraw = function() {
      // basic method, should be reimplemented in all derived objects
      // for the case when drawing should be repeated, probably with different options 
   }

   JSROOTPainter.ObjectPainter.prototype.ProvideTooltip = function(tip)
   {
      // basic method, painter can provide tooltip at specified coordinates
      // range.x1 .. range.x2, range.y1 .. range.y2
   }

   
   JSROOTPainter.ObjectPainter.prototype.FillContextMenu = function(menu)
   {
      this.AddMenuItem(menu,"Unzoom X","unx");
      this.AddMenuItem(menu,"Unzoom Y","uny");
      this.AddMenuItem(menu,"Unzoom","unxy");
   }
      
   JSROOTPainter.ObjectPainter.prototype.ExeContextMenu = function(cmd) 
   {
      if (cmd=="unx") this.Unzoom(true, false); else
      if (cmd=="uny") this.Unzoom(false, true); else
      if (cmd=="unxy") this.Unzoom(true, true);
   }
   
   JSROOTPainter.ObjectPainter.prototype.Unzoom = function(dox,doy) {
      var obj = this.first;
      if (!obj) obj = this;

      if (dox) {
         obj['zoom_xmin'] = 0;
         obj['zoom_xmax'] = 0;
      }
      if (doy) {
         obj['zoom_ymin'] = 0;
         obj['zoom_ymax'] = 0;
      }
      
      this.RedrawFrame();
   }

   JSROOTPainter.ObjectPainter.prototype.Zoom = function(xmin, xmax, ymin, ymax) {
      var obj = this.first;
      if (!obj) obj = this;
      obj['zoom_xmin'] = xmin;
      obj['zoom_xmax'] = xmax;
      obj['zoom_ymin'] = ymin;
      obj['zoom_ymax'] = ymax;
      this.RedrawFrame();
   }
   
   JSROOTPainter.ObjectPainter.prototype.UpdateObject = function(obj) {
      alert("JSROOTPainter.ObjectPainter.UpdateObject not implemented");  
   }
      
   
   /**
    * Now the real drawing functions (using d3.js)
    */

   JSROOTPainter.add3DInteraction = function(renderer, scene, camera, toplevel) {
      // add 3D mouse interactive functions
      var mouseX, mouseY, mouseDowned = false;
      var mouse = { x: 0, y: 0 }, INTERSECTED;

      var radius = 100;
      var theta = 0;
      var projector = new THREE.Projector();
      function findIntersection() {
         // find intersections
         if ( mouseDowned ) {
            if ( INTERSECTED ) {
               INTERSECTED.material.emissive.setHex( INTERSECTED.currentHex );
               renderer.render( scene, camera );
            }
            INTERSECTED = null;
            if (gStyle.Tooltip > 1) tooltip.hide();
            return;
         }
         var vector = new THREE.Vector3( mouse.x, mouse.y, 1 );
         projector.unprojectVector( vector, camera );
         var raycaster = new THREE.Raycaster( camera.position, vector.sub( camera.position ).normalize() );
         var intersects = raycaster.intersectObjects( scene.children, true );
         if ( intersects.length > 0 ) {
            var pick = null;
            for (var i=0;i<intersects.length;++i) {
               if ('emissive' in intersects[ i ].object.material) {
                  pick = intersects[ i ];
                  break;
               }
            }
            if (pick && INTERSECTED != pick.object ) {
               if ( INTERSECTED ) INTERSECTED.material.emissive.setHex( INTERSECTED.currentHex );
               INTERSECTED = pick.object;
               INTERSECTED.currentHex = INTERSECTED.material.emissive.getHex();
               INTERSECTED.material.emissive.setHex( 0x5f5f5f );
               renderer.render( scene, camera );
               if (gStyle.Tooltip > 1) tooltip.show(INTERSECTED.name.length > 0 ? INTERSECTED.name : INTERSECTED.parent.name, 200);
            }
         } else {
            if ( INTERSECTED ) {
               INTERSECTED.material.emissive.setHex( INTERSECTED.currentHex );
               renderer.render( scene, camera );
            }
            INTERSECTED = null;
            if (gStyle.Tooltip > 1) tooltip.hide();
         }
      };

      $( renderer.domElement ).on('touchstart mousedown',function (e) {
         //var touch = e.changedTouches[0] || {};
         if (gStyle.Tooltip > 1) tooltip.hide();
         e.preventDefault();
         var touch = e;
         if ('changedTouches' in e) touch = e.changedTouches[0];
         else if ('touches' in e) touch = e.touches[0];
         else if ('originalEvent' in e) {
            if ('changedTouches' in e.originalEvent) touch = e.originalEvent.changedTouches[0];
            else if ('touches' in e.originalEvent) touch = e.originalEvent.touches[0];
         }
         mouseX = touch.pageX;
         mouseY = touch.pageY;
         mouseDowned = true;
      });
      $( renderer.domElement ).on('touchmove mousemove', function(e) {
         if ( mouseDowned ) {
            var touch = e;
            if ('changedTouches' in e) touch = e.changedTouches[0];
            else if ('touches' in e) touch = e.touches[0];
            else if ('originalEvent' in e) {
               if ('changedTouches' in e.originalEvent) touch = e.originalEvent.changedTouches[0];
               else if ('touches' in e.originalEvent) touch = e.originalEvent.touches[0];
            }
            var moveX = touch.pageX - mouseX;
            var moveY = touch.pageY - mouseY;
            // limited X rotate in -45 to 135 deg
            if ( (moveY > 0 && toplevel.rotation.x < Math.PI*3/4) ||
                 (moveY < 0 &&  toplevel.rotation.x > -Math.PI/4) ) {
               toplevel.rotation.x += moveY*0.02;
            }
            toplevel.rotation.y += moveX*0.02;
            renderer.render( scene, camera );
            mouseX = touch.pageX;
            mouseY = touch.pageY;
         }
         else {
            e.preventDefault();
            var mouse_x = 'offsetX' in e.originalEvent ? e.originalEvent.offsetX : e.originalEvent.layerX;
            var mouse_y = 'offsetY' in e.originalEvent ? e.originalEvent.offsetY : e.originalEvent.layerY;
            mouse.x = ( mouse_x / renderer.domElement.width ) * 2 - 1;
            mouse.y = - ( mouse_y / renderer.domElement.height ) * 2 + 1;
            // enable picking once tootips are available...
            findIntersection();
         }
      });
      $( renderer.domElement ).on('touchend mouseup', function(e) {
         mouseDowned = false;
      });
      
      $( renderer.domElement ).on('mousewheel', function(e, d) {
         e.preventDefault();
         camera.position.z += d * 20;
         renderer.render( scene, camera );
      });
   };

   JSROOTPainter.createCanvas = function(element, idx) {
      var w = element.width(), h = w * 0.6666666;
      var render_to = '#histogram' + idx;
      d3.select(render_to).style("background-color", 'white');
      d3.select(render_to).style("width", "100%");

      var svg = d3.select(render_to)
                  .append("svg")
                  .attr("class","root_canvas")
                  .attr("width", w)
                  .attr("height", h)
                  .style("background-color", 'white');
      defs = svg.append('svg:defs');
      return svg;
   };

   JSROOTPainter.createFrame = function(vis, pad, histo, frame) {
      var w = vis.attr("width"), h = vis.attr("height");
      var width = w, height = h;
      var lm = w*0.12, rm = w*0.05, tm = h*0.12, bm = h*0.12;
      if (histo && histo['fOption'] && histo['fOption'].toLowerCase() == 'colz')
         rm = w * 0.13;
      var framecolor = 'white', bordermode = 0,
          bordersize = 0, linecolor = 'black',//root_colors[0],
          linestyle = 0, linewidth = 1;
      if (histo && histo['_typename'] == 'JSROOTIO.TF1') {
         linecolor = 'black';
         linewidth = 1;
      }
      if (frame) {
         bordermode = frame['fBorderMode'];
         bordersize = frame['fBorderSize'];
         linecolor = root_colors[frame['fLineColor']];
         linestyle = frame['fLineStyle'];
         linewidth = frame['fLineWidth'];
         if (pad) {
            var xspan = width / Math.abs(pad['fX2'] - pad['fX1']);
            var yspan = height / Math.abs(pad['fY2'] - pad['fY1']);
            px1 = (frame['fX1'] - pad['fX1']) * xspan;
            py1 = (frame['fY1'] - pad['fY1']) * yspan;
            px2 = (frame['fX2'] - pad['fX1']) * xspan;
            py2 = (frame['fY2'] - pad['fY1']) * yspan;
            if (px1 < px2) {pxl = px1; pxt = px2;}
            else           {pxl = px2; pxt = px1;}
            if (py1 < py2) {pyl = py1; pyt = py2;}
            else           {pyl = py2; pyt = py1;}
            lm = pxl;
            bm = pyl;
            w = pxt - pxl;
            h = pyt - pyl;
            tm = height - pyt;
            rm = width - pxt;
         }
         else {
            lm = frame['fX1'] * width;
            tm = frame['fY1'] * height;
            bm = (1.0 - frame['fY2']) * height;
            rm = (1.0 - frame['fX2']) * width;
            w -= (lm + rm);
            h -= (tm + bm);
         }
         framecolor = root_colors[frame['fFillColor']];
         if (frame['fFillStyle'] > 4000 && frame['fFillStyle'] < 4100)
            framecolor = 'none';
      }
      else {
         if (pad) {
            framecolor = root_colors[pad['fFrameFillColor']];
            if (pad['fFrameFillStyle'] > 4000 && pad['fFrameFillStyle'] < 4100)
               framecolor = 'none';
         }
         w -= (lm + rm);
         h -= (tm + bm);
      }
      if (typeof(framecolor) == 'undefined')
         framecolor = 'white';

      // Sergey: no idea why we need three levels of objects, but lets do it  
      
      var hframe = vis.append("svg:g")
            .attr("x", lm)
            .attr("y", tm)
            .attr("width", w)
            .attr("height", h)
            .attr("transform", "translate(" + lm + "," + tm + ")");

      hframe.append("svg:rect")
            .attr("x", 0)
            .attr("y", 0)
            .attr("width", w)
            .attr("height", h)
            .attr("fill", framecolor)
            .style("stroke", linecolor)
            .style("stroke-width", linewidth);

      // TODO: find a way to localize svg_frame without id 
      
      var svg_frame = hframe.append("svg")
            .attr("id", "svg_frame_" + (++frame_id))
            .attr("x", 0)
            .attr("y", 0)
            .attr("width", w)
            .attr("height", h)
            .attr("viewBox", "0 0 "+w+" "+h);
      
      // keep svg frame inside 
      hframe['root_svg_frame'] = d3.select("#svg_frame_" + frame_id);

      return hframe;
   }

   // =========================================================================

   JSROOTPainter.Func1DPainter = function(tf1) {
      JSROOTPainter.ObjectPainter.call(this);
      this.tf1 = tf1;
   }

   JSROOTPainter.Func1DPainter.prototype = Object.create( JSROOTPainter.ObjectPainter.prototype );

   JSROOTPainter.Func1DPainter.prototype.IsObject = function(obj) {
      return this.tf1 == obj;
   }
   
   JSROOTPainter.Func1DPainter.prototype.SetFrame = function(vis, pad, frame) {
      this.SetVis(vis);
      this.pad = pad;
      this.frame = frame;
      if (!this.frame) {
         alert("frame for the graph was not specified");
         return;
      }
      
      if (!this.first) {
         alert("first painter is not defined - it is a must for TF1");
         return;
      }
   }
   
   JSROOTPainter.Func1DPainter.prototype.Redraw = function() 
   {
      this.DrawBins();
   }
   
   JSROOTPainter.Func1DPainter.prototype.FillContextMenu = function(menu)
   {
      JSROOTPainter.ObjectPainter.prototype.FillContextMenu.call(this, menu);
   }
      
   JSROOTPainter.Func1DPainter.prototype.ExeContextMenu = function(cmd) 
   {
      JSROOTPainter.ObjectPainter.prototype.ExeContextMenu.call(this, cmd);
   }
   
   JSROOTPainter.Func1DPainter.prototype.Eval = function(x)
   {
      //return (x-5)*(x-5);
      return this.tf1.evalPar(x);
   }
   
   JSROOTPainter.Func1DPainter.prototype.CreateDummyHisto = function()
   {
      var xmin = 0, xmax = 0, ymin = 0, ymax = 0;       
      if (this.tf1['fNsave'] > 0) {
         // in the case where the points have been saved, useful for example
         // if we don't have the user's function
         var nb_points = this.tf1['fNpx'];
         for (var i=0;i<nb_points;++i) {
            var h = this.tf1['fSave'][i];
            if ((i==0) || (h > ymax)) ymax = h;
            if ((i==0) || (h < ymin)) ymin = h;
         }
         xmin = this.tf1['fSave'][nb_points+1];
         xmax = this.tf1['fSave'][nb_points+2];
      } else {
         // we don't have the points, so let's try to interpret the function
         // use fNpfits instead of fNpx if possible (to use more points)
         if (this.tf1['fNpfits'] <= 103) this.tf1['fNpfits'] = 103;
         xmin = this.tf1['fXmin'];
         xmax = this.tf1['fXmax'];   
         
         var nb_points = Math.max(this.tf1['fNpx'], this.tf1['fNpfits']);
         
         var binwidth = ((xmax - xmin) / (nb_points-1));
         var left = -1, right = -1;
         for (var i=0;i<nb_points;++i) {
            var h = this.Eval(xmin + (i * binwidth));
            if (isNaN(h)) continue;
            
            if (left<0) { left = i; ymax = h; ymin = h; }
            if ((right<0) || (right == i-1)) right = i;

            if (h > ymax) ymax = h;
            if (h < ymin) ymin = h;
         }
         
         if (left<right) { 
            xmax = xmin + right*binwidth;
            xmin = xmin + left*binwidth;
         }
         
//         $("#report").append("<br> find ymin " + ymin + "  ymax " + ymax);
      }
      
      if (ymax > 0.0) ymax *= 1.05;
      if (ymin < 0.0) ymin *= 1.05;

      var histo = JSROOTPainter.CreateDummyHisto();
      
      histo['fName'] = this.tf1['fName'] + "_hist";
      histo['fTitle'] = this.tf1['fTitle'];
      
      histo['fXaxis']['fXmin'] = xmin;
      histo['fXaxis']['fXmax'] = xmax;
      histo['fYaxis']['fXmin'] = ymin;
      histo['fYaxis']['fXmax'] = ymax;

      return histo;
   }
   

   JSROOTPainter.Func1DPainter.prototype.CreateBins = function() {
      
      var pthis = this;
      if (this.tf1['fNsave'] > 0) {
         // in the case where the points have been saved, useful for example
         // if we don't have the user's function
         var nb_points = this.tf1['fNpx'];
         var xmin = this.tf1['fSave'][nb_points+1];
         var xmax = this.tf1['fSave'][nb_points+2];
         var binwidth = (xmax - xmin) / nb_points;
         
         this['bins'] = d3.range(nb_points).map(function(p) {
            return {
               x: xmin + (p * binwidth),
               y: pthis.tf1['fSave'][p]
            };
         });
         this['interpolate_method'] = 'monotone';
      }
      else {
         var xmin = this.tf1['fXmin'];
         var xmax = this.tf1['fXmax'];   
         var nb_points = Math.max(this.tf1['fNpx'], this.tf1['fNpfits']);
         var binwidth = (xmax - xmin) / nb_points;
         this['bins'] = d3.range(nb_points).map(function(p) {
            var xx = xmin + (p * binwidth);
            var yy = pthis.Eval(xx);
            if (isNaN(yy)) yy = 0;
            return {
               x: xx,
               y: yy 
            };
         });
         this['interpolate_method'] = 'cardinal-open';
      }
   }

   JSROOTPainter.Func1DPainter.prototype.ProvideTooltip = function(tip)
   {
      return;
      
      if (!('bins' in this) || (this.bins==null)) return;
      
      var nbin = -1;
      var min = 1e20;
      
      for (var i=0; i<this.bins.length;i++) {
         var dist = Math.pow(this.bins[i].x - tip.x, 2) + Math.pow(this.bins[i].y - tip.y, 2);
         
         if ((nbin<0) || (dist<min)) {
            nbin = i;
            min = dist;  
         }
      } 
      
      if (nbin < 0) return;
      
      var bin = this.bins[nbin]; 
      
      tip['empty'] = false;
      tip.x = bin.x;
      tip.y = bin.y;
      tip['text'].push("tf1:" + this.tf1['fName']);
      tip['text'].push("bin: "+ nbin); 
   }

   
   JSROOTPainter.Func1DPainter.prototype.DrawBins = function() 
   {
      var w = this.frame.attr("width"), h = this.frame.attr("height");

      if (!('bins_id' in this))
         this['bins_id'] = "random_bins_id" + random_id++;
      
      d3.selectAll("#" + this.bins_id).remove();
         
      this['bins_g'] = this.first.svg_frame.append("svg:g").attr("id",this.bins_id);

      var pthis = this;
      var x = this.first.x;
      var y = this.first.y;
      
      var fillcolor = root_colors[this.tf1['fFillColor']];
      var linecolor = root_colors[this.tf1['fLineColor']];
      if (this.tf1['fFillColor'] == 0) {
         fillcolor = '#4572A7';
      }
      if (this.tf1['fLineColor'] == 0) {
         linecolor = '#4572A7';
      }
      
      var line = d3.svg.line()
         .x(function(d) { return x(d.x);})
         .y(function(d) { return y(d.y);})
         .interpolate(this.interpolate_method);

      this.bins_g.append("svg:path")
         .attr("class", "line")
         .attr("d", line(this.bins))
         .style("stroke", linecolor)
         .style("stroke-width", this.tf1['fLineWidth'])
         .style("stroke-dasharray", root_line_styles[this.tf1['fLineStyle']])
         .style("fill", "none");

      // add tooltips
      if (gStyle.Tooltip > 1)
         this.bins_g.selectAll("line") 
            .data(this.bins) 
            .enter() 
            .append("svg:circle") 
            .attr("cx", function(d) { return x(d.x); }) 
            .attr("cy", function(d) { return y(d.y); }) 
            .attr("r", 4) 
            .attr("opacity", 0) 
            .append("svg:title").text(function(d) {
               return "x = " + d.x.toPrecision(4) + " \ny = " + d.y.toPrecision(4);
            });
   }

   JSROOTPainter.Func1DPainter.prototype.UpdateObject = function(obj) {
      this.tf1 = obj;
      this.CreateBins();
   }

   
   JSROOTPainter.drawFunctionNew = function(vis, pad, tf1, frame) 
   {
      var painter = new JSROOTPainter.Func1DPainter(tf1);
      
      if (!('painters' in vis)) {
         var histo = painter.CreateDummyHisto();
         var hpainter = JSROOTPainter.drawHistogram1Dnew(vis, pad, histo, frame);
         frame = hpainter.frame;
      } else {
         if (frame==null) frame = vis['painters'][0].frame;
      }
      
      painter.SetFrame(vis, pad, frame);
      
      painter.CreateBins();
      
      painter.DrawBins();
      
      return painter;
   }

   
   // =======================================================================
   
   JSROOTPainter.GraphPainter = function(graph) {
      JSROOTPainter.ObjectPainter.call(this);
      this.graph = graph;
   }
   
   JSROOTPainter.GraphPainter.prototype = Object.create( JSROOTPainter.ObjectPainter.prototype );

   JSROOTPainter.GraphPainter.prototype.IsObject = function(obj) {
      return this.graph == obj;
   }
   
   JSROOTPainter.GraphPainter.prototype.SetFrame = function(vis, pad, frame) {
      this.SetVis(vis);
      this.pad = pad;
      this.frame = frame;
      if (!this.frame) {
         alert("frame for the graph was not specified");
         return;
      }
      
      if (!this.first) {
         alert("first painter is not defined - it is a must for graph");
         return;
      }
   }
   
   JSROOTPainter.GraphPainter.prototype.Redraw = function() 
   {
      this.DrawBins();
   }
   
   JSROOTPainter.GraphPainter.prototype.FillContextMenu = function(menu)
   {
      JSROOTPainter.ObjectPainter.prototype.FillContextMenu.call(this, menu);
   }
      
   JSROOTPainter.GraphPainter.prototype.ExeContextMenu = function(cmd) 
   {
      JSROOTPainter.ObjectPainter.prototype.ExeContextMenu.call(this, cmd);
   }

   JSROOTPainter.GraphPainter.prototype.DecodeOptions = function() {
      this.logx = false;
      this.logy = false;
      this.logz = false;
      this.gridx = false;
      this.gridy = false;
      this.draw_all = true;
      this.optionLine  = 0; 
      this.optionAxis  = 0;
      this.optionCurve = 0;
      this.optionStar  = 0;
      this.optionMark  = 0;
      this.optionBar   = 0;
      this.optionR     = 0;
      this.optionOne   = 0;
      this.optionE     = 0;
      this.optionFill  = 0;
      this.optionZ     = 0;
      this.optionCurveFill = 0;
      this.draw_errors = true;
      this.optionNone  = 0; // no any drawing
      this.opt = "LP";
      
      if ('fOption' in this.graph) {
         this.opt = this.graph['fOption'].toUpperCase();
         this.opt.replace('SAME', '');
      }

      if (this.opt.indexOf('L') != -1) this.optionLine = 1;
      if (this.opt.indexOf('A') != -1) this.optionAxis = 1;
      if (this.opt.indexOf('C') != -1) this.optionCurve= 1;
      if (this.opt.indexOf('*') != -1) this.optionStar = 1;
      if (this.opt.indexOf('P') != -1) this.optionMark = 1;
      if (this.opt.indexOf('B') != -1) this.optionBar  = 1;
      if (this.opt.indexOf('R') != -1) this.optionR    = 1;
      if (this.opt.indexOf('1') != -1) this.optionOne  = 1;
      if (this.opt.indexOf('F') != -1) this.optionFill = 1;
      if (this.opt.indexOf('2') != -1 || this.opt.indexOf('3') != -1 ||
            this.opt.indexOf('4') != -1 || this.opt.indexOf('5') != -1) this.optionE = 1;

      // if no drawing option is selected and if opt<>' ' nothing is done.
      if (this.optionLine + this.optionFill + this.optionCurve + this.optionStar + this.optionMark + this.optionBar + this.optionE == 0) {
         if (this.opt.length == 0) this.optionLine = 1;
                              else { this.optionNone = 1; return; }
      }
      if (this.optionStar) this.graph['fMarkerStyle'] = 3;
      
      if (this.optionCurve && this.optionFill) {
         this.optionCurveFill = 1;
         this.optionFill      = 0;
      }
      if (this.pad && typeof(this.pad) != 'undefined') {
         this.logx  = this.pad['fLogx'];
         this.logy  = this.pad['fLogy'];
         this.logz  = this.pad['fLogz'];
         this.gridx = this.pad['fGridx'];
         this.gridy = this.pad['fGridy'];
      }
      
      this.xaxis_type = this.logx ? 'logarithmic' : 'linear';
      this.yaxis_type = this.logy ? 'logarithmic' : 'linear';
      
      if (this.graph['_typename'] == 'JSROOTIO.TGraph') {
         // check for axis scale format, and convert if required
         if (this.graph['fHistogram']['fXaxis']['fTimeDisplay']) {
            this.xaxis_type = 'datetime';
         }
         
         if (this.graph['fHistogram']['fYaxis']['fTimeDisplay']) {
            this.yaxis_type = 'datetime';
         }
      }
      else if (this.graph['_typename'] == 'JSROOTIO.TGraphErrors') {
         this.maxEX = d3.max(this.graph['fEX']);
         this.maxEY = d3.max(this.graph['fEY']);
         if (this.maxEX < 1.0e-300 && this.maxEY < 1.0e-300)
            this.draw_errors = false;
      }
      this.seriesType = 'scatter';
      if (this.optionBar == 1)
         this.seriesType = 'bar';
      this.showMarker = false;
      if (this.optionMark == 1 || this.optionStar == 1)
         this.showMarker = true;
      
      if (this.optionLine == 1 || this.optionCurve == 1 || this.optionFill == 1)
         this.seriesType = 'line';

      if (this.optionBar == 1) {
         this.binwidth = 
            (this.graph['fHistogram']['fXaxis']['fXmax'] - 
             this.graph['fHistogram']['fXaxis']['fXmin']) /
                 this.graph['fNpoints'];
      }

   }
   
   JSROOTPainter.GraphPainter.prototype.CreateBins = function() {
      var pthis = this;
      
      // TODO: Sergey: one should remove all ifs out of the d3.range call - 
      //               just becose of performance
      
      this.bins = d3.range(this.graph['fNpoints']).map(function(p) {
         if (pthis.optionBar == 1) {
            return {
               x: pthis.graph['fX'][p] - (pthis.binwidth / 2),
               y: pthis.graph['fY'][p], // graph['fHistogram']['fXaxis']['fXmin'],
               bw: pthis.binwidth,
               bh: pthis.graph['fY'][p]
            }
         }
         else if (pthis.graph['_typename'] == 'JSROOTIO.TGraphErrors') {
            return {
               x: pthis.graph['fX'][p],
               y: pthis.graph['fY'][p],
               exlow:  pthis.graph['fEX'][p],
               exhigh: pthis.graph['fEX'][p],
               eylow:  pthis.graph['fEY'][p],
               eyhigh: pthis.graph['fEY'][p]
            };
         }
         else if (pthis.graph['_typename'] == 'JSROOTIO.TGraphAsymmErrors' ||
               pthis.graph['_typename'].match(/\bRooHist/)) {
            return {
               x: pthis.graph['fX'][p],
               y: pthis.graph['fY'][p],
               exlow:  pthis.graph['fEXlow'][p],
               exhigh: pthis.graph['fEXhigh'][p],
               eylow:  pthis.graph['fEYlow'][p],
               eyhigh: pthis.graph['fEYhigh'][p]
            };
         }
         else {
            return {
               x: pthis.graph['fX'][p],
               y: pthis.graph['fY'][p]
            };
         }
      });
      

      this.bins_lw = this.graph['fLineWidth']; // line width

      this.exclusionGraph = false;
      if (this.bins_lw <= 99) return;

      // special handling of exclusion graphs

      this.exclusionGraph = true;
      
      var ec, ff = 1;
         
      var normx, normy;
      var n = this.graph['fNpoints'];
      var glw = this.graph['fLineWidth'],
      xo = new Array(n+2), yo = new Array(n+2),
      xt = new Array(n+2), yt = new Array(n+2),
      xf = new Array(2*n+2), yf = new Array(2*n+2);
      // negative value means another side of the line...
      if (glw > 32767) {
         glw = 65536 - glw;
      }
      this.bins_lw = glw % 100; // line width
      if (this.bins_lw > 0) this.optionLine = 1;
      ec = root_colors[this.graph['fFillColor']];
      ec = ec.replace('rgb', 'rgba');
      ec = ec.replace(')', ', 0.20)');

      var a, i, j, nf, wk = (glw/100)*0.005;
      if (this.graph['fLineWidth'] > 32767)
         wk *= -1;
      
      var w = this.frame.attr("width"), h = this.frame.attr("height");

      var ratio = w / h;

      var xmin = this.first.xmin, xmax = this.first.xmax,
          ymin = this.first.ymin, ymax = this.first.ymax;
      for (i=0; i<n; i++) {
         xo[i] = (this.graph['fX'][i] - xmin) / (xmax - xmin);
         yo[i] = (this.graph['fY'][i] - ymin) / (ymax - ymin);
         if (w > h) yo[i] = yo[i] / ratio;
         else if (h > w) xo[i] = xo[i] / ratio;
      }
      // The first part of the filled area is made of the graph points.
      // Make sure that two adjacent points are different.
      xf[0] = xo[0];
      yf[0] = yo[0];
      nf = 0;
      for (i=1; i<n; i++) {
         if (xo[i] == xo[i-1] && yo[i] == yo[i-1]) continue;
         nf++;
         xf[nf] = xo[i];
         if (xf[i] == xf[i-1]) xf[i] += 0.000001; // add an epsilon to avoid exact vertical lines.
         yf[nf] = yo[i];
      }
      // For each graph points a shifted points is computed to build up
      // the second part of the filled area. First and last points are
      // treated as special cases, outside of the loop.
      if (xf[1] == xf[0]) {
         a = Math.PI / 2.0;
      } else {
         a = Math.atan((yf[1] - yf[0]) / (xf[1] - xf[0]));
      }
      if (xf[0] <= xf[1]) {
         xt[0] = xf[0] - wk * Math.sin(a);
         yt[0] = yf[0] + wk * Math.cos(a);
      } else {
         xt[0] = xf[0] + wk * Math.sin(a);
         yt[0] = yf[0] - wk * Math.cos(a);
      }
      if (xf[nf] == xf[nf-1]) {
         a = Math.PI / 2.0;
      } else {
         a = Math.atan((yf[nf] - yf[nf-1]) / (xf[nf] - xf[nf-1]));
      }
      if (xf[nf] >= xf[nf-1]) {
         xt[nf] = xf[nf] - wk * Math.sin(a);
         yt[nf] = yf[nf] + wk * Math.cos(a);
      } else {
         xt[nf] = xf[nf] + wk * Math.sin(a);
         yt[nf] = yf[nf] - wk * Math.cos(a);
      }

      var a1, a2, a3, xi0, yi0, xi1, yi1, xi2, yi2;
      for (i=1; i<nf; i++) {
         xi0 = xf[i];
         yi0 = yf[i];
         xi1 = xf[i+1];
         yi1 = yf[i+1];
         xi2 = xf[i-1];
         yi2 = yf[i-1];
         if (xi1 == xi0) {
            a1 = Math.PI / 2.0;
         } else {
            a1  = Math.atan((yi1 - yi0) / (xi1 - xi0));
         }
         if (xi1 < xi0) a1 = a1 + Math.PI;
         if (xi2 == xi0) {
            a2 = Math.PI / 2.0;
         } else {
            a2  = Math.atan((yi0 - yi2) / (xi0 - xi2));
         }
         if (xi0 < xi2) a2 = a2 + Math.PI;
         x1 = xi0 - wk * Math.sin(a1);
         y1 = yi0 + wk * Math.cos(a1);
         x2 = xi0 - wk * Math.sin(a2);
         y2 = yi0 + wk * Math.cos(a2);
         xm = (x1 + x2) * 0.5;
         ym = (y1 + y2) * 0.5;
         if (xm == xi0) {
            a3 = Math.PI / 2.0;
         } else {
            a3 = Math.atan((ym - yi0) / (xm - xi0));
         }
         x3 = xi0 - wk * Math.sin(a3 + (Math.PI / 2.0));
         y3 = yi0 + wk * Math.cos(a3 + (Math.PI / 2.0));
         // Rotate (x3,y3) by PI around (xi0,yi0) if it is not on the (xm,ym) side.
         if ((xm - xi0) * (x3 - xi0) < 0 && (ym - yi0) * (y3 - yi0) < 0) {
            x3 = 2 * xi0 - x3;
            y3 = 2 * yi0 - y3;
         }
         if ((xm == x1) && (ym == y1)) {
            x3 = xm;
            y3 = ym;
         }
         xt[i] = x3;
         yt[i] = y3;
      }
      // Close the polygon if the first and last points are the same
      if (xf[nf] == xf[0] && yf[nf] == yf[0]) {
         xm = (xt[nf] + xt[0]) * 0.5;
         ym = (yt[nf] + yt[0]) * 0.5;
         if (xm == xf[0]) {
            a3 = Math.PI / 2.0;
         } else {
            a3 = Math.atan((ym - yf[0]) / (xm - xf[0]));
         }
         x3 = xf[0] + wk * Math.sin(a3 + (Math.PI / 2.0));
         y3 = yf[0] - wk * Math.cos(a3 + (Math.PI / 2.0));
         if ((xm - xf[0]) * (x3 - xf[0]) < 0 && (ym - yf[0]) * (y3 - yf[0]) < 0) {
            x3 = 2 * xf[0] - x3;
            y3 = 2 * yf[0] - y3;
         }
         xt[nf] = x3;
         xt[0]  = x3;
         yt[nf] = y3;
         yt[0]  = y3;
      }
      // Find the crossing segments and remove the useless ones
      var xc, yc, c1, b1, c2, b2;
      var cross = false;
      var nf2 = nf;
      for (i=nf2; i>0; i--) {
         for (j=i-1; j>0; j--) {
            if (xt[i-1] == xt[i] || xt[j-1] == xt[j]) continue;
            c1  = (yt[i-1] - yt[i]) / (xt[i-1] - xt[i]);
            b1  = yt[i] - c1 * xt[i];
            c2  = (yt[j-1] - yt[j]) / (xt[j-1] - xt[j]);
            b2  = yt[j] - c2 * xt[j];
            if (c1 != c2) {
               xc = (b2 - b1) / (c1 - c2);
               yc = c1 * xc + b1;
               if (xc > Math.min(xt[i], xt[i-1]) && xc < Math.max(xt[i], xt[i-1]) &&
                     xc > Math.min(xt[j], xt[j-1]) && xc < Math.max(xt[j], xt[j-1]) &&
                     yc > Math.min(yt[i], yt[i-1]) && yc < Math.max(yt[i], yt[i-1]) &&
                     yc > Math.min(yt[j], yt[j-1]) && yc < Math.max(yt[j], yt[j-1])) {
                  nf++; xf[nf] = xt[i]; yf[nf] = yt[i];
                  nf++; xf[nf] = xc   ; yf[nf] = yc;
                  i = j;
                  cross = true;
                  break;
               } else {
                  continue;
               }
            } else {
               continue;
            }
         }
         if (!cross) {
            nf++;
            xf[nf] = xt[i];
            yf[nf] = yt[i];
         }
         cross = false;
      }
      nf++; xf[nf] = xt[0]; yf[nf] = yt[0]; nf++;
      for (i=0; i<nf; i++) {
         if (w > h) {
            xf[i] = xmin + (xf[i] * (xmax - xmin));
            yf[i] = ymin + (yf[i] * (ymax - ymin)) * ratio;
         }
         else if (h > w) {
            xf[i] = xmin + (xf[i] * (xmax - xmin)) * ratio;
            yf[i] = ymin + (yf[i] * (ymax - ymin));
         }
         else {
            xf[i] = xmin + (xf[i] * (xmax - xmin));
            yf[i] = ymin + (yf[i] * (ymax - ymin));
         }
         if (this.logx && xf[i] <= 0.0) xf[i] = xmin;
         if (this.logy && yf[i] <= 0.0) yf[i] = ymin;
      }
      
      this.excl = d3.range(nf).map(function(p) {
         return {
            x: xf[p],
            y: yf[p]
         };
      });
      
      this.excl_ec = ec;
      this.excl_ff = ff;
      
      /* some clean-up */
      xo.splice(0, xo.length); yo.splice(0, yo.length);
      xo = null; yo = null;
      xt.splice(0, xt.length); yt.splice(0, yt.length);
      xt = null; yt = null;
      xf.splice(0, xf.length); yf.splice(0, yf.length);
      xf = null; yf = null;
   }
   
   
   JSROOTPainter.GraphPainter.prototype.ProvideTooltip = function(tip)
   {
      if (!this.draw_content) return;

      if (!('bins' in this) || (this.bins==null)) return;
      
      var nbin = -1;
      var min = 1e20;
      
      for (var i=0; i<this.bins.length;i++) {
         var dist = Math.pow(this.bins[i].x - tip.x, 2) + Math.pow(this.bins[i].y - tip.y, 2);
         
         if ((nbin<0) || (dist<min)) {
            nbin = i;
            min = dist;  
         }
      } 
      
      if (nbin < 0) return;
      
      var bin = this.bins[nbin]; 
      
      tip['empty'] = false;
      tip.x = bin.x;
      tip.y = bin.y;
      tip['text'].push("graph:" + this.graph['fName']);
      tip['text'].push("bin: "+ nbin); 

      if (this.draw_errors) {
         tip['text'].push("error x = -" + bin.exlow.toPrecision(4) + "/+" + bin.exhigh.toPrecision(4));
         tip['text'].push("error y = -" + bin.eylow.toPrecision(4) + "/+" + bin.eyhigh.toPrecision(4));
      }
   }

   
   JSROOTPainter.GraphPainter.prototype.DrawBins = function() 
   {
      var w = this.frame.attr("width"), h = this.frame.attr("height");

      if (!('bins_id' in this))
         this['bins_id'] = "random_bins_id" + random_id++;
      
      d3.selectAll("#" + this.bins_id).remove();
         
      this['bins_g'] = this.first.svg_frame.append("svg:g").attr("id",this.bins_id);

      var pthis = this;
      
      function TooltipText(d) {
         if (pthis.draw_errors && ('exlow' in d))
            return "x = " + d.x.toPrecision(4) + " \ny = " + d.y.toPrecision(4) +
                   "\nerror x = -" + d.exlow.toPrecision(4) + "/+" + d.exhigh.toPrecision(4) +
                   "\nerror y = -" + d.eylow.toPrecision(4) + "/+" + d.eyhigh.toPrecision(4);
         else 
            return "x = " + d.x.toPrecision(4) + " \ny = " + d.y.toPrecision(4);
      }
      
      var x = this.first.x;
      var y = this.first.y;
      
      if (this.seriesType == 'line') {
         /* contour lines only */
         var line = d3.svg.line()
            .x(function(d) { return x(d.x);})
            .y(function(d) { return y(d.y);});
      }
      
      if (this.seriesType == 'bar') {
         var fillcolor = root_colors[this.graph['fFillColor']];
         if (typeof(fillcolor) == 'undefined') fillcolor = "rgb(204, 204, 204)";
         /* filled bar graph */
         var xdom = this.first.x.domain();
         var xfactor = xdom[1]-xdom[0];
         this.draw_errors = false;
         
         var nodes = this.bins_g
            .selectAll("bar_graph")
            .data(pthis.bins)
            .enter()
            .append("svg:rect")
            .attr("x", function(d) { return x(d.x)} )
            .attr("y", function(d) { return y(d.y)} )
            .attr("width", function(d) { return (w / (xdom[1]-xdom[0]))-1} )
            .attr("height", function(d) { return y(d.y) - y(d.y+d.bh); } )
            .style("fill", fillcolor);
         
         if (gStyle.Tooltip > 1)
            nodes.append("svg:title").text(function(d) {
                    return "x = " + d.x.toPrecision(4) + " \nentries = " + d.y.toPrecision(4);
                   });
      }
      if (this.exclusionGraph) {
         /* first draw exclusion area, and then the line */
         this.showMarker = false;
         if (this.graph['fFillStyle'] > 3000 && this.graph['fFillStyle'] <= 3025) {
            createFillPatterns(this.vis, this.graph['fFillStyle'], this.graph['fFillColor']);
            this.bins_g.append("svg:path")
               .attr("d", line(pthis.excl))
               .style("stroke", "none")
               .style("stroke-width", pthis.excl_ff)
               .style("fill", "url(#pat" + pthis.graph['fFillStyle'] + "_" + pthis.graph['fFillColor'] + ")")
               .style("antialias", "false");
         }
         else {
            this.bins_g.append("svg:path")
               .attr("d", line(pthis.excl))
               .style("stroke", "none")
               .style("stroke-width", pthis.excl_ff)
               .style("fill", pthis.excl_ec);
         }
      }
      if (this.seriesType == 'line') {
         this.bins_g.append("svg:path")
            .attr("d", line(pthis.bins))
            .attr("class", "draw_line")
            .style("stroke", (pthis.optionLine == 1) ? root_colors[pthis.graph['fLineColor']] : "none")
            .style("stroke-width", pthis.bins_lw)
            .style("stroke-dasharray", root_line_styles[pthis.graph['fLineStyle']])
            .style("fill", (pthis.optionFill == 1) ? root_colors[pthis.graph['fFillColor']] : "none");

         if (gStyle.Tooltip > 1)
            this.bins_g.selectAll("draw_line")
              .data(pthis.bins).enter()
              .append("svg:circle") 
              .attr("cx", function(d) { return x(d.x); }) 
              .attr("cy", function(d) { return y(d.y); }) 
              .attr("r", 3) 
              .attr("opacity", 0) 
              .append("svg:title").text(TooltipText);
      }
      if ((this.graph['_typename'] == 'JSROOTIO.TGraphErrors' ||
            this.graph['_typename'] == 'JSROOTIO.TGraphAsymmErrors' ||
            this.graph['_typename'].match(/\bRooHist/)) && this.draw_errors && !this.optionBar) {

         // here are up to five elements are collected, try to group them
         
         var nodes = this.bins_g.selectAll("g.node").data(this.bins).enter().append("svg:g");

         // Add x-error indicators

         // than doing filer append error bars
         var xerr = nodes.filter(function(d) { return (d.exlow>0) || (d.exhigh>0); })
              .append("svg:line")
               .attr("x1", function(d) { return x(d.x-d.exlow)} )
               .attr("y1", function(d) { return y(d.y)} )
               .attr("x2", function(d) { return x(d.x+d.exhigh)} )
               .attr("y2", function(d) { return y(d.y)} )
               .style("stroke", root_colors[this.graph['fLineColor']])
               .style("stroke-width", this.graph['fLineWidth']);

         nodes.filter(function(d) { return (d.exlow>0); })
              .append("svg:line")
               .attr("y1", function(d) { return y(d.y)-3} )
               .attr("x1", function(d) { return x(d.x-d.exlow)})
               .attr("y2", function(d) { return y(d.y)+3})
               .attr("x2", function(d) { return x(d.x-d.exlow)})
               .style("stroke", root_colors[this.graph['fLineColor']])
               .style("stroke-width", this.graph['fLineWidth']);
         
         nodes.filter(function(d) { return (d.exhigh>0); })
              .append("svg:line")
               .attr("y1", function(d) { return y(d.y)-3} )
               .attr("x1", function(d) { return x(d.x+d.exhigh) })
               .attr("y2", function(d) { return y(d.y)+3})
               .attr("x2", function(d) { return x(d.x+d.exhigh) })
               .style("stroke", root_colors[this.graph['fLineColor']])
               .style("stroke-width", this.graph['fLineWidth']);
               
         // Add y-error indicators

         var yerr = nodes.filter(function(d) { return (d.eylow>0) || (d.eyhigh>0); })
            .append("svg:line")
            .attr("x1", function(d) { return x(d.x)})
            .attr("y1", function(d) { return y(d.y-d.eylow) })
            .attr("x2", function(d) { return x(d.x)})
            .attr("y2", function(d) { return y(d.y+d.eyhigh) })
            .style("stroke", root_colors[this.graph['fLineColor']])
            .style("stroke-width", this.graph['fLineWidth']);

         nodes.filter(function(d) { return (d.eylow>0); })
            .append("svg:line")
            .attr("x1", function(d) { return x(d.x)-3})
            .attr("y1", function(d) { return y(d.y-d.eylow) })
            .attr("x2", function(d) { return x(d.x)+3})
            .attr("y2", function(d) { return y(d.y-d.eylow) })
            .style("stroke", root_colors[this.graph['fLineColor']])
            .style("stroke-width", this.graph['fLineWidth']);

         nodes.filter(function(d) { return (d.eyhigh>0); })
            .append("svg:line")
            .attr("x1", function(d) { return x(d.x)-3})
            .attr("y1", function(d) { return y(d.y+d.eyhigh) })
            .attr("x2", function(d) { return x(d.x)+3})
            .attr("y2", function(d) { return y(d.y+d.eyhigh) })
            .style("stroke", root_colors[this.graph['fLineColor']])
            .style("stroke-width", this.graph['fLineWidth']);


         if (gStyle.Tooltip > 1) {
            xerr.append("svg:title").text(TooltipText);
            yerr.append("svg:title").text(TooltipText);
         }
      } else this.draw_errors = false;
      
      if (this.showMarker) {
         /* Add markers */
         var filled = false;
         if ((this.graph['fMarkerStyle'] == 8) ||
             (this.graph['fMarkerStyle'] > 19 && this.graph['fMarkerStyle'] < 24) ||
             (this.graph['fMarkerStyle'] == 29))
            filled = true;

         var info_marker = getRootMarker(root_markers, this.graph['fMarkerStyle']);

         var shape = info_marker['shape'];
         var filled = info_marker['toFill'];
         var toRotate = info_marker['toRotate'];
         var markerSize = this.graph['fMarkerSize'];
         var markerScale = (shape == 0) ? 32 : 64;
         if (this.graph['fMarkerStyle'] == 1) markerScale = 1;

         switch (shape) {
            case 6:
               var marker = "M " + (-4 * markerSize) + " " + (-1 * markerSize)
                           + " L " + 4 * markerSize + " " + (-1 * markerSize)
                           + " L " + (-2.4 * markerSize) + " " + 4 * markerSize
                           + " L 0 " + (-4 * markerSize) + " L " + 2.8 * markerSize
                           + " " + 4 * markerSize + " z";
               break;
            case 7:
               var marker = "M " + (- 4 * markerSize) + " " + (-4 * markerSize)
                           + " L " + 4 * markerSize + " " + 4 * markerSize + " M 0 "
                           + (-4 * markerSize) + " 0 " + 4 * markerSize + " M "
                           + 4 * markerSize + " " + (-4 * markerSize) + " L "
                           + (-4 * markerSize) + " " + 4 * markerSize + " M "
                           + (-4 * markerSize) + " 0 L " + 4 * markerSize + " 0";
               break;
            default:
               var marker = d3.svg.symbol()
                           .type(d3.svg.symbolTypes[shape])
                           .size(markerSize * markerScale);
               break;
         }
         
         var markers = this.bins_g.selectAll("markers")
            .data(this.bins)
            .enter()
            .append("svg:path")
            .attr("transform", function(d) {return "translate(" + x(d.x) + "," + y(d.y) + ")"})
            .style("fill", filled ? root_colors[this.graph['fMarkerColor']] : "none")
            .style("stroke", root_colors[this.graph['fMarkerColor']])
            .attr("d", marker);
         
         
         if (gStyle.Tooltip > 1) 
            markers
            .append("svg:title")
            .text(TooltipText);
      }
   }

   
   JSROOTPainter.GraphPainter.prototype.UpdateObject = function(obj) {
      this.graph = obj;
      this.CreateBins();
   }

   
   JSROOTPainter.drawGraphNew = function(vis, pad, graph, frame) 
   {
      if (!('fHistogram' in graph)) {
         alert("drawing graphs without fHistogram field not (yet) supported");
         return;
      }
      
      if ((frame==null) || !('painters' in vis)) {
         var hpainter = JSROOTPainter.drawHistogram1Dnew(vis, pad, graph['fHistogram'], frame);
         frame = hpainter.frame;
      } 
      
      var painter = new JSROOTPainter.GraphPainter(graph);
      
      painter.SetFrame(vis, pad, frame);
      
      painter.DecodeOptions();
      
      painter.CreateBins();
      
      painter.DrawBins();
      
      return painter;
   }


   JSROOTPainter.drawGrid = function(vis, histo, pad, x, y) {
      var gridx = false, gridy = false;
      if (pad && typeof(pad) != 'undefined') {
         gridx = pad['fGridx'];
         gridy = pad['fGridy'];
      }
      var ndivx = histo['fXaxis']['fNdivisions'];
      var n1ax = ndivx%100;
      var n2ax = (ndivx%10000 - n1ax)/100;
      var n3ax = ndivx/10000;
      var nn3x = Math.max(n3ax,1);
      var nn2x = Math.max(n2ax,1)*nn3x;
      var nn1x = Math.max(n1ax,1)*nn2x;

      var ndivy = histo['fYaxis']['fNdivisions'];
      var n1ay = ndivy%100;
      var n2ay = (ndivy%10000 - n1ay)/100;
      var n3ay = ndivy/10000;
      var nn3y = Math.max(n3ay,1);
      var nn2y = Math.max(n2ay,1)*nn3y;
      var nn1y = Math.max(n1ay,1)*nn2y;

      vis.selectAll(".gridLine").remove();

      /* add a grid on x axis, if the option is set */
      if (gridx) {
         vis.selectAll("gridLine")
            .data(x.ticks(n1ax))
            .enter()
            .append("svg:line")
            .attr("class", "gridLine")
            .attr("x1", x)
            .attr("y1", vis.attr("height"))
            .attr("x2", x)
            .attr("y2", 0)
            .style("stroke", "black")
            .style("stroke-width", histo['fLineWidth'])
            .style("stroke-dasharray", root_line_styles[11]);
      }

      /* add a grid on y axis, if the option is set */
      if (gridy) {
         vis.selectAll("gridLine")
            .data(y.ticks(n1ay))
            .enter()
            .append("svg:line")
            .attr("class", "gridLine")
            .attr("x1", 0)
            .attr("y1", y)
            .attr("x2", vis.attr("width"))
            .attr("y2", y)
            .style("stroke", "black")
            .style("stroke-width", histo['fLineWidth'])
            .style("stroke-dasharray", root_line_styles[11]);
      }
   };

   
   
   // ============================================================
   
   JSROOTPainter.PavePainter = function(pave) {
      JSROOTPainter.ObjectPainter.call(this);
      this.pavetext = pave;
   }

   JSROOTPainter.PavePainter.prototype = Object.create( JSROOTPainter.ObjectPainter.prototype );

   
   JSROOTPainter.PavePainter.prototype.IsObject = function(obj) {
      return this.pavetext == obj;
   }

   
   JSROOTPainter.PavePainter.prototype.SetFrame = function(vis) {
      this.SetVis(vis);
   } 
   
   JSROOTPainter.PavePainter.prototype.DrawPaveText = function() {
      var pavetext = this.pavetext;
      var vis = this.vis;

      var i, j, lw, w = vis.attr("width"), h = vis.attr("height");
      
      var pos_x = pavetext['fX1NDC'] * w;
      var pos_y = (1.0 - pavetext['fY1NDC']) * h;
      var width = Math.abs(pavetext['fX2NDC'] - pavetext['fX1NDC']) * w;
      var height = Math.abs(pavetext['fY2NDC'] - pavetext['fY1NDC']) * h;
      pos_y -= height;
      var line, nlines = pavetext['fLines'].length;
      var font_size = Math.round(height / (nlines * 1.5));
      var fcolor = root_colors[pavetext['fFillColor']];
      var lcolor = root_colors[pavetext['fLineColor']];
      var tcolor = root_colors[pavetext['fTextColor']];
      var scolor = root_colors[pavetext['fShadowColor']];
      if (pavetext['fFillStyle'] == 0) fcolor = 'none';
      // align = 10*HorizontalAlign + VerticalAlign
      // 1=left adjusted, 2=centered, 3=right adjusted
      // 1=bottom adjusted, 2=centered, 3=top adjusted
      // "middle", "start", "end"

      var align = 'start', halign = Math.round(pavetext['fTextAlign']/10);
      var baseline = 'bottom', valign = pavetext['fTextAlign']%10;
      if (halign == 1) align = 'start';
      else if (halign == 2) align = 'middle';
      else if (halign == 3) align = 'end';
      if (valign == 1) baseline = 'bottom';
      else if (valign == 2) baseline = 'middle';
      else if (valign == 3) baseline = 'top';
      var lmargin = 0;
      switch (halign) {
         case 1:
            lmargin = pavetext['fMargin'] * width;
            break;
         case 2:
            lmargin = width/2;
            break;
         case 3:
            lmargin = width - (pavetext['fMargin'] * width);
            break;
      }
      
      var fontDetails = getFontDetails(root_fonts[Math.floor(pavetext['fTextFont']/10)]);
      var lwidth = pavetext['fBorderSize'] ? pavetext['fBorderSize'] : 0;

      this['pave'] = vis.append("svg:g")
         .attr("class", "pavestatbox")
         .attr("x", pos_x)
         .attr("y", pos_y)
         .attr("width", width)
         .attr("height", height)
         .attr("transform", "translate(" + pos_x + "," + pos_y + ")");

      this.pave.append("svg:rect")
         .attr("class", "pavedraw")
         .attr("x", 0)
         .attr("y", 0)
         .attr("height", height)
         .attr("width", width)
         .attr("fill", fcolor)
         .style("stroke-width", lwidth ? 1 : 0)
         .style("stroke", lcolor);

      if (nlines == 1) {
         line = this.translateLaTeX(pavetext['fLines'][0]['fTitle']);

         lw = stringWidth(vis, line, fontDetails['name'], fontDetails['weight'],
                          font_size, fontDetails['style']);
         if (lw > width)
            font_size *= 0.98 * (width / lw);

         this.pave.append("text")
            .attr("class", "pavetext")
            .attr("text-anchor", align)
            .attr("x", lmargin)
            .attr("y", (height/2) + (font_size/3))
            .attr("font-family", fontDetails['name'])
            .attr("font-weight", fontDetails['weight'])
            .attr("font-style", fontDetails['style'])
            .attr("font-size", font_size)
            .attr("fill", tcolor)
            .text(line);
      }
      else {
         
         var max_len = 0;
         for (j=0; j<nlines; ++j) {
            line = JSROOTPainter.translateLaTeX(pavetext['fLines'][j]['fTitle']);
            lw = lmargin + stringWidth(vis, line, fontDetails['name'], fontDetails['weight'],
                                       font_size, fontDetails['style']);
            if (lw > max_len) max_len = lw;
         }
         if (max_len > width)
            font_size *= 0.98 * (width / max_len);
         
         for (j=0; j<nlines; ++j) {
            var jcolor = root_colors[pavetext['fLines'][j]['fTextColor']];
            if (pavetext['fLines'][j]['fTextColor'] == 0)
               jcolor = tcolor;
            line = JSROOTPainter.translateLaTeX(pavetext['fLines'][j]['fTitle']);
            if (pavetext['_typename'] == 'JSROOTIO.TPaveStats') {
               var off_y = (j == 0) ? 0 : (font_size * 0.05);
               this.pave.append("text")
                  .attr("class", "pavetext")
                  .attr("text-anchor", (j == 0) ? "middle" : "start")
                  .attr("x", ((j==0) ? width/2 : pavetext['fMargin'] * width))
                  .attr("y", ((j == 0) ? off_y + (font_size * 1.2) : off_y + ((j+1) * (font_size * 1.4))))
                  .attr("font-family", fontDetails['name'])
                  .attr("font-weight", fontDetails['weight'])
                  .attr("font-style", fontDetails['style'])
                  .attr("font-size", font_size)
                  .attr("fill", jcolor)
                  .text(line);
            }
            else {
               this.pave.append("text")
                  .attr("class", "pavetext")
                  .attr("text-anchor", "start")
                  .attr("x", lmargin)
                  .attr("y", (j+1) * (font_size * 1.4))
                  .attr("font-family", fontDetails['name'])
                  .attr("font-weight", fontDetails['weight'])
                  .attr("font-style", fontDetails['style'])
                  .attr("font-size", font_size)
                  .attr("fill", jcolor)
                  .text(line);
            }
         }
      }

      if (pavetext['fBorderSize'] && pavetext['_typename'] == 'JSROOTIO.TPaveStats')
         this.pave.append("svg:line")
            .attr("class", "pavedraw")
            .attr("x1", 0)
            .attr("y1", font_size * 1.6)
            .attr("x2", width)
            .attr("y2", font_size * 1.6)
            .style("stroke", lcolor)
            .style("stroke-width", lwidth ? 1 : 'none');

      if (lwidth && lwidth > 1) {
         this.pave.append("svg:line")
            .attr("class", "pavedraw")
            .attr("x1", width+(lwidth/2))
            .attr("y1", lwidth+1)
            .attr("x2", width+(lwidth/2))
            .attr("y2", height+lwidth-1)
            .style("stroke", lcolor)
            .style("stroke-width", lwidth);
         this.pave.append("svg:line")
            .attr("class", "pavedraw")
            .attr("x1", lwidth+1)
            .attr("y1", height+(lwidth/2))
            .attr("x2", width+lwidth-1)
            .attr("y2", height+(lwidth/2))
            .style("stroke", lcolor)
            .style("stroke-width", lwidth);
      }
   };
   
   JSROOTPainter.PavePainter.prototype.AddLine = function(txt) {
      this.pavetext['fLines'].push( {'fTitle': txt, "fTextColor": 1} );
   }
   
   JSROOTPainter.PavePainter.prototype.FillStatistic = function(histo) 
   {
      if (!this.pavetext) return false;

      var histo = null;
      
      // we take histogram from first painter
      if (this.first && ('histo' in this.first)) histo = this.first.histo; 
      
      if (!histo) return false;
      
      var dostat = new Number(this.pavetext['fOptStat']);
      if (!dostat) dostat = new Number(gStyle.OptStat);

      if ((histo['fDimension'] == 1) || (histo['_typename'] == "JSROOTIO.TProfile")) {
         // clear lines

         this.pavetext['fLines'].length = 0;
         var print_name    = dostat%10;
         var print_entries = Math.floor(dostat/10)%10;
         var print_mean    = Math.floor(dostat/100)%10;
         var print_rms     = Math.floor(dostat/1000)%10;
         var print_under   = Math.floor(dostat/10000)%10;
         var print_over    = Math.floor(dostat/100000)%10;
         var print_integral= Math.floor(dostat/1000000)%10;
         var print_skew    = Math.floor(dostat/10000000)%10;
         var print_kurt    = Math.floor(dostat/100000000)%10;

         if (print_name > 0) {
            this.AddLine(histo['fName']);
         }
         
         if (print_entries > 0) {
            this.AddLine("Entries = " + this.first.GetStat("Entries"));
         } 
         
         if (print_mean > 0) {
            this.AddLine("Mean = " + this.first.GetStat("Mean"));
         }
         
         if (print_rms > 0) {
            this.AddLine("RMS = " + this.first.GetStat("RMS"));
         }

         if (print_under > 0) {
            this.AddLine("Underflow = " + this.first.GetStat("Underflow"));
         }

         if (print_over> 0) {
            this.AddLine("Overflow = " + this.first.GetStat("Overflow"));
         }

         if (print_integral> 0) {
            this.AddLine("Integral = " + this.first.GetStat("Integral"));
         }
         if (print_skew> 0) {
            this.AddLine("Skew = " + this.first.GetStat("Skew"));
         }
         if (print_kurt> 0) {
            this.AddLine("Kurt = " + this.first.GetStat("Kurt"));
         }
      }
      
      return true;
         
   }

   JSROOTPainter.PavePainter.prototype.Redraw = function() {

      if (('pave' in this) && (this.pave!=null)) {
         this.pave.selectAll(".pavetext").remove();
         this.pave.selectAll(".pavedraw").remove();
         d3.select(this.pave).remove();
         this.pave = null;
      }

      // if pavetext artificially disabled, do not redraw it
      if ('_Enabled' in this.pavetext) {
         if (!this.pavetext._Enabled) return;
      }
      
      // fill statistic
      if (!this.FillStatistic()) return;

      this.DrawPaveText();
   }


   JSROOTPainter.DrawPaveTextNew = function(vis, pavetext) {
      var painter = new JSROOTPainter.PavePainter(pavetext);
      
      painter.SetFrame(vis);
      
      if ('_AutoCreated' in pavetext) painter.FillStatistic();
      
      painter.DrawPaveText();
      
      return painter;
   }
   

   // =============================================================
   
   JSROOTPainter.HistPainter = function(histo) {
      JSROOTPainter.ObjectPainter.call(this);
      this.histo = histo;
   }

   JSROOTPainter.HistPainter.prototype = Object.create( JSROOTPainter.ObjectPainter.prototype );

   
   
   JSROOTPainter.HistPainter.prototype.IsObject = function(obj) {
      return this.histo == obj;
   }
   
   JSROOTPainter.HistPainter.prototype.Dimension = function() {
      if (!this.histo) return 0;
      return this.histo['fDimension'];
   }
   
   JSROOTPainter.HistPainter.prototype.SetFrame = function(vis, pad, frame) {
      this.draw_content = false;

      this.SetVis(vis);

      // here we deciding how histogram will look like and that and how will be shown 
      this.options = JSROOTPainter.decodeOptions(this.histo['fOption'], this.histo, pad);
      
      if (this.histo['_typename'] == "JSROOTIO.TProfile")
         this.options.Error = 11;
      
      this.frame = frame;
      
      if (this.frame==null)
         this.frame = JSROOTPainter.createFrame(vis, pad, this.histo, null);

      this.svg_frame = this.frame['root_svg_frame'];
      
      this.pad = pad;
      
      this.show_gridx = false;
      this.show_gridy = false;
      if (this.pad && typeof(this.pad) != 'undefined') {
         this.show_gridx = this.pad['fGridx'];
         this.show_gridy = this.pad['fGridy'];
      }
   }

   JSROOTPainter.HistPainter.prototype.CreateBins = function() {
      alert("HistPainter.prototype.CreateBins not implemented");
   }

   
   
   JSROOTPainter.HistPainter.prototype.UpdateObject = function(obj) 
   {
      if (obj['_typename'].indexOf("JSROOTIO.TH1") != 0) {
         alert("JSROOTPainter.HistPainter.UpdateObject - wrong class " + obj['_typename']);
         return;
      }
      
      this.histo = obj;
      
      this.CreateBins();
   }
      

   JSROOTPainter.HistPainter.prototype.CreateXY = function()
   {
      // here we create x,y objects wich maps our physical coordnates into pixels
      // while only first painter really need such object, all others just copy it
      
      if (this.first) {
         this['x'] = this.first['x']; 
         this['y'] = this.first['y'];
         return;
      }

      // special case used for drawing multiple graphs in the same frame
      var w = this.frame.attr("width"), h = this.frame.attr("height");

      if (this.options.Logx)
         this['x'] = d3.scale.log().domain([this.xmin, this.xmax]).range([0, w]);
      else
         this['x'] = d3.scale.linear().domain([this.xmin,this.xmax]).range([0, w]);

      if (this.zoom_xmin != this.zoom_xmax) 
         this.x.domain([this.zoom_xmin, this.zoom_xmax]);

      // $("#report").append("<br> ymin " + this.ymin + "  ymax " + this.ymax);
      
      if (this.options.Logy)
         this['y'] = d3.scale.log().domain([this.ymin, this.ymax]).range([h, 0]);
      else
         this['y'] = d3.scale.linear().domain([this.ymin, this.ymax]).range([h, 0]);

      if (this.zoom_ymin!=this.zoom_ymax)  
         this.y.domain([this.zoom_ymin, this.zoom_ymax]);
      
   }
   
   JSROOTPainter.HistPainter.prototype.CountStat = function()
   {
      alert("CountStat not implemented");
   }

   JSROOTPainter.HistPainter.prototype.DrawGrids = function() {
      // grid can only be drawn by first painter
      if (this.first) return;
      
      var ndivx = this.histo['fXaxis']['fNdivisions'];
      var n1ax = ndivx%100;
      var n2ax = (ndivx%10000 - n1ax)/100;
      var n3ax = ndivx/10000;
      var nn3x = Math.max(n3ax,1);
      var nn2x = Math.max(n2ax,1)*nn3x;
      var nn1x = Math.max(n1ax,1)*nn2x;

      var ndivy = this.histo['fYaxis']['fNdivisions'];
      var n1ay = ndivy%100;
      var n2ay = (ndivy%10000 - n1ay)/100;
      var n3ay = ndivy/10000;
      var nn3y = Math.max(n3ay,1);
      var nn2y = Math.max(n2ay,1)*nn3y;
      var nn1y = Math.max(n1ay,1)*nn2y;

      // $("#report").append(" draw grids");
      
      this.frame.selectAll(".gridLine").remove();
      /* add a grid on x axis, if the option is set */
      
      // add a grid on x axis, if the option is set
      if (this.show_gridx) {

         // $("#report").append(" draw grid x:" + n1ax);

         this.frame.selectAll("gridLine")
         .data(this.x.ticks(n1ax))
         .enter()
         .append("svg:line")
         .attr("class", "gridLine")
         .attr("x1", this.x)
         .attr("y1", this.frame.attr("height"))
         .attr("x2", this.x)
         .attr("y2", 0)
         .style("stroke", "black")
         .style("stroke-width", this.histo['fLineWidth'])
         .style("stroke-dasharray", root_line_styles[11]);
      }

      // add a grid on y axis, if the option is set
      if (this.show_gridy) {
         
         // $("#report").append(" draw grid y:" + n1ay);

         this.frame.selectAll("gridLine")
         .data(this.y.ticks(n1ay))
         .enter()
         .append("svg:line")
         .attr("class", "gridLine")
         .attr("x1", 0)
         .attr("y1", this.y)
         .attr("x2", this.frame.attr("width"))
         .attr("y2", this.y)
         .style("stroke", "black")
         .style("stroke-width", this.histo['fLineWidth'])
         .style("stroke-dasharray", root_line_styles[11]);
      }
   }
   
   JSROOTPainter.HistPainter.prototype.DrawBins = function() {
      alert("HistPainter.DrawBins not implemented");
   }
   
   JSROOTPainter.HistPainter.prototype.DrawAxes = function() {
      // axes can be drawn only for main (first) histogram
      if (this.first) return;
      
      var axis_frame = this.frame;
      
      var w = axis_frame.attr("width"), h = axis_frame.attr("height");
      var noexpx = this.histo['fXaxis'].TestBit(EAxisBits.kNoExponent);
      var noexpy = this.histo['fYaxis'].TestBit(EAxisBits.kNoExponent);
      var moreloglabelsx = this.histo['fXaxis'].TestBit(EAxisBits.kMoreLogLabels);
      var moreloglabelsy = this.histo['fYaxis'].TestBit(EAxisBits.kMoreLogLabels);

      if (this.histo['fXaxis']['fXmax'] < 100 && this.histo['fXaxis']['fXmax']/this.histo['fXaxis']['fXmin'] < 100) noexpx = true;
      if (this.histo['fYaxis']['fXmax'] < 100 && this.histo['fYaxis']['fXmax']/this.histo['fYaxis']['fXmin'] < 100) noexpy = true;

      var ndivx = this.histo['fXaxis']['fNdivisions'];
      var n1ax = ndivx%100;
      var n2ax = (ndivx%10000 - n1ax)/100;
      var n3ax = ndivx/10000;

      var ndivy = this.histo['fYaxis']['fNdivisions'];
      var n1ay = ndivy%100;
      var n2ay = (ndivy%10000 - n1ay)/100;
      var n3ay = ndivy/10000;

      /* X-axis label */
      var label = JSROOTPainter.translateLaTeX(this.histo['fXaxis']['fTitle']);
      var xAxisTitleFontSize = this.histo['fXaxis']['fTitleSize'] * h;
      var xAxisLabelOffset = 3 + (this.histo['fXaxis']['fLabelOffset'] * h);
      var xAxisLabelFontSize = this.histo['fXaxis']['fLabelSize'] * h;
      var xAxisFontDetails = getFontDetails(root_fonts[Math.floor(this.histo['fXaxis']['fTitleFont']/10)]);

      if (!('x_axis_label' in this)) 
         this['x_axis_label'] = 
            axis_frame.append("text").attr("class", "x_axis_label"); 
      
      this.x_axis_label
       .attr("x", w)
       .attr("y", h)
       .attr("text-anchor", "end")
       .attr("font-family", xAxisFontDetails['name'])
       .attr("font-weight", xAxisFontDetails['weight'])
       .attr("font-style", xAxisFontDetails['style'])
       .attr("font-size", xAxisTitleFontSize)
       .text(label)
       .attr("transform", "translate(0," + (xAxisLabelFontSize + xAxisLabelOffset * this.histo['fXaxis']['fTitleOffset'] + xAxisTitleFontSize) + ")");

      /* Y-axis label */
      label = JSROOTPainter.translateLaTeX(this.histo['fYaxis']['fTitle']);
      var yAxisTitleFontSize = this.histo['fYaxis']['fTitleSize'] * h;
      var yAxisLabelOffset = 3 + (this.histo['fYaxis']['fLabelOffset'] * w);
      var yAxisLabelFontSize = this.histo['fYaxis']['fLabelSize'] * h;
      var yAxisFontDetails = getFontDetails(root_fonts[Math.floor(this.histo['fYaxis']['fTitleFont'] /10)]);

      if (!('y_axis_label' in this)) 
         this['y_axis_label'] = 
            axis_frame.append("text").attr("class", "y_axis_label"); 

      this.y_axis_label
        .attr("x", 0)
        .attr("y", -yAxisLabelFontSize - yAxisTitleFontSize - yAxisLabelOffset * this.histo['fYaxis']['fTitleOffset'])
        .attr("font-family", yAxisFontDetails['name'])
        .attr("font-size", yAxisTitleFontSize)
        .attr("font-weight", yAxisFontDetails['weight'])
        .attr("font-style", yAxisFontDetails['style'])
        .attr("fill", "black")
        .attr("text-anchor", "end")
        .text(label)
        .attr("transform", "rotate(270, 0, 0)");

      var xAxisColor = this.histo['fXaxis']['fAxisColor'];
      var xDivLength = this.histo['fXaxis']['fTickLength'] * h;
      var yAxisColor = this.histo['fYaxis']['fAxisColor'];
      var yDivLength = this.histo['fYaxis']['fTickLength'] * w;

      var pthis = this;
      
      /*
       * Define the scales, according to the information from the pad
       */
      var dfx = d3.format(",.f"), dfy = d3.format(",.f");
      var xrange = this.histo['fXaxis']['fXmax'] - this.histo['fXaxis']['fXmin'];
      if (this.histo['fXaxis']['fTimeDisplay']) {
         if (n1ax > 8) n1ax = 8;
         var timeoffset = JSROOTPainter.getTimeOffset(this.histo['fXaxis']);
         dfx = d3.time.format("%Mm%S");
         if (xrange>31536000)
            dfx = d3.time.format("%Y");
         else if (xrange>2419200)
            dfx = d3.time.format("%Y/%m");
         else if (xrange>86400)
            dfx = d3.time.format("%Y/%m/%d");
         else if (xrange>3600)
            dfx = d3.time.format("%Hh%Mm%S");
         else if (xrange>60)
            dfx = d3.time.format("%Hh%M");

          this['x_axis'] = d3.svg.axis()
             .scale(this.x)
             .orient("bottom")
             .tickPadding(xAxisLabelOffset)
             .tickSize(-xDivLength, -xDivLength/2, -xDivLength/4)
             .tickFormat(function(d) {
                 var datime = new Date(timeoffset + (d * 1000));
                 return dfx(datime); })
             .ticks(n1ax);
      }
      else if (this.options.Logx) {
         this['x_axis'] = d3.svg.axis()
            .scale(this.x)
            .orient("bottom")
            .tickPadding(xAxisLabelOffset)
            .tickSize(-xDivLength, -xDivLength/2, -xDivLength/4)
            .tickFormat(function(d) { 
                var val = parseFloat(d);
                var vlog = Math.abs(JSROOTMath.log10(val));
                if (moreloglabelsx) {
                  if (vlog % 1 < 0.7 || vlog % 1 > 0.9999) {
                     if (noexpx) return val.toFixed();
                           else return JSROOTPainter.formatExp(val.toExponential(0));
                  }
                     else return null;
                }
                else {
                  if (vlog % 1 < 0.0001 || vlog % 1 > 0.9999) {
                    if (noexpx) return val.toFixed();
                           else return JSROOTPainter.formatExp(val.toExponential(0));
                  }
                  else return null;
                }
             });
      }
      else {
         this['x_axis'] = d3.svg.axis()
            .scale(this.x)
            .orient("bottom")
            .tickPadding(xAxisLabelOffset)
            .tickSubdivide(n2ax-1)
            .tickSize(-xDivLength, -xDivLength/2, -xDivLength/4)
            .tickFormat(function(d) {
                // avoid rounding problems around 0
                if ((Math.abs(d)<1e-14) && (Math.abs(xrange)>1e-5)) d = 0;
                return parseFloat(d.toPrecision(12));
             })
            .ticks(n1ax);
      }

      var yrange = this.histo['fYaxis']['fXmax'] - this.histo['fYaxis']['fXmin'];

      if (this.histo['fYaxis']['fTimeDisplay']) {
         if (n1ay > 8) n1ay = 8;
         var timeoffset = JSROOTPainter.getTimeOffset(this.histo['fYaxis']);
         dfy = d3.time.format("%Mm%S");

         if (yrange>31536000)
            dfy = d3.time.format("%Y");
         else if (yrange>2419200)
            dfy = d3.time.format("%Y/%m");
         else if (yrange>86400)
            dfy = d3.time.format("%Y/%m/%d");
         else if (yrange>3600)
            dfy = d3.time.format("%Hh%Mm%S");
         else if (yrange>60)
            dfy = d3.time.format("%Hh%M");

         this['y_axis'] = d3.svg.axis()
               .scale(this.y)
               .orient("left")
               .tickPadding(yAxisLabelOffset)
               .tickSize(-yDivLength, -yDivLength/2, -yDivLength/4)
               .tickFormat(function(d) {
                   var datime = new Date(timeoffset + (d * 1000));
                   return dfy(datime); })
               .ticks(n1ay);
      }
      else if (this.options.Logy) {
         this['y_axis'] = d3.svg.axis()
            .scale(this.y)
            .orient("left")
            .tickPadding(yAxisLabelOffset)
            .tickSize(-yDivLength, -yDivLength/2, -yDivLength/4)
            .tickFormat(function(d) { 
               var val = parseFloat(d);
               var vlog = Math.abs(JSROOTMath.log10(val));
               if (moreloglabelsy) {
                  if (vlog % 1 < 0.7 || vlog % 1 > 0.9999) {
                     if (noexpy) return val.toFixed();
                     else return JSROOTPainter.formatExp(val.toExponential(0));
                  }
                  else return null;
               }
               else {
                  if (vlog % 1 < 0.0001 || vlog % 1 > 0.9999) {
                     if (noexpy) return val.toFixed();
                     else return JSROOTPainter.formatExp(val.toExponential(0));
                  }
                  else return null;
               }});
      }
      else {
         if (n1ay >= 10) n1ay -= 2;
         this['y_axis'] = d3.svg.axis()
           .scale(this.y)
           .orient("left")
           .tickPadding(yAxisLabelOffset)
           .tickSubdivide(n2ay-1)
           .tickSize(-yDivLength, -yDivLength/2, -yDivLength/4)
           .tickFormat(function(d) {
              if ((Math.abs(d)<1e-14) && (Math.abs(yrange)>1e-5)) d = 0;
              return parseFloat(d.toPrecision(12));
           })
           .ticks(n1ay);
      }
      
      if (!('xax' in this))
         this['xax'] = axis_frame.append("svg:g")
                                 .attr("class", "xaxis");

      this.xax.attr("transform", "translate(0," + h + ")")
              .call(this.x_axis);

      if (!('yax' in this))
         this['yax'] = axis_frame.append("svg:g")
                                 .attr("class", "yaxis");

      this.yax.call(this.y_axis);

      var xAxisLabelFontDetails = getFontDetails(root_fonts[Math.floor(this.histo['fXaxis']['fLabelFont']/10)]);
      var yAxisLabelFontDetails = getFontDetails(root_fonts[Math.floor(this.histo['fXaxis']['fLabelFont']/10)]);

      this.xax.selectAll("text")
        .attr("font-family", xAxisLabelFontDetails['name'])
        .attr("font-size", xAxisLabelFontSize)
        .attr("font-weight", xAxisLabelFontDetails['weight'])
        .attr("font-style", xAxisLabelFontDetails['style']);
      this.yax.selectAll("text")
        .attr("font-family", yAxisLabelFontDetails['name'])
        .attr("font-size", yAxisLabelFontSize)
        .attr("font-weight", yAxisLabelFontDetails['weight'])
        .attr("font-style", yAxisLabelFontDetails['style']);
   }

   
   JSROOTPainter.HistPainter.prototype.DrawTitle = function() {
      var w = this.vis.attr("width"), h = this.vis.attr("height");
      var font_size = Math.round(0.050 * h);
      var l_title = JSROOTPainter.translateLaTeX(this.histo['fTitle']);
      
      if (!this.pad || typeof(this.pad) == 'undefined') {
         if (!('draw_title' in this))
            this['draw_title'] = this.vis.append("text").attr("class", "title");

         this.draw_title
            .attr("text-anchor", "middle")
            .attr("x", w/2)
            .attr("y", 0.07 * this.vis.attr("height"))
            .attr("font-family", "Arial")
            .attr("font-size", font_size)
            .text(l_title);
      }
   }
   
   JSROOTPainter.HistPainter.prototype.ToggleStat = function() {
      var stat = this.HasStat();
      
      if (stat == null) {
         
         // when histogram created first time, one need to draw it 
         stat = this.CreateStat();
         
         JSROOTPainter.DrawPaveTextNew(this.vis, stat);
         
         return;
      }
      
      if (!('_Enabled' in stat)) {
         stat['_Enabled'] = true;
      }
      
      if (stat._Enabled) stat._Enabled = false;
                    else stat._Enabled = true;
      
      this.RedrawFrame(stat);
   }
   
   JSROOTPainter.HistPainter.prototype.HasStat = function() {
      if (!('fFunctions' in this.histo)) return null;

      for (i=0; i<this.histo['fFunctions'].length; ++i) {
         
         var func = this.histo['fFunctions'][i];
         
         if (func['_typename'] == 'JSROOTIO.TPaveText' ||
             func['_typename'] == 'JSROOTIO.TPaveStats') {
            return func;
         }
      }

      return null;
   }
   
   
   JSROOTPainter.HistPainter.prototype.CreateStat = function() {
      
      if (!this.draw_content) return null;
      if (this.HasStat() != null) return null;
      
      var stats = {};
      stats['_typename'] = 'JSROOTIO.TPaveStats';
      
      stats['_AutoCreated'] = true;
      stats['fX1NDC'] = gStyle.StatX;
      stats['fY1NDC'] = gStyle.StatY;
      stats['fX2NDC'] = gStyle.StatX + gStyle.StatW;
      stats['fY2NDC'] = gStyle.StatY + gStyle.StatH;

      stats['fOptFit'] = 0;
      stats['fOptStat'] = gStyle.OptStat;
      stats['fLongest'] = 17;
      stats['fMargin'] = 0.05;

      stats['fBorderSize'] = 1;
      stats['fInit'] = 1;
      stats['fShadowColor'] = 1;
      stats['fCornerRadius'] = 0;

      stats['fX1'] = 1;
      stats['fY1'] = 100;
      stats['fX2'] = 1;
      stats['fY2'] = 100;

      stats['fResizing'] = false;
      stats['fUniqueID'] = 0;
      stats['fBits'] = 0x03000009;
      stats['fLineColor'] = 1;
      stats['fLineStyle'] = 1;
      stats['fLineWidth'] = 1;

      stats['fFillColor'] = gStyle.StatColor;
      stats['fFillStyle'] = gStyle.StatStyle;
      
      stats['fTextAngle'] = 0;
      stats['fTextSize'] = gStyle.StatFontSize;
      stats['fTextAlign'] = 12;
      stats['fTextColor'] = gStyle.StatTextColor;
      stats['fTextFont'] = gStyle.StatFont;
      
      stats['fLines'] = new Array;

      stats['fLines'].push({'fTitle': "Name", "fTextColor": 1});
      stats['fLines'].push({'fTitle': "Entries = 4075", "fTextColor": 1});
      stats['fLines'].push({'fTitle': "Mean = 2000", "fTextColor": 1});
      stats['fLines'].push({'fTitle': "RMS = 3000", "fTextColor": 1});

      stats['fLines'][0]['fTitle'] = this.histo['fName']; 
      
      if (!'fFunctions' in this.histo) this.histo['fFunctions'] = {};

      this.histo['fFunctions'].push(stats);
      
      return stats;
   }
   
   JSROOTPainter.HistPainter.prototype.DrawFunctions = function() {
      
      /// draw statistics box & other TPaveTexts, which are belongs to histogram
      
      // should be called once to create all painters, which are than updated separately
      
      if (!('fFunctions' in this.histo)) return;
      
      for (i=0; i<this.histo['fFunctions'].length; ++i) {
          
         var func = this.histo['fFunctions'][i];
         
          if (func['_typename'] == 'JSROOTIO.TPaveText' ||
              func['_typename'] == 'JSROOTIO.TPaveStats') {
               if (func['fX1NDC'] < 1.0 && func['fY1NDC'] < 1.0 &&
                   func['fX1NDC'] > 0.0 && func['fY1NDC'] > 0.0) {
                  JSROOTPainter.DrawPaveTextNew(this.vis, func);
               }
            }
          
          if (func['_typename'] == 'JSROOTIO.TF1') {
            if ((!this.pad && !func.TestBit(kNotDraw)) ||
               (this.pad && func.TestBit(EStatusBits.kObjInCanvas)))
                  JSROOTPainter.drawFunctionNew(this.vis, this.pad, func, this.frame);
         }
      }
   }
   
   JSROOTPainter.HistPainter.prototype.Redraw = function() {
//      if (console) console.time("XY");
      this.CreateXY();
//      if (console) console.timeEnd("XY");

//    if (console) console.time("Stat");
      this.CountStat();
//    if (console) console.time("Stat");
      
//      if (console) console.time("Grid");
      this.DrawGrids();
//      if (console) console.timeEnd("Grid");

//      if (console) console.time("Bins");
      this.DrawBins();
//      if (console) console.timeEnd("Bins");

//      if (console) console.time("Axes");
      this.DrawAxes();
//      if (console) console.timeEnd("Axes");
      
//      if (console) console.time("Title");
      this.DrawTitle();
//      if (console) console.timeEnd("Title");
   }
   
   JSROOTPainter.HistPainter.prototype.AddInteractive = function() {
      // only first painter in list allowed to add interactive functionality to the main pad
      if (this.first) return;

//      if (!this.draw_content) return;
      
      var width = this.frame.attr("width"), height = this.frame.attr("height");
      var e, origin, rect;
      
      var zoom = d3.behavior.zoom().x(this.x).y(this.y);
      this.frame.on("touchstart", startRectSel);
      this.frame.on("mousedown", startRectSel);
      if (gStyle.Tooltip == 1) {
         this.frame.on("mousemove", moveTooltip);
         this.frame.on("mouseout", finishTooltip);
      }
//      this.frame.on("mouseover", finishTooltip);
      this.frame.on("contextmenu", showContextMenu);
      
      var pthis = this;
      
      var mycnt = 0;
      
      var tool_tmout = null;
      var tool_pos = null;
      var tool_changed = false;
      var tool_moving_inside = false;
      var tool_visible = false;
      
      var tool_rec = this.frame.append("svg:rect")
         .attr("class", "tooltipbox")
         .attr("fill", "black")
         .style("opacity", "0")
         .style("stroke", "black")
         .style("stroke-width", 1);

//      var tool_text = this.frame.append("svg:text")
//          .attr("class", "tooltipbox")
//          .style("opacity", "0");

      
      function checkTooltip() {
         tool_tmout = null;

         if (tool_visible) return;

         if (tool_changed) {
            // restart timer, hope it will not changes next time

            // during last changes cursor leave area and we could ignore  
            if (!tool_moving_inside) return;
            
            tool_changed = false;
            tool_tmout = setTimeout(checkTooltip, 1000);
            return;
         }

         var tip = {};
         tip['x'] = pthis.x.invert(tool_pos[0]);
         tip['y'] = pthis.y.invert(tool_pos[1]);
         tip['text'] = new Array;
         
         pthis.CollectTooltips(tip);
         
         pthis.frame.selectAll(".tips").remove();
         
         // set text position
          pthis.frame.append("svg:text")
            .attr("class", "tips")
            .style("opacity", "0")
            .attr("x", tool_pos[0])
            .attr("y", tool_pos[1])
            .text("x:"+ tip.x.toPrecision(3));

         pthis.frame.append("svg:text")
            .attr("class", "tips")
            .style("opacity", "0")
            .attr("x", tool_pos[0])
            .attr("y", tool_pos[1] + 15)
            .text("y: " + tip.y.toPrecision(3));
         
         for (var n=0;n<tip.text.length;n++)
            pthis.frame.append("svg:text")
              .attr("class", "tips")
              .style("opacity", "0")
              .attr("x", tool_pos[0])
              .attr("y", tool_pos[1] + 30 + n*15)
              .text(tip.text[n]);
         
         pthis.frame.selectAll(".tips").transition().duration(500).style("opacity", 1);
         
         if (('x1' in tip) && ('y1' in tip)) {
            var x1 = pthis.x(tip.x1);
            var x2 = pthis.x(tip.x2);
            var y1 = pthis.y(tip.y1);
            var y2 = pthis.y(tip.y2); if (y2>height) y2 = height;
                 
            tool_rec.attr("x", x1)
                    .attr("y", y1)
                    .attr("width", (x2-x1 > 2) ? x2-x1 : 2)
                    .attr("height", Math.abs(y2-y1));
//                    .append("svg:title")
//                    .attr("timeout",0)
//                    .text("my\nmulti\nline\ntext");
               
            tool_rec.transition().duration(500).style("opacity", 0.3);
//            pthis.frame.on('mouseover')();
         }
         
//         tool_text
//          .attr("x", tool_pos[0])
//          .attr("y", tool_pos[1])
//          .text(tip_txt);
         
         
//         $("#report").append("  show tooltip");
         
         tool_tmout = setTimeout(closeTooltip, 3000);
         
         tool_visible = true;
      }
      
      function closeTooltip(force) {
         if (tool_visible) {
            if (force) {
               tool_rec.style("opacity", 0);
               pthis.frame.selectAll(".tips").style("opacity", 0);
            } else {
               tool_rec.transition().duration(1000).style("opacity", 0);
               pthis.frame.selectAll(".tips").transition().duration(1000).style("opacity", 0);
            }
            // $("#report").append("  hide tooltip");
            tool_visible = false;
         }
         clearTimeout(tool_tmout);
         tool_tmout = null;
      }
      
      function moveTooltip() {

         tool_moving_inside = true;
         
         // one could detect if mouse moved too far away, disable it faster
         if (tool_visible) return;
         
         tool_pos = d3.mouse(this);
         
         if (tool_tmout == null) {
            tool_changed = false;
            tool_tmout = setTimeout(checkTooltip, 300);
         } else
            tool_changed = true;
      }

      function finishTooltip() {
         tool_moving_inside = false;
      }
      
      function showContextMenu() {

         d3.event.preventDefault();

         $("#dialog").empty();
         
         pthis.FillContextMenu($("#dialog"));
         
         $("#dialog").data("Painter", pthis);
         //$("#dialog").data("Frame", vis);
         $("#dialog").data("shown", true);
         
         $("#dialog").dialog({ 
            title: pthis.histo['fName'],
            closeOnEscape: true,
            autoOpen: false,
            resizable: false,
            modal: false,
            width: "auto",
            height: "auto",
            position : {my: "left+3 top+3", of: d3.event, collision:"fit"}  
         });
         
         $( "#dialog" ).dialog("open");
       }
      
      function startRectSel() {
         d3.event.preventDefault();
         
         if ($("#dialog").data("shown")) {
            $( "#dialog" ).dialog("close");
            $( "#dialog" ).empty();
         }
         
         closeTooltip(true);
         
         pthis.frame.select("#zoom_rect").remove();
         e = this;
         var t = d3.event.changedTouches;
         origin = t ? d3.touches(e, t)[0] : d3.mouse(e);
         rect = pthis.frame.append("rect").attr("class", "zoom").attr("id", "zoom_rect");
         d3.select("body").classed("noselect", true);
         d3.select("body").style("-webkit-user-select", "none");
         
//         $("#report").append("<br> Start select x:" + origin[0] + "  y:" + origin[1]);
         
         origin[0] = Math.max(0, Math.min(width, origin[0]));
         origin[1] = Math.max(0, Math.min(height, origin[1]));
         
//         $("#report").append("<br> Start select x:" + origin[0] + "  y:" + origin[1]);
         
         pthis.frame.on("dblclick", unZoom);
         d3.select(window)
            .on("mousemove.zoomRect", moveRectSel)
            .on("mouseup.zoomRect", endRectSel, true);
         d3.select(window)
            .on("touchmove.zoomRect", moveRectSel)
            .on("touchend.zoomRect", endRectSel, true);
         d3.event.stopPropagation();
      };

      function unZoom() {
         d3.event.preventDefault();

         pthis.Unzoom(true,true);
      };

      function moveRectSel() {
         d3.event.preventDefault();
         var t = d3.event.changedTouches;
         var m = t ? d3.touches(e, t)[0] : d3.mouse(e);
         m[0] = Math.max(0, Math.min(width, m[0]));
         m[1] = Math.max(0, Math.min(height, m[1]));
         rect.attr("x", Math.min(origin[0], m[0]))
             .attr("y", Math.min(origin[1], m[1]))
             .attr("width", Math.abs(m[0] - origin[0]))
             .attr("height", Math.abs(m[1] - origin[1]));
      };

      function endRectSel() {
         d3.event.preventDefault();
         d3.select(window).on("touchmove.zoomRect", null).on("touchend.zoomRect", null);
         d3.select(window).on("mousemove.zoomRect", null).on("mouseup.zoomRect", null);
         d3.select("body").classed("noselect", false);
         var t = d3.event.changedTouches;
         var m = t ? d3.touches(e, t)[0] : d3.mouse(e);
         
//         $("#report").append("<br> End select x:" + m[0] + "  y:" + m[1]);
         
         m[0] = Math.max(0, Math.min(width, m[0]));
         m[1] = Math.max(0, Math.min(height, m[1]));

//         $("#report").append("<br> End select x:" + m[0] + "  y:" + m[1]);

         d3.select("body").style("-webkit-user-select", "auto");
         
         rect.remove();
         rect = null;
         
         if ((Math.abs(m[0] - origin[0]) < 10) || (Math.abs(m[1] - origin[1]) < 10)) return;

         pthis.Zoom(Math.min(pthis.x.invert(origin[0]), pthis.x.invert(m[0])),
                    Math.max(pthis.x.invert(origin[0]), pthis.x.invert(m[0])),
                    Math.min(pthis.y.invert(origin[1]), pthis.y.invert(m[1])),
                    Math.max(pthis.y.invert(origin[1]), pthis.y.invert(m[1])));
      }
   }
   


   JSROOTPainter.HistPainter.prototype.FillContextMenu = function(menu)
   {
      JSROOTPainter.ObjectPainter.prototype.FillContextMenu.call(this, menu);
      if (this.draw_content)
         this.AddMenuItem(menu,"Toggle stat","togstat");
   }
      
   JSROOTPainter.HistPainter.prototype.ExeContextMenu = function(cmd) {
      if (cmd == "togstat") {
         this.ToggleStat();
         return;
      } 
      
      JSROOTPainter.ObjectPainter.prototype.ExeContextMenu.call(this, cmd);
   } 

   
   // ======= TH1 painter =======================================================
   
   JSROOTPainter.Hist1DPainter = function(histo) {
      JSROOTPainter.HistPainter.call(this, histo);
   }

   JSROOTPainter.Hist1DPainter.prototype = Object.create( JSROOTPainter.HistPainter.prototype );


   JSROOTPainter.Hist1DPainter.prototype.CreateBins = function() {
      
      // from here we analyze object content
      // therefore code will be moved 
      this.fillcolor = root_colors[this.histo['fFillColor']];
      this.linecolor = root_colors[this.histo['fLineColor']];

      if (this.histo['fFillColor'] == 0) this.fillcolor = '#4572A7';
      if (this.histo['fLineColor'] == 0) this.linecolor = '#4572A7';
      
      this.hmin = 1.0e32;
      this.hmax = -1.0e32;
      this.stat_entries = d3.sum(this.histo['fArray']);
      
      for (var i=0;i<this.histo['fXaxis']['fNbins'];++i) {
         var value = this.histo.getBinContent(i+1);
         if (value < this.hmin) this.hmin = value;
         if (value > this.hmax) this.hmax = value;
      }
      var mul = (this.hmin < 0) ? 1.05 : 1.0;
      
      // used in CreateXY and tooltip providing
      this.xmin = this.histo['fXaxis']['fXmin'];
      this.xmax = this.histo['fXaxis']['fXmax'];
      
      this.ymin = this.histo['fYaxis']['fXmin'];
      this.ymax = this.histo['fYaxis']['fXmax'];
      
      if ((this.histo['fXaxis']['fNbins']==0) || ((Math.abs(this.hmin) < 1e-300 && Math.abs(this.hmax) < 1e-300))) {
         if (this.histo['fMinimum'] != -1111) this.ymin = this.histo['fMinimum'];
         if (this.histo['fMaximum'] != -1111) this.ymax = this.histo['fMaximum'];
         this.draw_content = false;
      } else {
         if (this.histo['fMinimum'] != -1111) this.hmin = this.histo['fMinimum'];
         if (this.histo['fMaximum'] != -1111) this.hmax = this.histo['fMaximum'];
         this.ymin = this.hmin * mul;
         this.ymax = this.hmax * 1.05;
         this.draw_content = true;
      }
      
      // If no any draw options specified, do not try draw histogram
      if (this.options.Bar == 0 && this.options.Hist == 0) {
         this.draw_content = false;
      }
      
      if (!this.draw_content) {
         this['bins'] = null;
         return;
      }

      // TODO: reduce bins size to actual number of pixels drawn
      
      // TODO: later x coordinate one should extract from axis array - it may be non-linear
      
      this['binwidth'] = ((this.xmax - this.xmin) / this.histo['fXaxis']['fNbins']);
      
      var pthis = this;
      
      this['bins'] = d3.range(this.histo['fXaxis']['fNbins']+1).map(function(p) {
         var offset = (pthis.options.Error > 0) ? (p * pthis.binwidth) - (pthis.binwidth / 2.0) : (p * pthis.binwidth);
         return {
            x:  pthis.histo['fXaxis']['fXmin'] + offset,
            y:  pthis.histo.getBinContent(p),
            xerr: pthis.binwidth / 2.0,
            yerr: pthis.histo.getBinError(p)
         };
      });
   }
   
   JSROOTPainter.Hist1DPainter.prototype.GetLeftIndex = function() {
      // be aware - here index starts from 0
      var left = 0;
      var obj = this.first;
      if (obj==null) obj = this;
      
      if (obj.zoom_xmin != obj.zoom_xmax) 
         left = Math.floor((obj.zoom_xmin - this.xmin) / this.binwidth);
      
      if (left<0) left = 0;
      
      return left; 
   }
   
   JSROOTPainter.Hist1DPainter.prototype.GetRightIndex = function(add) {
      // be aware - here index starts from 0, right is not included
      var right = this.histo['fXaxis']['fNbins'];
      var obj = this.first;
      if (obj==null) obj = this;
      if (obj.zoom_xmin != obj.zoom_xmax) {
         right = Math.round((obj.zoom_xmax - this.xmin) / this.binwidth + 0.5 + (add ? add : 0));
         if (right > this.histo['fXaxis']['fNbins']) right = this.histo['fXaxis']['fNbins']; 
      }
      return right;
   }

   JSROOTPainter.Hist1DPainter.prototype.getX = function(i) {
      return this.x(this.xmin + i*this.binwidth);
   }

   JSROOTPainter.Hist1DPainter.prototype.getY = function(i) {
      if ((i<=0) || (i>this.histo['fXaxis']['fNbins'] - 1)) this.y(0.);
      return this.y(this.histo.getBinContent(i+1));
   }

   JSROOTPainter.Hist1DPainter.prototype.CountStat = function()
   {
      this.stat_sum0 = 0;
      this.stat_sum1 = 0;
      this.stat_sum2 = 0;

      var left = this.GetLeftIndex();
      var right = this.GetRightIndex();
      
      // $("#report").append("<br> count statistic " + left + "  " + right);
      
      for (var i=left;i<right;i++) {
         var xx = this.xmin + i*this.binwidth;
         var yy = this.histo.getBinContent(i+1);
         this.stat_sum0 += yy;
         this.stat_sum1 += xx * yy;
         this.stat_sum2 += xx * xx * yy;
      }
   }
   
   JSROOTPainter.Hist1DPainter.prototype.GetStat = function(name) {
      // probably, can keep general for all TH kinds
      
      var pthis = this;
      
      var res = 0;
      var digits = 0;
      
      if (name == "Entries") { res = this.stat_entries; digits = 0; } 

      if (name == "Mean") {
         if (this.stat_sum0 > 0) res = this.stat_sum1/this.stat_sum0;
         digits = 2;
      }

      if (name == "RMS") {
         if (this.stat_sum0 > 0) 
            res = Math.sqrt(this.stat_sum2/this.stat_sum0 - Math.pow(this.stat_sum1/this.stat_sum0, 2));
         digits = 3;
      }
      
      if (name == "Underflow") {
         if (this.histo['fArray'].length > 0) res = this.histo['fArray'][0];
         digits = 0;
      }

      if (name == "Overflow") {
         if (this.histo['fArray'].length > 0) res = this.histo['fArray'][this.histo['fArray'].length-1];
         digits = 0;
      }

      if (name == "Integral") { res = this.stat_sum0; digits = 0; }
      
      if (res>1e8) return res.toExponential(3);

      return res.toFixed(digits);
   }

   
   JSROOTPainter.Hist1DPainter.prototype.DrawErrors = function() {
      var w = this.svg_frame.attr("width"), h = this.svg_frame.attr("height");
      /* Add a panel for each data point */
      var info_marker = getRootMarker(root_markers, this.histo['fMarkerStyle']);
      var shape = info_marker['shape'], filled = info_marker['toFill'],
          toRotate = info_marker['toRotate'], marker_size = this.histo['fMarkerSize'] * 32;

      if (this.histo['fMarkerStyle'] == 1) marker_size = 1;

      var marker = d3.svg.symbol()
          .type(d3.svg.symbolTypes[shape])
          .size(marker_size);

      var pthis = this;

      function TooltipText(d) {
         return "x = " + d.x.toPrecision(4) + " \ny = " + d.y.toPrecision(4) +
                " \nerror x = " + d.xerr.toPrecision(4) +
                " \nerror y = " + d.yerr.toPrecision(4);
      }

      /* Draw x-error indicators */
      var xerr = this.bins_g.selectAll("error_x")
            .data(this.bins)
            .enter()
            .append("svg:line")
            .attr("x1", function(d) { return pthis.x(d.x-d.xerr)} )
            .attr("y1", function(d) { return pthis.y(d.y)} )
            .attr("x2", function(d) { return pthis.x(d.x+d.xerr)} )
            .attr("y2", function(d) { return pthis.y(d.y)} )
            .style("stroke", root_colors[this.histo['fLineColor']])
            .style("stroke-width", this.histo['fLineWidth']);

      if (this.options.Error == 11) {
           this.bins_g.selectAll("e1_x")
               .data(this.bins)
               .enter()
               .append("svg:line")
               .attr("y1", function(d) { return pthis.y(d.y)-3} )
               .attr("x1", function(d) { return pthis.x(d.x-d.xerr)})
               .attr("y2", function(d) { return pthis.y(d.y)+3})
               .attr("x2", function(d) { return pthis.x(d.x-d.xerr)})
               .style("stroke", root_colors[this.histo['fLineColor']])
               .style("stroke-width", this.histo['fLineWidth']);
           this.bins_g.selectAll("e1_x")
               .data(this.bins)
               .enter()
               .append("svg:line")
               .attr("y1", function(d) { return pthis.y(d.y)-3} )
               .attr("x1", function(d) { return pthis.x(d.x+d.xerr) })
               .attr("y2", function(d) { return pthis.y(d.y)+3})
               .attr("x2", function(d) { return pthis.x(d.x+d.xerr) })
               .style("stroke", root_colors[this.histo['fLineColor']])
               .style("stroke-width", this.histo['fLineWidth']);
      }
         /* Draw y-error indicators */
      var yerr = this.bins_g.selectAll("error_y")
            .data(this.bins)
            .enter()
            .append("svg:line")
            .attr("x1", function(d) { return pthis.x(d.x)})
            .attr("y1", function(d) { return pthis.y(d.y-d.yerr) })
            .attr("x2", function(d) { return pthis.x(d.x)})
            .attr("y2", function(d) { return pthis.y(d.y+d.yerr) })
            .style("stroke", root_colors[this.histo['fLineColor']])
            .style("stroke-width", this.histo['fLineWidth']);

      if (this.options.Error == 11) {
         this.bins_g.selectAll("e1_y")
               .data(this.bins)
               .enter()
               .append("svg:line")
               .attr("x1", function(d) { return pthis.x(d.x)-3})
               .attr("y1", function(d) { return pthis.y(d.y-d.yerr) })
               .attr("x2", function(d) { return pthis.x(d.x)+3})
               .attr("y2", function(d) { return pthis.y(d.y-d.yerr) })
               .style("stroke", root_colors[this.histo['fLineColor']])
               .style("stroke-width", this.histo['fLineWidth']);
         this.bins_g.selectAll("e1_y")
               .data(this.bins)
               .enter()
               .append("svg:line")
               .attr("x1", function(d) { return pthis.x(d.x)-3})
               .attr("y1", function(d) { return pthis.y(d.y+d.yerr) })
               .attr("x2", function(d) { return pthis.x(d.x)+3})
               .attr("y2", function(d) { return pthis.y(d.y+d.yerr) })
               .style("stroke", root_colors[this.histo['fLineColor']])
               .style("stroke-width", this.histo['fLineWidth']);
      }
      var marks = this.bins_g.selectAll("markers")
            .data(this.bins)
            .enter()
            .append("svg:path")
            .attr("class", "marker")
            .attr("transform", function(d) {
               return "translate(" + pthis.x(d.x) + "," + pthis.y(d.y) + ")"
             })
            .style("fill", root_colors[this.histo['fMarkerColor']])
            .style("stroke", root_colors[this.histo['fMarkerColor']])
            .attr("d", marker);
      
      
      if (gStyle.Tooltip > 1) {
         marks.append("svg:title").text(TooltipText);
         xerr.append("svg:title").text(TooltipText);
         yerr.append("svg:title").text(TooltipText);
      }
   }

   
   JSROOTPainter.Hist1DPainter.prototype.DrawBins = function() {

      // TODO: limit number of drawn by number of visible pixels
      // one could select every second bin, for instance

      
      if (!this.draw_content) return;

      if (!('bins_id' in this))
         this['bins_id'] = "random_bins_id" + random_id++;
      
      d3.selectAll("#" + this.bins_id).remove();
         
      this['bins_g'] = this.svg_frame.append("svg:g").attr("id", this.bins_id);
      
      if (this.options.Error > 0) { this.DrawErrors(); return; }
      
      var pthis = this;
      
      if ((this.histo['fFillStyle'] < 4000 || this.histo['fFillStyle'] > 4100) && this.histo['fFillColor'] != 0) {

         // histogram filling 
         var area = d3.svg.area()
            .x(function(d) { return pthis.x(d.x);})
            .y0(function(d) { return pthis.y(0);})
            .y1(function(d) { return pthis.y(d.y);})
            .interpolate("step-before");
            
         if (this.histo['fFillStyle'] > 3000 && this.histo['fFillStyle'] <= 3025) {
            createFillPatterns(this.vis, this.histo['fFillStyle'], this.histo['fFillColor']);

            this.bins_g.append("svg:path")
               .attr("d", area(this.bins))
               .style("stroke", this.linecolor)
               .style("stroke-width", this.histo['fLineWidth'])
               .style("fill", "url(#pat" + this.histo['fFillStyle'] + "_" + this.histo['fFillColor'] + ")")
               .style("antialias", "false");
         }
         else {
            this.bins_g.append("svg:path")
               .attr("d", area(this.bins))
               .style("stroke", this.linecolor)
               .style("stroke-width", this.histo['fLineWidth'])
               .style("fill", this.fillcolor)
               .style("antialias", "false");
         }
      }
      else {
         
         // lets try to build line without any intermediate bins array
         
         var left = this.GetLeftIndex();
         var right = this.GetRightIndex(1);
         
         // histogram contour lines only 
         var line = d3.svg.line()
            .x(function(d,i) { return pthis.getX(left+i+1);} )
            .y(function(d,i) { return pthis.getY(left+i);} )
            .interpolate("step-before");

         this.bins_g.append("svg:path")
            .attr("d", line(d3.range(right-left+1))) // to draw one bar, one need two points
            .style("stroke", this.linecolor)
            .style("stroke-width", this.histo['fLineWidth'])
            .style("fill", "none")
            .style("stroke-dasharray", this.histo['fLineStyle'] > 1 ? root_line_styles[this.histo['fLineStyle']] : null)
            .style("antialias", "false");
      }
      
      if (gStyle.Tooltip > 1) {
         // TODO: limit number of tooltips by number of visible pixels
         var selwidth = this.x(2*this.binwidth)-this.x(this.binwidth);
         if (selwidth<2) selwidth = 1; 
         this.bins_g.selectAll("selections")
            .data(this.bins)
            .enter()
            .append("svg:line")
            .attr("x1", function(d) { return pthis.x(d.x-d.xerr) } )
            .attr("y1", function(d) { return pthis.y(d.y) } )
            .attr("x2", function(d) { return pthis.x(d.x-d.xerr) } )
            .attr("y2", function(d) { return pthis.y(0) } )
            .attr("opacity", 0)
            .style("stroke", "#4572A7")
            .style("stroke-width", selwidth)
            .on('mouseover', function() { d3.select(this).transition().duration(100).style("opacity", 0.3) } )
            .on('mouseout', function() { d3.select(this).transition().duration(100).style("opacity", 0) } )
            .append("svg:title").text(function(d) { return "x = [" + (d.x-(2*d.xerr)).toPrecision(4) + 
                    ", " + d.x.toPrecision(4) + "] \nentries = " + d.y;
            });

      }
      
   }

   JSROOTPainter.Hist1DPainter.prototype.ProvideTooltip = function(tip)
   {
      if (!this.draw_content) return;

      if (!('bins' in this) || (this.bins==null)) return;
      
      var nbin = Math.round((tip.x - this.xmin)/this.binwidth - 0.5);
      if ((nbin<0) || (nbin>=this.histo['fXaxis']['fNbins'])) return;

      var value = this.histo.getBinContent(nbin+1);

      var dist = value - tip.y;
      
      // cursor should be under hist line
      if (dist <= 0) return;

      // if somebody provides tooltip, which is closer to tip, than ignore our
      if (('dist' in tip) && (tip.dist< dist)) return;
      
      tip['empty'] = false;
      
      tip['dist'] = dist; 
      tip['text'].push("histo: "+this.histo['fName']);
      tip['text'].push("bin: "+ nbin);
      tip['text'].push("cont: " + value.toPrecision(4)); 
      
      tip['x1'] = this.xmin + this.binwidth*nbin;
      tip['x2'] = tip['x1'] + this.binwidth;
      tip['y1'] = value;
      tip['y2'] = this.ymin;
      
      // basic method, painter can provide tooltip at specified coordinates
      // range.x1 .. range.x2, range.y1 .. range.y2
   }

   
   JSROOTPainter.drawHistogram1Dnew = function(vis, pad, histo, hframe) {

      // create painter and add it to canvas
      var painter = new JSROOTPainter.Hist1DPainter(histo);

      painter.SetFrame(vis, pad, hframe);

//      if (console) console.time("CreateBins");

      painter.CreateBins();

//      if (console) console.timeEnd("CreateBins");
      
      painter.CreateXY();
      
      painter.CountStat();

      painter.DrawGrids();

      painter.DrawBins();

      painter.DrawAxes();
      
      //$("#report").append("<br> title");
      painter.DrawTitle();

      //$("#report").append("<br> stat");
      if (gStyle.AutoStat && (hframe==null)) painter.CreateStat();

      //$("#report").append("<br> func");
      painter.DrawFunctions();

      //$("#report").append("<br> interact");
      painter.AddInteractive();
      
      return painter;
   }
   
   // ==================== painter for TH2 histograms ==============================
   
   JSROOTPainter.Hist2DPainter = function(histo) {
      JSROOTPainter.HistPainter.call(this, histo);
      this.is3D = false;
   }

   JSROOTPainter.Hist2DPainter.prototype = Object.create( JSROOTPainter.HistPainter.prototype );

   
   JSROOTPainter.Hist2DPainter.prototype.FillContextMenu = function(menu)
   {
      JSROOTPainter.HistPainter.prototype.FillContextMenu.call(this, menu);
      if (this.is3D)
         this.AddMenuItem(menu,"Draw in 2D","draw2d");
      else
         this.AddMenuItem(menu,"Draw in 3D","draw3d");
      this.AddMenuItem(menu,"Toggle col","col");

   }
      
   JSROOTPainter.HistPainter.prototype.ExeContextMenu = function(cmd) {
      if (cmd == "draw2d") {
         this.is3D = false;
         this.RedrawFrame();
         return;
      } 

      if (cmd == "draw3d") {
         this.is3D = true;
         this.RedrawFrame();
         return;
      } 
      
      if (cmd == "col") {
         if (this.options.Color > 0) 
            this.options.Color = 0;
         else
            this.options.Color = 1;
         this.RedrawFrame();
         return;
      }

      JSROOTPainter.ObjectPainter.prototype.ExeContextMenu.call(this, cmd);
   } 


   JSROOTPainter.Hist2DPainter.prototype.CreateBins = function() {
      this.fillcolor = root_colors[this.histo['fFillColor']];
      this.linecolor = root_colors[this.histo['fLineColor']];

      if (this.histo['fFillColor'] == 0) this.fillcolor = '#4572A7';
      if (this.histo['fLineColor'] == 0) this.linecolor = '#4572A7';

      this.nbinsx = this.histo['fXaxis']['fNbins'];
      this.nbinsy = this.histo['fYaxis']['fNbins'];
      
      // used in CreateXY method
      this.xmin = this.histo['fXaxis']['fXmin'];
      this.xmax = this.histo['fXaxis']['fXmax']; 
      this.ymin = this.histo['fYaxis']['fXmin'];
      this.ymax = this.histo['fYaxis']['fXmax']; 
      
      this.scalex = (this.xmax - this.xmin) / this.nbinsx;
      this.scaley = (this.ymax - this.ymin) / this.nbinsy;
      
      this.maxbin = d3.max(this.histo['fArray']);
      this.minbin = d3.min(this.histo['fArray']);
      this.bins = new Array();
      for (var i=0; i<this.nbinsx; ++i) {
         for (var j=0; j<this.nbinsy; ++j) {
            var bin_content = this.histo.getBinContent(i+1, j+1);
            if (bin_content > this.minbin) {
               var point = {
                  x:this.xmin + (i*this.scalex),
                  y:this.ymin + (j*this.scaley),
                  z:bin_content
               };
               this.bins.push(point);
            }
         }
      }
   }
   
   JSROOTPainter.Hist2DPainter.prototype.CountStat = function()
   {
      var obj = this.first;
      if (obj==null) obj = this;

      var select_x = (obj.zoom_xmin != obj.zoom_xmax);
      var select_y = (obj.zoom_xmin != obj.zoom_xmax);
      // TODO: statistic filling
   }

   JSROOTPainter.Hist2DPainter.prototype.getValueColor = function(zc) {
      var wmin = this.minbin, wmax = this.maxbin; 
          wlmin = wmin, wlmax = wmax;
      var ndivz = this.histo['fContour'].length;
      if (ndivz<16) ndivz = 16;
      var scale = ndivz / (wlmax - wlmin);
      if (this.options.Logz) {
         if (wmin <= 0 && wmax > 0) wmin = Math.min(1.0, 0.001 * wmax);
         wlmin = Math.log(wmin)/Math.log(10);
         wlmax = Math.log(wmax)/Math.log(10);
      }
   
      if (default_palette.length == 0) {
         var saturation = 1,
             lightness = 0.5,
             maxHue = 280,
             minHue = 0,
             maxPretty = 50;
         for (var i=0 ; i<maxPretty ; i++) {
            var hue = (maxHue - (i + 1) * ((maxHue - minHue) / maxPretty))/360.0;
            var rgbval = JSROOTPainter.HLStoRGB(hue, lightness, saturation);
            default_palette.push(rgbval);
         }
      }
      if (this.options.Logz) zc = Math.log(zc)/Math.log(10);
      if (zc < wlmin) zc = wlmin;
      var ncolors = default_palette.length
      var color = Math.round(0.01 + (zc - wlmin) * scale);
      var theColor = Math.round((color + 0.99) * ncolors / ndivz);
      var icol = theColor % ncolors;
      if (icol < 0) icol = 0;

      return default_palette[icol];
   }
   
   JSROOTPainter.Hist2DPainter.prototype.DrawBins = function() 
   {
      if (!('bins_id' in this))
         this['bins_id'] = "random_bins_id" + random_id++;
      
      d3.selectAll("#" + this.bins_id).remove();

      this['bins_g'] = this.svg_frame.append("svg:g").attr("id", this.bins_id);

      var w = this.frame.attr("width"), h = this.frame.attr("height");

      var constx = (w / this.histo['fXaxis']['fNbins']) / this.maxbin;
      var consty = (h / this.histo['fYaxis']['fNbins']) / this.maxbin;
      var xdom = this.x.domain();
      var ydom = this.y.domain();
      var xfactor = Math.abs(this.histo['fXaxis']['fXmax']-this.histo['fXaxis']['fXmin']) / Math.abs(xdom[1]-xdom[0]);
      var yfactor = Math.abs(this.histo['fYaxis']['fXmax']-this.histo['fYaxis']['fXmin']) / Math.abs(ydom[1]-ydom[0]);

      var pthis = this;
      
      if (this.options.Scat > 0 && this.histo['fMarkerStyle'] > 1) {
         /* Add markers */
         var filled = false;
         if ((this.histo['fMarkerStyle'] == 8) ||
             (this.histo['fMarkerStyle'] > 19 && this.histo['fMarkerStyle'] < 24) ||
             (this.histo['fMarkerStyle'] == 29))
            filled = true;

         var info_marker = getRootMarker(root_markers, this.histo['fMarkerStyle']);

         var shape = info_marker['shape'];
         var filled = info_marker['toFill'];
         var toRotate = info_marker['toRotate'];
         var markerSize = this.histo['fMarkerSize'];
         var markerScale = (shape == 0) ? 32 : 64;
         if (this.histo['fMarkerStyle'] == 1) markerScale = 1;

         var marker = null;
         
         switch (shape) {
            case 6:
               marker = "M " + (-4 * markerSize) + " " + (-1 * markerSize)
                         + " L " + 4 * markerSize + " " + (-1 * markerSize)
                         + " L " + (-2.4 * markerSize) + " " + 4 * markerSize
                         + " L 0 " + (-4 * markerSize) + " L " + 2.8 * markerSize
                         + " " + 4 * markerSize + " z";
               break;
            case 7:
               marker = "M " + (- 4 * markerSize) + " " + (-4 * markerSize)
                        + " L " + 4 * markerSize + " " + 4 * markerSize + " M 0 "
                        + (-4 * markerSize) + " 0 " + 4 * markerSize + " M "
                        + 4 * markerSize + " " + (-4 * markerSize) + " L "
                        + (-4 * markerSize) + " " + 4 * markerSize + " M "
                        + (-4 * markerSize) + " 0 L " + 4 * markerSize + " 0";
               break;
            default:
               marker = d3.svg.symbol()
                       .type(d3.svg.symbolTypes[shape])
                       .size(markerSize * markerScale);
               break;
         }
         var markers = this.bins_g.selectAll(".marker")
            .data(this.bins)
            .enter()
            .append("svg:path")
            .attr("class", "marker")
            .attr("transform", function(d) {
               return "translate(" + pthis.x(d.x) + "," + pthis.y(d.y) + ")"
            })
            .style("fill", root_colors[this.histo['fMarkerColor']])
            .style("stroke", root_colors[this.histo['fMarkerColor']])
            .attr("d", marker);
         
         if (gStyle.Tooltip > 1)
            markers.append("svg:title")
                   .text(function(d) { return "x = " + d.x.toPrecision(4) + " \ny = " +
                                d.y.toPrecision(4) + " \nentries = " + d.z; });
      }
      else {
         var drawn_bins = this.bins_g.selectAll(".bins")
            .data(this.bins)
            .enter()
            .append("svg:rect")
            .attr("class", "bins")
            .attr("x", function(d) { 
               if (pthis.options.Color > 0)
                  return pthis.x(d.x);
               else
                  return pthis.x(d.x + (pthis.scalex / 2)) - (0.5 * d.z * ((w / pthis.nbinsx) / pthis.maxbin) * xfactor);
            })
            .attr("y", function(d) { 
               if (pthis.options.Color > 0)
                  return pthis.y(d.y + pthis.scaley);
               else
                  return pthis.y(d.y + (pthis.scaley / 2)) - (0.5 * d.z * ((h / pthis.nbinsy) / pthis.maxbin) * yfactor);
            })
            .attr("width", function(d) {
               if (pthis.options.Color > 0)
                  return pthis.x(d.x + pthis.scalex) - pthis.x(d.x);
               else
                  return d.z * ((w / pthis.nbinsx) / pthis.maxbin) * xfactor;
            })
            .attr("height", function(d) {
               if (pthis.options.Color > 0)
                  return pthis.y(d.y) - pthis.y(d.y + pthis.scaley);
               else
                  return d.z * ((h / pthis.nbinsy) / pthis.maxbin) * yfactor;
            })
            .style("stroke", function(d) {
               if (pthis.options.Color > 0)
                  return "none";
               else
                  return pthis.linecolor;
            })
            .style("fill", function(d) {
               var fillcol = pthis.fillcolor;
               if (pthis.options.Color > 0)
                  fillcol = pthis.getValueColor(d.z);
               this['origin'] = fillcol; 
               return fillcol;
            });
         
         if (gStyle.Tooltip > 1)
            drawn_bins 
             .on('mouseover', function() { d3.select(this).transition().duration(100).style("fill", (this['origin'] == "black") ? "grey" : "black"); } )
             .on('mouseout', function() { d3.select(this).transition().duration(100).style("fill", this['origin']); } )
             .append("svg:title")
             .text(function(d) { 
                return "x = [" + d.x.toPrecision(4) + ", " + (d.x + pthis.scalex).toPrecision(4) + 
                "] \ny = [" + d.y.toPrecision(4) + ", " + (d.y + pthis.scaley).toPrecision(4) + 
                "] \nentries = " + d.z;
              });
      }
   }

   JSROOTPainter.Hist2DPainter.prototype.ProvideTooltip = function(tip)
   {
      var i = Math.round((tip.x - this.scalex/2 - this.xmin) / this.scalex);
      var j = Math.round((tip.y - this.scaley/2 - this.ymin) / this.scaley);

      if ((i<0) || (i>=this.nbinsx) || (j<0) || (j>=this.nbinsy)) return;
      
      var value = this.histo.getBinContent(i, j);
      
      if (value <= this.minbin) return;
            
      tip['empty'] = false;
      
      tip['dist'] = 0; 
      tip['text'].push("histo: "+this.histo['fName']);
      tip['text'].push("binx:"+ i + " biny:" + j);
      tip['text'].push("cont: " + value); 
      
//      $("#report").append("<br> found " + bin.x.toPrecision(4) + " " + bin.y.toPrecision(4));
      
      tip['x1'] = this.xmin + i*this.scalex;
      tip['x2'] = this.xmin + (i+1)*this.scalex;
      tip['y1'] = this.ymin + (j+1)*this.scaley;
      tip['y2'] = this.ymin + j*this.scaley;
      
      // basic method, painter can provide tooltip at specified coordinates
      // range.x1 .. range.x2, range.y1 .. range.y2
   }

   
   JSROOTPainter.drawHistogram2Dnew = function(vis, pad, histo, hframe) {

      // create painter and add it to canvas
      var painter = new JSROOTPainter.Hist2DPainter(histo);

      painter.SetFrame(vis, pad, hframe);

//      if (console) console.time("CreateBins");

      painter.CreateBins();

//      if (console) console.timeEnd("CreateBins");
      
      painter.CreateXY();

      painter.CountStat();

      painter.DrawGrids();

      painter.DrawBins();

      painter.DrawAxes();
      
      //$("#report").append("<br> title");
      painter.DrawTitle();

      //$("#report").append("<br> stat");
      //if (gStyle.AutoStat) painter.CreateStat();

      //$("#report").append("<br> func");
      painter.DrawFunctions();

      //$("#report").append("<br> interact");
      painter.AddInteractive();
      
      return painter;
   }

   JSROOTPainter.Hist2DPainter.prototype.Redraw = function() {
      if (!this.is3D) {
         JSROOTPainter.HistPainter.prototype.Redraw.call(this);
         return;
      }
      
      this.Draw3D();
   }

   
   JSROOTPainter.Hist2DPainter.prototype.Draw3D = function() 
   {
      if ('bins_id' in this) {
         d3.selectAll("#" + this.bins_id).remove();
         this['bins_g'] = null;
      }
      
      var w = this.frame.attr("width"), h = this.frame.attr("height"), size = 100;
      
      var xmin = this.xmin, xmax = this.xmax;
      if (this.zoom_xmin != this.zoom_xmax) { xmin = this.zoom_xmin; xmax = this.zoom_xmax; }
      var ymin = this.ymin, ymax = this.ymax;
      if (this.zoom_ymin != this.zoom_ymax) { ymin = this.zoom_ymin; ymax = this.zoom_ymax; }
      

      if (this.options.Logx) {
         var tx = d3.scale.log().domain([xmin, xmax]).range([-size, size]);
         var utx = d3.scale.log().domain([-size, size]).range([xmin, xmax]);
      } else {
         var tx = d3.scale.linear().domain([xmin, xmax]).range([-size, size]);
         var utx = d3.scale.linear().domain([-size, size]).range([xmin, xmax]);
      }
      if (this.options.Logy) {
         var ty = d3.scale.log().domain([ymin, ymax]).range([-size, size]);
         var uty = d3.scale.log().domain([size, -size]).range([ymin, ymax]);
      } else {
         var ty = d3.scale.linear().domain([ymin, ymax]).range([-size, size]);
         var uty = d3.scale.linear().domain([size, -size]).range([ymin, ymax]);
      }
      if (this.options.Logz) {
         var tz = d3.scale.log().domain([this.minbin, Math.ceil( this.maxbin/10 )*10]).range([0, size*2]);
         var utz = d3.scale.log().domain([0, size*2]).range([this.minbin, Math.ceil( this.maxbin/10 )*10]);
      } else {
         var tz = d3.scale.linear().domain([this.minbin, Math.ceil( this.maxbin/10 )*10]).range([0, size*2]);
         var utz = d3.scale.linear().domain([0, size*2]).range([this.minbin, Math.ceil( this.maxbin/10 )*10]);
      }

      // three.js 3D drawing
      var scene = new THREE.Scene();

      var toplevel = new THREE.Object3D();
      toplevel.rotation.x = 30 * Math.PI / 180;
      toplevel.rotation.y = 30 * Math.PI / 180;
      scene.add( toplevel );

      var wireMaterial = new THREE.MeshBasicMaterial( {
         color: 0x000000,
         wireframe: true,
         wireframeLinewidth: 0.5,
         side: THREE.DoubleSide } );

      // create a new mesh with cube geometry
      var cube = new THREE.Mesh( new THREE.CubeGeometry( size*2, size*2, size*2 ), wireMaterial);
      cube.position.y = size;

      // add the cube to the scene
      toplevel.add( cube );

      var textMaterial = new THREE.MeshBasicMaterial( { color: 0x000000 } );

      // add the calibration vectors and texts
      var geometry = new THREE.Geometry();
      var imax, istep, len = 3, plen, sin45 = Math.sin(45);
      var text3d, text;
      var xmajors = tx.ticks(8);
      var xminors = tx.ticks(50);
      for ( i=-size, j=0, k=0; i<size; ++i ) {
         var is_major = ( utx( i ) <= xmajors[j] && utx( i+1 ) > xmajors[j] ) ? true : false;
         var is_minor = ( utx( i ) <= xminors[k] && utx( i+1 ) > xminors[k] ) ? true : false;
         plen = ( is_major ? len + 2 : len) * sin45;
         if ( is_major ) {
            text3d = new THREE.TextGeometry( xmajors[j], {
               size: 7,
               height: 0,
               curveSegments: 10
            });
            ++j;

            text3d.computeBoundingBox();
            var centerOffset = 0.5 * ( text3d.boundingBox.max.x - text3d.boundingBox.min.x );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( i-centerOffset, -13, size+plen );
            toplevel.add( text );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( i+centerOffset, -13, -size-plen );
            text.rotation.y = Math.PI;
            toplevel.add( text );
         }
         if ( is_major || is_minor ) {
            ++k;
            geometry.vertices.push( new THREE.Vector3( i, 0, size ) );
            geometry.vertices.push( new THREE.Vector3( i, -plen, size+plen ) );
            geometry.vertices.push( new THREE.Vector3( i, 0, -size ) );
            geometry.vertices.push( new THREE.Vector3( i, -plen, -size-plen ) );
         }
      }
      var ymajors = ty.ticks(8);
      var yminors = ty.ticks(50);
      for ( i=size, j=0, k=0; i>-size; --i ) {
         var is_major = ( uty( i ) <= ymajors[j] && uty( i-1 ) > ymajors[j] ) ? true : false;
         var is_minor = ( uty( i ) <= yminors[k] && uty( i-1 ) > yminors[k] ) ? true : false;
         plen = ( is_major ? len + 2 : len) * sin45;
         if ( is_major ) {
            text3d = new THREE.TextGeometry( ymajors[j], {
               size: 7,
               height: 0,
               curveSegments: 10
            });
            ++j;

            text3d.computeBoundingBox();
            var centerOffset = 0.5 * ( text3d.boundingBox.max.x - text3d.boundingBox.min.x );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( size+plen, -13, i+centerOffset );
            text.rotation.y = Math.PI/2;
            toplevel.add( text );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( -size-plen, -13, i-centerOffset );
            text.rotation.y = -Math.PI/2;
            toplevel.add( text );
         }
         if ( is_major || is_minor ) {
            ++k;
            geometry.vertices.push( new THREE.Vector3( size, 0, i ) );
            geometry.vertices.push( new THREE.Vector3( size+plen, -plen, i ) );
            geometry.vertices.push( new THREE.Vector3( -size, 0, i ) );
            geometry.vertices.push( new THREE.Vector3( -size-plen, -plen, i ) );
         }
      }
      var zmajors = tz.ticks(8);
      var zminors = tz.ticks(50);
      for ( i=0, j=0, k=0; i<(size*2); ++i ) {
         var is_major = ( utz( i ) <= zmajors[j] && utz( i+1 ) > zmajors[j] ) ? true : false;
         var is_minor = ( utz( i ) <= zminors[k] && utz( i+1 ) > zminors[k] ) ? true : false;
         plen = ( is_major ? len + 2 : len) * sin45;
         if ( is_major ) {
            text3d = new THREE.TextGeometry( zmajors[j], {
               size: 7,
               height: 0,
               curveSegments: 10
            });
            ++j;

            text3d.computeBoundingBox();
            var offset = 0.8 * ( text3d.boundingBox.max.x - text3d.boundingBox.min.x );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( size+offset+5, i-2.5, size+offset+5 );
            text.rotation.y = Math.PI*3/4;
            toplevel.add( text );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( size+offset+5, i-2.5, -size-offset-5 );
            text.rotation.y = -Math.PI*3/4;
            toplevel.add( text );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( -size-offset-5, i-2.5, size+offset+5 );
            text.rotation.y = Math.PI/4;
            toplevel.add( text );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( -size-offset-5, i-2.5, -size-offset-5 );
            text.rotation.y = -Math.PI/4;
            toplevel.add( text );
         }
         if ( is_major || is_minor ) {
            ++k;
            geometry.vertices.push( new THREE.Vector3( size, i, size ) );
            geometry.vertices.push( new THREE.Vector3( size+plen, i, size+plen ) );
            geometry.vertices.push( new THREE.Vector3( size, i, -size ) );
            geometry.vertices.push( new THREE.Vector3( size+plen, i, -size-plen ) );
            geometry.vertices.push( new THREE.Vector3( -size, i, size ) );
            geometry.vertices.push( new THREE.Vector3( -size-plen, i, size+plen ) );
            geometry.vertices.push( new THREE.Vector3( -size, i, -size ) );
            geometry.vertices.push( new THREE.Vector3( -size-plen, i, -size-plen ) );
         }
      }

      // add the calibration lines
      var lineMaterial = new THREE.LineBasicMaterial( { color: 0x000000 } );
      var line = new THREE.Line( geometry, lineMaterial );
      line.type = THREE.LinePieces;
      toplevel.add( line );

      // create the bin cubes
      var constx = (size*2 / this.nbinsx) / this.maxbin;
      var consty = (size*2 / this.nbinsy) / this.maxbin;

      var colorFlag = ( this.options.Color > 0);
      var fcolor = d3.rgb(root_colors[this.histo['fFillColor']]);
      var fillcolor = new THREE.Color( 0xDDDDDD );
      fillcolor.setRGB(fcolor.r/255, fcolor.g/255, fcolor.b/255);
      var bin, wei, hh;
      for (var i = 0; i < this.bins.length; ++i ) {
         hh = this.bins[i];
         if ((hh.x < xmin) || (hh.x > xmax) || (hh.y < ymin) || (hh.y > ymax)) continue;
         
         bin = THREE.SceneUtils.createMultiMaterialObject(
            new THREE.CubeGeometry( 2*size/this.nbinsx, hh.z, 2*size/this.nbinsy ),
            [ new THREE.MeshLambertMaterial( { color: fillcolor.getHex(), shading: THREE.NoShading } ),
              wireMaterial ] );
         bin.position.x = tx(hh.x + this.scalex/2);
         bin.position.y = hh.z/2;
         bin.position.z = -(ty(hh.y + this.scaley/2));
//         bin.name = "x: [" + hh.x.toPrecision(4) + ", " + (hh.x + this.scalex).toPrecision(4) + "]<br>" +
//                    "y: [" + hh.y.toPrecision(4) + ", " + (hh.y + this.scaley).toPrecision(4) + "]<br>" + 
//                    "entries: " + hh.z.toFixed();
         toplevel.add( bin );
      }
      // create a point light
      var pointLight = new THREE.PointLight( 0xcfcfcf );
      pointLight.position.set(0, 50, 250);
      scene.add(pointLight);

      //var directionalLight = new THREE.DirectionalLight( 0x7f7f7f );
      //directionalLight.position.set( 0, -70, 100 ).normalize();
      //scene.add( directionalLight );

      var camera = new THREE.PerspectiveCamera( 45, w / h, 1, 1000 );
      camera.position.set( 0, size/2, 500 );
      camera.lookat = cube;

      var renderer = Detector.webgl ? new THREE.WebGLRenderer( { antialias: true } ) :
                     new THREE.CanvasRenderer( { antialias: true } );
      renderer.setSize( w, h );
      $( this.vis[0][0] ).hide().parent().append( renderer.domElement );
      renderer.render( scene, camera );
      
      JSROOTPainter.add3DInteraction(renderer, scene, camera, toplevel);
   }

   

   JSROOTPainter.drawHistogram3D = function(vis, pad, histo, hframe) {
      var i, j, k, logx = false, logy = false, logz = false,
          gridx = false, gridy = false, gridz = false;
      var opt = histo['fOption'].toLowerCase();
      if (pad && typeof(pad) != 'undefined') {
         logx = pad['fLogx'];
         logy = pad['fLogy'];
         logz = pad['fLogz'];
         gridx = pad['fGridx'];
         gridy = pad['fGridy'];
         gridz = pad['fGridz'];
      }
      var fillcolor = root_colors[histo['fFillColor']];
      var linecolor = root_colors[histo['fLineColor']];
      if (histo['fFillColor'] == 0) {
         fillcolor = '#4572A7';
      }
      if (histo['fLineColor'] == 0) {
         linecolor = '#4572A7';
      }
      var nbinsx = histo['fXaxis']['fNbins'];
      var nbinsy = histo['fYaxis']['fNbins'];
      var nbinsz = histo['fZaxis']['fNbins'];
      var scalex = (histo['fXaxis']['fXmax'] - histo['fXaxis']['fXmin']) /
                    histo['fXaxis']['fNbins'];
      var scaley = (histo['fYaxis']['fXmax'] - histo['fYaxis']['fXmin']) /
                    histo['fYaxis']['fNbins'];
      var scalez = (histo['fZaxis']['fXmax'] - histo['fZaxis']['fXmin']) /
                    histo['fZaxis']['fNbins'];
      var maxbin = -1e32, minbin = 1e32;
      maxbin = d3.max(histo['fArray']);
      minbin = d3.min(histo['fArray']);
      var bins = new Array();
      for (i=0; i<=nbinsx+2; ++i) {
         for (var j=0; j<nbinsy+2; ++j) {
            for (var k=0; k<nbinsz+2; ++k) {
               var bin_content = histo.getBinContent(i, j, k);
               if (bin_content > minbin) {
                  var point = {
                     x:histo['fXaxis']['fXmin'] + (i*scalex),
                     y:histo['fYaxis']['fXmin'] + (j*scaley),
                     z:histo['fZaxis']['fXmin'] + (k*scalez),
                     n:bin_content
                  };
                  bins.push(point);
               }
            }
         }
      }
      var w = vis.attr("width"), h = vis.attr("height"), size = 100;
      if (logx) {
         var tx = d3.scale.log().domain([histo['fXaxis']['fXmin'], histo['fXaxis']['fXmax']]).range([-size, size]);
         var utx = d3.scale.log().domain([-size, size]).range([histo['fXaxis']['fXmin'], histo['fXaxis']['fXmax']]);
      } else {
         var tx = d3.scale.linear().domain([histo['fXaxis']['fXmin'], histo['fXaxis']['fXmax']]).range([-size, size]);
         var utx = d3.scale.linear().domain([-size, size]).range([histo['fXaxis']['fXmin'], histo['fXaxis']['fXmax']]);
      }
      if (logy) {
         var ty = d3.scale.log().domain([histo['fYaxis']['fXmin'], histo['fYaxis']['fXmax']]).range([-size, size]);
         var uty = d3.scale.log().domain([size, -size]).range([histo['fYaxis']['fXmin'], histo['fYaxis']['fXmax']]);
      } else {
         var ty = d3.scale.linear().domain([histo['fYaxis']['fXmin'], histo['fYaxis']['fXmax']]).range([-size, size]);
         var uty = d3.scale.linear().domain([size, -size]).range([histo['fYaxis']['fXmin'], histo['fYaxis']['fXmax']]);
      }
      if (logz) {
         var tz = d3.scale.log().domain([histo['fZaxis']['fXmin'], histo['fZaxis']['fXmax']]).range([-size, size]);
         var utz = d3.scale.log().domain([-size, size]).range([histo['fZaxis']['fXmin'], histo['fZaxis']['fXmax']]);
      } else {
         var tz = d3.scale.linear().domain([histo['fZaxis']['fXmin'], histo['fZaxis']['fXmax']]).range([-size, size]);
         var utz = d3.scale.linear().domain([-size, size]).range([histo['fZaxis']['fXmin'], histo['fZaxis']['fXmax']]);
      }

      // three.js 3D drawing
      var scene = new THREE.Scene();

      var toplevel = new THREE.Object3D();
      toplevel.rotation.x = 30 * Math.PI / 180;
      toplevel.rotation.y = 30 * Math.PI / 180;
      scene.add( toplevel );

      var wireMaterial = new THREE.MeshBasicMaterial( {
         color: 0x000000,
         wireframe: true,
         wireframeLinewidth: 0.5,
         side: THREE.DoubleSide } );

      // create a new mesh with cube geometry
      var cube = new THREE.Mesh( new THREE.CubeGeometry( size*2, size*2, size*2 ), wireMaterial);

      // add the cube to the scene
      toplevel.add( cube );

      var textMaterial = new THREE.MeshBasicMaterial( { color: 0x000000 } );

      // add the calibration vectors and texts
      var geometry = new THREE.Geometry();
      var imax, istep, len = 3, plen, sin45 = Math.sin(45);
      var text3d, text;
      var xmajors = tx.ticks(5);
      var xminors = tx.ticks(25);
      for ( i=-size, j=0, k=0; i<=size; ++i ) {
         var is_major = ( utx( i ) <= xmajors[j] && utx( i+1 ) > xmajors[j] ) ? true : false;
         var is_minor = ( utx( i ) <= xminors[k] && utx( i+1 ) > xminors[k] ) ? true : false;
         plen = ( is_major ? len + 2 : len) * sin45;
         if ( is_major ) {
            text3d = new THREE.TextGeometry( xmajors[j], {
               size: 7,
               height: 0,
               curveSegments: 10
            });
            ++j;

            text3d.computeBoundingBox();
            var centerOffset = 0.5 * ( text3d.boundingBox.max.x - text3d.boundingBox.min.x );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( i-centerOffset, -size-13, size+plen );
            toplevel.add( text );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( i+centerOffset, -size-13, -size-plen );
            text.rotation.y = Math.PI;
            toplevel.add( text );
         }
         if ( is_major || is_minor ) {
            ++k;
            geometry.vertices.push( new THREE.Vector3( i, -size, size ) );
            geometry.vertices.push( new THREE.Vector3( i, -size-plen, size+plen ) );
            geometry.vertices.push( new THREE.Vector3( i, -size, -size ) );
            geometry.vertices.push( new THREE.Vector3( i, -size-plen, -size-plen ) );
         }
      }
      var ymajors = ty.ticks(5);
      var yminors = ty.ticks(25);
      for ( i=size, j=0, k=0; i>-size; --i ) {
         var is_major = ( uty( i ) <= ymajors[j] && uty( i-1 ) > ymajors[j] ) ? true : false;
         var is_minor = ( uty( i ) <= yminors[k] && uty( i-1 ) > yminors[k] ) ? true : false;
         plen = ( is_major ? len + 2 : len) * sin45;
         if ( is_major ) {
            text3d = new THREE.TextGeometry( ymajors[j], {
               size: 7,
               height: 0,
               curveSegments: 10
            });
            ++j;

            text3d.computeBoundingBox();
            var centerOffset = 0.5 * ( text3d.boundingBox.max.x - text3d.boundingBox.min.x );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( size+plen, -size-13, i+centerOffset );
            text.rotation.y = Math.PI/2;
            toplevel.add( text );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( -size-plen, -size-13, i-centerOffset );
            text.rotation.y = -Math.PI/2;
            toplevel.add( text );
         }
         if ( is_major || is_minor ) {
            ++k;
            geometry.vertices.push( new THREE.Vector3( size, -size, i ) );
            geometry.vertices.push( new THREE.Vector3( size+plen, -size-plen, i ) );
            geometry.vertices.push( new THREE.Vector3( -size, -size, i ) );
            geometry.vertices.push( new THREE.Vector3( -size-plen, -size-plen, i ) );
         }
      }
      var zmajors = tz.ticks(5);
      var zminors = tz.ticks(25);
      for ( i=-size, j=0, k=0; i<=size; ++i ) {
         var is_major = ( utz( i ) <= zmajors[j] && utz( i+1 ) > zmajors[j] ) ? true : false;
         var is_minor = ( utz( i ) <= zminors[k] && utz( i+1 ) > zminors[k] ) ? true : false;
         plen = ( is_major ? len + 2 : len) * sin45;
         if ( is_major ) {
            text3d = new THREE.TextGeometry( zmajors[j], {
               size: 7,
               height: 0,
               curveSegments: 10
            });
            ++j;

            text3d.computeBoundingBox();
            var offset = 0.6 * ( text3d.boundingBox.max.x - text3d.boundingBox.min.x );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( size+offset+7, i-2.5, size+offset+7 );
            text.rotation.y = Math.PI*3/4;
            toplevel.add( text );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( size+offset+7, i-2.5, -size-offset-7 );
            text.rotation.y = -Math.PI*3/4;
            toplevel.add( text );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( -size-offset-7, i-2.5, size+offset+7 );
            text.rotation.y = Math.PI/4;
            toplevel.add( text );

            text = new THREE.Mesh( text3d, textMaterial );
            text.position.set( -size-offset-7, i-2.5, -size-offset-7 );
            text.rotation.y = -Math.PI/4;
            toplevel.add( text );
         }
         if ( is_major || is_minor ) {
            ++k;
            geometry.vertices.push( new THREE.Vector3( size, i, size ) );
            geometry.vertices.push( new THREE.Vector3( size+plen, i, size+plen ) );
            geometry.vertices.push( new THREE.Vector3( size, i, -size ) );
            geometry.vertices.push( new THREE.Vector3( size+plen, i, -size-plen ) );
            geometry.vertices.push( new THREE.Vector3( -size, i, size ) );
            geometry.vertices.push( new THREE.Vector3( -size-plen, i, size+plen ) );
            geometry.vertices.push( new THREE.Vector3( -size, i, -size ) );
            geometry.vertices.push( new THREE.Vector3( -size-plen, i, -size-plen ) );
         }
      }

      // add the calibration lines
      var lineMaterial = new THREE.LineBasicMaterial( { color: 0x000000 } );
      var line = new THREE.Line( geometry, lineMaterial );
      line.type = THREE.LinePieces;
      toplevel.add( line );

      // create the bin cubes
      var constx = (size*2 / histo['fXaxis']['fNbins']) / maxbin;
      var consty = (size*2 / histo['fYaxis']['fNbins']) / maxbin;
      var constz = (size*2 / histo['fZaxis']['fNbins']) / maxbin;

      var optFlag = ( opt.indexOf('colz') != -1 || opt.indexOf('col') != -1 );
      var fcolor = d3.rgb(root_colors[histo['fFillColor']]);
      var fillcolor = new THREE.Color( 0xDDDDDD );
      fillcolor.setRGB(fcolor.r/255, fcolor.g/255, fcolor.b/255);
      var bin, wei;
      for ( i = 0; i < bins.length; ++i ) {
         wei = ( optFlag ? maxbin : bins[i].n );
         if (opt.indexOf('box1') != -1) {
            bin = new THREE.Mesh( new THREE.SphereGeometry( 0.5 * wei * constx /*, 16, 16 */ ),
                      new THREE.MeshPhongMaterial( { color: fillcolor.getHex(),
                              specular: 0xbfbfbf /*, shading: THREE.NoShading */ } ) );
         }
         else {
            bin = THREE.SceneUtils.createMultiMaterialObject(
               new THREE.CubeGeometry( wei * constx, wei * constz, wei * consty ),
               [ new THREE.MeshLambertMaterial( { color: fillcolor.getHex(), shading: THREE.NoShading } ),
                 wireMaterial ] );
         }
         bin.position.x = tx( bins[i].x - (scalex/2));
         bin.position.y = tz( bins[i].z - (scalez/2));
         bin.position.z = -(ty( bins[i].y - (scaley/2)));
         bin.name = "x: [" + bins[i].x.toPrecision(4) + ", " + (bins[i].x + scalex).toPrecision(4) + "]<br>" +
                    "y: [" + bins[i].y.toPrecision(4) + ", " + (bins[i].y + scaley).toPrecision(4) + "]<br>" + 
                    "z: [" + bins[i].z.toPrecision(4) + ", " + (bins[i].z + scalez).toPrecision(4) + "]<br>" + 
                    "entries: " + bins[i].n.toFixed();
         toplevel.add( bin );
      }
      // create a point light
      var pointLight = new THREE.PointLight( 0xcfcfcf );
      pointLight.position.set(0, 50, 250);
      scene.add(pointLight);

      //var directionalLight = new THREE.DirectionalLight( 0x7f7f7f );
      //directionalLight.position.set( 0, -70, 100 ).normalize();
      //scene.add( directionalLight );

      var camera = new THREE.PerspectiveCamera( 45, w / h, 1, 1000 );
      camera.position.set( 0, 0, 500 );
      camera.lookat = cube;

      var renderer = Detector.webgl ? new THREE.WebGLRenderer( { antialias: true } ) :
                     new THREE.CanvasRenderer( { antialias: true } );
      renderer.setSize( w, h );
      $( vis[0][0] ).hide().parent().append( renderer.domElement );
      renderer.render( scene, camera );

      this.add3DInteraction(renderer, scene, camera, toplevel);
   }

   JSROOTPainter.drawHStack = function(vis, pad, stack, frame) {
      // paint the list of histograms
      // By default, histograms are shown stacked.
      //    -the first histogram is paint
      //    -then the sum of the first and second, etc

      if (!'fHists' in stack) return;
      if (stack['fHists'].length == 0) return;
      var histos = stack['fHists'];
      var nhists = stack['fHists'].length;
      if (nhists == 0) return;
      var opt = "";
      if ('fOption' in stack) opt = stack['fOption'].toLowerCase();
      var lsame = false;
      if (opt.indexOf("same") != -1) {
         lsame = true;
         opt.replace("same", "");
      }
      // compute the min/max of each axis
      var i, h;
      var xmin = 0, xmax = 0, ymin = 0, ymax = 0;
      for (var i=0; i<nhists; ++i) {
         h = histos[i];
         if (i==0 || h['fXaxis']['fXmin'] < xmin) xmin = h['fXaxis']['fXmin'];
         if (i==0 || h['fXaxis']['fXmax'] > xmax) xmax = h['fXaxis']['fXmax'];
         if (i==0 || h['fYaxis']['fXmin'] < ymin) ymin = h['fYaxis']['fXmin'];
         if (i==0 || h['fYaxis']['fXmax'] > ymax) ymax = h['fYaxis']['fXmax'];
      }
      var nostack = opt.indexOf("nostack") == -1 ? false : true;
      if (!nostack) stack.buildStack();
      
      var themin, themax;
      if (stack['fMaximum'] == -1111) themax = stack.getMaximum(opt);
      else themax = stack['fMaximum'];
      if (stack['fMinimum'] == -1111) {
         themin = stack.getMinimum(opt);
         if (pad && pad['fLogy']) {
            if (themin > 0) themin *= .9;
            else themin = themax * 1.e-3;
         }
         else if (themin > 0)
            themin = 0;
      }
      else themin = stack['fMinimum'];
      if (!('fHistogram' in stack)) {
         h = stack['fHists'][0];
         stack['fHistogram'] = new Object();
         stack['fHistogram']['_typename'] = stack['fHists'][0]['_typename'];
         stack['fHistogram']['fName'] = "unnamed";
         stack['fHistogram']['fBits'] = 0;
         stack['fHistogram']['TestBit'] = function(bit) { return false };
         stack['fHistogram']['fOption'] = "";
         stack['fHistogram']['fXaxis'] = JSROOTCore.clone(h['fXaxis']);
         stack['fHistogram']['fYaxis'] = JSROOTCore.clone(h['fYaxis']);
         stack['fHistogram']['fXaxis']['fXmin'] = xmin;
         stack['fHistogram']['fXaxis']['fXmax'] = xmax;
         stack['fHistogram']['fYaxis']['fXmin'] = ymin;
         stack['fHistogram']['fYaxis']['fXmax'] = ymax;
         stack['fHistogram']['fXaxis']['fNbins'] = 0;
         stack['fHistogram']['fYaxis']['fNbins'] = 0;
         stack['fHistogram']['fArray'] = new Array();
      }
      stack['fHistogram']['fTitle'] = stack['fTitle'];
      //var histo = JSROOTCore.clone(stack['fHistogram']);
      var histo = stack['fHistogram'];
      if (!histo.TestBit(TH1StatusBits.kIsZoomed)) {
         if (nostack && stack['fMaximum'] != -1111) histo['fMaximum'] = stack['fMaximum'];
         else {
            if (pad && pad['fLogy']) histo['fMaximum'] = themax*(1+0.2*JSROOTMath.log10(themax/themin));
            else histo['fMaximum'] = 1.05 * themax;
         }
         if (nostack && stack['fMinimum'] != -1111) histo['fMinimum'] = stack['fMinimum'];
         else {
            if (pad && pad['fLogy']) histo['fMinimum'] = themin/(1+0.5*JSROOTMath.log10(themax/themin));
            else histo['fMinimum'] = themin;
         }
      }
      if (!lsame) {
         if ('fOption' in stack) {
            if (histo['fOption'].indexOf(stack['fOption'] == -1))
               histo['fOption'] += stack['fOption'];
         }
         
         var painter = null;
         
         if (histo['_typename'].match(/\bJSROOTIO.TH1/))
            painter = JSROOTPainter.drawHistogram1Dnew(vis, pad, histo, frame);
         else if (histo['_typename'].match(/\bJSROOTIO.TH2/))
            painter = JSROOTPainter.drawHistogram2Dnew(vis, pad, histo, frame);
         
         if (painter!=null) frame = painter.frame;
      }
      for (var i=0; i<nhists; ++i) {
         if (nostack) 
            h = histos[i];
         else
            h = stack['fStack'][nhists-i-1];

         if ('fOption' in stack) {
            if (h['fOption'].indexOf(stack['fOption'] == -1))
               h['fOption'] += stack['fOption'];
         }
         
         h['fOption'] += "same";

         if (h['_typename'].match(/\bJSROOTIO.TH1/))
            JSROOTPainter.drawHistogram1Dnew(vis, pad, h, frame);
      }
   };

   JSROOTPainter.drawLatex = function(vis, string, x, y, attr) {
      var w = vis.attr("width"), h = vis.attr("height");
      while (string.indexOf('#') != -1)
         string = string.replace('#', '\\');
      string = string.replace(' ', '\\: ');

      var parse = new jsMath.Parser(string, null, null, null);
      parse.Parse();
      if (parse.error) return false;

      // method using jsMath do display formulae and LateX
      // unfortunately it works only on FireFox (Chrome displays it,
      // but at wrong coordinates, and IE doesn't support foreignObject
      // in SVG...)
      string = '\\displaystyle \\rm ' + string;
      var fo = vis.append("foreignObject")
         .attr("x", x)
         .attr("y", y)
         .attr("width", w - x)
         .attr("height", h - y);
      var math = fo.append("xhtml:div")
         .style("display", "inline")
         .style("color", attr['font-color'])
         .style('font-size', (attr['font-size']*0.98)+'px')
         .attr("class", "math")
         .html(string);
      jsMath.ProcessElement(math[0][0]);
      return true;
   };

   JSROOTPainter.drawLegend = function(vis, pad, pave) {
      var x=0, y=0, w=0, h=0;
      if (pave['fInit'] == 0) {
          x = pave['fX1'] * vis.attr("width")
          y = vis.attr("height") - pave['fY1'] * vis.attr("height");
          w = (pave['fX2'] - pave['fX1']) * vis.attr("width");
          h = (pave['fY2'] - pave['fY1']) * vis.attr("height");
      }
      else {
          x = pave['fX1NDC'] * vis.attr("width")
          y = vis.attr("height") - pave['fY1NDC'] * vis.attr("height");
          w = (pave['fX2NDC'] - pave['fX1NDC']) * vis.attr("width");
          h = (pave['fY2NDC'] - pave['fY1NDC']) * vis.attr("height");
      }
      y -= h;
      var fillcolor = root_colors[pave['fFillColor']];
      var lcolor = root_colors[pave['fLineColor']];
      var lwidth = pave['fBorderSize'] ? pave['fBorderSize'] : 0;
      if (pave['fFillStyle'] > 4000 && pave['fFillStyle'] < 4100)
         fillcolor = 'none';

      var p = vis.append("svg:g")
         .attr("width", w)
         .attr("height", h)
         .attr("transform", "translate(" + x + "," + y + ")");

      p.append("svg:rect")
         .attr("class", p)
         .attr("x", 0)
         .attr("y", 0)
         .attr("width", w)
         .attr("height", h)
         .attr("fill", fillcolor)
         .style("stroke-width", lwidth ? 1 : 0)
         .style("stroke", lcolor);

      var tcolor = root_colors[pave['fTextColor']];
      var tpos_x = pave['fMargin'] * w;
      var nlines = pave['fPrimitives'].length;
      var font_size = Math.round(h / (nlines * 1.5));
      //var font_size = Math.round(pave['fTextSize'] * vis.height());
      var fontDetails = getFontDetails(root_fonts[Math.floor(pave['fTextFont']/10)]);

      var max_len = 0, mul = 1.4;
      for (var j=0; j<nlines; ++j) {
         line = this.translateLaTeX(pave['fPrimitives'][j]['fLabel']);
         lw = tpos_x + stringWidth(vis, line, fontDetails['name'], fontDetails['weight'],
                                   font_size, fontDetails['style']);
         if (lw > max_len) max_len = lw;
      }
      if (max_len > w) {
         font_size *= 0.85 * (w / max_len);
         mul *= 0.95 * (max_len / w);
      }
      var x1 = pave['fX1NDC'];
      var x2 = pave['fX2NDC'];
      var y1 = pave['fY1NDC'];
      var y2 = pave['fY2NDC'];
      var margin = pave['fMargin']*( x2-x1 )/pave['fNColumns'];
      var yspace = (y2-y1)/nlines;
      var ytext = y2 + 0.5*yspace;  // y-location of 0th entry
      var boxw = margin*0.35;

      for (var i=0; i<nlines; ++i) {
         var leg = pave['fPrimitives'][i];
         var string = leg['fLabel'];
         var pos_y = ((i+1) * (font_size * mul)) - (font_size/3);
         var tpos_y = (i+1) * (font_size * mul);
         if (nlines == 1) {
            var pos_y = (h * 0.75) - (font_size/3);
            var tpos_y = h * 0.75;
         }

         var mo = gFile.GetMappedObject(leg['fObject']);
         if (mo) {
            leg['fFillColor']   = mo['fFillColor'];
            leg['fFillStyle']   = mo['fFillStyle'];
            leg['fLineColor']   = mo['fLineColor'];
            leg['fLineStyle']   = mo['fLineStyle'];
            leg['fLineWidth']   = mo['fLineWidth'];
            leg['fMarkerColor'] = mo['fMarkerColor'];
            leg['fMarkerSize']  = mo['fMarkerSize'];
            leg['fMarkerStyle'] = mo['fMarkerStyle'];
         }
         var line_color = root_colors[leg['fLineColor']];
         var line_width = leg['fLineWidth'];
         var line_style = root_line_styles[leg['fLineStyle']];
         var fill_color = root_colors[leg['fFillColor']];
         var fill_style = leg['fFillStyle'];
         var opt = leg['fOption'].toLowerCase();

         p.append("text")
            .attr("class", "text")
            .attr("text-anchor", "start")
            .attr("x", tpos_x)
            .attr("y", tpos_y)
            .attr("font-weight", fontDetails['weight'])
            .attr("font-style", fontDetails['style'])
            .attr("font-family", fontDetails['name'])
            .attr("font-size", font_size)
            .attr("fill", tcolor)
            .text(string);

         // Draw fill pattern (in a box)
         if (opt.indexOf('f') != -1) {
            // box total height is yspace*0.7
            // define x,y as the center of the symbol for this entry
            var xsym = margin/2;
            var ysym = ytext;
            var xf = new Array(4), yf = new Array(4);
            xf[0] = xsym - boxw;
            yf[0] = ysym - yspace*0.35;
            xf[1] = xsym + boxw;
            yf[1] = yf[0];
            xf[2] = xf[1];
            yf[2] = ysym + yspace*0.35;
            xf[3] = xf[0];
            yf[3] = yf[2];
            for (var j=0;j<4;j++) {
               xf[j] = xf[j] * vis.attr("width");
               yf[j] = yf[j] * vis.attr("height");
            }
            var ww = xf[1] - xf[0];
            var hh = yf[2] - yf[0];
            pos_y = pos_y - (hh/2);
            var pos_x = (tpos_x/2) - (ww/2);

            if (fill_style > 3000) {
               createFillPatterns(vis, fill_style, leg['fFillColor']);
               p.append("svg:rect")
                  .attr("x", pos_x)
                  .attr("y", pos_y)
                  .attr("width", ww)
                  .attr("height", hh)
                  .style("fill", "url(#pat" + fill_style + "_" + leg['fFillColor'] + ")")
                  .style("stroke-width", line_width)
                  .style("stroke", line_color);
            }
            else {
               p.append("svg:rect")
                  .attr("x", pos_x)
                  .attr("y", pos_y)
                  .attr("width", ww)
                  .attr("height", hh)
                  .attr("fill", fill_color)
                  .style("stroke-width", line_width)
                  .style("stroke", line_color);
            }
         }
         // Draw line
         if (opt.indexOf('l') != -1) {
            // line total length (in x) is margin*0.8
            var line_length = (0.7 * pave['fMargin']) * w;
            var pos_x = (tpos_x - line_length) / 2;
            p.append("svg:line")
               .attr("x1", pos_x)
               .attr("y1", pos_y)
               .attr("x2", pos_x + line_length)
               .attr("y2", pos_y)
               .style("stroke", line_color)
               .style("stroke-width", line_width)
               .style("stroke-dasharray", line_style);

         }
         // Draw error only
         if (opt.indexOf('e') != -1 && (opt.indexOf('l') == -1 || opt.indexOf('f') != -1)) {
         }
         // Draw Polymarker
         if (opt.indexOf('p') != -1) {

            var line_length = (0.7 * pave['fMargin']) * w;
            var pos_x = tpos_x / 2;

            var filled = false;
            if ((leg['fMarkerStyle'] == 8) ||
                (leg['fMarkerStyle'] > 19 && leg['fMarkerStyle'] < 24) ||
                (leg['fMarkerStyle'] == 29))
               filled = true;

            var info_marker = getRootMarker(root_markers, leg['fMarkerStyle']);

            var shape = info_marker['shape'];
            var filled = info_marker['toFill'];
            var toRotate = info_marker['toRotate'];
            var markerSize = leg['fMarkerSize'];
            var markerScale = 65;
            if (leg['fMarkerStyle'] == 1) markerScale = 1;

            switch (shape) {
               case 6:
                  var marker = "M " + (-4 * markerSize) + " " + (-1 * markerSize)
                              + " L " + 4 * markerSize + " " + (-1 * markerSize)
                              + " L " + (-2.4 * markerSize) + " " + 4 * markerSize
                              + " L 0 " + (-4 * markerSize) + " L " + 2.8 * markerSize
                              + " " + 4 * markerSize + " z";
                  break;
               case 7:
                  var marker = "M " + (- 4 * markerSize) + " " + (-4 * markerSize)
                              + " L " + 4 * markerSize + " " + 4 * markerSize + " M 0 "
                              + (-4 * markerSize) + " 0 " + 4 * markerSize + " M "
                              + 4 * markerSize + " " + (-4 * markerSize) + " L "
                              + (-4 * markerSize) + " " + 4 * markerSize + " M "
                              + (-4 * markerSize) + " 0 L " + 4 * markerSize + " 0";
                  break;
               default:
                  var marker = d3.svg.symbol()
                              .type(d3.svg.symbolTypes[shape])
                              .size(markerSize * markerScale);
                  break;
            }
            p.append("svg:path")
               .attr("transform", function(d) { return "translate(" + pos_x + "," + pos_y + ")"; })
               .style("fill", filled ? root_colors[leg['fMarkerColor']] : "none")
               .style("stroke", root_colors[leg['fMarkerColor']])
               .attr("d", marker);
         }
      }
      if (lwidth && lwidth > 1) {
         p.append("svg:line")
            .attr("x1", w+(lwidth/2))
            .attr("y1", lwidth+1)
            .attr("x2", w+(lwidth/2))
            .attr("y2", h+lwidth-1)
            .style("stroke", lcolor)
            .style("stroke-width", lwidth);
         p.append("svg:line")
            .attr("x1", lwidth+1)
            .attr("y1", h+(lwidth/2))
            .attr("x2", w+lwidth-1)
            .attr("y2", h+(lwidth/2))
            .style("stroke", lcolor)
            .style("stroke-width", lwidth);
      }
      return p;
   };

   JSROOTPainter.CreateDummyAxis = function() {
      var axis = {};

      axis['_typename'] = "ROOTIO.TAxis";
      axis['fBits'] = 0x3000008;
      axis['fBits2'] = 0;
      axis['fXmin'] = 0;
      axis['fXmax'] = 0;
      axis['fNbins'] = 0;
      axis['fN'] = 0;
      axis['fXbins'] = new Array;
      axis['fFirst'] = 0;
      axis['fLast'] = 0;
      axis['fName'] = "";
      axis['fTitle'] = "";
      axis['fTimeDisplay'] = false;
      axis['fTimeFormat'] = "";
      axis['fNdivisions'] = 510;
      axis['fAxisColor'] = 1;
      axis['fLabelColor'] = 1;
      axis['fLabelFont'] = 42;
      axis['fLabelOffset']  = 0.05;
      axis['fLabelSize']  = 0.035;
      axis['fTickLength'] = 0.03;
      axis['fTitleOffset'] = 1;
      axis['fTitleSize']  = 0.035;
      axis['fTitleColor'] = 1;
      axis['fTitleFont'] = 42;
      JSROOTCore.addMethods(axis);
      return axis;
   }

   JSROOTPainter.CreateDummyHisto = function() {
      var histo = {};
      histo['_typename'] = "ROOTIO.TH1I";
      histo['fBits'] = 0x3000008;
      histo['fName'] = "dummy_histo_" + random_id++;
      histo['fTitle'] = "dummytitle";
      histo['fMinimum'] = -1111;
      histo['fMaximum'] = -1111;
      histo['fOption'] = "";
      histo['fFillColor'] = 0;
      histo['fLineColor'] = 0;
      histo['fLineWidth'] = 1;
      histo['fBinStatErrOpt'] = 0;
      histo['fNcells'] = 0;
      histo['fN'] = 0;
      histo['fArray'] = new Array;
      histo['fSumw2'] = new Array;

      histo['fXaxis'] = JSROOTPainter.CreateDummyAxis();
      histo['fYaxis'] = JSROOTPainter.CreateDummyAxis();
      JSROOTCore.addMethods(histo);

      return histo;
   }

   
   JSROOTPainter.drawMultiGraphNew = function(vis, pad, mgraph, frame) {
      var i, maximum, minimum, rwxmin=0, rwxmax=0, rwymin=0, rwymax=0, uxmin=0, uxmax=0, dx, dy;
      var npt = 100;
      var histo = mgraph['fHistogram'];
      var graphs = mgraph['fGraphs'];
      var scalex = 1, scaley = 1;
      var logx = false, logy = false, logz = false, gridx = false, gridy = false;
      var draw_all = true;
      if (pad && typeof(pad) != 'undefined') {
         rwxmin = pad.fUxmin;
         rwxmax = pad.fUxmax;
         rwymin = pad.fUymin;
         rwymax = pad.fUymax;
         logx = pad['fLogx']; logy = pad['fLogy']; logz = pad['fLogz'];
         gridx = pad['fGridx']; gridy = pad['fGridy'];
      }
      if ('fHistogram' in mgraph && mgraph['fHistogram']) {
         minimum = mgraph['fHistogram']['fYaxis']['fXmin'];
         maximum = mgraph['fHistogram']['fYaxis']['fXmax'];
         if (pad && typeof(pad) != 'undefined') {
            uxmin   = this.padtoX(pad, rwxmin);
            uxmax   = this.padtoX(pad, rwxmax);
         }
      } else {
         var g = graphs[0];
         if (g) {
            var r = g.computeRange();
            rwxmin = r['xmin']; rwymin = r['ymin'];
            rwxmax = r['xmax']; rwymax = r['ymax'];
         }
         for (i=1; i<graphs.length; ++i) {
            var rx1,ry1,rx2,ry2;
            g = graphs[i];
            var r = g.computeRange();
            rx1 = r['xmin']; ry1 = r['ymin'];
            rx2 = r['xmax']; ry2 = r['ymax'];
            if (rx1 < rwxmin) rwxmin = rx1;
            if (ry1 < rwymin) rwymin = ry1;
            if (rx2 > rwxmax) rwxmax = rx2;
            if (ry2 > rwymax) rwymax = ry2;
            if (g['fNpoints'] > npt) npt = g['fNpoints'];
         }
         if (rwxmin == rwxmax) rwxmax += 1.;
         if (rwymin == rwymax) rwymax += 1.;
         dx = 0.05*(rwxmax-rwxmin);
         dy = 0.05*(rwymax-rwymin);
         uxmin = rwxmin - dx;
         uxmax = rwxmax + dx;
         if (logy) {
            if (rwymin <= 0) rwymin = 0.001*rwymax;
            minimum = rwymin/(1+0.5*JSROOTMath.log10(rwymax/rwymin));
            maximum = rwymax*(1+0.2*JSROOTMath.log10(rwymax/rwymin));
         } else {
            minimum  = rwymin - dy;
            maximum  = rwymax + dy;
         }
         if (minimum < 0 && rwymin >= 0) minimum = 0;
         if (maximum > 0 && rwymax <= 0) maximum = 0;
      }
      if (mgraph['fMinimum'] != -1111) rwymin = minimum = mgraph['fMinimum'];
      if (mgraph['fMaximum'] != -1111) rwymax = maximum = mgraph['fMaximum'];
      if (uxmin < 0 && rwxmin >= 0) {
         if (logx) uxmin = 0.9*rwxmin;
         //else                 uxmin = 0;
      }
      if (uxmax > 0 && rwxmax <= 0) {
         if (logx) uxmax = 1.1*rwxmax;
         //else                 uxmax = 0;
      }
      if (minimum < 0 && rwymin >= 0) {
         if(logy) minimum = 0.9*rwymin;
         //else                minimum = 0;
      }
      if (maximum > 0 && rwymax <= 0) {
         if(logy) maximum = 1.1*rwymax;
         //else                maximum = 0;
      }
      if (minimum <= 0 && logy) minimum = 0.001*maximum;
      if (uxmin <= 0 && logx) {
         if (uxmax > 1000) uxmin = 1;
         else              uxmin = 0.001*uxmax;
      }
      rwymin = minimum;
      rwymax = maximum;
      if ('fHistogram' in mgraph && mgraph['fHistogram']) {
         mgraph['fHistogram']['fYaxis']['fXmin'] = rwymin;
         mgraph['fHistogram']['fYaxis']['fXmax'] = rwymax;
      }
      
      // Create a temporary histogram to draw the axis (if necessary)
      if (!histo) {
         // $("#report").append("<br> create dummy histo");
         histo = JSROOTPainter.CreateDummyHisto(); 
         histo['fXaxis']['fXmin'] = rwxmin;
         histo['fXaxis']['fXmax'] = rwxmax;
         histo['fYaxis']['fXmin'] = rwymin;
         histo['fYaxis']['fXmax'] = rwymax;
      }

      // histogram painter will be first in the pad, will define axis and interactive actions
      var hpainter = JSROOTPainter.drawHistogram1Dnew(vis, pad, histo, frame);
      
      for (var i=0; i<graphs.length; ++i) 
         JSROOTPainter.drawGraphNew(vis, pad, graphs[i], hpainter.frame);
   };
   
   
   JSROOTPainter.drawCanvas = function(canvas, idx) {
      var render_to = '#histogram' + idx,
          w = $(render_to).width(),
          factor = w / Math.abs(canvas['fUtoPixel']),
          h = Math.abs(canvas['fVtoPixel']) * factor,
          fillcolor = root_colors[canvas['fFillColor']];
      if (canvas['fFillStyle'] > 4000 && canvas['fFillStyle'] < 4100)
         fillcolor = 'none';

      d3.select(render_to).style("background-color", fillcolor);
      d3.select(render_to).style("width", "100%");

      var svg = d3.select(render_to).append("svg")
          .attr("width", w)
          .attr("height", h)
          .style("background-color", fillcolor);
      defs = svg.append('svg:defs');

      JSROOTPainter.drawPrimitives(svg, canvas);
      return svg;
   };

   
   JSROOTPainter.drawObject = function(obj, idx) 
   {
      var render_to = '#histogram' + idx;
      if (typeof($(render_to)[0]) == 'undefined') {
         obj = null;
         $("#report").append("<br>no place for draw"); 
         return;
      }
      $(render_to).empty();

      if (obj['_typename'].match(/\bTCanvas/)) {
         svg = JSROOTPainter.drawCanvas(obj, idx);
         return;
      }
      
      svg = JSROOTPainter.createCanvas($(render_to), idx);
      if (svg == null) return false;
      if (obj['_typename'].match(/\bJSROOTIO.TH1/)) {
         return JSROOTPainter.drawHistogram1Dnew(svg, null, obj, null);
      }
      else if (obj['_typename'].match(/\bJSROOTIO.TH2/)) {
         JSROOTPainter.drawHistogram2Dnew(svg, null, obj, null);
      }
      else if (obj['_typename'].match(/\bJSROOTIO.TH3/)) {
         JSROOTPainter.drawHistogram3D(svg, null, obj, null);
      }
      else if (obj['_typename'].match(/\bJSROOTIO.THStack/)) {
         JSROOTPainter.drawHStack(svg, null, obj, null)
      }
      else if (obj['_typename'].match(/\bJSROOTIO.TProfile/)) {
         return JSROOTPainter.drawHistogram1Dnew(svg, null, obj, null);
      }
      else if (obj['_typename'] == 'JSROOTIO.TF1') {
         JSROOTPainter.drawFunctionNew(svg, null, obj, null);
      }
      else if (obj['_typename'].match(/\bTGraph/) ||
            obj['_typename'].match(/\bRooHist/) ||
            obj['_typename'].match(/\RooCurve/)) {
         JSROOTPainter.drawGraphNew(svg, null, obj, null);
      }
      else if (obj['_typename'] == 'JSROOTIO.TMultiGraph') {
         JSROOTPainter.drawMultiGraphNew(svg, null, obj, null);
      }
      else if (typeof(drawUserObject) == 'function') {
         drawUserObject(obj, svg);
      }
   };
   

   JSROOTPainter.drawPad = function(vis, pad) {
      var width = vis.attr("width"), height = vis.attr("height");
      var x = pad['fAbsXlowNDC'] * width;
      var y = height - pad['fAbsYlowNDC'] * height;
      var w = pad['fAbsWNDC'] * width;
      var h = pad['fAbsHNDC'] * height;
      y -= h;

      var fillcolor = root_colors[pad['fFillColor']];
      if (pad['fFillStyle'] > 4000 && pad['fFillStyle'] < 4100)
         fillcolor = 'none';

      var border_width = pad['fLineWidth'];
      var border_color = root_colors[pad['fLineColor']];
      if (pad['fBorderMode'] == 0) {
         border_width = 0;
         border_color = 'none';
      }

      var new_pad = vis.append("svg:g")
         .attr("width", w)
         .attr("height", h)
         .attr("transform", "translate(" + x + "," + y + ")");

      new_pad.append("svg:rect")
         .attr("class", new_pad)
         .attr("x", 0)
         .attr("y", 0)
         .attr("width", w)
         .attr("height", h)
         .attr("fill", fillcolor)
         .style("stroke-width", border_width)
         .style("stroke", border_color);

      this.drawPrimitives(new_pad, pad);
      return new_pad;
   };

   JSROOTPainter.drawPaletteAxis = function(vis, palette, minbin, maxbin) {
      var width = vis.attr("width");
      var height = vis.attr("height");

      var pos_x = palette['fX1NDC'] * width;
      var pos_y = height - palette['fY1NDC'] * height;
      var s_width = Math.abs(palette['fX2NDC'] - palette['fX1NDC']) * width;
      var s_height = Math.abs(palette['fY2NDC'] - palette['fY1NDC']) * height;
      pos_y -= s_height;

      /*
       * Draw palette pad
       */
      var pal = vis.append("svg:g")
          .attr("height", s_height)
          .attr("width", s_width)
          .attr("transform", "translate(" + pos_x + ", " + pos_y + ")");

      var axis = palette['fAxis'];

      /*
       * Draw the default palette
       */
      var rectHeight = s_height / default_palette.length;
      pal.selectAll("colorRect")
         .data(default_palette)
         .enter()
         .append("svg:rect")
         .attr("class", "colorRect")
         .attr("x", 0)
         .attr("y", function(d, i) { return s_height - (i + 1)* rectHeight;})
         .attr("width", s_width)
         .attr("height", rectHeight)
         .attr("fill", function(d) { return d;})
         .attr("stroke", function(d) {return d;});
      /*
       * Build and draw axes
       */
      var nbr1 = 8;//Math.max((ndiv % 10000) % 100, 1);
      var nbr2 = 0;//Math.max(Math.round((ndiv % 10000) / 100), 1);
      var nbr3 = 0;//Math.max(Math.round(ndiv / 10000), 1);

      var z = d3.scale.linear().clamp(true)
               .domain([minbin, maxbin])
               .range([s_height, 0])
               .nice();

      var axisOffset = axis['fLabelOffset'] * width;
      var tickSize = axis['fTickSize'] * width;
      var z_axis = d3.svg.axis()
                  .scale(z)
                  .orient("right")
                  .tickSubdivide(nbr2)
                  .tickPadding(axisOffset)
                  .tickSize(-tickSize, -tickSize/2, 0)
                  .ticks(nbr1);

       var zax = pal.append("svg:g")
               .attr("class", "zaxis")
               .attr("transform", "translate(" + s_width + ", 0)")
               .call(z_axis);

      var axisFontDetails = getFontDetails(root_fonts[Math.floor(axis['fLabelFont'] /10)]);
      var axisLabelFontSize = axis['fLabelSize'] * height;
      zax.selectAll("text")
         .attr("font-size", axisLabelFontSize)
         .attr("font-weight", axisFontDetails['weight'])
         .attr("font-style", axisFontDetails['style'])
         .attr("font-family", axisFontDetails['name'])
         .attr("fill", root_colors[axis['fLabelColor']]);

      /*
       * Add palette axis title
       */
      var title = axis['fTitle'];
      if (title != "") {
         axisFontDetails = getFontDetails(root_fonts[Math.floor(axis['fTitleFont'] /10)]);
         var axisTitleFontSize = axis['fTitleSize'] * height;
         pal.append("text")
               .attr("class", "Z axis label")
               .attr("x", s_width + axisLabelFontSize + axisLabelOffset)
               .attr("y", s_height)
               .attr("text-anchor", "end")
               .attr("font-family", axisFontDetails['name'])
               .attr("font-weight", axisFontDetails['weight'])
               .attr("font-style", axisFontDetails['style'])
               .attr("font-size", axisTitleFontSize )
               .text(title);
      }
   };

   JSROOTPainter.drawPaveLabel = function(vis, pavelabel) {
      var w = vis.attr("width"), h = vis.attr("height");
      var pos_x = pavelabel['fX1NDC'] * w;
      var pos_y = (1.0 - pavelabel['fY1NDC']) * h;
      var width = Math.abs(pavelabel['fX2NDC'] - pavelabel['fX1NDC']) * w;
      var height = Math.abs(pavelabel['fY2NDC'] - pavelabel['fY1NDC']) * h;
      pos_y -= height;
      var font_size = Math.round(height / 1.9);
      var fcolor = root_colors[pavelabel['fFillColor']];
      var lcolor = root_colors[pavelabel['fLineColor']];
      var tcolor = root_colors[pavelabel['fTextColor']];
      var scolor = root_colors[pavelabel['fShadowColor']];
      if (pavelabel['fFillStyle'] == 0) fcolor = 'none';
      // align = 10*HorizontalAlign + VerticalAlign
      // 1=left adjusted, 2=centered, 3=right adjusted
      // 1=bottom adjusted, 2=centered, 3=top adjusted
      var align = 'start', halign = Math.round(pavelabel['fTextAlign']/10);
      var baseline = 'bottom', valign = pavelabel['fTextAlign']%10;
      if (halign == 1) align = 'start';
      else if (halign == 2) align = 'middle';
      else if (halign == 3) align = 'end';
      if (valign == 1) baseline = 'bottom';
      else if (valign == 2) baseline = 'middle';
      else if (valign == 3) baseline = 'top';
      var lmargin = 0;
      switch (halign) {
         case 1:
            lmargin = pavelabel['fMargin'] * width;
            break;
         case 2:
            lmargin = width/2;
            break;
         case 3:
            lmargin = width - (pavelabel['fMargin'] * width);
            break;
      }
      var lwidth = pavelabel['fBorderSize'] ? pavelabel['fBorderSize'] : 0;
      var fontDetails = getFontDetails(root_fonts[Math.floor(pavelabel['fTextFont']/10)]);

      var pave = vis.append("svg:g")
         .attr("width", width)
         .attr("height", height)
         .attr("transform", "translate(" + pos_x + "," + pos_y + ")");

      pave.append("svg:rect")
         .attr("class", pave)
         .attr("x", 0)
         .attr("y", 0)
         .attr("width", width)
         .attr("height", height)
         .attr("fill", fcolor)
         .style("stroke-width", lwidth ? 1 : 0)
         .style("stroke", lcolor);

      var line = this.translateLaTeX(pavelabel['fLabel']);

      var lw = stringWidth(vis, line, fontDetails['name'], fontDetails['weight'],
                           font_size, fontDetails['style']);
      if (lw > width)
         font_size *= 0.98 * (width / lw);

      pave.append("text")
         .attr("class", "text")
         .attr("text-anchor", align)
         .attr("x", lmargin)
         .attr("y", (height/2) + (font_size/3))
         .attr("font-weight", fontDetails['weight'])
         .attr("font-style", fontDetails['style'])
         .attr("font-family", fontDetails['name'])
         .attr("font-size", font_size)
         .attr("fill", tcolor)
         .text(line);

      if (lwidth && lwidth > 1) {
         pave.append("svg:line")
            .attr("x1", width+(lwidth/2))
            .attr("y1", lwidth+1)
            .attr("x2", width+(lwidth/2))
            .attr("y2", height+lwidth-1)
            .style("stroke", lcolor)
            .style("stroke-width", lwidth);
         pave.append("svg:line")
            .attr("x1", lwidth+1)
            .attr("y1", height+(lwidth/2))
            .attr("x2", width+lwidth-1)
            .attr("y2", height+(lwidth/2))
            .style("stroke", lcolor)
            .style("stroke-width", lwidth);
      }
   };

   JSROOTPainter.drawPaveText = function(vis, pavetext) {
      var i, j, lw, w = vis.attr("width"), h = vis.attr("height");
      var pos_x = pavetext['fX1NDC'] * w;
      var pos_y = (1.0 - pavetext['fY1NDC']) * h;
      var width = Math.abs(pavetext['fX2NDC'] - pavetext['fX1NDC']) * w;
      var height = Math.abs(pavetext['fY2NDC'] - pavetext['fY1NDC']) * h;
      pos_y -= height;
      var line, nlines = pavetext['fLines'].length;
      var font_size = Math.round(height / (nlines * 1.5));
      var fcolor = root_colors[pavetext['fFillColor']];
      var lcolor = root_colors[pavetext['fLineColor']];
      var tcolor = root_colors[pavetext['fTextColor']];
      var scolor = root_colors[pavetext['fShadowColor']];
      if (pavetext['fFillStyle'] == 0) fcolor = 'none';
      // align = 10*HorizontalAlign + VerticalAlign
      // 1=left adjusted, 2=centered, 3=right adjusted
      // 1=bottom adjusted, 2=centered, 3=top adjusted
      // "middle", "start", "end"
      var align = 'start', halign = Math.round(pavetext['fTextAlign']/10);
      var baseline = 'bottom', valign = pavetext['fTextAlign']%10;
      if (halign == 1) align = 'start';
      else if (halign == 2) align = 'middle';
      else if (halign == 3) align = 'end';
      if (valign == 1) baseline = 'bottom';
      else if (valign == 2) baseline = 'middle';
      else if (valign == 3) baseline = 'top';
      var lmargin = 0;
      switch (halign) {
         case 1:
            lmargin = pavetext['fMargin'] * width;
            break;
         case 2:
            lmargin = width/2;
            break;
         case 3:
            lmargin = width - (pavetext['fMargin'] * width);
            break;
      }
      var fontDetails = getFontDetails(root_fonts[Math.floor(pavetext['fTextFont']/10)]);
      var lwidth = pavetext['fBorderSize'] ? pavetext['fBorderSize'] : 0;

      var pave = vis.append("svg:g")
         .attr("x", pos_x)
         .attr("y", pos_y)
         .attr("width", width)
         .attr("height", height)
         .attr("transform", "translate(" + pos_x + "," + pos_y + ")");

      pave.append("svg:rect")
         .attr("class", "frame")
         .attr("x", 0)
         .attr("y", 0)
         .attr("height", height)
         .attr("width", width)
         .attr("fill", fcolor)
         .style("stroke-width", lwidth ? 1 : 0)
         .style("stroke", lcolor);

      if (nlines == 1) {
         line = this.translateLaTeX(pavetext['fLines'][0]['fTitle']);

         lw = stringWidth(vis, line, fontDetails['name'], fontDetails['weight'],
                          font_size, fontDetails['style']);
         if (lw > width)
            font_size *= 0.98 * (width / lw);

         pave.append("text")
            .attr("class", "stat text")
            .attr("text-anchor", align)
            .attr("x", lmargin)
            .attr("y", (height/2) + (font_size/3))
            .attr("font-family", fontDetails['name'])
            .attr("font-weight", fontDetails['weight'])
            .attr("font-style", fontDetails['style'])
            .attr("font-size", font_size)
            .attr("fill", tcolor)
            .text(line);
      }
      else {
         var max_len = 0;
         for (j=0; j<nlines; ++j) {
            line = this.translateLaTeX(pavetext['fLines'][j]['fTitle']);
            lw = lmargin + stringWidth(vis, line, fontDetails['name'], fontDetails['weight'],
                                       font_size, fontDetails['style']);
            if (lw > max_len) max_len = lw;
         }
         if (max_len > width)
            font_size *= 0.98 * (width / max_len);

         for (j=0; j<nlines; ++j) {
            var jcolor = root_colors[pavetext['fLines'][j]['fTextColor']];
            if (pavetext['fLines'][j]['fTextColor'] == 0)
               jcolor = tcolor;
            line = this.translateLaTeX(pavetext['fLines'][j]['fTitle']);
            if (pavetext['_typename'] == 'JSROOTIO.TPaveStats') {
               var off_y = (j == 0) ? 0 : (font_size * 0.05);
               pave.append("text")
                  .attr("class", "stat text")
                  .attr("text-anchor", (j == 0) ? "middle" : "start")
                  .attr("x", ((j==0) ? width/2 : pavetext['fMargin'] * width))
                  .attr("y", ((j == 0) ? off_y + (font_size * 1.2) : off_y + ((j+1) * (font_size * 1.4))))
                  .attr("font-family", fontDetails['name'])
                  .attr("font-weight", fontDetails['weight'])
                  .attr("font-style", fontDetails['style'])
                  .attr("font-size", font_size)
                  .attr("fill", jcolor)
                  .text(line);
            }
            else {
               pave.append("text")
                  .attr("class", "stat text")
                  .attr("text-anchor", "start")
                  .attr("x", lmargin)
                  .attr("y", (j+1) * (font_size * 1.4))
                  .attr("font-family", fontDetails['name'])
                  .attr("font-weight", fontDetails['weight'])
                  .attr("font-style", fontDetails['style'])
                  .attr("font-size", font_size)
                  .attr("fill", jcolor)
                  .text(line);
            }
         }
      }

      if (pavetext['fBorderSize'] && pavetext['_typename'] == 'JSROOTIO.TPaveStats')
         pave.append("svg:line")
            .attr("x1", 0)
            .attr("y1", font_size * 1.6)
            .attr("x2", width)
            .attr("y2", font_size * 1.6)
            .style("stroke", lcolor)
            .style("stroke-width", lwidth ? 1 : 'none');

      if (lwidth && lwidth > 1) {
         pave.append("svg:line")
            .attr("x1", width+(lwidth/2))
            .attr("y1", lwidth+1)
            .attr("x2", width+(lwidth/2))
            .attr("y2", height+lwidth-1)
            .style("stroke", lcolor)
            .style("stroke-width", lwidth);
         pave.append("svg:line")
            .attr("x1", lwidth+1)
            .attr("y1", height+(lwidth/2))
            .attr("x2", width+lwidth-1)
            .attr("y2", height+(lwidth/2))
            .style("stroke", lcolor)
            .style("stroke-width", lwidth);
      }
   };

   JSROOTPainter.drawPrimitives = function(vis, pad) {
      var i, j, frame = null;
      var primitives = pad['fPrimitives'];
      for (i=0; i<primitives.length; ++i) {
         var classname = primitives[i]['_typename'];
         
         // $("#report").append("<br> draw primitive " + classname);
         
         if (classname == 'JSROOTIO.TFrame') {
            frame = JSROOTPainter.createFrame(vis, pad, null, primitives[i]);
         }
         if (classname == 'JSROOTIO.TPad') {
            this.drawPad(vis, primitives[i])
         }
         if (classname == 'JSROOTIO.TPaveLabel') {
            this.drawPaveLabel(vis, primitives[i]);
         }
         if (classname == 'JSROOTIO.TLegend') {
            this.drawLegend(vis, pad, primitives[i]);
         }
         if (classname == 'JSROOTIO.TPaveText') {
            this.drawPaveText(vis, primitives[i]);
         }
         if (classname == 'JSROOTIO.TLatex') {
            this.drawText(vis, pad, primitives[i]);
         }
         if (classname == 'JSROOTIO.TText') {
            this.drawText(vis, pad, primitives[i]);
         }
         if (classname.match(/\bJSROOTIO.TH1/) || 
             (classname=="JSROOTIO.TProfile")) {
            JSROOTPainter.drawHistogram1Dnew(vis, pad, primitives[i], frame);
         }
         if (classname.match(/\bJSROOTIO.TH2/)) {
            JSROOTPainter.drawHistogram2Dnew(vis, pad, primitives[i], frame);
         }
         if (classname.match(/\bJSROOTIO.THStack/)) {
            this.drawHStack(vis, pad, primitives[i], frame)
         }
         if (classname == 'JSROOTIO.TF1') {
            JSROOTPainter.drawFunctionNew(vis, pad, primitives[i], frame);
         }
         if (classname.match(/\bTGraph/) ||
             classname.match(/\bRooHist/) ||
             classname.match(/\RooCurve/)) {
            JSROOTPainter.drawGraphNew(vis, pad, primitives[i], frame);
         }
         if (classname == 'JSROOTIO.TMultiGraph') {
            //$("#report").append("<br> draw multigraph");
            JSROOTPainter.drawMultiGraphNew(vis, pad, primitives[i], frame);
         }
      }
   };


   JSROOTPainter.drawText = function(vis, pad, text) {
      // align = 10*HorizontalAlign + VerticalAlign
      // 1=left adjusted, 2=centered, 3=right adjusted
      // 1=bottom adjusted, 2=centered, 3=top adjusted
      var i, w = vis.attr("width"), h = vis.attr("height");
      var align = 'start', halign = Math.round(text['fTextAlign']/10);
      var baseline = 'bottom', valign = text['fTextAlign']%10;
      if (halign == 1) align = 'start';
      else if (halign == 2) align = 'middle';
      else if (halign == 3) align = 'end';
      if (valign == 1) baseline = 'bottom';
      else if (valign == 2) baseline = 'middle';
      else if (valign == 3) baseline = 'top';
      var lmargin = 0;
      switch (halign) {
         case 1:
            lmargin = text['fMargin'] * w;
            break;
         case 2:
            lmargin = w/2;
            break;
         case 3:
            lmargin = w - (text['fMargin'] * w);
            break;
      }
      var font_size = Math.round(text['fTextSize'] * 0.7 * h);
      if (text.TestBit(kTextNDC)) {
         var pos_x = pad['fX1'] + text['fX']*(pad['fX2'] - pad['fX1']);
         var pos_y = pad['fY1'] + text['fY']*(pad['fY2'] - pad['fY1']);
      }
      else {
         font_size = Math.round(text['fTextSize'] * h);
         var pos_x = this.xtoPad(text['fX'], pad);
         var pos_y = this.ytoPad(text['fY'], pad);
      }
      pos_x = ((Math.abs(pad['fX1'])+pos_x)/(pad['fX2'] - pad['fX1']))*w;
      pos_y = (1-((Math.abs(pad['fY1'])+pos_y)/(pad['fY2'] - pad['fY1'])))*h;
      var tcolor = root_colors[text['fTextColor']];
      var fontDetails = getFontDetails(root_fonts[Math.floor(text['fTextFont']/10)]);

      var string = text['fTitle'];
      // translate the LaTeX symbols
      if (text['_typename'] == 'JSROOTIO.TLatex')
         string = this.translateLaTeX(text['fTitle']);

      vis.append("text")
         .attr("class", "text")
         .attr("x", pos_x)
         .attr("y", pos_y)
         .attr("font-family", fontDetails['name'])
         .attr("font-weight", fontDetails['weight'])
         .attr("font-style", fontDetails['style'])
         .attr("font-size", font_size)
         .attr("text-anchor", align)
         .attr("fill", tcolor)
         .text(string);
   };

   /**
    * List tree (dtree) related functions
    */

   JSROOTPainter.displayBranches = function(branches, dir_id, k) {
      for (var i=0; i<branches.length; ++i) {
         var nb_leaves = branches[i]['fLeaves'].length;
         var disp_name = branches[i]['fName'];
         var node_img = source_dir+'img/branch.png';
         var node_title = disp_name;
         var tree_link = "";
         if (nb_leaves == 0) {
            node_img = source_dir+'img/leaf.png';
         }
         else if (nb_leaves == 1 && branches[i]['fLeaves'][0]['fName'] == disp_name) {
            node_img = source_dir+'img/leaf.png';
            nb_leaves--;
         }
         if (branches[i]['fBranches'].length > 0) {
            node_img = source_dir+'img/branch.png';
         }
         key_tree.add(k, dir_id, disp_name, tree_link, node_title, '', node_img, node_img);
         nid = k; k++;
         for (var j=0; j<nb_leaves; ++j) {
            var disp_name = branches[i]['fLeaves'][j]['fName'];
            var node_title = disp_name;
            var node_img = source_dir+'img/leaf.png';
            var tree_link = "";
            key_tree.add(k, nid, disp_name, tree_link, node_title, '', node_img, node_img);
            k++;
         }
         if (branches[i]['fBranches'].length > 0) {
            k = JSROOTPainter.displayBranches(branches[i]['fBranches'], nid, k);
         }
      }
      return k;
   };

   JSROOTPainter.displayTree = function(tree, container, dir_id) {
      var tree_link = '';
      var content = "<p><a href='javascript: key_tree.openAll();'>open all</a> | <a href='javascript: key_tree.closeAll();'>close all</a></p>";
      var k = key_tree.aNodes.length;
      JSROOTPainter.displayBranches(tree['fBranches'], dir_id, k);
      content += key_tree;
      $(container).append(content);
      key_tree.openTo(dir_id, true);
   };

   JSROOTPainter.displayListOfKeys = function(keys, container) {
      delete key_tree;
      var content = "<p><a href='javascript: key_tree.openAll();'>open all</a> | <a href='javascript: key_tree.closeAll();'>close all</a></p>";
      key_tree = new dTree('key_tree');
      key_tree.config.useCookies = false;
      key_tree.add(0, -1, 'File Content');
      var k = 1;
      var tree_link = '';
      for (var i=0; i<keys.length; ++i) {
         var message = keys[i]['className']+' is not yet implemented.';
         tree_link = "javascript:  alert('" + message + "')";
         var node_img = source_dir+'img/page.gif';
         if (keys[i]['className'].match(/\bTH1/) ||
             keys[i]['className'].match(/\bRooHist/)) {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/histo.png';
         }
         else if (keys[i]['className'].match(/\bTH2/)) {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/histo2d.png';
         }
         else if (keys[i]['className'].match(/\bTH3/)) {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/histo3d.png';
         }
         else if (keys[i]['className'].match(/\bTGraph/) ||
             keys[i]['className'].match(/\RooCurve/)) {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/graph.png';
         }
         else if (keys[i]['className'] ==  'TF1') {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/graph.png';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['className'] ==  'TProfile') {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/profile.png';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['name'] == 'StreamerInfo') {
            tree_link = "javascript: displayStreamerInfos(gFile.fStreamerInfo.fStreamerInfos);";
            node_img = source_dir+'img/question.gif';
         }
         else if (keys[i]['className'] == 'TDirectory') {
            tree_link = "javascript: showDirectory('"+keys[i]['name']+"',"+keys[i]['cycle']+","+(i+1)+");";
            node_img = source_dir+'img/folder.gif';
         }
         else if (keys[i]['className'] == 'TList' || keys[i]['className'] == 'TObjArray') {
            tree_link = "javascript: showCollection('"+keys[i]['name']+"',"+keys[i]['cycle']+","+(i+1)+");";
            node_img = source_dir+'img/folder.gif';
         }
         else if (keys[i]['className'] == 'TTree' || keys[i]['className'] == 'TNtuple') {
            tree_link = "javascript: readTree('"+keys[i]['name']+"',"+keys[i]['cycle']+","+(i+1)+");";
            node_img = source_dir+'img/tree.png';
         }
         else if (keys[i]['className'].match('TGeoManager') ||
                  keys[i]['className'].match('TGeometry')) {
            node_img = source_dir+'img/folder.gif';
         }
         else if (keys[i]['className'].match('TCanvas')) {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/canvas.png';
            node_title = keys[i]['name'];
         }
         if (keys[i]['name'] != '' && keys[i]['className'] != 'TFile')
            if (keys[i]['className'] == 'TDirectory' || keys[i]['className'] == 'TList' ||
                keys[i]['className'] == 'TObjArray')
               key_tree.add(k, 0, keys[i]['name']+';'+keys[i]['cycle'], tree_link, keys[i]['name'], '', node_img,
                            source_dir+'img/folderopen.gif');
            else if (keys[i]['className'] == 'TTree' || keys[i]['className'] == 'TNtuple')
               key_tree.add(k, 0, keys[i]['name']+';'+keys[i]['cycle'], tree_link, keys[i]['name'], '', node_img, node_img);
            else
               key_tree.add(k, 0, keys[i]['name']+';'+keys[i]['cycle'], tree_link, keys[i]['name'], '', node_img);
            k++;
      }
      content += key_tree;
      $(container).append(content);
      // to display the first object in the file, uncomment the following line
      // setTimeout( function() { showObject(keys[0]['name'],keys[0]['cycle']); }, 20 );
   };

   JSROOTPainter.addDirectoryKeys = function(keys, container, dir_id) {
      var pattern_th1 = /TH1/g;
      var pattern_th2 = /TH2/g;
      var tree_link = '';
      var content = "<p><a href='javascript: key_tree.openAll();'>open all</a> | <a href='javascript: key_tree.closeAll();'>close all</a></p>";
      var k = key_tree.aNodes.length;
      var dir_name = key_tree.aNodes[dir_id]['title'];
      for (var i=0; i<keys.length; ++i) {
         var disp_name = keys[i]['name'];
         keys[i]['name'] = dir_name + '/' + keys[i]['name'];
         var message = keys[i]['className']+' is not yet implemented.';
         tree_link = "javascript:  alert('" + message + "')";
         var node_img = source_dir+'img/page.gif';
         var node_title = keys[i]['className'];
         if (keys[i]['className'].match(/\bTH1/) ||
             keys[i]['className'].match(/\bRooHist/)) {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/histo.png';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['className'].match(/\bTH2/)) {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/histo2d.png';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['className'].match(/\bTH3/)) {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/histo3d.png';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['className'].match(/\bTGraph/) ||
             keys[i]['className'].match(/\RooCurve/)) {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/graph.png';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['className'] ==  'TProfile') {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/profile.png';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['name'] == 'StreamerInfo') {
            tree_link = "javascript: displayStreamerInfos(gFile.fStreamerInfo.fStreamerInfos);";
            node_img = source_dir+'img/question.gif';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['className'] == 'TDirectory') {
            tree_link = "javascript: showDirectory('"+keys[i]['name']+"',"+keys[i]['cycle']+","+k+");";
            node_img = source_dir+'img/folder.gif';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['className'] == 'TList' || keys[i]['className'] == 'TObjArray') {
            tree_link = "javascript: showCollection('"+keys[i]['name']+"',"+keys[i]['cycle']+","+k+");";
            node_img = source_dir+'img/folder.gif';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['className'] == 'TTree' || keys[i]['className'] == 'TNtuple') {
            tree_link = "javascript: readTree('"+keys[i]['name']+"',"+keys[i]['cycle']+","+k+");";
            node_img = source_dir+'img/tree.png';
            node_title = keys[i]['name'];
         }
         else if (keys[i]['className'].match('TCanvas')) {
            tree_link = "javascript: showObject('"+keys[i]['name']+"',"+keys[i]['cycle']+");";
            node_img = source_dir+'img/canvas.png';
            node_title = keys[i]['name'];
         }
         if (keys[i]['name'] != '' && keys[i]['className'] != 'TFile') {
            if (keys[i]['className'] == 'TDirectory' || keys[i]['className'] == 'TList' ||
                keys[i]['className'] == 'TObjArray')
               key_tree.add(k, dir_id, disp_name+';'+keys[i]['cycle'], tree_link, node_title, '', node_img,
                            source_dir+'img/folderopen.gif');
            else if (keys[i]['className'] == 'TNtuple' || keys[i]['className'] == 'TTree')
               key_tree.add(k, dir_id, disp_name+';'+keys[i]['cycle'], tree_link, node_title, '', node_img, node_img);
            else
               key_tree.add(k, dir_id, disp_name+';'+keys[i]['cycle'], tree_link, node_title, '', node_img);
            k++;
         }
      }
      content += key_tree;
      $(container).append(content);
      key_tree.openTo(dir_id, true);
   };

   JSROOTPainter.addCollectionContents = function(list, container, dir_id) {
      var pattern_th1 = /TH1/g;
      var pattern_th2 = /TH2/g;
      var tree_link = '';
      var content = "<p><a href='javascript: key_tree.openAll();'>open all</a> | <a href='javascript: key_tree.closeAll();'>close all</a></p>";
      var k = key_tree.aNodes.length;
      var dir_name = key_tree.aNodes[dir_id]['title'];
      for (var i=0; i<list.length; ++i) {
         var disp_name = list[i]['fName'];
         list[i]['_name'] = dir_name + '/' + list[i]['fName'];
         var message = list[i]['_typename']+' is not yet implemented.';
         tree_link = "javascript:  alert('" + message + "')";
         var node_img = source_dir+'img/page.gif';
         var node_title = list[i]['_typename'];
         if (list[i]['_typename'].match(/\bTH1/) ||
             list[i]['_typename'].match(/\bTH2/) ||
             list[i]['_typename'].match(/\bTH3/) ||
             list[i]['_typename'].match(/\bTGraph/) ||
             list[i]['_typename'].match(/\bRooHist/) ||
             list[i]['_typename'].match(/\RooCurve/)) {
            //tree_link = "javascript: displayMappedObject('"+list[i]['_name']+"');";
            tree_link = "javascript: displayMappedObject('"+list[i]['_name']+"','"+list[i]['_listname']+"',"+list[i]['pos']+");";
            node_img = source_dir+'img/graph.png';
            node_title = list[i]['_name'];
         }
         else if (list[i]['_typename'] == 'TList' || list[i]['_typename'] == 'TObjArray') {
            tree_link = "javascript: showCollection('"+list[i]['_name']+"', 0, "+k+");";
            node_img = source_dir+'img/folder.gif';
            node_title = list[i]['_name'];
         }
         if (list[i]['fName'] != '' && list[i]['_typename'] != 'TFile') {
            key_tree.add(k, dir_id, disp_name, tree_link, node_title, '', node_img);
            k++;
         }
      }
      content += key_tree;
      $(container).append(content);
      key_tree.openTo(dir_id, true);
   };

   JSROOTPainter.displayStreamerInfos = function(streamerInfo, container) {

      delete d;
      var content = "<p><a href='javascript: d_tree.openAll();'>open all</a> | <a href='javascript: d_tree.closeAll();'>close all</a></p>";
      d_tree = new dTree('d_tree');
      d_tree.config.useCookies = false;
      d_tree.add(0, -1, 'Streamer Infos');

      var k = 1;
      var pid = 0;
      var cid = 0;
      var key;
      for (key in streamerInfo) {
         var entry = streamerInfo[key]['name'];
         d_tree.add(k, 0, entry); k++;
      }
      var j=0;
      for (key in streamerInfo) {
         if (typeof(streamerInfo[key]['checksum']) != 'undefined')
            d_tree.add(k, j+1, 'Checksum: ' + streamerInfo[key]['checksum']); ++k;
         if (typeof(streamerInfo[key]['classversion']) != 'undefined')
            d_tree.add(k, j+1, 'Class Version: ' + streamerInfo[key]['classversion']); ++k;
         if (typeof(streamerInfo[key]['title']) != 'undefined')
            d_tree.add(k, j+1, 'Title: ' + streamerInfo[key]['title']); ++k;
         if (typeof(streamerInfo[key]['elements']) != 'undefined') {
            d_tree.add(k, j+1, 'Elements'); pid=k; ++k;
            for (var l=0; l<streamerInfo[key]['elements']['array'].length; ++l) {
               if (typeof(streamerInfo[key]['elements']['array'][l]['element']) != 'undefined') {
                  d_tree.add(k, pid, streamerInfo[key]['elements']['array'][l]['element']['name']); cid=k; ++k;
                  d_tree.add(k, cid, streamerInfo[key]['elements']['array'][l]['element']['title']); ++k;
                  d_tree.add(k, cid, streamerInfo[key]['elements']['array'][l]['element']['typename']); ++k;
               }
               else if (typeof(streamerInfo[key]['elements']['array'][l]['name']) != 'undefined') {
                  d_tree.add(k, pid, streamerInfo[key]['elements']['array'][l]['name']); cid=k; ++k;
                  d_tree.add(k, cid, streamerInfo[key]['elements']['array'][l]['title']); ++k;
                  d_tree.add(k, cid, streamerInfo[key]['elements']['array'][l]['typename']); ++k;
               }
            }
         }
         else if (typeof(streamerInfo[key]['array']) != 'undefined') {
            for (var l=0; l<streamerInfo[key]['array'].length; ++l) {
               d_tree.add(k, j+1, streamerInfo[key]['array'][l]['str']); ++k;
            }
         }
         ++j;
      }
      content += d_tree;
      $(container).html(content);
   };

   var style = "<style>\n"
      +".xaxis path, .xaxis line, .yaxis path, .yaxis line, .zaxis path, .zaxis line {\n"
      +"   fill: none;\n"
      +"   stroke: #000;\n"
      +"   shape-rendering: crispEdges;\n"
      +"}\n"
      +".brush .extent {\n"
      +"  stroke: #fff;\n"
      +"  fill-opacity: .125;\n"
      +"  shape-rendering: crispEdges;\n"
      +"}\n"
      +"rect.zoom {\n"
      +"  stroke: steelblue;\n"
      +"  fill-opacity: 0.1;\n"
      +"}\n"
      /* Correct overflow not hidden in IE9 */
      +"svg:not(:root) { overflow: hidden; }\n"
      +"</style>\n";
   $(style).prependTo("body");

})();


// JSROOTD3Painter.js ends


