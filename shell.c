#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERV_PORT 5140
#define BUFF_SIZE 4096
#define WELCOME "****************************************\n** Welcome to the information server. **\n****************************************\n"
#define PROMPT "% "

int send_msg(int, char *, char *);
int readline(int, char *, int);
void conn_handler(int, char *);

int main(int argc, char *argv[]) {
    char buff[BUFF_SIZE];
    int listenfd = 0;
    int optval = 1;
    int connfd = 0;
    int clilen = 0;
    int fake_pid = 0;
    int conn_pid = 0;
    int status = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in conn_addr;

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server: socket error");
        exit(-1);
    }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

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
        if((fake_pid = fork()) < 0) {           /* fork error */
            perror("server: fork error");
        } else if(fake_pid == 0) {              /* fake child process */
            if((conn_pid = fork()) < 0) {
                perror("server: fork error");
            } else if(conn_pid == 0) {          /* actual child process */
                close(listenfd);
                conn_handler(connfd, buff);
                exit(0);
            } else {
                exit(0);
            }
        } else {                                /* parent process */
            close(connfd);
            waitpid(fake_pid, &status, 0);
        }
    }

    return 0;
}

int send_msg(int fd, char *buff, char *msg) {
    int n = 0;
    if(buff == msg) {
        if((n = write(fd, buff, strlen(msg))) < 0)
            perror("server: write error");
    } else {
        memset(buff, 0, BUFF_SIZE);
        strncpy(buff, msg, strlen(msg));
        if((n = write(fd, buff, strlen(msg))) < 0)
            perror("server: write error");
    }
    return n;
}

int readline(int fd, char *buff, int maxlen) {
    int n = 0;
    memset(buff, 0, BUFF_SIZE);
    if((n = read(fd, buff, maxlen)) < 0)
        perror("server: read error");
    return n;
}

void conn_handler(int fd, char *buff) {
    send_msg(fd, buff, WELCOME);
    send_msg(fd, buff, PROMPT);
    while(1) {
        readline(fd, buff, BUFF_SIZE);
        send_msg(fd, buff, buff);
        send_msg(fd, buff, PROMPT);
    }
    return;
}
