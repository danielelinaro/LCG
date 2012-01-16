#include <cmath>
#include "events.h"
#include "thread_safe_queue.h"
#include "poisson_generator.h"

namespace dynclamp {

extern ThreadSafeQueue<Event*> eventsQueue;

namespace generators {

Poisson::Poisson(double rate, ullong seed, uint id, double dt)
        : Generator(id, dt), m_rate(rate), m_random(seed), m_tNextSpike(0.0)
{
        calculateTimeNextSpike();
}

bool Poisson::hasNext() const
{
        return true;
}

double Poisson::output() const
{
        return 0.0;
}

void Poisson::step()
{
        if (GetGlobalTime() >= m_tNextSpike) {
                emitEvent(new SpikeEvent(this));
                calculateTimeNextSpike();
        }
}

void Poisson::calculateTimeNextSpike()
{
        m_tNextSpike += (- log(m_random.doub()) / m_rate);
        Logger(Debug, "%e\n", m_tNextSpike);
}

} // namespace generators

} // namespace dynclamp

