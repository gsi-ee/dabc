
//JSROOT.define([], () => {
//
//   "use strict";
//
//   if (typeof DABC != "object") {
//      let e1 = new Error("elder.js requires DABC to be already loaded");
//      e1.source = "elder.js";
//      throw e1;
//   }
//// we like to refer DABC later

let ELDER = {};


ELDER.ConvertC = function(hpainter, item, obj) {

   if (!obj) return;
   let ix = 0;
   let xmin=0.0;  let xmax=0.0; let ymin=0.0;  let ymax=0.0; 

   let res = {};
   for (let k in obj) res[k] = obj[k]; // first copy all keys
   for (let k in res) delete obj[k];   // then delete them from source
   
   // first fetch the assigned histogram here:
   
   let histogramname=res.linked_histogram;
   let histofullpath = null;
   
//       if (!JSROOT.hpainter) return;
//
//      let histofullpath = null;
//
//      JSROOT.hpainter.forEachItem(h => {
//         if ((h._name == cond.fxHistoName) && h._kind && (h._kind.indexOf("ROOT.TH") == 0))
//            histofullpath = JSROOT.hpainter.itemFullName(h);
//      });
//
//      if (histofullpath === null) {
//         histofullpath = "../../Histograms/" + cond.fxHistoName;
//
//         let hitem = JSROOT.hpainter.findItem({ name: histofullpath, force: true });
//
//         hitem._kind = "ROOT.TH1I";
//      }
//
//      function drawCond(hpainter) {
//         if (!hpainter) return console.log("fail to draw histogram " + histofullpath);
//         condpainter.drawCondition();
//         condpainter.addToPadPrimitives();
//         return condpainter.drawLabel();
//      }
//
//      return JSROOT.hpainter.display(histofullpath, "divid:" + condpainter.getDomId()).then(hp => drawCond(hp));
   
   
   if (res._kind == 'ELDER.WINCON1') {
      obj=JSROOT.createTPolyLine(4); // what about this JAM?
      Object.assign(obj, { fName: res._name, fTitle: res._title, fFillStyle: 1001 });
      xmin=res.limits[0];
      xmax=res.limits[1];
      obj.fX[0]=xmin;  obj.fY[0]=0;
      obj.fX[1]=xmin;  obj.fY[1]=1000;// get maximum of currenthistogram here?
      obj.fX[2]=xmax;  obj.fY[2]=1000;
      obj.fX[3]=xmax;  obj.fY[3]=0;
//     obj.SetPoint(0,xmin,0);
//      obj.SetPoint(1,xmin,1000); // get maximum of currenthistogram here?
//      obj.SetPoint(2,xmax,1000);
//      obj.SetPoint(3,xmax,0);

   } else if (res._kind == 'ELDER.WINCON2') {
      obj=JSROOT.createTPolyLine(4); // res.numpoints
      Object.assign(obj, { fName: res._name, fTitle: res._title, fFillStyle: 1001 });
      xmin=res.limits[0];
      xmax=res.limits[1];
      ymin=res.limits[2];
      ymax=res.limits[3];
      obj.fX[0]=xmin;  obj.fY[0]=ymin;
      obj.fX[1]=xmin;  obj.fY[1]=ymax;
      obj.fX[2]=xmax;  obj.fY[2]=ymax;
      obj.fX[3]=xmax;  obj.fY[3]=ymin;

//      obj.SetPoint(0,xmin,ymin);
//      obj.SetPoint(1,xmin,ymax);
//      obj.SetPoint(2,xmax,ymax);
//      obj.SetPoint(3,xmax,ymin);
      
   } else if (res._kind == 'ELDER.POLYCON') {
       obj=JSROOT.createTPolyLine(res.numpoints/2);
       Object.assign(obj, { fName: res._name, fTitle: res._title, fFillStyle: 1001 });
       while(ix<res.numpoints/2)
       {
             obj.fX[ix]=res.limits[2*ix];  
             obj.fY[ix]=res.limits[2*ix+1];
           //obj.SetPoint(ix,res.limits[2*ix],res.limits[2*ix+1]);
       }    
    }
   obj._typename = 'TPolyLine';
   return obj;
};


JSROOT.addDrawFunc({
   name: "kind:ELDER.WINCON1",
   make_request: DABC.ReqH,
   after_request: ELDER.ConvertC,
   icon: "img_histo1d",
   opt: "same"
});


JSROOT.addDrawFunc({
   name: "kind:ELDER.WINCON2",
   make_request: DABC.ReqH,
   after_request: ELDER.ConvertC,
   icon: "img_histo2d",
   opt: "same"
});

JSROOT.addDrawFunc({
   name: "kind:ELDER.POLYCON",
   make_request: DABC.ReqH,
   after_request: ELDER.ConvertC,
   icon: "img_histo2d",
   opt: "same"
});



globalThis.ELDER = ELDER;


//})








   
   