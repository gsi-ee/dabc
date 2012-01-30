# this is a program to extract switches names from ibtracert output
{
   if (NR==2) sw1=substr($8, 6, 10); 
   if (NR==3) sw2=substr($8, 6, 9); 
   if (NR==4) sw3=substr($8, 6, 10); 
}
END { 
       if (NR==4) printf("%s\n",sw1); 
             else printf("%s %s %s\n",sw1,sw2,sw3);
    }
