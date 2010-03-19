-------------------------------------------------------
This project is a remote DIM server for MBS systems.
Program mMbsDimStatus is a DIM server running on Linux
providing parameters from MBS nodes (v45 and up). 
At GSI, enable usage by
> . dabclogin
You need a running DIM name server (started by command dimDns).
The node of the DIM name server must be set before in DIM_DNS_NODE.
Then you can start the server by
> mbsmoni [-t sec] @file | node1 [node2 [node3...]]
The program creates DIM services for all MBS nodes specified
and updates them periodically reading the MBS status.
Besides as arguments the list of nodes to be monitored 
can also be in a text file (@file).
Services to be provided can be configured in file dimsetup.txt
(copy from $DABCSYS/applications/mbsmoni)

The DABC generic GUI can be used to display:
> moni
