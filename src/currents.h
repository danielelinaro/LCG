#ifndef CURRENTS_H
#define CURRENTS_H

#include "dynamical_entity.h"
#include "neurons.h"

#define IC_FRACTION     m_state[0]              // (1)

#define HH_NA_M         m_state[1]
#define HH_NA_H         m_state[2]

#define HH_K_N          m_state[1]

#define NIC_NOPEN       m_state[1]              // (1)

#define IC_AREA         m_parameters[0]         // (um^2)
#define IC_GBAR         m_parameters[1]         // (S/cm^2)
#define IC_E            m_parameters[2]         // (mV)

#define NIC_GAMMA       m_parameters[3]         // (pS)
#define NIC_NCHANNELS   m_parameters[4]         // (1)

#define V_TEST          -65

namespace dynclamp {

namespace ionic_currents {

class IonicCurrent : public DynamicalEntity {
public:
        IonicCurrent(double area, double gbar, double E, uint id = GetId(), double dt = GetGlobalDt());
        double output() const;
protected:
        virtual void addPost(Entity *entity);
protected:
        neurons::Neuron *m_neuron;
};

class HHSodium : public IonicCurrent {
public:
        HHSodium(double area, double gbar = 0.12, double E = 50, uint id = GetId(), double dt = GetGlobalDt());

protected:
        void evolve();

	double vtrap(double x, double y) const;
        double alpham(double v) const;
        double betam(double v) const;
        double alphah(double v) const;
        double betah(double v) const;

};

class HHPotassium : public IonicCurrent {
public:
        HHPotassium(double area, double gbar = 0.036, double E = -77, uint id = GetId(), double dt = GetGlobalDt());

protected:
        void evolve();

	double vtrap(double x, double y) const;
        double alphan(double v) const;
        double betan(double v) const;

};

class NoisyIonicCurrent : public IonicCurrent {
public:
        NoisyIonicCurrent(double area, double gbar, double E, double gamma, uint id = GetId(), double dt = GetGlobalDt());
};


} // namespace ionic_currents

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* HHSodiumFactory(dictionary& args);
dynclamp::Entity* HHPotassiumFactory(dictionary& args);
        
#ifdef __cplusplus
}
#endif

#endif

