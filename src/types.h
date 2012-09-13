#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <map>

#define LABEL_LEN       30

namespace dynclamp {
class Entity;
}

typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ullong;
typedef double real;
typedef std::vector<double> array;
typedef std::vector<std::string> strings;
typedef std::map<std::string,std::string> dictionary;

#ifdef __cplusplus
extern "C" {
#endif
typedef dynclamp::Entity* (*Factory)(dictionary&);
#ifdef __cplusplus
}
#endif

#endif

