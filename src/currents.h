#ifndef CURRENTS_H
#define CURRENTS_H

#include "dynamical_entity.h"
#include "neurons.h"
#include "randlib.h"

#define IC_FRACTION     m_state[0]              // (1)

#define HH_NA_M         m_state[1]
#define HH_NA_H         m_state[2]

#define HH_K_N          m_state[1]

#define NIC_NOPEN       m_state[1]              // (1)

#define HH_NA_CN_M      m_state[2]
#define HH_NA_CN_H      m_state[3]

#define HH_K_CN_N       m_state[2]

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
        IonicCurrent(double area, double gbar, double E, uint id = GetId());
        virtual bool initialise();
        double output() const;
protected:
        virtual void addPost(Entity *entity);
protected:
        neurons::Neuron *m_neuron;
};

class HHSodium : public IonicCurrent {
public:
        HHSodium(double area, double gbar = 0.12, double E = 50, uint id = GetId());

        virtual bool initialise();

	static double vtrap(double x, double y);
        static double alpham(double v);
        static double betam(double v);
        static double alphah(double v);
        static double betah(double v);

protected:
        void evolve();
};

class HHPotassium : public IonicCurrent {
public:
        HHPotassium(double area, double gbar = 0.036, double E = -77, uint id = GetId());

        virtual bool initialise();

	static double vtrap(double x, double y);
        static double alphan(double v);
        static double betan(double v);

protected:
        void evolve();
};

class NoisyIonicCurrent : public IonicCurrent {
public:
        NoisyIonicCurrent(double area, double gbar, double E, double gamma, uint id = GetId());
        virtual bool initialise();
};

class HHSodiumCN : public NoisyIonicCurrent {
public:
        HHSodiumCN(double area, ullong seed = GetRandomSeed(), double gbar = 0.12, double E = 50, double gamma = 10,
                   uint id = GetId());
        ~HHSodiumCN();
        virtual bool initialise();

public:
        static const uint numberOfStates = 8;

protected:
        void evolve();

private:
        NormalRandom *m_rand;
        double m_z[numberOfStates-1];
};

class HHPotassiumCN : public NoisyIonicCurrent {
public:
        HHPotassiumCN(double area, ullong seed = GetRandomSeed(), double gbar = 0.036, double E = -77, double gamma = 10,
                      uint id = GetId());
        ~HHPotassiumCN();
        virtual bool initialise();

public:
        static const uint numberOfStates = 5;

protected:
        void evolve();

private:
        NormalRandom *m_rand;
        double m_z[numberOfStates-1];
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
dynclamp::Entity* HHSodiumCNFactory(dictionary& args);
dynclamp::Entity* HHPotassiumCNFactory(dictionary& args);
        
#ifdef __cplusplus
}
#endif

#endif

