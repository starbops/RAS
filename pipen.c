#include "pipen.h"

void initpipe(int ppt[], int pptsize, int pps[][2], int ppssize) {
    int i = 0;

    for(i = 0; i < pptsize; i++)
        ppt[i] = 0;
    for(i = 0; i < pptsize; i++) {
        pps[i][0] = -1;
        pps[i][1] = -1;
    }
}
