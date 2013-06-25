#ifndef H5REC_H
#define H5REC_H

#include <time.h>
#include <hdf5.h>
#include <string>
#include <vector>
#include <deque>
#include "common.h"

#define GROUP_NAME_LEN   128
#define DATASET_NAME_LEN 128
//#define ENTITIES_GROUP   "/Entities"
//#define INFO_GROUP       "/Info"
#define COMMENTS_GROUP   "/Comments"
//#define DATA_DATASET     "Data"
//#define METADATA_DATASET "Metadata"
//#define PARAMETERS_GROUP "Parameters"
//#define H5_FILE_VERSION  2

namespace lcg {

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

class H5RecorderCore {
public:
        H5RecorderCore(bool compress, hsize_t bufferSize = 20480, const char *filename = NULL);
        H5RecorderCore(bool compress, hsize_t chunkSize, uint numberOfChunks, const char *filename = NULL);
        virtual ~H5RecorderCore();

        //virtual bool initialise();
        //virtual void terminate();

        hsize_t bufferSize() const;
        hsize_t chunkSize() const;
        uint numberOfChunks() const;

        const char* filename() const;

        static bool isCompressionAvailable();

        void addComment(const char *message, const time_t *timestamp = NULL);

public:
        static const hsize_t unlimitedSize;
        static const double  fillValue;

protected:
        virtual bool openFile();
        virtual void closeFile();
        virtual void deleteComments();

        //virtual void addPre(Entity *entity);
        //virtual bool allocateForEntity(Entity *entity, int dataRank,
        //                               const hsize_t *dataDims, const hsize_t *maxDataDims, const hsize_t *chunkDims);

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

protected:
        std::deque<Comment*> m_comments;

        // the handle of the file
        hid_t m_fid;
        // whether compression is turned on or off
        bool m_compress;
        // the name of the file
        char m_filename[FILENAME_MAXLEN];
        // tells whether the filename should be generated from the timestamp
        bool m_makeFilename;
         
        // number of inputs
        //uint m_numberOfInputs;

        // H5 stuff
        hid_t m_infoGroup;
        hid_t m_commentsGroup;
        std::vector<hid_t> m_groups;
        std::vector<hid_t> m_dataspaces;
        std::vector<hid_t> m_datasets;

//private:
        // the size of each chunk of data that is saved to the H5 file
        hsize_t m_chunkSize;
        // the number of chunks of data
        uint m_numberOfChunks; 
        // the total size of the buffer, given by the size of each chunk times the number of chunks
        hsize_t m_bufferSize;
};

class ChunkedH5Recorder {
public:
        ChunkedH5Recorder(int numberOfRecords, int recordLength, bool compress = true, const char *filename = NULL);
        ~ChunkedH5Recorder();

        int write(int recordNumber, const double *data, ssize_t n);

private:
        int open();
        void close();

private:
};

}

#endif


