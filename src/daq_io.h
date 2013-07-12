#ifndef DAQ_IO_H
#define DAQ_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <comedilib.h>
#include <time.h>
#include <math.h>

#include <vector>

#include "configuration.h"

char *cmd_src(int src, char *buf);
void dump_cmd(FILE *out, comedi_cmd *cmd);
int setup_io(const std::vector<channel_opts*>& channels, double dt, double tend);

#endif // DAQ_IO_H

