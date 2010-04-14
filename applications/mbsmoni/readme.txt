-------------------------------------------------------
This project is a remote DIM server for MBS systems.
Program mMbsDimStatus is a DIM server running on Linux
providing parameters from MBS nodes (v45 and up). 
At GSI, enable usage by:

> . dabclogin

The node of the DIM name server must be set:

> export DIM_DNS_NODE=nodename

You need a running DIM name server:

> dimDns &

Optionally but recommended is the DIM Information Display:

> dimDid &

(A window pops up, click View->All Servers)
Then you start the MBS server (can be on different node) by

> mbsmoni [-t sec]  [-s setupfile] @file | node1 [node2 [node3...]]

The program creates DIM services for all MBS nodes specified
and updates them periodically reading the MBS status.
Besides as arguments the list of nodes to be monitored 
can also be in a text file (@file).
Services to be provided can be configured in file dimsetup
or file specified with -s.
(copy template from $DABCSYS/applications/mbsmoni)
With one name server a second MBS monitor can be started only on
a different node, or by a different user.

The DABC generic GUI can be used to display:

> moni

The GUI or the server or MBS can be restarted any time.

Full sequence again:

> . dabclogin
> export DIM_DNS_NODE=nodename
> dimDns &
> dimDid &
> mbsmoni [-t sec]  [-s setupfile] @file | node1 [node2 [node3...]] &
> moni &
