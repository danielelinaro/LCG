#include <math.h>
#include "events.h"
#include "thread_safe_queue.h"
#include "poisson_generator.h"

dynclamp::Entity* PoissonFactory(dictionary& args)
{
        uint id;
        double rate;
        ullong seed;
        id = dynclamp::GetIdFromDictionary(args);
        dynclamp::GetSeedFromDictionary(args, &seed);
        if ( ! dynclamp::CheckAndExtractDouble(args, "rate", &rate))
                return NULL;
        return new dynclamp::generators::Poisson(rate, seed, id);
}

namespace dynclamp {

extern ThreadSafeQueue<Event*> eventsQueue;

namespace generators {

Poisson::Poisson(double rate, ullong seed, uint id)
        : Generator(id), m_rate(rate), m_random(seed), m_tNextSpike(0.0)
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

