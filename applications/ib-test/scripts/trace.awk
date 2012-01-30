# this is a program to trace all connection between nodes
{
   nodename[NR] = $1; 
   nodelid[NR] = $2; 
}
END { 
   for (i1=1; i1<NR; i1++)
      for (i2=1; i2<NR; i2++) 
        if (i1 != i2) {
        
          printf("%s -> %s %5d -> %5d ", nodename[i1], nodename[i2], nodelid[i1], nodelid[i2]);
        
           var = sprintf("ibtracert %d %d | awk -f compress.awk", nodelid[i1], nodelid[i2]);
      
           system(var);
         
#         printf("%s\n", var2);
         
#         printf("%s -> %s %d -> %d\n", nodename[i1], nodename[i2], nodelid[i1], nodelid[i2]);
      }
}
