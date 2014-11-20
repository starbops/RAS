#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "cmd.h"
#include "pipen.h"

const int serv_port = 5140;
const char root_dir[] = "./ras";
const char default_path[] = "bin:.";
const char motd[] = "****************************************\n"
                    "** Welcome to the information server. **\n"
                    "****************************************\n";
const char prompt[] = "%% ";

void reaper(int);
int read_line(char *, int);
void conn_handler();

int main(char *argv[], int argc) {
    int listenfd = 0;
    int connfd = 0;
    int connpid = 0;
    int connlen = 0;
    int optval = 1;
    struct sockaddr_in serv_addr;
    struct sockaddr_in conn_addr;

    signal(SIGCHLD, &reaper);

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server: socket error");
        exit(-1);
    }

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(serv_port);

    if(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("server: bind error");
        exit(-1);
    }

    if(listen(listenfd, 10) < 0) {
        perror("server: listen error");
        exit(-1);
    }

    while(1) {
        connlen = sizeof(conn_addr);
        if((connfd = accept(listenfd, (struct sockaddr *)&conn_addr, (socklen_t *)&connlen)) < 0) {
            perror("server: accept error");
            exit(-1);
        }

        if((connpid = fork()) < 0) {
            perror("server: fork error");
            exit(-1);
        } else if(connpid == 0) {
            close(listenfd);
            dup2(connfd, STDIN_FILENO);
            dup2(connfd, STDOUT_FILENO);
            dup2(connfd, STDERR_FILENO);
            close(connfd);
            chdir(root_dir);
            conn_handler();
            exit(0);
        } else {
            close(connfd);
        }
    }

    return 0;
}

void reaper(int sig) {
    int status;
    while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0) {}
}

int read_line(char *buf, int size) {
    char *pch = NULL;
    int n = 0;

    memset(buf, 0, size);
    if((n = read(STDIN_FILENO, buf, size)) < 0) {
        perror("server: read error");
        exit(-1);
    } else if(n == 0) {
        exit(0);
    } else {
        if((pch = strchr(buf, '/')) != NULL) {
            fprintf(stderr, "Character '/' is not allowed.\n");
            fflush(stderr);
            return 0;
        }

        pch = strchr(buf, '\r');
        if((pch = strrchr(buf, '\r')) != NULL) {
            *pch = '\0';
        } else {
            buf[n-1] = '\0';
        }

        return strlen(buf);
    }
}

void conn_handler() {
    char buf[MAX_CMDLINE_LEN];
    int cmdlen = 0;
    int cmdnum = 0;
    int ppt[MAX_PIPE_NUM];
    int pps[MAX_PIPE_NUM][2];
    struct cmd cmds[MAX_CMDLINE_NUM];

    initpipe(ppt, MAX_PIPE_NUM, pps, MAX_PIPE_NUM);

    setenv("PATH", default_path, 1);
    fprintf(stdout, motd);
    fflush(stdout);

    while(1) {
        fprintf(stdout, prompt);
        fflush(stdout);
        if((cmdlen = read_line(buf, MAX_CMDLINE_LEN)) == 0) {
            continue;
        } else {
            cmdnum = parser(cmds, buf);
            process_line(cmds, cmdnum);
            clear(cmds, cmdnum);
        }
    }
}
