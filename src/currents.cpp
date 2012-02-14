#include "currents.h"
#include <math.h>

dynclamp::Entity* HHSodiumFactory(dictionary& args)
{
        uint id;
        double area, gbar, E, dt;
        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);
        if (!dynclamp::CheckAndExtractDouble(args, "area", &area))
                return NULL;
        if (!dynclamp::CheckAndExtractDouble(args, "gbar", &gbar))
                gbar = 0.12;
        if (!dynclamp::CheckAndExtractDouble(args, "E", &E))
                E = 50;
        return new dynclamp::ionic_currents::HHSodium(area, gbar, E, id, dt);
}

dynclamp::Entity* HHPotassiumFactory(dictionary& args)
{
        uint id;
        double area, gbar, E, dt;
        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);
        if (!dynclamp::CheckAndExtractDouble(args, "area", &area))
                return NULL;
        if (!dynclamp::CheckAndExtractDouble(args, "gbar", &gbar))
                gbar = 0.036;
        if (!dynclamp::CheckAndExtractDouble(args, "E", &E))
                E = -77;
        return new dynclamp::ionic_currents::HHPotassium(area, gbar, E, id, dt);
}

namespace dynclamp {

namespace ionic_currents {

IonicCurrent::IonicCurrent(double area, double gbar, double E, uint id, double dt)
        : DynamicalEntity(id, dt), m_neuron(NULL)
{
        m_parameters.push_back(area);   // area -> m_parameters[0]
        m_parameters.push_back(gbar);   // gbar -> m_parameters[1]
        m_parameters.push_back(E);      // E -> m_parameters[2]

        m_state.push_back(0);           // fraction of open channels -> m_state[0]
}

double IonicCurrent::output() const
{
        ////        (1)      * ( nS / cm^2 ) *            mV               * (    cm^2     )
        //return IC_FRACTION * 1e9 * IC_GBAR * (IC_E - m_neuron->output()) * IC_AREA * 1e-8; // (pA)
        return IC_FRACTION * 10 * IC_GBAR * (IC_E - m_neuron->output()) * IC_AREA; // (pA)
}

void IonicCurrent::addPost(Entity *entity)
{
        Logger(Debug, "IonicCurrent::addPost(Entity*)\n");
        Entity::addPost(entity);
        neurons::Neuron *n = dynamic_cast<neurons::Neuron*>(entity);
        if (n != NULL) {
                Logger(Debug, "Connected to a neuron (id #%d).\n", entity->id());
                m_neuron = n;
        }
        else {
                Logger(Debug, "Entity #%d is not a neuron.\n", entity->id());
        }
}
        
//~~

HHSodium::HHSodium(double area, double gbar, double E, uint id, double dt)
        : IonicCurrent(area, gbar, E, id, dt)
{
        m_state.push_back(0);           // m
        m_state.push_back(0);           // h
}

double HHSodium::vtrap(double x, double y) const {
        if (fabs(x/y) < 1e-6)
	        return y*(1. - x/y/2.);
        return x/(exp(x/y) - 1.);
}

double HHSodium::alpham(double v) const {
	return 0.1 * vtrap(-(v+40.),10.);
}

double HHSodium::betam(double v) const {
	return 4. * exp(-(v+65.)/18.);
}

double HHSodium::alphah(double v) const {
	return 0.07 * exp(-(v+65.)/20.);
}

double HHSodium::betah(double v) const {
	return 1.0 / (exp(-(v+35.)/10.) + 1.);
}

void HHSodium::evolve()
{
        double v, dt, am, bm, ah, bh, minf, hinf, taum, tauh;
        v = m_neuron->output();
        dt = GetGlobalDt();
        am = alpham(v);
        bm = betam(v);
        ah = alphah(v);
        bh = betah(v);
        taum = 1.0 / (am + bm);
        tauh = 1.0 / (ah + bh);
        minf = am * taum;
        hinf = ah * tauh;
        HH_NA_M = HH_NA_M + dt * (minf - HH_NA_M) / (taum*1e-3);
        HH_NA_H = HH_NA_H + dt * (hinf - HH_NA_H) / (tauh*1e-3);
        IC_FRACTION = HH_NA_M*HH_NA_M*HH_NA_M*HH_NA_H;
}

//~~

HHPotassium::HHPotassium(double area, double gbar, double E, uint id, double dt)
        : IonicCurrent(area, gbar, E, id, dt)
{
        m_state.push_back(0);           // n
}

double HHPotassium::vtrap(double x, double y) const {
        if (fabs(x/y) < 1e-6)
	        return y*(1. - x/y/2.);
        return x/(exp(x/y) - 1.);
}

double HHPotassium::alphan(double v) const {
	return 0.01*vtrap(-(v+55.),10.);
}

double HHPotassium::betan(double v) const {
	return 0.125*exp(-(v+65.)/80.);
}

void HHPotassium::evolve()
{
        double v, dt, an, bn, ninf, taun;
        v = m_neuron->output();
        dt = GetGlobalDt();
        an = alphan(v);
        bn = betan(v);
        taun = 1.0 / (an + bn);
        ninf = an * taun;
        HH_K_N = HH_K_N + dt * (ninf - HH_K_N) / (taun*1e-3);
        IC_FRACTION = HH_K_N*HH_K_N*HH_K_N*HH_K_N;
}

//~~

NoisyIonicCurrent::NoisyIonicCurrent(double area, double gbar, double E, double gamma, uint id, double dt)
        : IonicCurrent(area, gbar, E, id, dt)
{
        m_parameters.push_back(gamma);  // gamma -> m_parameters[3]
        m_parameters.push_back(ceil(10000 * (IC_AREA*IC_GBAR/NIC_GAMMA)));     // number of channels -> m_parameters[4]

        m_state.push_back(NIC_NCHANNELS * IC_FRACTION); // number of open channels -> m_state[1]
}

} // namespace ionic_currents

} // namespace dynclamp

