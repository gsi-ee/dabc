# DIM name server node
#export DIM_DNS_NODE=lxg0500.gsi.de
#export DIMDIR=/misc/goofy/cbm/daq/controls/smi/dim_v16r10
#export LD_LIBRARY_PATH=/misc/goofy/cbm/daq/controls/smi/dim_v16r10/linux
# depending where the jar files are, directories must be added:
export CLASSPATH=.:$DABCSYS/gui/java/generic/:$DIMDIR/jdim/classes

# GUI is called by
# java xgui.xGui
alias gui="java xgui.xGui"
