{
   if ($1 == "Ca") var=substr($5, 2, 9); 
   if ($4 == "lid" && (match(var,"node?-???") || match(var,"quad0??") || match(var,"storage??") || match(var,"meta0?") || match(var,"login0?"))) printf("%s %s %s\n",var,$5,$7) 
}
