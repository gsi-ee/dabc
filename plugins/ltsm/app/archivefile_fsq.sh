#!/bin/bash
############## archive local files from data disk to fsq/lustre/tsm  #######
### 26-feb-2024 JAM derived from hades calibration archiving script
######################################################################################
FILE=$1
FSQCBIN="/mbs/storage/PCx86_Linux_5.10-64_Deb/bin/fsqc"
# please adjust current beamtime/run folder:
#BEAMTIME="t060"
BEAMTIME="test"
FILESYS="/lustre"
ARCHIVEPATH=${FILESYS}/tasca/${BEAMTIME}
# please adjust path to your local files directory here:
DISKPATH="/daq/usr/adamczew/data"
SERVER="lxfsq03.gsi.de"
NODE="tasca"
PASS="xxxxxxxxx"
cd ${DISKPATH}
REV=$( ${FSQCBIN}  -o "lustre_tsm"  -n "${NODE}" -f "${FILESYS}" -p "${PASS}" -s "${SERVER}"  -a "${ARCHIVEPATH}"  "${FILE}"  )
echo archived file ${DISKPATH}/${FILE} to ${ARCHIVEPATH}/${FILE} with result $REV
