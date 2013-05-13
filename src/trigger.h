#ifndef TRIGGER_H
#define TRIGGER_H

#include "entity.h"
#include "utils.h"

namespace lcg {

class Trigger : public Entity {
public:
        Trigger(uint id = GetId());
        virtual double output();
protected:
        void emitTrigger() const;
};

#define PT_FREQUENCY m_parameters["f"]

class PeriodicTrigger : public Trigger {
public:
        PeriodicTrigger(double frequency, uint id = GetId());

        void setFrequency(double frequency);
        double period() const;
        void setPeriod(double period);

        virtual bool initialise();
        virtual void step();
private:
        double m_period;
        double m_tNextTrigger;
};

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* PeriodicTriggerFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif


#endif

