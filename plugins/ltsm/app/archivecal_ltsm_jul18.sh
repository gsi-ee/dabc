#!/bin/bash
############## archive TDC calibration files from data disk to ltsm archive #######
### v 0.2 26-oct-2018 by JAM (j.adamczewski@gsi.de)
######################################################################################
RUN=$(echo $1 | /usr/bin/cut -d'/' -f1)
CALIBDIR=$(echo $1 | /usr/bin/cut -d'/' -f2)
FOLDER=${RUN}_${HOSTNAME}
FILE=${FOLDER}.tar.gz
DATE=$(/bin/date +"%d-%m-%Y %H:%M:%S")
DESC="manually archived with ltsmc on ${DATE} by ${USER} from ${HOSTNAME}"
LTSMCBIN="/usr/local/bin/ltsmc"
BEAMTIME="jul18"
FILESYS="/lustre/hebe"
ARCHIVEPATH=${FILESYS}/hades/raw/${BEAMTIME}/default/tsm/cal
DISKPATH=/home/hadaq/oper/${CALIBDIR}
SERVER="lxltsm01"
NODE="hades"
PASS="wDhgcvFF7"
OWNER="hadesdst"
if [ ! -d "$DISKPATH/$RUN" ]
then
	  echo "Folder $RUN not found on $DISKPATH. "
	  exit;
fi
# first pack the cal file
cd ${DISKPATH}
mv ${RUN} ${FOLDER}
/bin/tar cf ${FOLDER}.tar ${FOLDER}/
/bin/gzip -f  ${FOLDER}.tar
if [ ! -f "$DISKPATH/$FILE" ]
then
	  echo "file $FILE not found on $DISKPATH. "
	  exit;
fi
# archive it with ltsmc pipe mode:
REV=$(cat "${DISKPATH}/${FILE}" | ${LTSMCBIN} --pipe -n "${NODE}" -f "${FILESYS}" -p "${PASS}" -s "${SERVER}" -o "${OWNER}" -d "'${DESC}'" "${ARCHIVEPATH}/${FILE}")
#echo cat "${DISKPATH}/${FILE}" 
#echo ${LTSMCBIN} --pipe -n "${NODE}" -f "${FILESYS}" -p "${PASS}" -s "${SERVER}" -o "${OWNER}" -d "'${DESC}'" "${ARCHIVEPATH}/${FILE}"
#REV=no
#
echo archived file ${DISKPATH}/${FILE} to ${ARCHIVEPATH}/${FILE} with result $REV
