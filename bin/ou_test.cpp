#include <iostream>
#include <cstdlib>
#include <ctime>
#include "utils.h"
#include "ou.h"

using namespace dynclamp;

int main()
{
        double t;
        double tend = 100;
        double sigma = 50;
        double tau = 10e-3;
        double eta0 = 250;
        OU ou(sigma, tau, eta0);

        SetLoggingLevel(Critical);

        for (t=0.; t<=tend; t+=GetGlobalDt()) {
                ou.readAndStoreInputs();
                std::cout << t << " " << ou.output() << std::endl;
                ou.step();
        }

        return 0;
}

