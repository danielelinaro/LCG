#ifndef RECORDERS_H
#define RECORDERS_H

#include <stdio.h>

#include <vector>
#include <queue>

#include <boost/thread.hpp>

#include "hdf5.h"

#include "utils.h"
#include "entity.h"

namespace dynclamp {

namespace recorders {

class Recorder : public Entity {
public:
        Recorder(uint id = GetId());
        virtual double output();
};

class ASCIIRecorder : public Recorder {
public:
        ASCIIRecorder(const char *filename = NULL, uint id = GetId());
        ~ASCIIRecorder();
        virtual bool initialise();
        virtual void step();
        virtual void terminate();

private:
        void openFile();
        void closeFile();

private:
        bool m_makeFilename;
        char m_filename[FILENAME_MAXLEN];
        FILE *m_fid;
        bool m_closeFile;
};


#define GROUP_NAME_LEN   128
#define DATASET_NAME_LEN 128
#define ENTITIES_GROUP   "/Entities"
#define INFO_GROUP       "/Info"
#define DATA_DATASET     "Data"
#define METADATA_DATASET "Metadata"
#define PARAMETERS_GROUP "Parameters"
#define H5_FILE_VERSION  2

class BaseH5Recorder : public Recorder {
public:
        BaseH5Recorder(bool compress, hsize_t bufferSize = 20480, const char *filename = NULL, uint id = GetId());
        BaseH5Recorder(bool compress, hsize_t chunkSize, uint numberOfChunks, const char *filename = NULL, uint id = GetId());
        virtual ~BaseH5Recorder();
        virtual bool initialise();
        virtual void terminate();

        hsize_t bufferSize() const;
        hsize_t chunkSize() const;
        uint numberOfChunks() const;

public:
        static const hsize_t rank;
        static const hsize_t unlimitedSize;
        static const double  fillValue;

protected:
        bool isCompressionAvailable() const;

        virtual bool openFile();
        virtual void closeFile();

        virtual bool finaliseInit() = 0;
        
        virtual void addPre(Entity *entity);
        virtual void finaliseAddPre(Entity *entity) = 0;

        virtual bool allocateForEntity(Entity *entity);

        virtual bool createGroup(const std::string& groupName, hid_t *grp);
        virtual bool createUnlimitedDataset(const std::string& datasetName, hid_t *dspace, hid_t *dset);

        virtual bool writeStringAttribute(hid_t objId, const std::string& attrName, const std::string& attrValue);
        virtual bool writeScalarAttribute(hid_t objId, const std::string& attrName, double attrValue);
        virtual bool writeArrayAttribute(hid_t objId, const std::string& attrName,
                                         const double *data, const hsize_t *dims, int ndims);
        virtual bool writeData(const std::string& datasetName, int rank, const hsize_t *dims,
                               const double *data, const std::string& label = "");

protected:
        // the handle of the file
        hid_t m_fid;
        // whether compression is turned on or off
        bool m_compress;
        // the name of the file
        char m_filename[FILENAME_MAXLEN];
        // tells whether the filename should be generated from the timestamp
        bool m_makeFilename;
         
        // number of inputs
        uint m_numberOfInputs;

        // H5 stuff
        hid_t m_infoGroup;
        std::vector<hid_t> m_groups;
        std::vector<hid_t> m_dataspaces;
        std::vector<hid_t> m_datasets;

private:
        // the size of each chunk of data that is saved to the H5 file
        hsize_t m_chunkSize;
        // the number of chunks of data
        uint m_numberOfChunks; 
        // the total size of the buffer, given by the size of each chunk times the number of chunks
        hsize_t m_bufferSize;
};

class H5Recorder : public BaseH5Recorder {
public:
        H5Recorder(bool compress, const char *filename = NULL, uint id = GetId());
        ~H5Recorder();
        virtual void step();
        virtual void terminate();

public:
        /*!
         * The number of buffers used for storing the input data: they need to be at least 2,
         * so that the realtime thread writes in one, while the thread created by H5Recorder
         * saves the data in the other buffer to file.
         */
        static const uint    numberOfBuffers;
        static const hsize_t chunkSize;
        static const uint    numberOfChunks;

protected:
        virtual bool finaliseInit();
        virtual void finaliseAddPre(Entity *entity);
        virtual void closeFile();

private:
        void startWriterThread();
        void stopWriterThread();
        void buffersWriter();

private:
        // the data
        std::vector<double**> m_data;
        // the queue of the indices of the buffers to save
        std::deque<uint> m_dataQueue;

        // position in the buffer
        uint m_bufferPosition;
        // the length of each buffer
        hsize_t *m_bufferLengths;
        // the thread that continuosly waits for data to save
        boost::thread m_writerThread;
        // the index of the buffer in which the main thread saves data
        uint m_bufferInUse;
        // multithreading stuff for controlling access to the queue m_dataQueue;
        boost::mutex m_mutex;
        boost::condition_variable m_cv;
        bool m_threadRun;

        // H5 stuff
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

dynclamp::Entity* ASCIIRecorderFactory(string_dict& args);
dynclamp::Entity* H5RecorderFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif


#endif

