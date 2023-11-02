#!/bin/bash
############## archive TDC calibration files from data disk to ltsm archive #######
### v 0.2 26-oct-2018 by JAM (j.adamczewski@gsi.de)
### 12-feb-2019 JAM changed to archive mar19 (to be optimized yet)
### 28 may-2020 JAM new script for jun20
### 30 nov-2020 JAM new script for feb21
### 23 nov-2021 JAM new script for feb22
### 08-nov-2022 JAM adjusted for fsqc instead ltsmc
### 04-oct-2023 JAM adjust path and server for feb24 beamtime
### 2-nov-2023 JAM correct for changed fsqc file interface (no pipe option)
######################################################################################
RUN=$(echo $1 | /usr/bin/cut -d'/' -f2)
CALIBDIR=$(echo $1 | /usr/bin/cut -d'/' -f1)
FOLDER=${RUN}_${HOSTNAME}
FILE=${FOLDER}.tar.gz
DATE=$(/bin/date +"%d-%m-%Y %H:%M:%S")
DESC="manually archived with ltsmc on ${DATE} by ${USER} from ${HOSTNAME}"
LTSMCBIN="/home/hadaq/ltsm/install/bin/fsqc"
BEAMTIME="feb24"
FILESYS="/lustre"
ARCHIVEPATH=${FILESYS}/hades/raw/${BEAMTIME}/cal
DISKPATH=/home/hadaq/oper/${CALIBDIR}
SERVER="lxfsq08"
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
#REV=$(cat "${DISKPATH}/${FILE}" | ${LTSMCBIN} --pipe -n "${NODE}" -f "${FILESYS}" -p "${PASS}" -s "${SERVER}" -o "${OWNER}" -d "'${DESC}'" "${ARCHIVEPATH}/${FILE}")
# new change to fsqc:
cd ${DISKPATH}
REV=$( ${LTSMCBIN}  -o "lustre_tsm"  -n "${NODE}" -f "${FILESYS}" -p "${PASS}" -s "${SERVER}"  -a "${ARCHIVEPATH}"  "${FILE}"  )

#echo cat "${DISKPATH}/${FILE}" 
#echo ${LTSMCBIN} --pipe -n "${NODE}" -f "${FILESYS}" -p "${PASS}" -s "${SERVER}" -o "${OWNER}" -d "'${DESC}'" "${ARCHIVEPATH}/${FILE}"
#REV=no
#
echo archived file ${DISKPATH}/${FILE} to ${ARCHIVEPATH}/${FILE} with result $REV
