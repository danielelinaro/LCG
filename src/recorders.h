#ifndef RECORDERS_H
#define RECORDERS_H

#include <stdio.h>
#include <boost/thread.hpp>

#include "hdf5.h"

#include "utils.h"
#include "entity.h"

namespace dynclamp {

namespace recorders {

class Recorder : public Entity {
public:
        Recorder(uint id = GetId(), double dt = GetGlobalDt());
        virtual double output() const;
};

class ASCIIRecorder : public Recorder {
public:
        ASCIIRecorder(const char *filename, uint id = GetId(), double dt = GetGlobalDt());
        ASCIIRecorder(FILE *fid, uint id = GetId(), double dt = GetGlobalDt());
        ~ASCIIRecorder();

        virtual void step();
private:
        FILE *m_fid;
        bool m_closeFile;
};


#define H5_CREATE_ERROR  1
#define H5_DATASET_ERROR 2
#define H5_SHUFFLE_ERROR 3
#define H5_DEFLATE_ERROR 4

#define H5_NO_GZIP_COMPRESSION 5
#define H5_NO_FILTER_INFO      6
#define H5_NO_SHUFFLE_FILTER   7

class H5Recorder : public Recorder {
public:
        H5Recorder(const char *filename, bool compress, uint id = GetId(), double dt = GetGlobalDt());
        ~H5Recorder();

        virtual void step();

public:
        static const hsize_t rank;
        static const hsize_t maxSize;
        static const hsize_t chunkSize;
        static const uint    numberOfChunks; 
        static const hsize_t bufferSize;
        static const double  fillValue;

protected:
        virtual void addPre(Entity *entity, double input);

private:
        int isCompressionAvailable() const;
        int open(const char *filename, bool compress);
        void close();
        void buffersWriter();

private:
        // the handle of the file
        hid_t m_fid;
        // whether compression is turned on or off
        bool m_compressed;
        // dataset creation property list
        hid_t m_datasetPropertiesList;
        // the data
        std::vector<double**> m_data;
        // number of inputs
        uint m_numberOfInputs;

        // position in the buffer
        uint m_bufferPosition;
        hsize_t m_bufferLength;
        uint m_bufferInUse;
        uint m_bufferToSave;
        boost::thread m_writerThread;
        boost::mutex m_mutex;
        boost::condition_variable m_cv;
        bool m_dataReady;
        bool m_threadRun;

        std::vector<hid_t> m_dataspaces;
        std::vector<hid_t> m_datasets;
        hsize_t m_offset;
        hsize_t m_datasetSize;

};

} // namespace recorders

} // namespace dynclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* ASCIIRecorderFactory(dictionary& args);
dynclamp::Entity* H5RecorderFactory(dictionary& args);
	
#ifdef __cplusplus
}
#endif


#endif

