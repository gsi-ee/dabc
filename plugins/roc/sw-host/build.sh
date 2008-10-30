#! /bin/sh

# to be able run script one need password-less login to the Hidelberg machine
# For that one should do:
#   ssh-keygen -t rsa
#   ssh-copy-id -i ~/.ssh/id_rsa.pub '-Xp 922 gsi@portal.kip.uni-heidelberg.de'


echo "Build source package sw-host.tar.gz"
rm -f sw-host.tar.gz
tar cf sw-host.tar *.h *.c
gzip sw-host.tar

echo "Copy source to Heidelberg"

scp -P 922 sw-host.tar.gz gsi@portal.kip.uni-heidelberg.de:roc/ppc/SDK_projects/syscoreshell

echo "Login and compile image on Heidelberg machine"

ssh -Xp 922 gsi@portal.kip.uni-heidelberg.de ". /opt/Xilinx/ISE/settings.sh; . /opt/Xilinx/EDK/settings.sh; cd roc/ppc/SDK_projects/syscoreshell; tar xzf sw-host.tar.gz; cd Release; rm -f image.bin syscoreshell.elf; make clean; make all; powerpc-eabi-objcopy -O binary syscoreshell.elf image.bin"

echo "Copy image.bin ftom Heidelberg"
rm -f image.bin Release/image.bin Release/syscoreshell.elf
scp -P 922 gsi@portal.kip.uni-heidelberg.de:roc/ppc/SDK_projects/syscoreshell/Release/image.bin .

if  [[ -f image.bin ]]; then
  scp -P 922 gsi@portal.kip.uni-heidelberg.de:roc/ppc/SDK_projects/syscoreshell/Release/syscoreshell.elf Release/
  cp image.bin Release/
  
  echo "Check image.bin that it is up to date"
  ls -l image.bin
  
  if [ "x$1" != "x" ]; then
    echo "Upload image immediately to board $1"
    ./upload $1 -sw image.bin
  fi
else
  echo "File image.bin was not build"
fi


