#ifndef PID_H
#define PID_H

#include "entity.h"
#include "utils.h"

namespace dynclamp {

#define PID_BASELINE m_parameters[0]
#define PID_GP       m_parameters[1]
#define PID_GI       m_parameters[2]
#define PID_GD       m_parameters[3]

class PID : public Entity {
public:
        PID(double baseline, double gp, double gi, double gd = 0.0, uint id = GetId());
        virtual double output() const;
        virtual bool initialise();
        virtual void step();
        void handleEvent(const Event *event);
private:
        double m_output;
        double m_erri;
        double m_errpPrev;
};

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* PIDFactory(dictionary& args);
        
#ifdef __cplusplus
}
#endif

#endif

