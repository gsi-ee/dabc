This project is a remote DIM server for MBS systems.
You need a running DIM name server.
Then you can start 
> mMbsDimStatus [-t sec] @file | node1 [node2 [node3...]]
The program creates DIM services for all MBS nodes specified
and updates them periodically reading the MBS status.
The DABC generic GUI can be used to display.

Besides as arguments the list of nodes to be monitored 
can also be in a text file.