#!../../bin/linux-x86_64/ebctrl

## You may have to change ebctrl to something else
## everywhere it appears in this file
## Set EPICS environment

< envPaths
epicsEnvSet(FILESIZE,"1500")
epicsEnvSet(EBTYPE,"slave")
epicsEnvSet(EBNUM,"2")
epicsEnvSet(ERRBITLOG, "1")
epicsEnvSet(ERRBITWAIT, "30")
epicsEnvSet(EPICS_CA_ADDR_LIST,"140.181.116.58:5070 140.181.116.58:5080")
# set to multicast
#epicsEnvSet(EPICS_CA_ADDR_LIST,"140.181.116.255")
# use depc294 as ca repeater:
#epicsEnvSet(EPICS_CA_ADDR_LIST,"140.181.91.34")
epicsEnvSet(EPICS_CA_SERVER_PORT,"5080")
epicsEnvSet(EPICS_CA_AUTO_ADDR_LIST,"NO")
epicsEnvSet(PATH,"/home/joern/epicsinstall/base-3.14.12.2/bin/linux-x86_64:/usr/local/bin:/usr/bin:/bin:/usr/bin/X11:.")

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/ebctrl.dbd")
ebctrl_registerRecordDeviceDriver(pdbbase)

## Load record instance
#dbLoadTemplate "db/userHost.substitutions"
dbLoadRecords("db/evtbuild.db","eb=eb02")
dbLoadRecords("db/netmem.db","eb=eb02")
dbLoadRecords("db/errbit1.db","eb=eb02")
dbLoadRecords("db/errbit2.db","eb=eb02")
dbLoadRecords("db/trignr1.db","eb=eb02")
dbLoadRecords("db/trignr2.db","eb=eb02")
dbLoadRecords("db/portnr1.db","eb=eb02")
dbLoadRecords("db/portnr2.db","eb=eb02")
dbLoadRecords("db/trigtype.db","eb=eb02")
dbLoadRecords("db/cpu.db","eb=eb02")
dbLoadRecords("db/errbitstat.db","eb=eb02")
#dbLoadRecords("db/totalevtstat.db")
#dbLoadRecords("db/genrunid.db","eb=eb02")

## Set this to see messages from mySub
var evtbuildDebug 0
var netmemDebug 0
var genrunidDebug 0
var writerunidDebug 0
var errbit1Debug 0
var errbit2Debug 0
var trigtypeDebug 0
var cpuDebug 0
var errbitstatDebug 0



cd ${TOP}/iocBoot/${IOC}
iocInit()

## Start any sequence programs
#seq sncExample,"user=scsHost"
dbl > ${TOP}/iocBoot/${IOC}/eb02.dbl