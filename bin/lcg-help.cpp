#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
        char cmd[64];
        sprintf(cmd, "lcg-%s -h", argv[1]);
        system(cmd);
        return 0;
}

