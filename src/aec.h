#ifndef AEC_H
#define AEC_H

#include <stddef.h>

namespace dynclamp {

class AEC {
public:
        AEC(const char *kernelFile);
        AEC(const double *kernel, size_t kernelSize);
        ~AEC();

        void pushBack(double I);
        double compensate(double V) const;

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

