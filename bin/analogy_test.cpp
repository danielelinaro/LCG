#include "analogy_io.h"

using namespace lcg;

int main()
{
        uint subdevice = 0, channel = 0;
        AnalogyAnalogIO io("analogy0", subdevice, &channel, 1);
        return 0;
}

