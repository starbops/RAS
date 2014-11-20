#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * CMDLINE  :   whole line
 * CMD      :   single command
 * LEN      :   number of characters
 * NUM      :   number of words
 */

#define MAX_CMDLINE_LEN 10240
#define MAX_CMDLINE_NUM 10240
#define MAX_CMD_LEN 256
#define MAX_CMD_NUM 256

struct cmd {
    char **argv;
    int argc;
    int pipefrom;
    int pipeto;
};

int parser(struct cmd [], char *);
void clear(struct cmd [], int);
int preprocess(struct cmd []);
void process_line(struct cmd [], int);
void postprocess();
