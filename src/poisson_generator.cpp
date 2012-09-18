#include <math.h>
#include "events.h"
#include "thread_safe_queue.h"
#include "poisson_generator.h"
#include "engine.h"

dynclamp::Entity* PoissonFactory(string_dict& args)
{
        uint id;
        double rate;
        ullong seed;
        id = dynclamp::GetIdFromDictionary(args);
        seed = dynclamp::GetSeedFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "rate", &rate)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a Poisson generator.\n");
                return NULL;
        }
        return new dynclamp::generators::Poisson(rate, seed, id);
}

namespace dynclamp {

extern ThreadSafeQueue<Event*> eventsQueue;

namespace generators {

Poisson::Poisson(double rate, ullong seed, uint id)
        : Generator(id), m_random(seed)
{
        POISSON_RATE = rate;
        setName("PoissonGenerator");
}

bool Poisson::initialise()
{
        calculateTimeNextSpike();
        return true;
}

bool Poisson::hasNext() const
{
        return true;
}

double Poisson::output()
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
        m_tNextSpike += (- log(m_random.doub()) / POISSON_RATE);
        Logger(Debug, "%e\n", m_tNextSpike);
}

} // namespace generators

} // namespace dynclamp

