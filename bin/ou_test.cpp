#include <iostream>
#include <cstdlib>
#include <ctime>
#include "ou.h"

int main()
{
        double t;
        double dt = 1e-4;
        double tend = 100;
        double sigma = 5;
        double tau = 10e-3;
        double eta0 = 0;
        dynclamp::OU ou(dt, sigma, tau, eta0);

        for (t=0.; t<=tend; t+=dt) {
                std::cout << t << " " << ou.getOutput() << std::endl;
                //std::cout << ou.getOutput() << std::endl;
                ou.step();
        }

        return 0;
}

