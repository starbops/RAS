#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERV_PORT 5140
#define BUFF_SIZE 4096

int main(int argc, char *argv[]) {
    char buff[BUFF_SIZE];
    int listenfd = 0;
    int connfd = 0;
    int clilen = 0;
    int conn_pid = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in conn_addr;

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server: socket error");
        exit(-1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    if(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("server: bind error");
        exit(-1);
    }

    if(listen(listenfd, 3) < 0) {
        perror("server: listen error");
        exit(-1);
    }

    while(1) {
        clilen = sizeof(conn_addr);
        if((connfd = accept(listenfd, (struct sockaddr *)&conn_addr, (socklen_t *)&clilen)) < 0) {
            perror("server: accept error");
        }
        if((conn_pid = fork()) < 0) {           /* fork error */
            perror("server: fork error");
        } else if(conn_pid == 0) {              /* child process */
            close(listenfd);
            memset(buff, 0, BUFF_SIZE);
            strcpy(buff, "Hello there");
            write(connfd, buff, strlen(buff));
            exit(0);
        } else {                                /* parent process */
            close(connfd);
        }
    }

    return 0;
}
