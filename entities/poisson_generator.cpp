#include <math.h>
#include "events.h"
#include "thread_safe_queue.h"
#include "poisson_generator.h"

lcg::Entity* PoissonFactory(string_dict& args)
{
        uint id;
        double rate;
        ullong seed;
        id = lcg::GetIdFromDictionary(args);
        seed = lcg::GetSeedFromDictionary(args);
        if ( ! lcg::CheckAndExtractDouble(args, "rate", &rate)) {
                lcg::Logger(lcg::Critical, "Unable to build a Poisson generator.\n");
                return NULL;
        }
        if (rate == 0.) {
                lcg::Logger(lcg::Critical, "The rate of spike emission cannot be equal to 0.\n");
                return NULL;
        }
        return new lcg::generators::Poisson(rate, seed, id);
}

namespace lcg {

extern ThreadSafeQueue<Event*> eventsQueue;

namespace generators {

Poisson::Poisson(double rate, ullong seed, uint id)
        : Generator(id), m_random(seed) 
{
		setHasOutput(false);
        POISSON_RATE = rate;
        POISSON_SEED = (double) seed;
        if (POISSON_RATE == 0.) {
                Logger(Critical, "The rate of spike emission cannot be equal to 0.\n");
                throw "Invalid rate.";
        }
        else if (POISSON_RATE < 0.) {
                m_deterministic = true;
                m_period = -1. / POISSON_RATE;
        }
        else {
                m_deterministic = false;
                m_period = 1. / POISSON_RATE;
        }
        setName("PoissonGenerator");
}

bool Poisson::initialise()
{
        m_tNextSpike = 0.;
        m_random.setSeed((ullong) POISSON_SEED);
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
        if (m_deterministic)
                m_tNextSpike += m_period;
        else 
                m_tNextSpike += (- log(m_random.doub()) * m_period);
        Logger(Debug, "%e\n", m_tNextSpike);
}

} // namespace generators

} // namespace lcg

