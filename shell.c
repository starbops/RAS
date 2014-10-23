#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERV_PORT 5140
#define BUFF_SIZE 10240
#define CMDLINE_LENGTH 10240
#define SINGLE_CMD_WORD 256
#define WELCOME "****************************************\n** Welcome to the information server. **\n****************************************\n"
#define PROMPT "% "

struct cmd {
    char **argv;
    int argc;
    int is_piped;
};

int send_msg(int, char *, char *);
int read_line(int, char *);
int parse_cmd(char *, struct cmd []);
void clear_cmd(struct cmd [], int);
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

int read_line(int fd, char *buff) {
    int n = 0;
    memset(buff, 0, BUFF_SIZE);
    if((n = read(fd, buff, BUFF_SIZE)) < 0)
        perror("server: read error");
    else if(n == 0)
        n = 0;
    else
        buff[n-1] = '\0';
    return n;
}

int parse_cmd(char *buff, struct cmd cmds[]) {
    char *pch = NULL;
    char *tmp_cmd[SINGLE_CMD_WORD]; /* single command */
    int c = 0;  /* command number */
    int i = 0;  /* word number in whole command line */
    int w = 0;  /* word number in single command */
    pch = strtok(buff, " \n\r\t");
    for(i = 0; pch != NULL; i++) {
        tmp_cmd[i] = pch;
        if(strcmp(pch, "|") == 0) {
            cmds[c].argv = (char **)malloc((i + 1) * sizeof(char *));
            for(w = 0; w < i + 1; w++) {
                cmds[c].argv[w] = tmp_cmd[w];
                printf("%s\n", cmds[c].argv[w]);
            }
            cmds[c].argc = w;
            printf("%d\n", cmds[c].argc);
            c++;
            i = -1;
        }
        pch = strtok(NULL, " \n\r\t");
    }
    cmds[c].argv = (char **)malloc((i + 1) * sizeof(char *));
    for(w = 0; w < i; w++) {
        cmds[c].argv[w] = tmp_cmd[w];
        printf("%s\n", cmds[c].argv[w]);
    }
    cmds[c].argc = w;
    printf("%d\n", cmds[c].argc);
    return c + 1;
}

void clear_cmd(struct cmd cmds[], int cmd_count) {
    int i = 0;
    for(i = 0; i < cmd_count; i++)
        free(cmds[i].argv);
    return;
}

void conn_handler(int fd, char *buff) {
    struct cmd cmds[CMDLINE_LENGTH];
    int cmd_count = 0;
    send_msg(fd, buff, WELCOME);
    while(1) {
        send_msg(fd, buff, PROMPT);
        if(read_line(fd, buff) == 0)
            break;
        cmd_count = parse_cmd(buff, cmds);
        send_msg(fd, buff, buff);
        clear_cmd(cmds, cmd_count);
    }
    return;
}
