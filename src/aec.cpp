#include <stdio.h>
#include "utils.h"
#include "aec.h"

namespace dynclamp {

AEC::AEC(const char *kernelFile)
{
        if (kernelFile == NULL) {
                m_length = 1;
                m_kernel = new double[m_length];
                m_current = new double[m_length];
                m_kernel[0] = 0.0;
                m_current[0] = 0.0;
        }
        else {
                FILE *fid;
                double tmp;
        
                fid = fopen(kernelFile, "r");
                if (fid == NULL) {
                        Logger(Critical, "%s: no such file.\n", kernelFile);
                        throw "Unable to open kernel file.";
                }
        
                m_length = 0;
                while (fscanf(fid, "%le\n", &tmp) != EOF)
                        m_length++;
                fclose(fid);
        
                fid = fopen(kernelFile, "r");
                m_kernel = new double[m_length];
                m_current = new double[m_length];
                for (int i=0; i<m_length; i++) {
                        fscanf(fid, "%le\n", &m_kernel[i]);
                        m_current[i] = 0.0;
                }
                fclose(fid);
        }
        Logger(Debug, "The kernel has %d samples.\n", m_length);
}

AEC::AEC(const double *kernel, size_t kernelSize)
        : m_length(kernelSize)
{
        m_kernel = new double[m_length];
        m_current = new double[m_length];
        for (int i=0; i<m_length; i++) {
                m_kernel[i] = kernel[i];
                m_current[i] = 0.0;
        }
}

AEC::~AEC()
{
        delete m_kernel;
        delete m_current;
}

bool AEC::initialise()
{
        for (int i=0; i<m_length; i++) {
                m_current[i] = 0.0;
        }
        m_pos = 0;
        return true;
}

void AEC::pushBack(double I)
{
        m_current[m_pos] = I*1e-12;
        m_pos = (m_pos+1) % m_length;
}

double AEC::compensate(double V) const
{
        return V - 1e3*convolve();
}

double AEC::convolve() const
{
        int i, j;
        double U = 0.0;
        for (i=m_pos-1, j=0; i>=0; i--, j++)
                U += m_current[i] * m_kernel[j];
        for (i=m_length-1; j<m_length; i--, j++)
                U += m_current[i] * m_kernel[j];
        return U;
}

const double* AEC::kernel() const
{
        return m_kernel;
}

size_t AEC::kernelLength() const
{
        return m_length;
}

} // namespace dynclamp

