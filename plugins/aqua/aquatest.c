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

int main(int argc, char *argv[])
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;

    char* recvBuff = malloc(512*1024);

    int64_t header[2];

    ssize_t sz;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 10);

    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        if (connfd <= 0) { printf("Fail to accept connection\n"); close(listenfd); return 0; }

        while (1) {
           sz = recv(connfd, header, sizeof(header), MSG_WAITALL);
           if (sz == 0) continue;
           if (sz != sizeof(header)) { printf("Error when receive header %u res %d\n", sizeof(header), sz); close(listenfd); return 0; }

           sz = recv(connfd, recvBuff, header[1], MSG_WAITALL);
           if (sz != header[1]) { printf("Error when receive buffer %ld res %d\n", header[1], sz); close(listenfd); return 0; }

           printf("get buffer %ld\n", header[1]);
        }

     }

    return 0;
}
