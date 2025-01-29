\page user_plugin User readout plugin for DABC (libDabcUser.so)

\subpage user_plugin_doc

\ingroup dabc_plugins


\page user_plugin_doc Example of a user defined readout plugin


## Introduction

This Plugin provides an example implementation of a user Input transport.
The data is formatted as MBS events and can be provided at a stream server for monitoring
software like Go4, or stored into an lmd file. By default, the example fills one subevent with
random gauss data that can be used for the standard Go4 example unpackers.



## Compilation

The plugin will be automatically compiled inside DABC and the library `libDabcUser.so` is
placed under $DABCSYS/lib. If copied to another location, after ". dabclogin" the libraries can be build locally under
subdirectory x86_64/lib.


##  Usage
 To run the example, use dabc with the corresponding configuration file

     dabc_exe UserReadout.xml


All parameters of the user input can be passed as url options in the port definition of the first
 (and only) data input:

     <InputPort name="Input0" url="user://host:12345" urlopt1="size=2000&cratid=1&procid=9&ctrlid=3&debug"/>

Following options are implemented for the default example:
- bufsize      : allocated readout buffer size (bytes)
- size         : maximum size of each subevent without headers (bytes)
- cratid       : define mbs subevent header id
- procid       : define mbs subevent header id
- ctrlid       : define mbs subevent header id
- earlytrigclr : reset the trigger before readout is done (buffered hardware only)
- debug        : print additional debug outputs

By default, the data is just send to an mbs stream server socket with specified port number:

     <OutputPort name="Output0" url="mbs://Stream:6900"/>

To activate writing an additional lmd file, the number of ouputs must be set to 2:

     <NumOutputs value="2"/>

File name can be defined with

    <OutputPort name="Output1" url="lmd://myfile.lmd?maxsize=1500&log=2"/>



## Monitoring

Once running, the dabc application can be monitored and controlled by default
with a standard web browser using the address:

     http://localhost:8091/

This offers a simple ratemeter available at

     UserRadoutExample/Readout/InputDataRate

Additionally, the State of the Application may be changed from remote using the command handles at subfoler App:
 - DoConfigure : Initialize DAQ
 - DoStart     : Start Acquisition
 - DoStop      : Stop Acquisition
 - DoHalt      : shutdown DAQ (does not terminate DABC process!)



## How to adjust the example
The class user::Input offers several user functions
that can be modified to adjust the code to a custom data acquisition system:

~~~~~~~~~~~~~~~~{.c}
- virtual int user::Input::User_Init ();
       Initialize the readout hardware on startup
- virtual int user::Input::User_Readout (uint32_t* pdat, unsigned long& size);
       Fill the data at the buffer pointer. size returns filled size in bytes
- virtual int user::Input::User_WaitForTrigger ();
      Here any kind of external trigger can be waited for
- virtual int user::Input::User_ResetTrigger();
     Used to clear any kind of external trigger information
- virtual int user::Input::User_Cleanup();
     shut down the readout hardware properly at the end
~~~~~~~~~~~~~~~~

Additionally, constructor and destructor of this class and the member variables may be modified as you like.


One may just change these functions directly in the private DABCSYS installation and recompile DABC.

However, to run several readout variants with the same DABC installation, it is recommended to copy the directory
plugins/user to another location and compile it locally.
PLEASE NOTE that this case the full (relative) path to the local library  libDabcUser.so must be specified
in the `<Run>` section of the configuration file UserReadout.xml, e.g.

     <lib value="x86_64/lib/libDabcUser.so"/>

Otherwise, the dabc will load the default library from $DABCSYS/lib instead the modified one!


Additionally, the class user::Input may be further modified with additional data members and configuration file
tags. Extracting valules from the configuration url is donein constructor of user::Input class.
