#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERV_PORT 5140
#define BUFF_SIZE 10240
#define PATH_LENGTH 256
#define INIT_PATH "bin"
#define CMDLINE_LENGTH 10240
#define SINGLE_CMD_WORD 256
#define WELCOME "****************************************\n** Welcome to the information server. **\n****************************************\n"
#define PROMPT "% "

struct cmd {
    char **argv;
    int argc;
    int is_piped;
};

int send_msg(int, char *);
int read_line(int, char *);
int parse_line(char *, struct cmd []);
void clear_line(struct cmd [], int);
int execute_line(int, struct cmd [], char []);
void connection_handler(int);

int main(int argc, char *argv[]) {
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
                connection_handler(connfd);
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

int send_msg(int fd, char *msg) {
    char *sbuff = NULL;
    int msg_len = strlen(msg);
    int n = 0;
    sbuff = (char *)malloc((msg_len + 1) * sizeof(char));
    strncpy(sbuff, msg, strlen(msg) + 1);
    sbuff[msg_len] = '\0';
    if((n = write(fd, sbuff, strlen(msg))) < 0)
        perror("server: write error");
    free(sbuff);
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

int parse_line(char *buff, struct cmd cmds[]) {
    char *pch = NULL;               /* single word */
    char *tmp_cmd[SINGLE_CMD_WORD]; /* single command */
    int c = 0;                      /* command number */
    int i = 0;                      /* word number in whole command line */
    int w = 0;                      /* word number in single command */
    pch = strtok(buff, " \n\r\t");
    for(i = 0; pch != NULL; i++) {
        tmp_cmd[i] = pch;
        if(strcmp(pch, "|") == 0) {
            cmds[c].argv = (char **)malloc((i + 1) * sizeof(char *));
            for(w = 0; w < i + 1; w++)
                cmds[c].argv[w] = tmp_cmd[w];
            cmds[c].argc = w - 1;   /* excluding pipe character */
            cmds[c].is_piped = 1;
            c++;
            i = -1;
        }
        pch = strtok(NULL, " \n\r\t");
    }
    cmds[c].argv = (char **)malloc((i + 1) * sizeof(char *));
    for(w = 0; w < i; w++)
        cmds[c].argv[w] = tmp_cmd[w];
    cmds[c].argc = w;
    cmds[c].is_piped = 0;
    return c + 1;
}

void clear_line(struct cmd cmds[], int cmd_count) {
    int i = 0;
    for(i = 0; i < cmd_count; i++)
        free(cmds[i].argv);
    return;
}

int execute_line(int fd, struct cmd cmds[], char path[]) {
    int status = 0;
    if(strcmp(cmds[0].argv[0], "exit") == 0)
        status = 1;
    else if(strcmp(cmds[0].argv[0], "printenv") == 0) {
        send_msg(fd, path);
    }
    else if(strcmp(cmds[0].argv[0], "setenv") == 0) {
        strncpy(path, cmds[0].argv[2], strlen(cmds[0].argv[2]));
    }
    else {
        ;
    }
    return status;
}

void connection_handler(int fd) {
    struct cmd cmds[CMDLINE_LENGTH];
    char buff[BUFF_SIZE];
    char path[PATH_LENGTH];
    int line_len;
    int cmd_count = 0;
    int status = 0;
    /*int i = 0, j = 0;*/
    strncpy(path, INIT_PATH, strlen(INIT_PATH));
    send_msg(fd, WELCOME);
    while(1) {
        send_msg(fd, PROMPT);
        if((line_len = read_line(fd, buff)) == 0)    /* client close connection */
            break;
        else if(line_len == 1)                       /* enter key */
            continue;
        cmd_count = parse_line(buff, cmds);
        /*for(i = 0; i < cmd_count; i++) {
            printf("argc: %d\nis_piped: %d\nargv: ", cmds[i].argc, cmds[i].is_piped);
            for(j = 0; j < cmds[i].argc; j++)
                printf("%s ", cmds[i].argv[j]);
            printf("\n\n");
        }*/
        if((status = execute_line(fd, cmds, path)) == 1) {
            clear_line(cmds, cmd_count);
            break;
        }
        clear_line(cmds, cmd_count);
    }
    return;
}
