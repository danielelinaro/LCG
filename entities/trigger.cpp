#include "trigger.h"

lcg::Entity* PeriodicTriggerFactory(string_dict& args)
{
        uint id;
        double frequency;
        id = lcg::GetIdFromDictionary(args);
        if ( ! lcg::CheckAndExtractDouble(args, "frequency", &frequency)) {
                lcg::Logger(lcg::Critical, "Unable to build a periodic trigger.\n");
                return NULL;
        }
        return new lcg::PeriodicTrigger(frequency, id);
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

PeriodicTrigger::PeriodicTrigger(double frequency, uint id)
        : Trigger(id)
{
        if (frequency <= 0)
                throw "Frequency should be positive";
        PT_FREQUENCY = frequency;
        m_period = 1.0 / PT_FREQUENCY;
}

bool PeriodicTrigger::initialise()
{
        m_tNextTrigger = m_period;
        return true;
}

void PeriodicTrigger::step()
{
        if (GetGlobalTime() >= m_tNextTrigger) {
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

