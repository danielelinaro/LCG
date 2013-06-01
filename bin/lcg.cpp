#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
        char cmd[64], *ptr;
        ptr = cmd;
        sprintf(ptr, "lcg-%s", argv[1]);
        ptr += strlen(ptr);
        for (int i=2; i<argc; i++, ptr+=strlen(ptr))
                sprintf(ptr, " %s", argv[i]);
        system(cmd);
        return 0;
}

