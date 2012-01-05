#ifndef RECORDER_H
#define RECORDER_H

#include "utils.h"
#include "entity.h"

namespace dynclamp {

class Recorder : public Entity {
public:
        Recorder(const char *filename, uint id = GetId(), double dt = GetGlobalDt());
        virtual ~Recorder();
};

} // namespace dynclamp

#endif

