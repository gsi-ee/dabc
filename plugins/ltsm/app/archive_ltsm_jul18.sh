#!/bin/bash
############## manually archive file from data disk to ltsm archive s#################
### v 0.1 22-aug-2018 by JAM (j.adamczewski@gsi.de)
######################################################################################
FILE=$1
DATE=$(/bin/date +"%d-%m-%Y %H:%M:%S")
DESC="manually archived with ltsmc on ${DATE} by ${USER} from ${HOSTNAME}"
LTSMCBIN="/usr/local/bin/ltsmc"
BEAMTIME="jul18"
FILESYS="/lustre/hebe"
ARCHIVEPATH=${FILESYS}/hades/raw/${BEAMTIME}/default/tsm
DISKPATH="/data01/data"
SERVER="lxltsm01"
NODE="hades"
PASS="wDhgcvFF7"
OWNER="hadesdst"
if [ ! -f "$DISKPATH/$FILE" ]
then
	  echo "file $FILE not found on $DISKPATH. "
	  exit;
fi
#
REV=$(cat "${DISKPATH}/${FILE}" | ${LTSMCBIN} --pipe -n "${NODE}" -f "${FILESYS}" -p "${PASS}" -s "${SERVER}" -o "${OWNER}" -d "'${DESC}'" "${ARCHIVEPATH}/${FILE}")
#echo cat "${DISKPATH}/${FILE}" 
#echo ${LTSMCBIN} --pipe -n "${NODE}" -f "${FILESYS}" -p "${PASS}" -s "${SERVER}" -o "${OWNER}" -d "'${DESC}'" "${ARCHIVEPATH}/${FILE}"
#REV=no
#
echo archived file ${DISKPATH}/${FILE} to ${ARCHIVEPATH}/${FILE} with result $REV
