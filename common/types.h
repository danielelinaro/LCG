/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    types.h
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

#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <map>

#define LABEL_LEN       30

namespace lcg {
class Entity;
class Stream;
}

typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ullong;
typedef double real;
typedef std::vector<double> array;
typedef std::vector<std::string> strings;
typedef std::map<std::string,std::string> string_dict;
typedef std::map<std::string,double> double_dict;

#ifdef __cplusplus
extern "C" {
#endif
typedef lcg::Entity* (*NttFactory)(string_dict&);
typedef lcg::Stream* (*StrmFactory)(string_dict&);
#ifdef __cplusplus
}
#endif

#endif

