#ifndef AEC_H
#define AEC_H

#include <stddef.h>

namespace dynclamp {

class AEC {
public:
        AEC(const char *kernelFile = NULL);
        AEC(const double *kernel, size_t kernelSize);
        ~AEC();

        virtual bool initialise(double I = 0, double V = 0);

        bool hasKernel() const;

        void pushBack(double I);
        double compensate(double V);

        size_t kernelLength() const;
        const double* kernel() const;

private:
        double convolve();

private:
        size_t m_length;
        unsigned int m_pos;
        double *m_kernel;
        double *m_current;
        double m_buffer[2];
        bool m_withKernel;
};

} // namespace dynclamp

#endif

