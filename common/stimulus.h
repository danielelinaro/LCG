#ifndef STIMULI_H
#define STIMULI_H

#include "types.h"
#include "common.h"

class Stimulus {
public:
        Stimulus(double dt = -1., const char *filename = NULL);
        virtual ~Stimulus();

        const char* stimulusFile() const;
        bool setStimulusFile(const char *filename);
        void setDt(double dt);
        double dt() const;

        const double* data(size_t *length) const;
        const double* metadata(size_t *rows, size_t *cols) const;
        double& operator[](int i);
        double& at(int i);

        size_t length() const;
        double duration() const;

private:
        bool parseStimulusFile();
        void freeMemory();

private:
        char m_filename[FILENAME_MAXLEN];
        double m_dt;
        double *m_stimulus, *m_metadata;
        size_t m_length, m_metadataRows, m_metadataCols;
};

#endif

