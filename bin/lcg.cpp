#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
        char cmd[4096], *ptr;
        if (argc > 1) {
                ptr = cmd;
                if (argv[1][0] == '-')
                        sprintf(ptr, "lcg-help %s", argv[1]);
                else
                        sprintf(ptr, "lcg-%s", argv[1]);
                ptr += strlen(ptr);
                for (int i=2; i<argc; i++, ptr+=strlen(ptr))
                        sprintf(ptr, " %s", argv[i]);
        }
        else {
                // the user didn't specify any command, give them help...
                sprintf(cmd, "lcg-help");
        }
        system(cmd);
        return 0;
}

