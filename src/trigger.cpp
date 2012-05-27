#include "trigger.h"
#include "engine.h"

dynclamp::Entity* PeriodicTriggerFactory(dictionary& args)
{
        uint id;
        double frequency;
        id = dynclamp::GetIdFromDictionary(args);
        if ( ! dynclamp::CheckAndExtractDouble(args, "frequency", &frequency)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a periodic trigger.\n");
                return NULL;
        }
        return new dynclamp::PeriodicTrigger(frequency, id);
}

namespace dynclamp {

Trigger::Trigger(uint id) : Entity(id)
{
        setName("Trigger");
}

double Trigger::output() const
{
        return 0.0;
}

void Trigger::emitTrigger() const
{
        emitEvent(new TriggerEvent(this));
}

PeriodicTrigger::PeriodicTrigger(double frequency, uint id)
        : Trigger(id), m_period(1.0/frequency)
{
        if (frequency <= 0)
                throw "Frequency should be positive";
        m_parameters.push_back(frequency);
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

double PeriodicTrigger::frequency() const
{
        return PT_FREQUENCY;
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
        m_period = period;
        PT_FREQUENCY = 1.0 / period;
}

} // namespace dynclamp

