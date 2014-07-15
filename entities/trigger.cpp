#include "trigger.h"

lcg::Entity* PeriodicTriggerFactory(string_dict& args)
{
        uint id;
        double frequency;
	double tdelay;
	double tend;

        id = lcg::GetIdFromDictionary(args);
        if ( ! lcg::CheckAndExtractDouble(args, "frequency", &frequency)) {
                lcg::Logger(lcg::Critical, "Unable to build a periodic trigger.\n");
                return NULL;
        }
	
        if ( ! lcg::CheckAndExtractDouble(args, "delay", &tdelay)) {
		tdelay = 0;
	}

        if ( ! lcg::CheckAndExtractDouble(args, "tend", &tend)) {
		tend = INFINITY;
	}
        return new lcg::PeriodicTrigger(frequency, tdelay, tend, id);
}

namespace lcg {

Trigger::Trigger(uint id) : Entity(id)
{
        setName("Trigger");
}

double Trigger::output()
{
        return 0.0;
}

void Trigger::emitTrigger() const
{
        emitEvent(new TriggerEvent(this));
}

PeriodicTrigger::PeriodicTrigger(double frequency, double tdelay, double tend, uint id)
        : Trigger(id)
{
        setFrequency(frequency);
	m_tDelay = tdelay;
	m_tEnd = tend;
}

bool PeriodicTrigger::initialise()
{
        m_tNextTrigger = m_period + m_tDelay;
        return true;
}

void PeriodicTrigger::step()
{
        if (GetGlobalTime() >= m_tNextTrigger 
		&& GetGlobalTime() >= m_tDelay 
		&& GetGlobalTime() <= m_tEnd) {
                emitTrigger();
                m_tNextTrigger += m_period;
        }
}

void PeriodicTrigger::setFrequency(double frequency)
{
        if (frequency <= 0)
                throw "Frequency should be positive";
        PT_FREQUENCY = frequency;
        m_period = 1.0 / frequency;
}

double PeriodicTrigger::period() const
{
        return m_period;
}

void PeriodicTrigger::setPeriod(double period)
{
        if (period <= 0)
                throw "Period should be positive";
        PT_FREQUENCY = 1.0 / period;
        m_period = period;
}

} // namespace lcg

