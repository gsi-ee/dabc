# JavaScript ROOT

The JSROOT project intends to implement ROOT graphics for web browsers.
Reading of binary ROOT files is also supported.
It is the successor of the JSRootIO project.


## Installing JSROOT

The actual version of JSROOT can be found in ROOT repository, etc/http/ subfolder.
All necessary files are located there. Just copy them on any web server or use them directly from the file system.
The latest version of JSROOT can also be found online on <http://root.cern.ch/js/> or <http://web-docs.gsi.de/~linev/js/>.
 

## Reading ROOT files in JSROOT

[The main page](http://root.cern.ch/js/3.1/) of the JSROOT project provides the possibility to interactively open ROOT files and draw objects like histogram or canvas.

The following parameters can be specified in the URL string:

- file - name of the file, which will be automatically open with page loading
- item - name of the item to be displayed
- opt - drawing option for the item
- items - array of objects to display like ['hpx;1', 'hpxpy;1']
- opts - array of options ['any', 'colz']
- layout - can be 'collapsible', 'tabs' or 'gridNxM' where N and M are integer values
- nobrowser - do not display file browser
- autoload - name of JavaScript to load
- optimize - drawing optimization 0:off, 1:only large histograms (default), 2:always
- interactive - enable/disable interactive functions 0-disable all, 1-enable all
- noselect - hide file-selection part in the browser (only when file name is specified)

Examples: 

- <http://root.cern.ch/js/3.1/index.htm?file=../files/hsimple.root&item=hpx;1>
- <http://root.cern.ch/js/3.1/index.htm?file=../files/hsimple.root&nobrowser&item=hpxpy;1&opt=colz>
- <http://root.cern.ch/js/3.2/index.htm?file=../files/hsimple.root&noselect&layout=grid2x2&item=hprof;1>

One can very easy integrate JSROOT graphic into other HTML pages using a __iframe__ tag:  

<iframe width="600" height="500" src="http://root.cern.ch/js/3.1/index.htm?nobrowser&file=../files/hsimple.root&item=hpxpy;1&opt=colz">
</iframe>

In principle, one could open any ROOT file placed in the web, providing the full URL to it like:

<http://web-docs.gsi.de/~linev/js/3.1/?file=http://root.cern.ch/js/files/hsimple.root&item=hpx>

But one should be aware of [Cross-Origin Request blocking](https://developer.mozilla.org/en/http_access_control),
when the browser blocks requests to files from domains other than current web page.

There are two solutions. Either one configures the web-server accordingly or one copies the JSROOT to the same location than where the data files are located.
In the second case, one could use the server with its default settings.

A simple case is to copy only the top index.htm file on the server and specify the full path to JSRootCore.js script like:

    ...
    <script type="text/javascript" src="http://root.cern.ch/js/3.1/scripts/JSRootCore.js"></script>
    ...  

In such case one can also specify a custom files list:

    ...
     <div id="simpleGUI" files="userfile1.root;subdir/usefile2.root">
       loading scripts ...
     </div>
    ...


## JSROOT with THttpServer

THttpServer provides http access to objects from running ROOT application.
JSROOT is used to implement the user interface in the web browsers.

The layout of the main page coming from THttpServer is similar to the file I/O one.
One could browse existing items and display them. A snapshot of running
server can be seen on the [demo page](http://root.cern.ch/js/3.1/demo/).

One could also specify similar URL parameters to configure the displayed items and drawing options.

It is also possible to display one single item from the THttpServer server like:

<http://root.cern.ch/js/3.1/demo/Files/job1.root/hpxpy/draw.htm?opt=colz>


##  Data monitoring with JSROOT

### Monitoring with http server

The best possibility to organize the monitoring of data from a running application
is to use THttpServer. In such case the client can always access the latest 
changes and request only the items currently displayed in the browser. 
To enable monitoring, one should activate the appropriate checkbox or 
provide __monitoring__ parameter in the URL string like: 

<http://root.cern.ch/js/3.1/demo/Files/job1.root/hprof/draw.htm?monitoring=1000>

The parameter value is the update interval in milliseconds.


### JSON file-based monitoring

Solid file-based monitoring (without integration of THttpServer into application) can be 
implemented in JSON format. There is the TBufferJSON class, which is capable to potentially 
convert any ROOT object (beside TTree) into JSON. Any ROOT application can use such class to 
create JSON files for selected objects and write such files in a directory, 
which can be accessed via web server. Then one can use JSROOT to read such files and display objects in a web browser.
There is a demonstration page showing such functionality:

<http://root.cern.ch/js/3.1/demo/demo.htm>

<iframe width="500" height="300" src="http://root.cern.ch/js/3.1/demo/demo.htm">
</iframe>

This demo page reads in cycle 20 json files and displays them.

If one has a web server which already provides such JSON file, one could specify the URL to this file like:

<http://root.cern.ch/js/3.1/demo/demo.htm?addr=Canvases/c1/root.json.gz>

Here the same problem with [Cross-Origin Request](https://developer.mozilla.org/en/http_access_control) can appear.
If the web server configuration cannot be changed, just copy JSROOT to the web server itself.


### Binary file-based monitoring (not recommended)

Theoretically, one could use binary ROOT files to implement monitoring.
With such approach, a ROOT-based application creates and regularly updates content of a ROOT file, which can be accessed via normal web server. From the browser side, JSROOT could regularly read the specified objects and update their drawings. But such solution has three major caveats.

First of all, one need to store the data of all objects, which could only be potentially displayed in the browser. In case of 10 objects it does not matter, but for 1000 or 100000 objects this will be a major performance penalty. With such big amount of data one will never achieve high update rate. 

The second problem is I/O. To read the first object from the ROOT file, one need to perform several (about 7) file-reading operations via http protocol. 
There is no http file locking mechanism (at least not for standard web servers), 
therefore there is no guarantee that the file content is not changed/replaced between consequent read operations. Therefore, one should expect frequent I/O failures while trying to monitor data from ROOT binary files. There is a workaround for the problem - one could load the file completely and exclude many partial I/O operations by this. To achieve this with JSROOT, one should add "+" sign at the end of the file name.

The third problem is the limitations of ROOT I/O in JavaScript. Although it tries to fully repeat binary I/O with the streamer info evaluation, the JavaScript ROOT I/O will never have 100% functionality of native ROOT. Especially, the custom streamers are a problem for JavaScript - one need to implement them once again and keep them synchronous with ROOT itself. And ROOT is full of custom streamers! Therefore it is just a nice feature and it is great that one can read binary files from a web browser, but one should never rely on the fact that such I/O works for all cases. 
Let say that major classes like TH1 or TGraph or TCanvas will be supported, but one will never see full support of TTree or RooWorkspace in JavaScript.

If somebody still want to test such functionality, try monitoring parameter like:

<http://root.cern.ch/js/3.1/index.htm?nobrowser&file=../files/hsimple.root+&item=hpx;1&monitoring=2000>

In this particular case, the histogram is not changing.


## Stand-alone usage of JSROOT

Even without any server-side application, JSROOT provides nice ROOT-like graphics,
which could be used in arbitrary HTML pages.
There is and [example page](http://root.cern.ch/js/3.1/demo/example.htm), 
where a 2-D histogram is artificially generated and displayed.
Details about the JSROOT API can be found in the next chapters.


## JSROOT API

JSROOT consists of several libraries (.js files). They are all provided in the ROOT
repository and are available in the 'etc/http/scripts/' subfolder.
Only the central classes and functions will be documented here.
  
### Scripts loading

Before JSROOT can be used, all appropriate scripts should be loaded.
Any HTML pages where JSROOT is used should include the JSRootCore.js script.
The <head> section of the HTML page should have the following line:

    <script type="text/javascript" src="http://root.cern.ch/js/3.1/scripts/JSRootCore.js"></script>  

Here, the default location of JSROOT is specified. One could have a local copy on the file system or on a private web server. When JSROOT is used with THttpServer, the address looks like:

    <script type="text/javascript" src="http://your_root_server:8080/jsrootsys/scripts/JSRootCore.js"></script>  

Then one should call the JSROOT.AssertPrerequisites(kind,callback,debug) method, which accepts the following arguments:

- kind - functionality to load. It can be a combination of:
    + '2d' normal drawing for 1D/2D objects
    + '3d' 3D drawing for 2D/3D histograms
    + 'io' binary file I/O
    + 'user:scirpt.js' load user scripts, should be at the end of kind string
- callback - call back function which is called when all necessary scripts are loaded
- debug - id of HTML element where debug information will be shown while scripts are loading


JSROOT.AssertPrerequisites should be called before any other JSROOT functions can be used.
At the best, one should call it with `onload` handler like:

    <body onload="JSROOT.AssertPrerequisites('2d', userInitFunction, 'drawing')">
       <div id="drawing">loading...</div>
    </body>

Internally, the JSROOT.loadScript(urllist, callback, debug) method is used. It can be useful when some other scripts should be loaded as well. __urllist__ is a string with scripts names, separated by ';' symbol. If a script name starts with __$$$__ (triple dollar sign), the script will be loaded from a location relative to the main JSROOT directory. 
This location is automatically detected when JSRootCore.js script is loaded.


### Use of JSON

It is strongly recommended to use JSON when communicating with ROOT application.
THttpServer  provides a JSON representation for every registered object with an url address like:

    http://your_root_server:8080/Canvases/c1/root.json

One can also generate JSON representation, using the [TBufferJSON](http://root.cern.ch/root/html/TBufferJSON.html) class.

To access data from a remote web server, it is recommended to use the [XMLHttpRequest](http://en.wikipedia.org/wiki/XMLHttpRequest) class.
JSROOT provides a special method to create such class and properly handle it in different browsers.
For receiving JSON from a server one could use following code:

    var req = JSROOT.NewHttpRequest("http://your_root_server:8080/Canvases/c1/root.json", 'object', userCallback);
    req.send(null);

In the callback function, one gets JavaScript object (or null in case of failure)


### Objects drawing

After an object has been created, one can directly draw it. If somewhere in a HTML page there is a `<div>` element:

    ...
    <div id="drawing"></div>
    ...
 
One could use the JSROOT.draw function:

    JSROOT.draw("drawing", obj, "colz");

The first argument is the id of the HTML div element, where drawing will be performed. The second argument is the object to draw and the third one is the drawing option.
One is also able to update the drawing with a new version of the object:

    // after some interval request object again
    JSROOT.redraw("drawing", obj2, "colz");

The JSROOT.redraw function will call JSROOT.draw if the drawing was not performed before.


### File API

JSROOT defines the JSROOT.TFile class, which can be used to access binary ROOT files.

    var filename = "http://root.cern.ch/js/files/hsimple.root";
    var f = new JSROOT.TFile(filename, fileReadyCallback);

One should always remember that all I/O operations are asynchronous in JSROOT.
Therefore, callback functions are used to react when the I/O operation completed.
For example, reading an object from a file and displaying it will look like:

    new JSROOT.TFile(filename, function(file) {
       file.ReadObject("hpxpy;1", function(obj) {
          JSROOT.draw("drawing", obj, "colz");
       });
    });


## Links collection

Many different examples of JSROOT usage can be found on [links collection](http://root.cern.ch/js/3.1/jslinks.htm) page 

