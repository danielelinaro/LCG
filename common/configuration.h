#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "common.h"
#include "types.h"
#include <vector>
#include <stdio.h>
#include <string.h>

class Channel;
class InputChannel;
class OutputChannel;

int parse_configuration_file(const char *filename, std::vector<InputChannel*>& input_channels, std::vector<OutputChannel*>& output_channels);
void print_channel(Channel* opts);

#endif

