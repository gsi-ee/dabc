#
ps x | grep $1 > .ps.$HOSTNAME.$1
grep -v grep .ps.$HOSTNAME.$1 > .head.ps.$HOSTNAME.$1
#
cat .head.ps.$HOSTNAME.$1 |
while read zeile
    do
      set $zeile     # set pos.parameter of ps-line 
         if [ $? -eq 0 ]   #remove process if it exists
          then 
              kill -9 $1
          fi
    done
#                          
