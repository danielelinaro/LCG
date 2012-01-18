#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <map>

#define SEED 5061983

typedef unsigned int uint;
typedef unsigned long ulong;
typedef double real;
typedef std::vector<double> array;
typedef std::map<std::string,std::string> dictionary;

namespace dynclamp {
class Entity;
}

#ifdef __cplusplus
extern "C" {
#endif
typedef dynclamp::Entity* (*Factory)(dictionary&);
#ifdef __cplusplus
}
#endif

#endif

