#ifndef RECORDERS_H
#define RECORDERS_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <hdf5.h>
#include <vector>
#include <queue>

#include "utils.h"
#include "entity.h"
#include "common.h"

namespace lcg {

namespace recorders {

class Comment {
public:
        Comment(const char *msg, const time_t *tstamp = NULL) {
                struct tm* now;
                if (tstamp) {
                        now = localtime(tstamp);
                }
                else {
                        time_t ts = time(NULL);
                        now = localtime(&ts);
                }
                sprintf(m_msg, "%d-%02d-%02d %02d:%02d:%02d >>> ", now->tm_year+1900, now->tm_mon+1, now->tm_mday,
                                now->tm_hour, now->tm_min, now->tm_sec);
                strncpy(m_msg+24, msg, COMMENT_MAXLEN-24);
        }
        const char *message() const {
                return m_msg;
        }
private:
        char m_msg[COMMENT_MAXLEN];
};

class Recorder : public Entity {
public:
        Recorder(uint id = GetId());
        virtual ~Recorder();
        virtual double output();
        void addComment(const char *message, const time_t *timestamp = NULL);
protected:
        virtual void deleteComments();
protected:
        std::deque<Comment*> m_comments;
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
#define COMMENTS_GROUP   "/Comments"
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

        const char* filename() const;

public:
        static const hsize_t unlimitedSize;
        static const double  fillValue;

protected:
        bool isCompressionAvailable() const;

        virtual bool openFile();
        virtual void closeFile();

        virtual bool finaliseInit() = 0;
        
        virtual void addPre(Entity *entity);
        virtual void finaliseAddPre(Entity *entity) = 0;

        virtual bool allocateForEntity(Entity *entity, int dataRank,
                                       const hsize_t *dataDims, const hsize_t *maxDataDims, const hsize_t *chunkDims);

        virtual bool createGroup(const std::string& groupName, hid_t *grp);
        virtual bool createUnlimitedDataset(const std::string& datasetName,
                                            int rank, const hsize_t *dataDims, const hsize_t *maxDataDims, const hsize_t *chunkDims,
                                            hid_t *dspace, hid_t *dset);

        virtual bool writeStringAttribute(hid_t objId, const std::string& attrName, const std::string& attrValue);
        virtual bool writeScalarAttribute(hid_t objId, const std::string& attrName, double attrValue);
        virtual bool writeArrayAttribute(hid_t objId, const std::string& attrName,
                                         const double *data, const hsize_t *dims, int ndims);
        virtual bool writeData(const std::string& datasetName, int rank, const hsize_t *dims,
                               const double *data, const std::string& label = "");

        virtual void writeComments();

#if defined(HAVE_LIBRT)
        // sets the priority of the calling thread to max_priority - 1
        virtual void reducePriority() const;
#endif

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
        hid_t m_commentsGroup;
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
        H5Recorder(bool compress = true, const char *filename = NULL, uint id = GetId());
        ~H5Recorder();
        virtual void step();
        virtual void terminate();

public:
        /*!
         * The number of buffers used for storing the input data: they need to be at least 2,
         * so that the realtime thread writes in one, while the thread created by H5Recorder
         * saves the data in the other buffer to file.
         */
        static const uint numberOfBuffers;
        static const int  rank;

protected:
        virtual bool finaliseInit();
        virtual void finaliseAddPre(Entity *entity);

private:
        void startWriterThread();
        void stopWriterThread();
        static void* buffersWriter(void *arg);

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
        pthread_t m_writerThread;
        // the index of the buffer in which the main thread saves data
        uint m_bufferInUse;
        // multithreading stuff for controlling access to the queue m_dataQueue;
        pthread_mutex_t m_mutex;
        pthread_cond_t m_cv;
        bool m_threadRun;

        hsize_t m_datasetSize;
};

class TriggeredH5Recorder : public BaseH5Recorder {
public:
        TriggeredH5Recorder(double before, double after, bool compress = true, const char *filename = NULL, uint id = GetId());
        ~TriggeredH5Recorder();
        virtual void step();
        virtual void terminate();
        virtual void handleEvent(const Event *event);

public:
        /*!
         * The number of buffers used for storing the input data: they need to be at least 2,
         * so that the realtime thread writes in one, while the thread created by H5Recorder
         * saves the data in the other buffer to file.
         */
        static const uint numberOfBuffers;
        static const int rank;

protected:
        virtual bool finaliseInit();
        virtual void finaliseAddPre(Entity *entity);

private:
        static void* buffersWriter(void *arg);

private:
        bool m_recording;
        // the data
        std::vector<double**> m_data;
        // a temporary buffer for unrolling the circular buffers
        double *m_tempData;
        // position in the buffer
        uint m_bufferPosition;
        // the index of the buffer in which the main thread saves data
        uint m_bufferInUse;
        // the number of steps to take after a trigger event is received, before writing the buffer to file
        uint m_maxSteps;
        // the number of steps that have been taken
        uint m_nSteps;
        // the thread that saves the data once the buffers are full
        pthread_t m_writerThread;
        hsize_t m_datasetSize[2];
};

} // namespace recorders

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* ASCIIRecorderFactory(string_dict& args);
lcg::Entity* H5RecorderFactory(string_dict& args);
lcg::Entity* TriggeredH5RecorderFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif


#endif

