## Usage of `dogmaprint`

1. Start readout

    dabc_exe plugins/dogma/app/readout.xml

2. Run print on any other node

    dogmaprint host:6002 -raw

    dogmaprint host:6002 -rate



## Usage of `dogmacmd`

This is demo how one use DABC command channel to submit arbitrary commands
to remote node. Any kind of protocol can be implemented on top.

1. Start on the control node

    dabc_exe plugins/dogma/app/dogma.xml

2. Connect to running node

    dogmacmd nodename:12345
