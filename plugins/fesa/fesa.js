// example file for custom web output

(function(){

   if (typeof JSROOT != "object") {
      var e1 = new Error("fesa.js requires JSROOT to be already loaded");
      e1.source = "fesa.js";
      throw e1;
   }
   if (typeof DABC != "object") {
      var e1 = new Error("fesa.js requires dabc.js to be already loaded");
      e1.source = "fesa.js";
      throw e1;
   }

   FESA = {};

   FESA.MakeItemRequest = function(h, item, fullpath, option) {
      return { req: "dabc.bin", kind:"bin" }; // use binary request to receive data
   }

   FESA.AfterItemRequest = function(h, item, data, option) {
      if (!data) return null;

      var hist = JSROOT.CreateHistogram("TH2I", 16, 16);
      hist.fName  = "BeamProfile";
      hist.fTitle = "Beam profile from FESA";
      hist.fOption = "col";

      if ((data==null) || (data.length != 16*16*4)) {
         alert("no proper data for beam profile");
         return false;
      }

      var o = 0;

      function ntou4() {
         // convert (read) 4 bytes into uint32_t
         var n  = (data.charCodeAt(o)  & 0xff);
         n += (data.charCodeAt(o+1) & 0xff) << 8;
         n += (data.charCodeAt(o+2) & 0xff) << 16;
         n += (data.charCodeAt(o+3) & 0xff) << 24;
         return n;
      }

      for (var iy=0;iy<16;iy++)
         for (var ix=0;ix<16;ix++) {
            var value = ntou4(); o+=4;
            var bin = hist.getBin(ix+1, iy+1);
            hist.setBinContent(bin, value);
         }

      return hist;
   }

   JSROOT.addDrawFunc({
      name: "kind:FESA.2D",
      make_request: FESA.MakeItemRequest,
      after_request: FESA.AfterItemRequest,
      icon: "img_histo2d",
      opt: "col;colz"
   });

}());