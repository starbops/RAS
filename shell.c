#include <arpa/inet.h>
#include <fcntl.h>
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
    /* -1: no pipe          *
     *  0: file redirection *
     *  1: normal pipe      *
     * >1: numbered-pipe    */
    int pipeto;
    int pipefrom;
};
struct pp {
    int fds[2];
    int is_set;
};
void reaper(int);
int send_msg(int, char *);
int read_line(int, char *);
int parse_line(char *, struct cmd []);
int preprocess_line(struct cmd [], int, struct pp []);
void clear_line(struct cmd [], int);
void do_magic(struct cmd [], int);
int execute_line(struct cmd [], int);
void connection_handler();

int main(int argc, char *argv[]) {
    int listenfd = 0;
    int optval = 1;
    int connfd = 0;
    int clilen = 0;
    int conn_pid = 0;
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

    signal(SIGCHLD, &reaper);

    while(1) {
        clilen = sizeof(conn_addr);
        if((connfd = accept(listenfd, (struct sockaddr *)&conn_addr, (socklen_t *)&clilen)) < 0) {
            perror("server: accept error");
        }
        if((conn_pid = fork()) < 0) {
            perror("server: fork error");
        } else if(conn_pid == 0) {          /* child process */
            close(listenfd);
            dup2(connfd, fileno(stdin));
            dup2(connfd, fileno(stdout));
            dup2(connfd, fileno(stderr));
            connection_handler();
            exit(0);
        } else {                            /* parent process */
            close(connfd);
        }
    }

    return 0;
}

void reaper(int sig) {
    int status;
    while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0) {
    }
    return;
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
    int checkpoint = 0;             /* end of single command */
    pch = strtok(buff, " \n\r\t");
    for(i = 0; pch != NULL; i++) {
        tmp_cmd[i] = pch;
        if(strcmp(pch, ">") == 0) {         /* file redirection */
            cmds[c].is_piped = 0;
            cmds[c].pipeto = -1;
            checkpoint = 1;
        } else if(strcmp(pch, "|") == 0) {  /* normal pipe */
            cmds[c].is_piped = 1;
            cmds[c].pipeto = c;
            checkpoint = 1;
        } else if(*pch - '|' == 0) {        /* numbered-pipe */
            cmds[c].is_piped = (int)strtol(pch + 1, NULL, 10);
            cmds[c].pipeto = c;
            checkpoint = 1;
        }
        if(checkpoint == 1) {
            cmds[c].argv = (char **)malloc((i + 1) * sizeof(char *));
            for(w = 0; w < i; w++)
                cmds[c].argv[w] = tmp_cmd[w];
            cmds[c].argv[w] = NULL; /* pipe character is not included */
            cmds[c].argc = w;
            cmds[c].pipefrom = -1;
            c++;
            i = -1;
            checkpoint = 0;
        }
        pch = strtok(NULL, " \n\r\t");
    }
    cmds[c].argv = (char **)malloc((i + 1) * sizeof(char *));
    for(w = 0; w < i; w++)
        cmds[c].argv[w] = tmp_cmd[w];
    cmds[c].argv[w] = NULL;
    cmds[c].argc = w;
    cmds[c].is_piped = -1;
    cmds[c].pipeto = -1;
    cmds[c].pipefrom = -1;

    return c + 1;
}

int preprocess_line(struct cmd cmds[], int cn, struct pp pps[]) {
    int i = 0;
    int j = 0;
    int dst = 0;
    int pn = 0;
    for(i = 0; i < cn; i++) {
        if(cmds[i].is_piped > 0) {
            dst = i + cmds[i].is_piped;
            cmds[dst].pipefrom = i;
            for(j = i + 1; j < cn; j++) {
                if(j > dst)
                    break;
                if(j + cmds[j].is_piped == dst) {
                    cmds[j].is_piped = -1;
                    cmds[j].pipeto = i;
                }
            }
        }
    }
    for(i = 0; i < cn; i++) {
        if (cmds[i].is_piped > 0) {
            /*pipe(pps[i].fds);*/
            pps[i].is_set = 1;
            pn++;
        }
    }
    /*for(i = 0; i < cn; i++) {
        printf("%d is pipe to %d\tpipe from %d\n", i, cmds[i].pipeto, cmds[i].pipefrom);
        fflush(stdout);
    }
    for(i = 0; i < cn; i++) {
        printf("%d is set to %d\n", i, pps[i].is_set);
        fflush(stdout);
    }*/
    /* reset */
    for(i = 0; i < cn; i++)
        if(pps[i].is_set == 1) {
            pps[i].is_set = 0;
        }
    return pn;
}

void clear_line(struct cmd cmds[], int cn) {
    int i = 0;
    for(i = 0; i < cn; i++)
        free(cmds[i].argv);
    return;
}

void do_magic(struct cmd cmds[], int cn) {
    int cpid = 0;
    int filefd = 0;
    int i = 0;
    int new_pipefds[2];
    int old_pipefds[2];
    int status = 0;
    if(cn > 1 && cmds[cn - 2].is_piped == 0)
        filefd = open(cmds[cn - 1].argv[0], O_CREAT | O_RDWR, S_IREAD | S_IWRITE);
    for(i = 0; i < cn; i++) {
        if(i != cn - 1)                 /* no new pipe for last command */
            pipe(new_pipefds);
        if((cpid = fork()) < 0) {       /* fork for every commands */
            perror("server: fork error");
        } else if(cpid == 0) {                  /* child process */
            if(i != 0) {                            /* no head */
                close(old_pipefds[1]);
                dup2(old_pipefds[0], fileno(stdin));
                close(old_pipefds[0]);
            }
            if(i == cn - 2 && cmds[i].is_piped == 0) {
                dup2(filefd, fileno(stdout));
                close(filefd);
            } else if(i != cn - 1) {                        /* no tail */
                close(new_pipefds[0]);
                dup2(new_pipefds[1], fileno(stdout));
                close(new_pipefds[1]);
            }
            execvp(cmds[i].argv[0], cmds[i].argv);
            exit(1);
        } else {                                /* parent process */
            if(i != 0) {                            /* no head */
                close(old_pipefds[0]);
                close(old_pipefds[1]);
            }
            if(i == cn - 2 && cmds[i].is_piped == 0)
                i++;
            if(i != cn -1) {                        /* no tail */
                old_pipefds[0] = new_pipefds[0];
                old_pipefds[1] = new_pipefds[1];
            }
            waitpid(cpid, &status, 0);
        }
    }
    if(cn > 1) {
        close(old_pipefds[0]);
        close(old_pipefds[1]);
    }
    return;
}

int execute_line(struct cmd cmds[], int cn) {
    char *envar = NULL;
    int status = 0;
    if(strcmp(cmds[0].argv[0], "exit") == 0)
        status = 1;
    else if(strcmp(cmds[0].argv[0], "printenv") == 0) {
        if((envar = getenv(cmds[0].argv[1])) != NULL)
            send_msg(fileno(stdout), envar);
    }
    else if(strcmp(cmds[0].argv[0], "setenv") == 0) {
        setenv(cmds[0].argv[1], cmds[0].argv[2], 1);
    }
    else
        ;/*do_magic(cmds, cn);*/

    return status;
}

void connection_handler() {
    struct cmd cmds[CMDLINE_LENGTH];
    struct pp pps[CMDLINE_LENGTH];
    char buff[BUFF_SIZE];
    int line_len = 0;
    int cn = 0;
    int pn = 0;
    /*int i = 0;*/
    int status = 0;
    setenv("PATH", "bin:.", 1);
    send_msg(fileno(stdout), WELCOME);
    while(1) {
        send_msg(fileno(stdout), PROMPT);
        if((line_len = read_line(fileno(stdin), buff)) == 0)    /* client close connection */
            break;
        else if(line_len == 1)                       /* enter key */
            continue;
        cn = parse_line(buff, cmds);
        pn = preprocess_line(cmds, cn, pps);
        if((status = execute_line(cmds, cn)) == 1) {
            clear_line(cmds, cn);
            break;
        }
        clear_line(cmds, cn);
    }
    return;
}
