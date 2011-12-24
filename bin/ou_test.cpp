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
        double sigma = 5;
        double tau = 10e-3;
        double eta0 = 0;
        OU ou(sigma, tau, eta0);

        for (t=0.; t<=tend; t+=GetGlobalDt()) {
                std::cout << t << " " << ou.getOutput() << std::endl;
                ou.step();
        }

        return 0;
}

