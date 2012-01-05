#include "recorder.h"

namespace dynclamp {

Recorder::Recorder(const char *filename, uint id, double dt)
        : Entity(id, dt)
{
        // open the file
}

Recorder::~Recorder()
{
        // close the file
}

} // namespace dynclamp

