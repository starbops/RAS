#include "cmd.h"

int parser(struct cmd cmds[], char *buf) {
    char *pch = NULL;
    char *words[MAX_CMD_NUM];
    int checkpoint = 0;
    int cmdindex = 0;
    int i = 0, j = 0;
    int pipeto = 0;

    pch = strtok(buf, " \t\r\n");
    while(pch != NULL) {
        words[i] = pch;

        if(strcmp(words[i], "|") == 0) {
            pipeto = cmdindex;
            checkpoint = 1;
        } else if(*words[i] - '|' == 0) {
            pipeto = cmdindex + (int)strtol(pch + 1, NULL, 10) - 1;
            checkpoint = 1;
        }

        if(checkpoint != 0) {
            checkpoint = 0;
            cmds[cmdindex].argv = (char **)malloc(i * sizeof(char *));
            for(j = 0; j < i; j++)
                cmds[cmdindex].argv[j] = words[j];
            cmds[cmdindex].argc = i;
            cmdindex++;
            i = 0;
        } else {
            i++;
        }

        pch = strtok(NULL, " \t\r\n");
    }

    if(i > 0) {
        cmds[cmdindex].argv = (char **)malloc(i * sizeof(char *));
        for(j = 0; j < i; j++)
            cmds[cmdindex].argv[j] = words[j];
        cmds[cmdindex].argc = i;
        cmdindex++;
    }

    return cmdindex;
}

void clear(struct cmd cmds[], int cmdnum) {
    int i = 0;

    for(i = 0; i < cmdnum; i++)
        free(cmds[i].argv);
}

int preprocess(struct cmd cmds[]) {
    char *envar = NULL;
    int is_builtin = 0;

    if(strcmp(cmds[0].argv[0], "printenv") == 0 && cmds[0].argc == 2) {
        if((envar = getenv(cmds[0].argv[1])) != NULL) {
            fprintf(stdout, "%s=%s\n", cmds[0].argv[1], envar);
            fflush(stdout);
        }
        is_builtin = 1;
    } else if(strcmp(cmds[0].argv[0], "setenv") == 0 && cmds[0].argc == 3) {
        setenv(cmds[0].argv[1], cmds[0].argv[2], 1);
        is_builtin = 1;
    } else if(strcmp(cmds[0].argv[0], "exit") == 0) {
        exit(0);
    }

    return is_builtin;
}

void process_line(struct cmd cmds[], int cmdnum) {
    int cpid = 0;
    int status = 0;

    if(preprocess(cmds))
        return;

    if((cpid = fork()) < 0) {
        perror("server: fork error");
        exit(-1);
    } else if(cpid == 0) {
        execvp(cmds[0].argv[0], cmds[0].argv);
        fprintf(stderr, "Unknown command: [%s].\n", cmds[0].argv[0]);
        fflush(stderr);
        exit(-1);
    } else {
        waitpid(cpid, &status, 0);
    }
}
