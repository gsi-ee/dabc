#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

int main()
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;

    char* recvBuff = malloc(512*1024);
    if (!recvBuff) return 1;

    int64_t header[2];
    int64_t totalsz, totalbuf, totallost, lastcnt;

    time_t tm, lasttm;

    ssize_t sz;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 10);

    lastcnt = -1;
    totalsz = 0; totalbuf = 0; totallost = 0;
    lasttm = time(NULL);


    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        if (connfd <= 0) {
           printf("Fail to accept connection\n");
           close(listenfd);
           free(recvBuff);
           return 0;
        }

        printf("Client connected\n");

        while (1) {
           sz = recv(connfd, header, sizeof(header), MSG_WAITALL);
           if (sz == 0) continue;
           if (sz != sizeof(header)) {
              printf("Error when receive header %lu res %ld\n", (long unsigned) sizeof(header), (long) sz);
              close(listenfd);
              free(recvBuff);
              return 0;
           }

           sz = recv(connfd, recvBuff, header[1], MSG_WAITALL);
           if (sz != header[1]) {
              printf("Error when receive buffer %ld res %ld\n", (long) header[1], (long) sz);
              close(listenfd);
              free(recvBuff);
              return 0;
           }

           // printf("get buffer %ld\n", header[1]);

           totalsz+=sz;
           totalbuf++;
           if ((lastcnt>0) && (header[0]-lastcnt>1)) totallost+=(header[0]-lastcnt-1);

           lastcnt = header[0];

           tm = time(NULL);
           if (tm - lasttm > 3) {
              printf("Bufs %6lld  Size %8.3f MB  Lost %3lld\n", (long long) totalbuf, totalsz*1e-6, (long long) totallost);
              lasttm = tm;
           }
        }

     }

    free(recvBuff);
    return 0;
}
