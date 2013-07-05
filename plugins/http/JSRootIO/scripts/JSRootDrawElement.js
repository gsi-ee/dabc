
JSROOT = {};


JSROOT.source_dir = "jsrootiosys/";


// class should keep complete information to draw arbitrary element

JSROOT.DrawElement = function() {
   this.fullname = "";     // full name of element, can include file name, directory, version
   
   this.displayname = "";  // name, which is shown on display
   
   this.topframeid = "";    // top frame which contain all subelements    
   
   this.infoframeid = "";   // frame for different information, remains visible even when collapsed

   this.drawframeid = "";   // frame for drawing (can be the only frame)
   
   this.version = 0;
   
   return this;
}



JSROOT.DrawElement.prototype.makeCollapsible = function() {
   
   if (this.infoframeid == "") return;
   
   var element = $("#"+this.infoframeid);
   
   element
       .addClass("ui-accordion-header ui-helper-reset ui-state-default ui-corner-top ui-corner-bottom")
       .hover(function() { $(this).toggleClass("ui-state-hover"); })
       .prepend('<span class="ui-icon ui-icon-triangle-1-e"></span>')
       
       .click(function() {
          $(this)
             .toggleClass("ui-accordion-header-active ui-state-active ui-state-default ui-corner-bottom")
             .find("> .ui-icon").toggleClass("ui-icon-triangle-1-e ui-icon-triangle-1-s").end()
             .next().toggleClass("ui-accordion-content-active").slideToggle(0);
          return false;
       });

//   var btns = $("#"+  this.infoframeid + "_btn");
   element.append('<button type="button" class="closeButton" title="close canvas" onclick="JSROOT.elements.closeFrame(\''+this.topframeid+'\')"><img src="'+JSROOT.source_dir+'/img/remove.gif"/></button>');
   

   
   var draw = $("#"+  this.drawframeid);
   draw.addClass("ui-accordion-content  ui-helper-reset ui-widget-content ui-corner-bottom").hide();

   element
      .toggleClass("ui-accordion-header-active ui-state-active ui-state-default ui-corner-bottom")
      .find("> .ui-icon").toggleClass("ui-icon-triangle-1-e ui-icon-triangle-1-s");
   
   draw.toggleClass("ui-accordion-content-active").slideToggle(0);
};

JSROOT.DrawElement.prototype.setInfo = function(info)
{
   if (this.infoframeid == "") return;
   
   var child = document.getElementById(this.infoframeid + "_txt");
   // if (child) $("#report").append("<br>child " + child.innerHTML);
   if (child) child.innerHTML = info;
       else $("#report").append("<br> not found child "+this.infoframeid);
}

JSROOT.DrawElement.prototype.appendText = function(txt)
{
   if (this.drawframeid == "") return;
   var draw = $("#"+  this.drawframeid);
   draw.append(txt);
}

// ===============================================


JSROOT.DrawElements = function() {
   this.arr = new Array;     // array of all known elements
   this.global_counter = 0;  // counter to generate unique
   
   return this;
} 


JSROOT.DrawElements.prototype.Find = function(_item) {
   for (var i in this.arr) {
      if (this.arr[i].itemname == _item) return this.arr[i];
   }
}

// produce unique identifier
JSROOT.DrawElements.prototype.generateUniqueId = function() {
   return "draw_" + this.global_counter++; 
}

JSROOT.DrawElements.prototype.generateNewFrame = function(holder)
{
   var node = $("#" + holder);
   var id = this.generateUniqueId();
   node.append("<div id = '" + id + "'/>");
   return id;
}


// initialize draw element in already existing node with id topid
// element should be created before
// one can reserve place for information and buttons
JSROOT.DrawElements.prototype.initElement = function(topid, elem, info) {
   var topnode = $("#" + topid);
   
   if (!topnode) {
      alert("Node with id " + topid + " not exists");
      return;
   }
   
   if (!elem) return;
   
   elem.topframeid = topid;
   // keep DrawElement as data entry in the top node
   topnode.data("DrawElement", elem);
   
   this.arr.push(elem);
   
   // if no extra information is requested, use topframe for drawing
   if (!info) {
      elem.drawframeid = elem.topframeid;
      elem.infoframeid = "";
      return;
   }
   
   elem.infoframeid = elem.topframeid + "_info";
   elem.drawframeid = elem.topframeid + "_draw";
   
   var entryInfo = "<h5 id='"+elem.infoframeid+"'><a id='"+elem.infoframeid+"_txt'> </a></h5>";
   entryInfo += "<div id='" + elem.drawframeid+"'/>";

   topnode.append(entryInfo);
}


//list of drawn elements 
JSROOT.elements = new JSROOT.DrawElements();

