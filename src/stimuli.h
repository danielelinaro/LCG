#ifndef STIMULI_H
#define STIMULI_H

#include "types.h"
#include "common.h"

class Stimulus {
public:
        Stimulus(const char *filename, double dt);
        virtual ~Stimulus();

        const char* filename() const;
        double dt() const;

        const double* data(size_t *length) const;
        const double* metadata(size_t *rows, size_t *cols) const;
        double operator[](int i) const;

        size_t length() const;
        double duration() const;

private:
        bool parse(const char *filename);

protected:
        char m_filename[FILENAME_MAXLEN];
        double m_dt;
        double *m_stimulus, *m_metadata;
        size_t m_length, m_metadataRows, m_metadataCols;
};

int allocate_stimuli(const std::vector<std::string>& filenames, double dt);
void free_stimuli();

#endif

