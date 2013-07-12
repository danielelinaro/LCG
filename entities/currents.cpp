#include <math.h>
#include "currents.h"
#include "utils.h"

lcg::Entity* HHSodiumFactory(string_dict& args)
{
        uint id;
        double area, gbar, E;
        id = lcg::GetIdFromDictionary(args);
        if (!lcg::CheckAndExtractDouble(args, "area", &area)) {
                lcg::Logger(lcg::Critical, "Unable to build an HH sodium current.\n");
                return NULL;
        }
        if (!lcg::CheckAndExtractDouble(args, "gbar", &gbar))
                gbar = 0.12;
        if (!lcg::CheckAndExtractDouble(args, "E", &E))
                E = 50;
        return new lcg::ionic_currents::HHSodium(area, gbar, E, id);
}

lcg::Entity* HHPotassiumFactory(string_dict& args)
{
        uint id;
        double area, gbar, E;
        id = lcg::GetIdFromDictionary(args);
        if (!lcg::CheckAndExtractDouble(args, "area", &area)) {
                lcg::Logger(lcg::Critical, "Unable to build an HH potassium current.\n");
                return NULL;
        }
        if (!lcg::CheckAndExtractDouble(args, "gbar", &gbar))
                gbar = 0.036;
        if (!lcg::CheckAndExtractDouble(args, "E", &E))
                E = -77;
        return new lcg::ionic_currents::HHPotassium(area, gbar, E, id);
}

lcg::Entity* HH2SodiumFactory(string_dict& args)
{
        uint id;
        double area, gbar, E, vtraub, temperature;
        id = lcg::GetIdFromDictionary(args);
        if (!lcg::CheckAndExtractDouble(args, "area", &area)) {
                lcg::Logger(lcg::Critical, "Unable to build an HH2 sodium current.\n");
                return NULL;
        }
        if (!lcg::CheckAndExtractDouble(args, "gbar", &gbar))
                gbar = 0.003;
        if (!lcg::CheckAndExtractDouble(args, "E", &E))
                E = 50;
        if (!lcg::CheckAndExtractDouble(args, "vtraub", &vtraub))
                vtraub = -63;
        if (!lcg::CheckAndExtractDouble(args, "temperature", &temperature))
                temperature = 36;
        return new lcg::ionic_currents::HH2Sodium(area, gbar, E, vtraub, temperature, id);
}

lcg::Entity* HH2PotassiumFactory(string_dict& args)
{
        uint id;
        double area, gbar, E, vtraub, temperature;
        id = lcg::GetIdFromDictionary(args);
        if (!lcg::CheckAndExtractDouble(args, "area", &area)) {
                lcg::Logger(lcg::Critical, "Unable to build an HH2 potassium current.\n");
                return NULL;
        }
        if (!lcg::CheckAndExtractDouble(args, "gbar", &gbar))
                gbar = 0.005;
        if (!lcg::CheckAndExtractDouble(args, "E", &E))
                E = -90;
        if (!lcg::CheckAndExtractDouble(args, "vtraub", &vtraub))
                vtraub = -63;
        if (!lcg::CheckAndExtractDouble(args, "temperature", &temperature))
                temperature = 36;
        return new lcg::ionic_currents::HH2Potassium(area, gbar, E, vtraub, temperature, id);
}

lcg::Entity* MCurrentFactory(string_dict& args)
{
        uint id;
        double area, gbar, E, taumax, temperature;
        id = lcg::GetIdFromDictionary(args);
        if (!lcg::CheckAndExtractDouble(args, "area", &area)) {
                lcg::Logger(lcg::Critical, "Unable to build an HH2 potassium current.\n");
                return NULL;
        }
        if (!lcg::CheckAndExtractDouble(args, "gbar", &gbar))
                gbar = 0.005;
        if (!lcg::CheckAndExtractDouble(args, "E", &E))
                E = -90;
        if (!lcg::CheckAndExtractDouble(args, "taumax", &taumax))
                taumax = 1000;
        if (!lcg::CheckAndExtractDouble(args, "temperature", &temperature))
                temperature = 36;
        return new lcg::ionic_currents::MCurrent(area, gbar, E, taumax, temperature, id);
}

lcg::Entity* HHSodiumCNFactory(string_dict& args)
{
        uint id;
        double area, gbar, E, gamma;
        ullong seed;
        id = lcg::GetIdFromDictionary(args);
        seed = lcg::GetSeedFromDictionary(args);
        if (!lcg::CheckAndExtractDouble(args, "area", &area)) {
                lcg::Logger(lcg::Critical, "Unable to build an HH noisy sodium current.\n");
                return NULL;
        }
        if (!lcg::CheckAndExtractDouble(args, "gbar", &gbar))
                gbar = 0.12;
        if (!lcg::CheckAndExtractDouble(args, "E", &E))
                E = 50;
        if (!lcg::CheckAndExtractDouble(args, "gamma", &gamma))
                gamma = 10;
        return new lcg::ionic_currents::HHSodiumCN(area, seed, gbar, E, gamma, id);
}

lcg::Entity* HHPotassiumCNFactory(string_dict& args)
{
        uint id;
        double area, gbar, E, gamma;
        ullong seed;
        id = lcg::GetIdFromDictionary(args);
        seed = lcg::GetSeedFromDictionary(args);
        if (!lcg::CheckAndExtractDouble(args, "area", &area)) {
                lcg::Logger(lcg::Critical, "Unable to build an HH noisy potassium current.\n");
                return NULL;
        }
        if (!lcg::CheckAndExtractDouble(args, "gbar", &gbar))
                gbar = 0.036;
        if (!lcg::CheckAndExtractDouble(args, "E", &E))
                E = -77;
        if (!lcg::CheckAndExtractDouble(args, "gamma", &gamma))
                gamma = 10;
        return new lcg::ionic_currents::HHPotassiumCN(area, seed, gbar, E, gamma, id);
}

namespace lcg {

namespace ionic_currents {

double vtrap(double x, double y) {
        if (fabs(x/y) < 1e-6)
	        return y*(1. - x/y/2.);
        return x/(exp(x/y) - 1.);
}

IonicCurrent::IonicCurrent(double area, double gbar, double E, uint id)
        : DynamicalEntity(id), m_neuron(NULL)
{
        IC_AREA = area;
        IC_GBAR = gbar;
        IC_E = E;

        m_state.push_back(0);           // fraction of open channels -> m_state[0]

        Logger(Debug, "Area = %g\n", IC_AREA);

        setName("IonicCurrent");
        setUnits("pA");
}

bool IonicCurrent::initialise()
{
        IC_FRACTION = 0.0;
        Logger(Info, "Area = %g\n", IC_AREA);
        return true;
}

double IonicCurrent::output()
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
        setName("HHSodiumCurrent");
        setUnits("pA");
}

bool HHSodium::initialise()
{
        for (uint i=0; i<m_state.size(); i++)
                m_state[i] = 0.0;
        return true;
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
        setName("HHPotassiumCurrent");
        setUnits("pA");
}

bool HHPotassium::initialise()
{
        for (uint i=0; i<m_state.size(); i++)
                m_state[i] = 0.0;
        return true;
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
        HH_K_N = HH_K_N + ONE_OVER_SIX * (k1+2*k2+2*k3+k4);
        */

        IC_FRACTION = HH_K_N*HH_K_N*HH_K_N*HH_K_N;
}

//~~

HH2Sodium::HH2Sodium(double area, double gbar, double E, double vtraub, double temperature, uint id)
        : IonicCurrent(area, gbar, E, id)
{
        m_state.push_back(0);           // m
        m_state.push_back(0);           // h
        HH2_VTRAUB = vtraub;
        HH2_TEMPERATURE = temperature;
        m_tadj = pow(3., (HH2_TEMPERATURE - 36.) / 10.);
        setName("HH2SodiumCurrent");
        setUnits("pA");
}

bool HH2Sodium::initialise()
{
        for (uint i=0; i<m_state.size(); i++)
                m_state[i] = 0.0;
        return true;
}

double HH2Sodium::alpham(double v) {
	return 0.32 * vtrap(13.-v, 4.);
}

double HH2Sodium::betam(double v) {
	return 0.28 * vtrap(v-40., 5.);
}

double HH2Sodium::alphah(double v) {
	return 0.128 * exp((17.-v)/18.);
}

double HH2Sodium::betah(double v) {
	return 4. / (1. + exp(0.2*(40.-v)));
}

void HH2Sodium::evolve()
{
        double dt, v, am, bm, ah, bh, minf, hinf, taum, tauh;
        dt = GetGlobalDt();
        v = m_neuron->output() - HH2_VTRAUB; // convert to Traub convention
        am = alpham(v);
        bm = betam(v);
        ah = alphah(v);
        bh = betah(v);
        taum = 1.0 / (am + bm);
        tauh = 1.0 / (ah + bh);
        minf = am * taum;
        hinf = ah * tauh;
        taum = taum / m_tadj;
        tauh = tauh / m_tadj;

        // Euler
        HH2_NA_M = HH2_NA_M + dt * (minf - HH2_NA_M) / (taum*1e-3);
        HH2_NA_H = HH2_NA_H + dt * (hinf - HH2_NA_H) / (tauh*1e-3);

        IC_FRACTION = HH2_NA_M*HH2_NA_M*HH2_NA_M*HH2_NA_H;
}

//~~

HH2Potassium::HH2Potassium(double area, double gbar, double E, double vtraub, double temperature, uint id)
        : IonicCurrent(area, gbar, E, id)
{
        m_state.push_back(0);           // n
        HH2_VTRAUB = vtraub;
        HH2_TEMPERATURE = temperature;
        m_tadj = pow(3., (HH2_TEMPERATURE - 36.) / 10.);
        setName("HH2PotassiumCurrent");
        setUnits("pA");
}

bool HH2Potassium::initialise()
{
        for (uint i=0; i<m_state.size(); i++)
                m_state[i] = 0.0;
        return true;
}

double HH2Potassium::alphan(double v) {
	return 0.032 * vtrap(15.-v, 5.);
}

double HH2Potassium::betan(double v) {
	return 0.5 * exp(0.025*(10.-v));
}

void HH2Potassium::evolve()
{
        double dt, v, an, bn, ninf, taun;
        dt = GetGlobalDt();
        v = m_neuron->output() - HH2_VTRAUB;
        an = alphan(v);
        bn = betan(v);
        taun = 1.0 / (an + bn);
        ninf = an * taun;
        taun = taun / m_tadj;
        
        // Euler
        HH2_K_N = HH2_K_N + dt * (ninf - HH2_K_N) / (taun*1e-3);

        IC_FRACTION = HH2_K_N*HH2_K_N*HH2_K_N*HH2_K_N;
}

//~~

MCurrent::MCurrent(double area, double gbar, double E, double taumax, double temperature, uint id)
        : IonicCurrent(area, gbar, E, id)
{
        m_state.push_back(0);           // m
        IM_TAUMAX = taumax;
        IM_TEMPERATURE = temperature;
        m_tadj = pow(2.3, (IM_TEMPERATURE - 36.) / 10.);
        m_tauPeak = IM_TAUMAX / m_tadj;
        setName("MCurrent");
        setUnits("pA");
}

bool MCurrent::initialise()
{
        for (uint i=0; i<m_state.size(); i++)
                m_state[i] = 0.0;
        return true;
}

void MCurrent::evolve()
{
        double dt, v, minf, taum;
        dt = GetGlobalDt();
        v = m_neuron->output();
        taum = m_tauPeak / (3.3 * exp(0.05*(v+35)) + exp(-0.05*(v+35)));
        minf = 1. / (1. + exp(-0.1*(v+35)));

        // Euler
        IM_M = IM_M + dt * (minf - IM_M) / (taum*1e-3);
        IC_FRACTION = IM_M;
}

//~~

TCurrent::TCurrent(double area, double gbar, double E,
                   double q10, double shift, double cao,
                   double caiInf, double taur, double depth,
                   double temperature, uint id)
        : IonicCurrent(area, gbar, E, id)
{
        m_state.push_back(0);           // cai
        m_state.push_back(0);           // h

        IT_Q10 = q10;
        IT_SHIFT = shift;
        IT_CAO = cao;
        IT_CAIINF = caiInf;
        IT_TAUR = taur;
        IT_DEPTH = depth;
        IT_TEMPERATURE = temperature;

        m_phi_h = pow(IT_Q10, (IT_TEMPERATURE - 24.) / 10.);
        setName("TCurrent");
        setUnits("pA");
}

bool TCurrent::initialise()
{
        IT_CAI = IT_CAIINF;
        IT_H = 0.0;
        return true;
}

void TCurrent::evolve()
{
        double dt, v, minf, taum;
        dt = GetGlobalDt();
        v = m_neuron->output();
        /*
        taum = m_tauPeak / (3.3 * exp(0.05*(v+35)) + exp(-0.05*(v+35)));
        minf = 1. / (1. + exp(-0.1*(v+35)));

        // Euler
        IT_H = IT_H + dt * (hinf - IT_H) / (tauh*1e-3);
        IC_FRACTION = m_inf * m_inf * IT_H;
        */
}

//~~

NoisyIonicCurrent::NoisyIonicCurrent(double area, double gbar, double E, double gamma, uint id)
        : IonicCurrent(area, gbar, E, id)
{
        NIC_GAMMA = gamma;
        NIC_NCHANNELS = ceil(10000 * (IC_AREA*IC_GBAR/NIC_GAMMA));
        m_state.push_back(NIC_NCHANNELS * IC_FRACTION); // number of open channels -> m_state[1]
        Logger(Info, "The number of channels is %.0f.\n", NIC_NCHANNELS);
        setName("NoisyIonicCurrent");
        setUnits("pA");
}

bool NoisyIonicCurrent::initialise()
{
        if (! IonicCurrent::initialise())
                return false;
        for (uint i=0; i<m_state.size(); i++)
                m_state[i] = 0.0;
        return true;
}

//~~

HHSodiumCN::HHSodiumCN(double area, ullong seed, double gbar, double E, double gamma, uint id)
        : NoisyIonicCurrent(area, gbar, E, gamma, id),
          m_rand(new NormalRandom(0, 1, seed))
{
        m_parameters["seed"] = seed;
        m_state.push_back(0.0);   // m
        m_state.push_back(0.0);   // h
        for (uint i=0; i<numberOfStates; i++)
                m_z[i] = 0.0;
        setName("HHSodiumWithChannelNoiseCurrent");
        setUnits("pA");
}

HHSodiumCN::~HHSodiumCN()
{
        delete m_rand;
}

bool HHSodiumCN::initialise()
{
        if (! NoisyIonicCurrent::initialise())
                return false;
        for (uint i=0; i<numberOfStates; i++)
                m_z[i] = 0.0;
        return true;
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
	
        Logger(Debug, "Tau:\t");
        for(i=0; i<numberOfStates-1; i++)
                Logger(Debug, " %15.6e", tau_z[i]);
        Logger(Debug, "\nVar:\t");
        for(i=0; i<numberOfStates-1; i++)
                Logger(Debug, " %15.6e", var_z[i]);
        Logger(Debug, "\nMu:\t");
        for(i=0; i<numberOfStates-1; i++)
                Logger(Debug, " %15.6e", mu_z[i]);
        Logger(Debug, "\nNoise:\t");
        for(i=0; i<numberOfStates-1; i++)
                Logger(Debug, " %15.6e", noise_z[i]);
        Logger(Debug, "\n");

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

        Logger(Debug, "%15.6e %15.6e %15.6e\n", HH_NA_CN_M, HH_NA_CN_H, IC_FRACTION);
}

//~~

HHPotassiumCN::HHPotassiumCN(double area, ullong seed, double gbar, double E, double gamma, uint id)
        : NoisyIonicCurrent(area, gbar, E, gamma, id),
          m_rand(new NormalRandom(0, 1, seed))
{
        m_parameters["seed"] = seed;
        m_state.push_back(0.0);   // n
        for (uint i=0; i<numberOfStates; i++)
                m_z[i] = 0.0;
        setName("HHPotassiumWithChannelNoiseCurrent");
        setUnits("pA");
}

HHPotassiumCN::~HHPotassiumCN()
{
        delete m_rand;
}

bool HHPotassiumCN::initialise()
{
        if (! NoisyIonicCurrent::initialise())
                return false;
        for (uint i=0; i<numberOfStates; i++)
                m_z[i] = 0.0;
        return true;
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

} // namespace lcg

