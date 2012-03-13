#include "currents.h"
#include <math.h>

dynclamp::Entity* HHSodiumFactory(dictionary& args)
{
        uint id;
        double area, gbar, E;
        id = dynclamp::GetIdFromDictionary(args);
        if (!dynclamp::CheckAndExtractDouble(args, "area", &area))
                return NULL;
        if (!dynclamp::CheckAndExtractDouble(args, "gbar", &gbar))
                gbar = 0.12;
        if (!dynclamp::CheckAndExtractDouble(args, "E", &E))
                E = 50;
        return new dynclamp::ionic_currents::HHSodium(area, gbar, E, id);
}

dynclamp::Entity* HHPotassiumFactory(dictionary& args)
{
        uint id;
        double area, gbar, E;
        id = dynclamp::GetIdFromDictionary(args);
        if (!dynclamp::CheckAndExtractDouble(args, "area", &area))
                return NULL;
        if (!dynclamp::CheckAndExtractDouble(args, "gbar", &gbar))
                gbar = 0.036;
        if (!dynclamp::CheckAndExtractDouble(args, "E", &E))
                E = -77;
        return new dynclamp::ionic_currents::HHPotassium(area, gbar, E, id);
}

dynclamp::Entity* HHSodiumCNFactory(dictionary& args)
{
        uint id;
        double area, gbar, E, gamma;
        ullong seed;
        id = dynclamp::GetIdFromDictionary(args);
        dynclamp::GetSeedFromDictionary(args, &seed);
        if (!dynclamp::CheckAndExtractDouble(args, "area", &area))
                return NULL;
        if (!dynclamp::CheckAndExtractDouble(args, "gbar", &gbar))
                gbar = 0.12;
        if (!dynclamp::CheckAndExtractDouble(args, "E", &E))
                E = 50;
        if (!dynclamp::CheckAndExtractDouble(args, "gamma", &gamma))
                gamma = 10;
        return new dynclamp::ionic_currents::HHSodiumCN(area, seed, gbar, E, gamma, id);
}

dynclamp::Entity* HHPotassiumCNFactory(dictionary& args)
{
        uint id;
        double area, gbar, E, gamma;
        ullong seed;
        id = dynclamp::GetIdFromDictionary(args);
        dynclamp::GetSeedFromDictionary(args, &seed);
        if (!dynclamp::CheckAndExtractDouble(args, "area", &area))
                return NULL;
        if (!dynclamp::CheckAndExtractDouble(args, "gbar", &gbar))
                gbar = 0.036;
        if (!dynclamp::CheckAndExtractDouble(args, "E", &E))
                E = -77;
        if (!dynclamp::CheckAndExtractDouble(args, "gamma", &gamma))
                gamma = 10;
        return new dynclamp::ionic_currents::HHPotassiumCN(area, seed, gbar, E, gamma, id);
}

namespace dynclamp {

namespace ionic_currents {

IonicCurrent::IonicCurrent(double area, double gbar, double E, uint id)
        : DynamicalEntity(id), m_neuron(NULL)
{
        m_parameters.push_back(area);   // area -> m_parameters[0]
        m_parameters.push_back(gbar);   // gbar -> m_parameters[1]
        m_parameters.push_back(E);      // E -> m_parameters[2]

        m_state.push_back(0);           // fraction of open channels -> m_state[0]
}

void IonicCurrent::initialise()
{
        IC_FRACTION = 0.0;
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

HHSodium::HHSodium(double area, double gbar, double E, uint id)
        : IonicCurrent(area, gbar, E, id)
{
        m_state.push_back(0);           // m
        m_state.push_back(0);           // h
}

void HHSodium::initialise()
{
        for (uint i=0; i<m_state.size(); i++)
                m_state[i] = 0.0;
}

double HHSodium::vtrap(double x, double y) {
        if (fabs(x/y) < 1e-6)
	        return y*(1. - x/y/2.);
        return x/(exp(x/y) - 1.);
}

double HHSodium::alpham(double v) {
	return 0.1 * vtrap(-(v+40.),10.);
}

double HHSodium::betam(double v) {
	return 4. * exp(-(v+65.)/18.);
}

double HHSodium::alphah(double v) {
	return 0.07 * exp(-(v+65.)/20.);
}

double HHSodium::betah(double v) {
	return 1.0 / (exp(-(v+35.)/10.) + 1.);
}

void HHSodium::evolve()
{
        double dt, v, am, bm, ah, bh, minf, hinf, taum, tauh;
        dt = GetGlobalDt();
        v = m_neuron->output();
        am = alpham(v);
        bm = betam(v);
        ah = alphah(v);
        bh = betah(v);
        taum = 1.0 / (am + bm);
        tauh = 1.0 / (ah + bh);
        minf = am * taum;
        hinf = ah * tauh;

        // Euler
        HH_NA_M = HH_NA_M + dt * (minf - HH_NA_M) / (taum*1e-3);
        HH_NA_H = HH_NA_H + dt * (hinf - HH_NA_H) / (tauh*1e-3);

        // Runge-Kutta 4
        /*
        double k1, k2, k3, k4;
        k1 = dt * (minf-HH_NA_M)/(taum*1e-3);
        k2 = dt * (minf-(HH_NA_M+0.5*k1))/(taum*1e-3);
        k3 = dt * (minf-(HH_NA_M+0.5*k2))/(taum*1e-3);
        k4 = dt * (minf-(HH_NA_M+k3))/(taum*1e-3);
        HH_NA_M = HH_NA_M + 0.1666666667 * (k1+2*k2+2*k3+k4);
        k1 = dt * (hinf-HH_NA_H)/(tauh*1e-3);
        k2 = dt * (hinf-(HH_NA_H+0.5*k1))/(tauh*1e-3);
        k3 = dt * (hinf-(HH_NA_H+0.5*k2))/(tauh*1e-3);
        k4 = dt * (hinf-(HH_NA_H+k3))/(tauh*1e-3);
        HH_NA_H = HH_NA_H + 0.1666666667 * (k1+2*k2+2*k3+k4);
        */

        IC_FRACTION = HH_NA_M*HH_NA_M*HH_NA_M*HH_NA_H;
}

//~~

HHPotassium::HHPotassium(double area, double gbar, double E, uint id)
        : IonicCurrent(area, gbar, E, id)
{
        m_state.push_back(0);           // n
}

void HHPotassium::initialise()
{
        for (uint i=0; i<m_state.size(); i++)
                m_state[i] = 0.0;
}

double HHPotassium::vtrap(double x, double y) {
        if (fabs(x/y) < 1e-6)
	        return y*(1. - x/y/2.);
        return x/(exp(x/y) - 1.);
}

double HHPotassium::alphan(double v) {
	return 0.01*vtrap(-(v+55.),10.);
}

double HHPotassium::betan(double v) {
	return 0.125*exp(-(v+65.)/80.);
}

void HHPotassium::evolve()
{
        double dt, v, an, bn, ninf, taun;
        dt = GetGlobalDt();
        v = m_neuron->output();
        an = alphan(v);
        bn = betan(v);
        taun = 1.0 / (an + bn);
        ninf = an * taun;
        
        // Euler
        HH_K_N = HH_K_N + dt * (ninf - HH_K_N) / (taun*1e-3);

        // Runge-Kutta 4
        /*
        double k1, k2, k3, k4;
        k1 = dt * (ninf-HH_K_N)/(taun*1e-3);
        k2 = dt * (ninf-(HH_K_N+0.5*k1))/(taun*1e-3);
        k3 = dt * (ninf-(HH_K_N+0.5*k2))/(taun*1e-3);
        k4 = dt * (ninf-(HH_K_N+k3))/(taun*1e-3);
        HH_K_N = HH_K_N + 0.1666666667 * (k1+2*k2+2*k3+k4);
        */

        IC_FRACTION = HH_K_N*HH_K_N*HH_K_N*HH_K_N;
}

//~~

NoisyIonicCurrent::NoisyIonicCurrent(double area, double gbar, double E, double gamma, uint id)
        : IonicCurrent(area, gbar, E, id)
{
        m_parameters.push_back(gamma);  // gamma -> m_parameters[3]
        m_parameters.push_back(ceil(10000 * (IC_AREA*IC_GBAR/NIC_GAMMA)));     // number of channels -> m_parameters[4]
        m_state.push_back(NIC_NCHANNELS * IC_FRACTION); // number of open channels -> m_state[1]
        Logger(Info, "The number of channels is %.0f.\n", NIC_NCHANNELS);
}

void NoisyIonicCurrent::initialise()
{
        for (uint i=0; i<m_state.size(); i++)
                m_state[i] = 0.0;
}

//~~

HHSodiumCN::HHSodiumCN(double area, ullong seed, double gbar, double E, double gamma, uint id)
        : NoisyIonicCurrent(area, gbar, E, gamma, id),
          m_rand(new NormalRandom(0, 1, seed))
{
        m_state.push_back(0.0);   // m
        m_state.push_back(0.0);   // h
        for (uint i=0; i<numberOfStates; i++)
                m_z[i] = 0.0;
}

HHSodiumCN::~HHSodiumCN()
{
        delete m_rand;
}

void HHSodiumCN::initialise()
{
        for (uint i=0; i<m_state.size(); i++)
                m_state[i] = 0.0;
        for (uint i=0; i<numberOfStates; i++)
                m_z[i] = 0.0;
}

void HHSodiumCN::evolve()
{
        double dt, v, am, bm, ah, bh, m_inf, h_inf, tau_m, tau_h;
        double m3_inf, one_minus_m, one_minus_h, fraction;
        double tau_z[numberOfStates-1], var_z[numberOfStates-1], mu_z[numberOfStates-1], noise_z[numberOfStates-1];
	uint i;
	
        dt = GetGlobalDt();
        v = m_neuron->output();

	// m
	am = HHSodium::alpham(v);
	bm = HHSodium::betam(v);
	tau_m = 1.0/(am+bm);
	m_inf = am*tau_m;
        tau_m *= 1e-3;
	m3_inf = m_inf*m_inf*m_inf;

	// h
	ah = HHSodium::alphah(v);
	bh = HHSodium::betah(v);
	tau_h = 1.0/(ah+bh);
	h_inf = ah*tau_h;
	tau_h *= 1e-3;

	// auxiliary variables
	one_minus_m = 1.-m_inf;
	one_minus_h = 1.-h_inf;
	// tau's
	tau_z[0] = tau_h;
	tau_z[1] = tau_m;
	tau_z[2] = tau_m/2.;
	tau_z[3] = tau_m/3.;
	tau_z[4] = tau_m*tau_h/(tau_m+tau_h);
	tau_z[5] = tau_m*tau_h/(tau_m+2*tau_h);
	tau_z[6] = tau_m*tau_h/(tau_m+3*tau_h);
	// variances
	var_z[0] = (1.0/NIC_NCHANNELS) * m3_inf*m3_inf*h_inf * one_minus_h;
	var_z[1] = (1.0/NIC_NCHANNELS) * 3*m3_inf*m_inf*m_inf*h_inf*h_inf * one_minus_m;
	var_z[2] = (1.0/NIC_NCHANNELS) * 3*m3_inf*m_inf*h_inf*h_inf * one_minus_m*one_minus_m;
	var_z[3] = (1.0/NIC_NCHANNELS) * m3_inf*h_inf*h_inf * one_minus_m*one_minus_m*one_minus_m;
	var_z[4] = (1.0/NIC_NCHANNELS) * 3*m3_inf*m_inf*m_inf*h_inf * one_minus_m*one_minus_h;
	var_z[5] = (1.0/NIC_NCHANNELS) * 3*m3_inf*m_inf*h_inf * one_minus_m*one_minus_m*one_minus_h;
	var_z[6] = (1.0/NIC_NCHANNELS) * m3_inf*h_inf * one_minus_m*one_minus_m*one_minus_m*one_minus_h;	

	for(i=0; i<numberOfStates-1; i++) {
		mu_z[i] = exp(-dt/tau_z[i]);
		noise_z[i] = sqrt(var_z[i]*(1-mu_z[i]*mu_z[i])) * m_rand->random();
	}
	
	/* forward Euler for m and h (they are deterministic) */
	HH_NA_CN_M = HH_NA_CN_M + dt * (m_inf - HH_NA_CN_M) / tau_m;
	HH_NA_CN_H = HH_NA_CN_H + dt * (h_inf - HH_NA_CN_H) / tau_h;
	
	IC_FRACTION = HH_NA_CN_M*HH_NA_CN_M*HH_NA_CN_M*HH_NA_CN_H;
	for(i=0; i<numberOfStates-1; i++) {
		m_z[i] = m_z[i]*mu_z[i] + noise_z[i];
		IC_FRACTION += m_z[i];
	}

	if(IC_FRACTION < 0)
		IC_FRACTION = 0;
	else if(IC_FRACTION > 1)
		IC_FRACTION = 1;
}

//~~

HHPotassiumCN::HHPotassiumCN(double area, ullong seed, double gbar, double E, double gamma, uint id)
        : NoisyIonicCurrent(area, gbar, E, gamma, id),
          m_rand(new NormalRandom(0, 1, seed))
{
        m_state.push_back(0.0);   // n
        for (uint i=0; i<numberOfStates; i++)
                m_z[i] = 0.0;
}

HHPotassiumCN::~HHPotassiumCN()
{
        delete m_rand;
}

void HHPotassiumCN::initialise()
{
        for (uint i=0; i<m_state.size(); i++)
                m_state[i] = 0.0;
        for (uint i=0; i<numberOfStates; i++)
                m_z[i] = 0.0;
}

void HHPotassiumCN::evolve()
{
        double dt, v, an, bn, n_inf, tau_n;
        double n4_inf, one_minus_n, fraction;
        double tau_z[numberOfStates-1], var_z[numberOfStates-1], mu_z[numberOfStates-1], noise_z[numberOfStates-1];
	int i;
	
        dt = GetGlobalDt();
        v = m_neuron->output();

	an = HHPotassium::alphan(v);
	bn = HHPotassium::betan(v);
	tau_n = 1.0/(an+bn);
	n_inf = an*tau_n;
        tau_n *= 1e-3;
	n4_inf = n_inf*n_inf*n_inf*n_inf;
	one_minus_n = 1-n_inf;
	var_z[0] = (1.0/NIC_NCHANNELS) * 4*n4_inf*n_inf*n_inf*n_inf * one_minus_n;
	var_z[1] = (1.0/NIC_NCHANNELS) * 6*n4_inf*n_inf*n_inf * one_minus_n*one_minus_n;
	var_z[2] = (1.0/NIC_NCHANNELS) * 4*n4_inf*n_inf * one_minus_n*one_minus_n*one_minus_n;
	var_z[3] = (1.0/NIC_NCHANNELS) * n4_inf * one_minus_n*one_minus_n*one_minus_n*one_minus_n;
	
	for(i=0; i<numberOfStates-1; i++) {
		tau_z[i] = tau_n/(i+1);
		mu_z[i] = exp(-dt/tau_z[i]);
		noise_z[i] = sqrt(var_z[i]*(1-mu_z[i]*mu_z[i])) * m_rand->random();
	}

	/* forward Euler for n (it is deterministic) */
        HH_K_CN_N = HH_K_CN_N + dt * (n_inf - HH_K_CN_N)/tau_n;

	/* noisy terms */
	IC_FRACTION = HH_K_CN_N*HH_K_CN_N*HH_K_CN_N*HH_K_CN_N;
	for(i=0; i<numberOfStates-1; i++) {
		m_z[i] = m_z[i]*mu_z[i] + noise_z[i];
		IC_FRACTION += m_z[i];
	}

	if(IC_FRACTION < 0)
		IC_FRACTION = 0;
	else if(IC_FRACTION > 1)
		IC_FRACTION = 1;
        
}

} // namespace ionic_currents

} // namespace dynclamp

