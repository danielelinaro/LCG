#include <sys/stat.h>
#include <errno.h>

#include <vector>

#include "stimulus.h"
#include "generate_trial.h"
#include "common.h"
#include "utils.h"
using namespace lcg;

Stimulus::Stimulus(double dt, const char *filename) :
        m_dt(dt), m_stimulus(NULL), m_metadata(NULL), m_length(0), m_metadataRows(0), m_metadataCols(0)
{
        if (filename != NULL) {
                if (!setStimulusFile(filename))
                        throw "Must provide a valid stimulus file.";
        }
}

Stimulus::~Stimulus()
{
        freeMemory();
}

void Stimulus::freeMemory()
{
        if (m_stimulus != NULL) {
                delete m_stimulus;
                delete m_metadata;
                m_stimulus = NULL;
                m_metadata = NULL;
        }
}

bool Stimulus::setStimulusFile(const char *filename)
{
        struct stat buf;
        if (strlen(filename)==0 || stat(filename,&buf)!=0) {
                Logger(Critical, "%s: no such file.\n", filename);
                return false;
        }
        strncpy(m_filename, filename, FILENAME_MAXLEN);
        if (!parseStimulusFile()) {
                Logger(Critical, "Error while parsing the stimulus file.");
                return false;
        }
        return true;
}

const char* Stimulus::stimulusFile() const
{
        return m_filename;
}

bool Stimulus::parseStimulusFile()
{
        bool retval = true;
        int i, j, err;
        uint length;
        double **metadata;

        freeMemory();

        err = generate_trial(m_filename, GetLoggingLevel() <= Debug,
                              0, NULL, &m_stimulus, &length,
                              1.0/m_dt, m_dt);
        m_length = length;
        Logger(Debug,"Passed %lf - %lf to generate_trial.\n",1.0/m_dt,m_dt);
        if (err) {
                if (m_stimulus)
                        free(m_stimulus);
                Logger(Critical, "Error in <generate_trial>\n");
                return false;
        }
        Logger(Debug,"The number of points in the stimulus is: %d, which will last for: %lf (s).\n",
                        m_length,m_length*m_dt);
        metadata = new double*[MAXROWS];
        if (!metadata) {
                Logger(Critical, "Unable to allocate memory for <parsed_data>.\n");
                return false;
        }
        for (i=0; i<MAXROWS; i++)  { 
                metadata[i] = new double[MAXCOLS];
                if (metadata[i] == NULL) {
                        Logger(Critical, "Unable to allocate memory for <parsed_data>.\n");
                        for (j=0; j<i; i++)
                                delete metadata[j];
                        delete metadata;
                        return false;
                }
        }
        
        if (readmatrix(m_filename, metadata, &m_metadataRows, &m_metadataCols) != 0) {
                Logger(Critical, "Unable to parse file [%s].\n", m_filename);
                retval = false;
                free(m_stimulus);
                goto endParse;
        }

        m_metadata = new double[m_metadataRows*m_metadataCols];
        for (i=0; i<m_metadataRows; i++) {
                for (j=0; j<m_metadataCols; j++) {
                        m_metadata[i*m_metadataCols + j] = metadata[i][j];
                        Logger(Debug, "%7.1lf ", m_metadata[i*m_metadataCols + j]);
                }
                Logger(Debug, "\n");
        }

endParse:
        for (i=0; i<MAXROWS; i++)
                delete metadata[i];
        delete metadata;

        return retval;
}

double Stimulus::duration() const
{
        return length() * dt();
}

double Stimulus::dt() const
{
        return m_dt;
}

void Stimulus::setDt(double dt)
{
        if (dt <= 0.)
                throw "Timestep must be positive";
        m_dt = dt;
}

size_t Stimulus::length() const
{
        return m_length;
}

const double* Stimulus::data(size_t *length) const
{
        *length = m_length;
        return m_stimulus;
}

const double* Stimulus::metadata(size_t *rows, size_t *cols) const
{
        *rows = m_metadataRows;
        *cols = m_metadataCols;
        return m_metadata;
}

double& Stimulus::operator[](int i)
{
        if (i<0 || i>=length())
                throw "Index out of bounds";
        return m_stimulus[i];
}

double& Stimulus::at(int i)
{
        if (i<0 || i>=length())
                throw "Index out of bounds";
        return m_stimulus[i];
}

