/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    h5rec.h
 *
 *   Copyright (C) 2012,2013,2014 Daniele Linaro
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *=========================================================================*/

#ifndef H5REC_H
#define H5REC_H

#include <pthread.h>
#include <time.h>
#include <string.h>
#include <hdf5.h>
#include <vector>
#include <deque>
#include "types.h"
#include "common.h"

#define GROUP_NAME_LEN   128
#define DATASET_NAME_LEN 128
#define ENTITIES_GROUP   "/Entities"
#define INFO_GROUP       "/Info"
#define COMMENTS_GROUP   "/Comments"
#define DATA_DATASET     "Data"
#define METADATA_DATASET "Metadata"
#define PARAMETERS_GROUP "Parameters"
#define H5_FILE_VERSION  2

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
        virtual bool initialiseFile();
        virtual void deleteComments();

        virtual bool createGroup(const char *groupName, hid_t *grp);
        virtual bool createUnlimitedDataset(const char *datasetName,
                                            int rank, const hsize_t *dataDims, const hsize_t *maxDataDims, const hsize_t *chunkDims,
                                            hid_t *dspace, hid_t *dset);

        virtual bool writeStringAttribute(hid_t objId, const char *attrName, const char *attrValue);
        virtual bool writeScalarAttribute(hid_t objId, const char *attrName, double attrValue);
        virtual bool writeScalarAttribute(hid_t objId, const char *attrName, long attrValue);
        virtual bool writeArrayAttribute(hid_t objId, const char *attrName,
                                         const double *data, const hsize_t *dims, int ndims);
        virtual bool writeData(const char *datasetName, int rank, const hsize_t *dims,
                               const double *data, const char *label = "");

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

class ChunkedH5Recorder : public H5RecorderCore {
public:
        ChunkedH5Recorder(bool compress = true, const char *filename = NULL);
        ~ChunkedH5Recorder();
        bool addRecord(uint id, const char *name, const char *units,
                       size_t recordLength, const double_dict& parameters,
                       const double *metadata = NULL, const size_t *metadataDims = NULL);
        bool writeRecord(uint id, const double *data, size_t length);
        bool writeMetadata(uint id, const double *data, size_t rows, size_t cols);
        bool writeRecordingDuration(double duration);
        bool writeTimeStep(double dt);

        void waitForWriterThreads();
        
public:
        static const int rank;

private:
        static void* writerThread(void *arg);

private:
        std::vector<uint> m_ids;
        std::vector<hsize_t> m_datasetSizes;
        std::vector<pthread_t*> m_writerThreads;
};

}

#endif


