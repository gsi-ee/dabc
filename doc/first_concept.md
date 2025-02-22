# First requirements for DABC {#dabc_first_concept}

\tableofcontents

This is short description of required functionality,
possible use cases and implementation details for some basic components of [dabc] framework.

## Basic overview
I propose to build our framework as data flow model,
where two main types of entities are exists: data processing units (modules)
and communication (transport) layer between modules.
This kind of approach used in SystemC and many ideas from this framework can be used.

## Transport layer
This is major component of complete framework.
Its main aim is the transport of basic data types (integer, floats)
and arbitrary data packets between different parts of the distributed system.
Transport can be local (inside one application) or global.
Transport can be pear-to-pear or network-kind, when in addition destination address is specified.
Interface from program code to transport should be done via 'port' entity,
which should not depend from specific transport implementation.

In normal operations mode user uses only port functionality to send/receive data.
Ports are connected with each other during initialization phase.
At this phase configured implementation of transport should be created and assigned for this port.
By default, all ports are bidirectional, but one should be able to restrict port only
for input or only for output. Each port can have name and, if required, data type identifier (string or number id).
Transport layer approach gives flexibility to exchange different transports
without exchange of data processing code. Another advantage of intermediate transport
layer between modules is possibility to store flowing data into the file. Later this data can be used for debugging of parts of the complete system instead of real data one can provide data from log file(s).

## Processor unit (module)
This is place for program code. All data manipulation should happen only inside modules.
Communication with other modules should only be done via ports.
In module constructor required number of ports should be created and registered to the module.
Module should have functionality to list all ports afterwards.

## Connection configuration approach
First of all, application should be able to browse/search full hierarchy of the modules.
As a result, one will have access to all ports, parameters, control values,
which are available via module interface. During application startup modules should be created and local (application-wide) interconnection should be established. Connection with external nodes (or other application from the same node)
can be configured locally or via control interface from operator node.
This is static connection approach, while connections should be established
before module can start working. But such approach may fail when network has dynamic nature,
 i.e. number of nodes or number of running processes increases/decreases during system run.
 Then system should be able dynamically establish new connection or brake existing ones.
 One possible solution is dynamic connection management,
 where modules could requests necessary connection to external module.
 For such functionality complete hierarchy of nodes/applications/processes/modules
 should have unique identifiers.
 One should avoid situations that module itself decides which transport used for connection.
 Connection manager should itself create appropriate transport and establish connections.

## Buffer management
As long as generic transport should support zero-copy data transfer,
one should have application-wide buffer management,
which takes care about buffers allocation/holding/release action.

## Use cases
1. Generic DAQ with several modules, connected via transport layer. One can imagine situation, when all modules run on single node, but one may also require to reconfigure same DAQ to run on several nodes. \\
2. Specific B-Net implementation. In this case one cannot use standard interface. Most probably, one should directly access transport from module to explore all functionality of specific network (for instance, MPI or verbs).

## Implementation details
### Communication port
Here one supposes to have one common class (TTransport), which deliver interface,
used by TPort class to send/receive data. TPort should not have any transport-specific code.
Important question is usage of templates. SystemC port class is defined as template,
where transferred data type is template parameter. This allows arbitrary port kinds,
but complicates implementation for us, especially when one needs to communicate
with different nodes and OS. I propose to support only following types:
arbitrary binary buffer, integer (int64t), 64-bit double, null-terminated string.
As result, one should implement four TPort::Send() methods, each for mentioned data types.
In port constructor user should clearly specify which type will be used.
Type identifier can be integer or string. Some types, deriver from binary buffer,
can has it own types identifier. For instance, output port of event builder,
can be identified as 'DABCFullEvent'. Probably, one can use here name,
derived from data-format plugin, for instance 'MBS10\_1Event'.
While port will have type identifier parameter, one can provide type-safety check
during modules connection. Generally, types of connected port should match with each other.
 One also can detect error situations like two output ports are connected or similar.
Each port should have configurable size input and output queues.
In the queue one store send/received values. For binary data buffer identifier is saved.
Very important question: how we notify module when data is arrived (send).
First, one should be able to use polling approach.
For that appropriate flag should be defined in TPort class.
One should be able to use queue size for this purpose.
Second, kind of TPort::Wait() method should be implemented.
It suspends receiver thread until new data is arrived.
When it happens, data should be placed in receiving queue as well.
Third, one can support call-back function, which is called when new data is arrived.
In call-back function, defined in module class, one can immediately process incoming data.
Seems to be, call-back approach cannot be combined with first two (wait or pool)
for single port, therefore one should clearly specify port notification type
at the time when port is created. One should also investigate,
if notification required for the output port.

### Transport
Specific kind of transports (dapl, socket, pipe) should be implemented, as TTransport subclasses. Interface of TTransport class should be suited both for pear-to-pear and network-kind transport. For instance, TTransport::Send() method can have two arguments, where first is data to transfer (buffer reference or integer or float or string) and second argument is destination address (by default, -1 and ignored by peer-to-peer transport). \\
For the local (inside application) communication one need implementation which is able to transfer data between two threads. Actually, it can be single object which is associated with both input and output ports. Only buffer reference (buffer id) will be moved. Buffer itself will remain busy until receiver releases it. Actually, one can imagine situation, when several modules running in single thread and should communicate with each other. In this case call-back function approach can be most optimal (no need for additional communication threads at all).
For the global (inter-node or inter-application) communications one need of course separate objects on receiver and sender sizes. In that case transport object is responsible for release/lock of the buffers.

### Module
Module is aggregation of input/output ports and methods to transform data from inputs to outputs. Module should keep (be able to reproduce) list of all available ports.
Combination of several modules can be combined in one valid super-module. From outside it should be seen as normal module with several ports, inherited from sub-modules.
Probably, module should have separate list of parameters, which can be changed/viewed from outside by controlling system.

### Buffers management
It should be application-wide manager for buffers, allocated for data transport or just for application use.
Because of this reason one can add some methods to TPort class, which allocates required number and size of buffers, which are suitable for transport, used for port connection.
Buffers in manager should be grouped in subsets. Inside subset all buffers can (have to?) have same size. Buffer id (64 bit) can be combined from subset-id (16 bit) and buffer number (48 bit).
Subset approach allows us to create one object per subset (like TBuffersSubset), not one object per buffer. This is very useful in the case, when many small buffers should be allocated. This also can be used in dapl/verbs transport implementation. There one should only register complete subset memory region and not separate memory buffers.
Actually, TBuffersSubset also can consist of set of normal buffers, where one object correspond to one buffer.
For each buffer in subset one should keep additional information. It should be at least boolean flag, which indicate if buffer in use. One can also allow additional buffer information like data format type, individual buffer size, buffer shift and so on for each separate buffer in subset.

## Generic question
Which OS should be supported? Only Linux or any Unix-like? LynxOS support?
Windows support: CYGWIN or native Win32/Win64 API or not supported at all? Probably, one can support some basic module functionality plus threading plus socket connection to enable some end-user activity on Windows. An be recognized, as low-priority task.
Do we using only glibc and pthread, or some additional library? Other thread implementations?
How we use templates? Most concern can be about list of objects like list of ports, list of parameters and so on. If we do not use template, ROOT-like approach with TNamed and TList classes should be use. This is not type-safe, but simplify search by name or similar functionality. In case of templates search methods should be implemented each time.
Do we use some basic class like TObject at all? Probably, yes. It can be useful not only in lists, but also to introduce hierarchy via parent-child relationship between objects. This provide us unique naming scheme for all objects in the system. Another solution for hierarchy: object manager, where any object property (name, config parameters and so on) stored independent from object itself.

## Requirements
Lets summarize requirements for different components of the framework.

### Port
   * communication entity for sending/receiving data from/to user code;
   * single TPort class, independent from specific transport implementation;
   * bi-directional or only input or only output (configurable on creation time)
   * supported data format: binary buffer, null-terminated string, integer, floats (configurable on creation time);
   * safety checks during connection: type safety, in/out comparison;
   * notification via wait/poll mechanism or via call-back (configurable on creation time);

### Transport
   * bidirectional data transfer between modules ports;
   * same basic TTransport class for all implementation;
   * both peer-to-peer and network in one interface;
   * local transport between modules in two threads;
   * local transport between modules in single thread;
   * global transport via pipe, socket, dapl, mpi, verbs;
   * logging of data flow.

### Module
   * keep list of ports;
   * keep list of parameters;
   * can be triggered by timer;
   * debugging possibility with use of log data;
   * unique identifier (name) over all system.

### Buffer management
   * buffer managed by subsets of the same size and kind of buffers;
   * TBufferSubset can has special implementation for specific transports (like verbs, dapl);
   * TPort should has methods to allocate appropriate subsets in manager;

### Generic
   * use of exception for error handling;
   * full support of Linux, LynxOS;
   * partial support of Windows;
   * no templates in basic transport functionality;
   * objects hierarchy via object manager or parent-child relationship;
   * basic TObject class, TList, TObjArray classes.

\author S.Linev
\date 23.01.2007
