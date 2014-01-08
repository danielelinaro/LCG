/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    configuration.h
 *
 *   Copyright (C) 2012,2013,2014 Daniele Linaro
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *=========================================================================*/

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

