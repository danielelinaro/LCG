#ifndef AEC_H
#define AEC_H

#include <stddef.h>

namespace dynclamp {

class AEC {
public:
        AEC(const char *kernelFile = NULL);
        AEC(const double *kernel, size_t kernelSize);
        ~AEC();

        virtual void initialise();

        void pushBack(double I);
        double compensate(double V) const;

        size_t kernelLength() const;
        const double* kernel() const;

private:
        double convolve() const;

private:
        size_t m_length;
        unsigned int m_pos;
        double *m_kernel;
        double *m_current;
};

} // namespace dynclamp

#endif

