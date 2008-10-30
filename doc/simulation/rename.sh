#! /usr/local/bin/bash
# Replace strings in file names and tex files
# J.Adamczewski 18.03.2002
# H.Essel 30.7.2002: two args
# DAQ demonstrator documents Project
# Experiment Data Processing at DVEE department, GSI
# usage: rename.sh old new
FILELIST='*.tex'
#echo $FILELIST
if [ $# -ne 2 ] ; then
echo 'args: "oldstring" "newstring"'
else
echo Replace $1 by $2 in
echo "*.tex"
for FILENAME in $FILELIST;
do
#echo $FILENAME;
cat  $FILENAME | awk -v rep=$2 -v str=$1 '{ gsub(str,rep) ; print }' > temp.txt 
mv temp.txt $FILENAME;
NEWNAME=`echo $FILENAME | awk -v rep=$2 -v str=$1 '{ gsub(str,rep) ; print }'`;
echo $NEWNAME;
mv $FILENAME $NEWNAME;
done
fi
