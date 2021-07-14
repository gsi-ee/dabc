# MBS utilities documentation {#mbs_util}

Together with [MBS plugin in DABC](@ref mbs_plugin) two command-line
utilities are provided: mbsprint and mbscmd.

## Utility mbsprint

Printout of MBS events from MBS servers or LMD files. Example:

~~~~~~~~~~~~~
shell> mbsprint mbs://r4-5/Stream -num 10 -hex
shell> mbsprint file.lmd -dec
~~~~~~~~~~~~~


## Utility mbscmd

Getting log information, statistic information, access to command channel.

~~~~~~~~~~~~~
shell> mbscmd r4-5 -cmd 'type event'
~~~~~~~~~~~~~

To be able submit commands or access log information, one should
call in mbs 'sta cmdrem' and 'sta logrem' commands. This functionality
available in MBS from version 6.3.

mbscmd utility can be used to submit commands to MBS prompter

~~~~~~~~~~~~~
shell> mbscmd x86l-33 -prompter -cmd '@startup' -cmd 'sta acq'
~~~~~~~~~~~~~

To be able submit commands, prompter should be started with '-r' options:

~~~~~~~~~~~~~
mbsnode> prm -r clinet_host_name
~~~~~~~~~~~~~

Where client_host_name is host name, from which mbscmd will be used.
