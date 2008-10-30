#! /bin/bash
# prepare XDAQ xml setup file from xml template
# Joern Adamczewski, gsi Darmstadt
# 30-Jan-2007 (with i2o)... 
# 19-Apr-2007 (for core transport)
# 20-Feb-2008 (extended for mbs setup with readout and event builder nodes)
# 04-Jul-2008 simplified mbs setup for use with dabc installed at gsi
# 08-Jul-2008 changed for templates installed at DABCSYS (with Qt configurator frontend)
#
##########################################################
if [ $# -lt 2 ] ; then
    echo
    echo 'DABC setup file generator for DAQ with MBS Readout (v1.0 4-Jul-2008) --------------------------------'
    echo 'usage: generateSetupMbs controllernode.gsi.de useSeparateEventbuilders(=0/1)'
    echo '       if useSeparateEventbuilders==0, symmetric worker nodes are set up from readernodes.txt'
    echo '       if useSeparateEventbuilders==1, additional list of eventbuilders is used from buildernodes.txt'
else

    READERNODELIST=`cat readernodes.txt`
    BUILDERNODELIST=`cat buildernodes.txt`    
    #echo readers: $READERNODELIST
    #echo builders: $BUILDERNODELIST
    HEADER=`echo "<?xml version='1.0'?> <xc:Partition xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"  xmlns:soapenc=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:xc=\"http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30\"> "`
    FOOTER=`echo "</xc:Partition>"`
    if [[ "$2" -gt 0 ]]; then
        READER=`echo dabc::xd::Readout`
        READERTEMP=`echo $DABCSYS/gui/Qt/configurator/bnetReadoutTemp.xml`   
    else
        READER=`echo dabc::xd::Worker`    
        READERTEMP=`echo $DABCSYS/gui/Qt/configurator/bnetWorkerTemp.xml`   
    fi 
    BUILDER=`echo dabc::xd::Eventbuilder`
    CONTROLLER=`echo dabc::xd::Controller`
    CONTNODE=$1
    declare -i INSTCOUNTA
    INSTCOUNTA=1
    declare -i READOUTCOUNT
    READOUTCOUNT=0
    declare -a MBSINPUT    
    TEMPNAME=`echo setup.tmp`
    NEWNAME=`echo "SetupMbs.xml"`
    echo Generating setup file $NEWNAME 
    echo $HEADER > $NEWNAME 
    for READOUT in $READERNODELIST;
        do
        # figure out xdaq node and mbs readouts
        READOUTCOUNT=-1
        MBSINPUT[0]=`echo `
        MBSINPUT[1]=`echo `
        MBSINPUT[2]=`echo `
        MBSINPUT[3]=`echo `
        MBSINPUT[4]=`echo `
        IFSOLD=$IFS
        IFS=':'
        for NODE in $READOUT
            do
                # echo testing $NODE from $READOUT
                    if [[ "$READOUTCOUNT" -eq "-1" ]]; then
                        NODENAME=$NODE
                        echo "using readout node  $NODENAME"
                    else
                        MBSINPUT[$READOUTCOUNT]=$NODE
                        echo "  - with mbsreadout$READOUTCOUNT : ${MBSINPUT[$READOUTCOUNT]}"
                    fi  
                let "READOUTCOUNT+=1"; 
            done
        IFS=$IFSOLD
        echo "preparing $READER instance $INSTCOUNTA  for node $NODENAME" 
        cat  "$READERTEMP" | awk -v rep=$NODENAME -v str=XDAQNODE '{ gsub(str,rep) ; print }' \
                | awk -v rep=$READER -v str=READERCLASS '{ gsub(str,rep) ; print }' \
                | awk -v rep=$CONTNODE -v str=CONTROLLERNODE '{ gsub(str,rep) ; print }'\
                | awk -v rep=$CONTROLLER -v str=CONTROLLERCLASS '{ gsub(str,rep) ; print }'\
                | awk -v rep=$INSTCOUNTA -v str=READERINSTANCE '{ gsub(str,rep) ; print }'\
                | awk -v rep=$NODECOUNT -v str=NUMNODES '{ gsub(str,rep) ; print }'\
                | awk -v rep=$READOUTCOUNT -v str=NUMREADOUTS '{ gsub(str,rep) ; print }'\
                | awk -v rep=${MBSINPUT[0]} -v str=MBSNODE0 '{ gsub(str,rep) ; print }'\
                | awk -v rep=${MBSINPUT[1]} -v str=MBSNODE1 '{ gsub(str,rep) ; print }'\
                | awk -v rep=${MBSINPUT[2]} -v str=MBSNODE2 '{ gsub(str,rep) ; print }'\
                | awk -v rep=${MBSINPUT[3]} -v str=MBSNODE3 '{ gsub(str,rep) ; print }'\
                | awk -v rep=${MBSINPUT[4]} -v str=MBSNODE4 '{ gsub(str,rep) ; print }'\
            >> $NEWNAME ;
        let "INSTCOUNTA+=1";
        done
    if [[ "$2" -gt 0 ]]; then
    ############# eventbuilders for asymmetric setup:
        for NODENAME in $BUILDERNODELIST;
            do
            echo "preparing $BUILDER instance $INSTCOUNTA  for node $NODENAME" 
            cat  $DABCSYS/gui/Qt/configurator/bnetEventbuilderTemp.xml | awk -v rep=$NODENAME -v str=XDAQNODE '{ gsub(str,rep) ; print }' \
                    | awk -v rep=$BUILDER -v str=BUILDERCLASS '{ gsub(str,rep) ; print }' \
                    | awk -v rep=$CONTNODE -v str=CONTROLLERNODE '{ gsub(str,rep) ; print }'\
                    | awk -v rep=$CONTROLLER -v str=CONTROLLERCLASS '{ gsub(str,rep) ; print }'\
                    | awk -v rep=$INSTCOUNTA -v str=BUILDERINSTANCE '{ gsub(str,rep) ; print }'\
                    | awk -v rep=$NODECOUNT -v str=NUMNODES '{ gsub(str,rep) ; print }'\
                >> $NEWNAME ;
            let "INSTCOUNTA+=1";
        done
    ########### end asymmetric eventbuilders
    fi
    let "INSTCOUNTA=0" 
    echo  > $TEMPNAME
    cat  $DABCSYS/gui/Qt/configurator/bnetControllerTemp.xml | awk -v rep=$CONTNODE -v str=CONTROLLERNODE '{ gsub(str,rep) ; print }'\
            | awk -v rep=$CONTROLLER -v str=CONTROLLERCLASS '{ gsub(str,rep) ; print }'\
            | awk -v rep=$INSTCOUNTA -v str=CONTROLLERINSTANCE '{ gsub(str,rep) ; print }'\
        >> $TEMPNAME;
    cat $TEMPNAME >> $NEWNAME
    echo $FOOTER >> $NEWNAME 
fi
