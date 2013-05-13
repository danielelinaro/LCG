#ifndef CURRENTS_H
#define CURRENTS_H

#include "dynamical_entity.h"
#include "neurons.h"
#include "randlib.h"

namespace lcg {

namespace ionic_currents {

double vtrap(double x, double y);

#define IC_FRACTION     m_state[0]              // (1)

#define IC_AREA         m_parameters["area"]         // (um^2)
#define IC_GBAR         m_parameters["gbar"]         // (S/cm^2)
#define IC_E            m_parameters["E"]            // (mV)

class IonicCurrent : public DynamicalEntity {
public:
        IonicCurrent(double area, double gbar, double E, uint id = GetId());
        virtual bool initialise();
        double output();
protected:
        virtual void addPost(Entity *entity);
protected:
        neurons::Neuron *m_neuron;
};

#define HH_NA_M         m_state[1]
#define HH_NA_H         m_state[2]

class HHSodium : public IonicCurrent {
public:
        HHSodium(double area, double gbar = 0.12, double E = 50, uint id = GetId());

        virtual bool initialise();

        static double alpham(double v);
        static double betam(double v);
        static double alphah(double v);
        static double betah(double v);

protected:
        void evolve();
};

#define HH_K_N          m_state[1]

class HHPotassium : public IonicCurrent {
public:
        HHPotassium(double area, double gbar = 0.036, double E = -77, uint id = GetId());

        virtual bool initialise();

        static double alphan(double v);
        static double betan(double v);

protected:
        void evolve();
};

#define HH2_NA_M        m_state[1]
#define HH2_NA_H        m_state[2]

#define HH2_VTRAUB      m_parameters["vtraub"]         // (mV)
#define HH2_TEMPERATURE m_parameters["temperature"]    // (celsius)

class HH2Sodium : public IonicCurrent {
public:
        HH2Sodium(double area, double gbar = 0.003, double E = 50, double vtraub = -63., double temperature = 36., uint id = GetId());

        virtual bool initialise();

        static double alpham(double v);
        static double betam(double v);
        static double alphah(double v);
        static double betah(double v);

protected:
        void evolve();

private:
        double m_tadj;
};

#define HH2_K_N         m_state[1]

class HH2Potassium : public IonicCurrent {
public:
        HH2Potassium(double area, double gbar = 0.005, double E = -90, double vtraub = -63., double temperature = 36., uint id = GetId());

        virtual bool initialise();

        static double alphan(double v);
        static double betan(double v);

protected:
        void evolve();

private:
        double m_tadj;
};

#define IM_M        m_state[1]

#define IM_TAUMAX      m_parameters["tauMax"]         // (ms)
#define IM_TEMPERATURE m_parameters["temperature"]    // (celsius)

/*!
 * \class MCurrent
 * \brief M-current, responsible for the adaptation of firing rate and the 
 *        afterhyperpolarization (AHP) of cortical pyramidal cells
 *
 * First-order model described by hodgkin-Hyxley like equations.
 * K+ current, activated by depolarization, noninactivating.
 *
 * Model taken from Yamada, W.M., Koch, C. and Adams, P.R.  Multiple 
 * channels and calcium dynamics.  In: Methods in Neuronal Modeling, 
 * edited by C. Koch and I. Segev, MIT press, 1989, p 97-134.
 *
 * See also: McCormick, D.A., Wang, Z. and Huguenard, J. Neurotransmitter 
 * control of neocortical neuronal activity and excitability. 
 * Cerebral Cortex 3: 387-398, 1993.
 */
class MCurrent : public IonicCurrent {
public:
        MCurrent(double area, double gbar = 0.005, double E = -90, double taumax = 1000., double temperature = 36., uint id = GetId());
        virtual bool initialise();

protected:
        void evolve();

private:
        double m_tadj;
        double m_tauPeak;
};

#define IT_CAI      m_state[1]
#define IT_H        m_state[2]

#define IT_Q10          m_parameters["q10"]           // (1)
#define IT_SHIFT        m_parameters["shift"]         // (mV)
#define IT_CAO          m_parameters["cao"]           // (mM)
#define IT_CAIINF       m_parameters["caiInf"]        // (mM)
#define IT_TAUR         m_parameters["taur"]          // (ms)
#define IT_DEPTH        m_parameters["depth"]         // (um)
#define IT_TEMPERATURE  m_parameters["temperature"]   // (celsius)

#define FARADAY         (96489)

/*!
 * \class TCurrent
 * \brief Low threshold calcium current
 * 
 * Ca++ current responsible for low threshold spikes (LTS)
 * in thalamocortical cells.
 * 
 * Model based on the data of Huguenard & McCormick, J Neurophysiol
 * 68: 1373-1383, 1992 and Huguenard & Prince, J Neurosci.
 * 12: 3804-3817, 1992.
 */
class TCurrent : public IonicCurrent {
public:
        TCurrent(double area, double gbar = 0.002, double E = 120.,
                 double q10 = 3., double shift = 2., double cao = 2.,
                 double caiInf = 2.4e-4, double taur = 0.005, double depth = 0.1,
                 double temperature = 36., uint id = GetId());
        virtual bool initialise();

protected:
        void evolve();

private:
        double m_phi_h;
};

#define NIC_NOPEN       m_state[1]              // (1)

#define NIC_GAMMA       m_parameters["gamma"]   // (pS)
#define NIC_NCHANNELS   m_parameters["N"]       // (1)

class NoisyIonicCurrent : public IonicCurrent {
public:
        NoisyIonicCurrent(double area, double gbar, double E, double gamma, uint id = GetId());
        virtual bool initialise();
};

#define HH_NA_CN_M      m_state[2]
#define HH_NA_CN_H      m_state[3]

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

#define HH_K_CN_N       m_state[2]

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

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

///// DETERMINISTIC /////
lcg::Entity* HHSodiumFactory(string_dict& args);
lcg::Entity* HHPotassiumFactory(string_dict& args);
lcg::Entity* HH2SodiumFactory(string_dict& args);
lcg::Entity* HH2PotassiumFactory(string_dict& args);
lcg::Entity* MCurrentFactory(string_dict& args);
lcg::Entity* TCurrentFactory(string_dict& args);

///// STOCHASTIC /////
lcg::Entity* HHSodiumCNFactory(string_dict& args);
lcg::Entity* HHPotassiumCNFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif

