#include <algorithm>
#include "h5rec.h"
#include "utils.h"

namespace lcg {

const hsize_t H5RecorderCore::unlimitedSize = H5S_UNLIMITED;
const double  H5RecorderCore::fillValue     = 0.0;

H5RecorderCore::H5RecorderCore(bool compress, hsize_t bufferSize, const char *filename)
        : m_fid(-1), m_bufferSize(bufferSize),
          m_groups(), m_dataspaces(), m_datasets()
{
        if (filename == NULL) {
                m_makeFilename = true;
        }
        else {
                m_makeFilename = false;
                strncpy(m_filename, filename, FILENAME_MAXLEN);
        }
        if (compress && isCompressionAvailable())
                m_compress = true;
        else
                m_compress = false;

        if (m_bufferSize % 1024 == 0) {
                m_chunkSize = 1024;
                m_numberOfChunks = m_bufferSize / m_chunkSize;
        }
        else {
                m_chunkSize = m_bufferSize;
                m_numberOfChunks = 1;
        }
}

H5RecorderCore::H5RecorderCore(bool compress, hsize_t chunkSize, uint numberOfChunks, const char *filename)
        : m_fid(-1), m_bufferSize(chunkSize*numberOfChunks),
          m_chunkSize(chunkSize), m_numberOfChunks(numberOfChunks),
          m_groups(), m_dataspaces(), m_datasets()
{
        if (filename == NULL || !strlen(filename)) {
                m_makeFilename = true;
        }
        else {
                m_makeFilename = false;
                strncpy(m_filename, filename, FILENAME_MAXLEN);
        }
        if (compress && isCompressionAvailable())
                m_compress = true;
        else
                m_compress = false;

}

H5RecorderCore::~H5RecorderCore()
{
        deleteComments();
}

void H5RecorderCore::addComment(const char *message, const time_t *timestamp)
{
        m_comments.push_back(new Comment(message, timestamp));
}

void H5RecorderCore::deleteComments()
{
        std::deque<Comment*>::iterator it;
        for (it=m_comments.begin(); it!=m_comments.end(); it++)
                delete *it;
}

hsize_t H5RecorderCore::bufferSize() const
{
        return m_bufferSize;
}

hsize_t H5RecorderCore::chunkSize() const
{
        return m_chunkSize;
}

uint H5RecorderCore::numberOfChunks() const
{
        return m_numberOfChunks;
}

bool H5RecorderCore::isCompressionAvailable()
{
        htri_t avail;
        herr_t status;
        uint filterInfo;
        
        avail = H5Zfilter_avail(H5Z_FILTER_DEFLATE);
        if (!avail) {
                Logger(Important, "GZIP compression is not available on this system.\n");
                return false;
        }
        Logger(All, "GZIP compression is available.\n");

        status = H5Zget_filter_info(H5Z_FILTER_DEFLATE, &filterInfo);
        if (!(filterInfo & H5Z_FILTER_CONFIG_ENCODE_ENABLED)) {
                Logger(Important, "Unable to get filter info: disabling compression.\n");
                return false;
        }
        Logger(All, "Obtained filter info.\n");
        
        avail = H5Zfilter_avail(H5Z_FILTER_SHUFFLE);
        if (!avail) {
                Logger(Important, "\nThe shuffle filter is not available on this system.\n");
                return false;
        }
        Logger(All, "The shuffle filter is available.\n");
        
        status = H5Zget_filter_info (H5Z_FILTER_SHUFFLE, &filterInfo);
        if ( !(filterInfo & H5Z_FILTER_CONFIG_ENCODE_ENABLED) ) {
                Logger(Important, "Unable to get filter info: disabling compression.\n");
                return false;
        }
        Logger(Debug, "Compression is enabled.\n");

        return true;
}

bool H5RecorderCore::initialiseFile()
{
        Logger(Debug, "H5RecorderCore::initialiseFile()\n");

        if (!createGroup(INFO_GROUP, &m_infoGroup)) {
                Logger(Critical, "Unable to create Info group.\n");
                return false;
        }
        Logger(Debug, "Successfully created Info group.\n");
        writeScalarAttribute(m_infoGroup, "version", (long) H5_FILE_VERSION);

        hid_t grp;
        if (!createGroup(ENTITIES_GROUP, &grp)) {
                Logger(Critical, "Unable to create Entities group.\n");
                return false;
        }
        Logger(Debug, "Successfully created Entities group.\n");

        if (!createGroup(EVENTS_GROUP, &grp)) {
                Logger(Critical, "Unable to create Events group.\n");
                return false;
        }
        Logger(Debug, "Successfully created Events group.\n");
        return true;
}

bool H5RecorderCore::openFile()
{
        Logger(All, "--- H5RecorderCore::openFile() ---\n");
        Logger(All, "Opening file %s.\n", m_filename);

        m_fid = H5Fcreate(m_filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        if(m_fid < 0)
                return false;

        addComment("Opened file.");
        return true;
}

void H5RecorderCore::closeFile()
{
        Logger(All, "--- H5RecorderCore::closeFile() ---\n");
        if (m_fid != -1) {
                int i;
                addComment("Closed file.");
                writeComments();
                for (i=0; i<m_datasets.size(); i++)
                        H5Dclose(m_datasets[i]);
                for (i=0; i<m_dataspaces.size(); i++)
                        H5Sclose(m_dataspaces[i]);
                for (i=0; i<m_groups.size(); i++)
                        H5Gclose(m_groups[i]);
                H5Fclose(m_fid);
                m_fid = -1;
        }
}

const char* H5RecorderCore::filename() const
{
        return m_filename;
}

void H5RecorderCore::writeComments()
{
        char tag[4];
        int i = 1;
        if (!createGroup(COMMENTS_GROUP, &m_commentsGroup)) {
                Logger(Important, "Unable to create group for comments.\n");
                return;
        }
        while (m_comments.size() > 0) {
                Comment *c = m_comments.front();
                sprintf(tag, "%03d", i);
                writeStringAttribute(m_commentsGroup, tag, c->message());
                m_comments.pop_front();
                delete c;
                i++;
        }
}

bool H5RecorderCore::createGroup(const char *groupName, hid_t *grp)
{
        *grp = H5Gcreate2(m_fid, groupName, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (*grp < 0)
                return false;
        m_groups.push_back(*grp);
        return true;
}

bool H5RecorderCore::createUnlimitedDataset(const char *datasetName,
                                            int rank, const hsize_t *dataDims, const hsize_t *maxDataDims, const hsize_t *chunkDims,
                                            hid_t *dspace, hid_t *dset, hid_t dataTypeID)
{
        herr_t status;
        
        // create the dataspace with unlimited dimensions
        *dspace = H5Screate_simple(rank, dataDims, maxDataDims);
        if (*dspace < 0) {
                Logger(Critical, "Unable to create dataspace.\n");
                return false;
        }
        else {
                Logger(Debug, "Dataspace created.\n");
        }

        // modify dataset creation properties, i.e. enable chunking.
        hid_t cparms = H5Pcreate(H5P_DATASET_CREATE);

        if (cparms < 0) {
                Logger(Critical, "Unable to create dataset properties.\n");
                H5Sclose(*dspace);
                return false;
        }
        else {
                Logger(Debug, "Dataset properties created.\n");
        }

        status = H5Pset_chunk(cparms, rank, chunkDims);
        if (status < 0) {
                Logger(Critical, "Unable to set chunking.\n");
                H5Sclose(*dspace);
                H5Pclose(cparms);
                return false;
        }
        else {
                Logger(Debug, "Chunking set.\n");
        }

        status = H5Pset_fill_value(cparms, H5T_IEEE_F64LE, &fillValue);
        if (status < 0) {
                Logger(Critical, "Unable to set fill value.\n");
                H5Sclose(*dspace);
                H5Pclose(cparms);
                return false;
        }
        else {
                Logger(Debug, "Fill value set.\n");
        }

        if (m_compress) {
                // Add the shuffle filter and the gzip compression filter to the
                // dataset creation property list.
                // The order in which the filters are added here is significant -
                // we will see much greater results when the shuffle is applied
                // first.  The order in which the filters are added to the property
                // list is the order in which they will be invoked when writing
                // data.
                if (H5Pset_shuffle(cparms) < 0 || H5Pset_deflate(cparms, 9) < 0)
                        Logger(Important, "Unable to enable compression.\n");
                else 
                        Logger(Debug, "Successfully enabled compression.\n");
        }

        // create a new dataset within the file using cparms creation properties.
        *dset = H5Dcreate2(m_fid, datasetName,
                           dataTypeID, *dspace,
                           H5P_DEFAULT, cparms, H5P_DEFAULT);
        if (*dset < 0) {
                Logger(Critical, "Unable to create dataset.\n");
                H5Sclose(*dspace);
                H5Pclose(cparms);
                return false;
        }
        else {
                Logger(Debug, "Dataset created.\n");
        }

        H5Pclose(cparms);
        m_datasets.push_back(*dset);
        m_dataspaces.push_back(*dspace);

        return true;
}

bool H5RecorderCore::writeStringAttribute(hid_t dataset,
                                      const char *attrName,
                                      const char *attrValue)
{
        hid_t dspace, atype, attr;
        herr_t status;
        bool retval = true;
        
        dspace = H5Screate(H5S_SCALAR);
        if (dspace < 0) {
                Logger(Critical, "Error in H5Screate.\n");
                return false;
        }

        atype = H5Tcopy(H5T_C_S1);
        if (atype < 0) {
                Logger(Critical, "Error in H5Tcopy.\n");
                H5Sclose(dspace);
                return false;
        }
        Logger(Debug, "Successfully copied type.\n");

        status = H5Tset_size(atype, strlen(attrValue) + 1);
        if (status < 0) {
                Logger(Critical, "Error in H5Tset_size.\n");
                H5Tclose(atype);
                H5Sclose(dspace);
                return false;
        }
        Logger(Debug, "Successfully set type size.\n");

        status = H5Tset_strpad(atype, H5T_STR_NULLTERM);
        if (status < 0) {
                Logger(Critical, "Error in H5Tset_strpad.\n");
                H5Tclose(atype);
                H5Sclose(dspace);
                return false;
        }
        Logger(Debug, "Successfully set type padding string.\n");

        attr = H5Acreate2(dataset, attrName, atype, dspace, H5P_DEFAULT, H5P_DEFAULT);
        if (attr < 0) {
                Logger(Critical, "Error in H5Acreate2.\n");
                H5Tclose(atype);
                H5Sclose(dspace);
                return false;
        }
        Logger(Debug, "Successfully created attribute.\n");

        status = H5Awrite(attr, atype, attrValue);
        if (status < 0) {
                Logger(Critical, "Error in H5Awrite.\n");
                retval = false;
        }
        else {
                Logger(Debug, "Successfully written string attribute.\n");
        }

        H5Aclose(attr);
        H5Tclose(atype);
        H5Sclose(dspace);

        return retval;
}

bool H5RecorderCore::writeScalarAttribute(hid_t dataset, const char *attrName, long attrValue)
{
        Logger(Debug, "H5RecorderCore::writeScalarAttribute(%d, %s, %g)\n", dataset, attrName, attrValue);
        hid_t aid, attr;
        herr_t status;

        aid = H5Screate(H5S_SCALAR);
        if (aid < 0) {
                Logger(Critical, "Unable to create scalar.\n");
                return false;
        }

        attr = H5Acreate2(dataset, attrName, H5T_NATIVE_LONG, aid, H5P_DEFAULT, H5P_DEFAULT);
        if (attr < 0) {
                H5Sclose(aid);
                Logger(Critical, "Unable to create scalar attribute.\n");
                return false;
        }

        status = H5Awrite(attr, H5T_NATIVE_LONG, &attrValue);

        H5Aclose(attr);
        H5Sclose(aid);

        if (status < 0) {
                Logger(Critical, "Unable to write attribute.\n");
                return false;
        }

        return true;
}  

bool H5RecorderCore::writeScalarAttribute(hid_t dataset, const char *attrName, double attrValue)
{
        Logger(Debug, "H5RecorderCore::writeScalarAttribute(%d, %s, %g)\n", dataset, attrName, attrValue);
        hid_t aid, attr;
        herr_t status;

        aid = H5Screate(H5S_SCALAR);
        if (aid < 0) {
                Logger(Critical, "Unable to create scalar.\n");
                return false;
        }

        attr = H5Acreate2(dataset, attrName, H5T_IEEE_F64LE, aid, H5P_DEFAULT, H5P_DEFAULT);
        if (attr < 0) {
                H5Sclose(aid);
                Logger(Critical, "Unable to create scalar attribute.\n");
                return false;
        }

        status = H5Awrite(attr, H5T_IEEE_F64LE, &attrValue);

        H5Aclose(attr);
        H5Sclose(aid);

        if (status < 0) {
                Logger(Critical, "Unable to write attribute.\n");
                return false;
        }

        return true;
}

bool H5RecorderCore::writeArrayAttribute(hid_t dataset, const char *attrName,
                                         const double *data, const hsize_t *dims, int ndims)
{
        hid_t dspace, attr;
        herr_t status;
        bool retval = true;
        
        dspace = H5Screate(H5S_SIMPLE);
        if (dspace < 0) {
                Logger(Critical, "Error in H5Screate.\n");
                return false;
        }

        status = H5Sset_extent_simple(dspace, ndims, dims, NULL);
        if (status < 0) {
                Logger(Critical, "Error in H5Sset_extent_simple.\n");
                H5Sclose(dspace);
                return false;
        }
        Logger(Debug, "Successfully allocated space for the data.\n");

        attr = H5Acreate2(dataset, attrName, H5T_IEEE_F64LE, dspace, H5P_DEFAULT, H5P_DEFAULT);
        if (attr < 0) {
                Logger(Critical, "Error in H5Acreate2.\n");
                H5Sclose(dspace);
                return false;
        }
        Logger(Debug, "Successfully created attribute.\n");

        status = H5Awrite(attr, H5T_IEEE_F64LE, data);
        if (status < 0) {
                Logger(Critical, "Error in H5Awrite.\n");
                retval = false;
        }
        else {
                Logger(Debug, "Successfully written array attribute.\n");
        }

        H5Aclose(attr);
        H5Sclose(dspace);

        return retval;
        return true;
}

bool H5RecorderCore::writeData(const char *datasetName, int rank, const hsize_t *dims,
                               const double *data, const char *label)
{
        hid_t dspace, dset;
        herr_t status;

        dspace = H5Screate(H5S_SIMPLE);
        if (dspace < 0) {
                Logger(Critical, "Unable to create dataspace.\n");
                return false;
        }
        Logger(Debug, "Successfully created dataspace.\n");

        status = H5Sset_extent_simple(dspace, rank, dims, NULL);
        if (status < 0) {
                Logger(Critical, "Unable to set the size of the dataset.\n");
                H5Sclose(dspace);
                return false;
        }
        Logger(Debug, "Successfully set dataspace size.\n");

        dset = H5Dcreate2(m_fid, datasetName, H5T_IEEE_F64LE, dspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (dset < 0) {
                Logger(Critical, "Unable to create dataset [%s].\n", datasetName);
                return false;
        }
        Logger(Debug, "Successfully created dataset [%s].\n", datasetName);

        status = H5Dwrite(dset, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
        if (status < 0) {
                Logger(Critical, "Unable to write data to dataset [%s].\n", datasetName);
                H5Dclose(dset);
                H5Sclose(dspace);
                return false;
        }
        Logger(Debug, "Successfully written data.\n");

        if (strlen(label))
                writeStringAttribute(dset, "Label", label);

        H5Dclose(dset);
        H5Sclose(dspace);

        return true;
}


//~~~

struct thread_data {
        thread_data(ChunkedH5Recorder *recorder, uint id, const double *data, size_t length)
                : m_self(recorder), m_id(id), m_data(data), m_length(length) {}
        ChunkedH5Recorder *m_self;
        uint m_id;
        const double *m_data;
        hsize_t m_length;
};

const int ChunkedH5Recorder::rank = 1;

ChunkedH5Recorder::ChunkedH5Recorder(bool compress, const char *filename)
        : H5RecorderCore(compress, 1024, 20, filename), m_ids(), m_datasetSizes(), m_writerThreads()
{
        if (m_makeFilename)
                MakeFilename(m_filename, "h5");
        openFile();
        initialiseFile();
}

ChunkedH5Recorder::~ChunkedH5Recorder()
{
        waitForWriterThreads();
        for (int i=0; i<m_writerThreads.size(); i++)
                delete m_writerThreads[i];
        closeFile();
}

bool ChunkedH5Recorder::addRecord(uint id, const char *name, const char *units,
                size_t recordLength, const double_dict& parameters,
                const double *metadata, const size_t *metadataDims)
{
        if (std::find(m_ids.begin(), m_ids.end(), id) != m_ids.end()) {
                Logger(Critical, "ID %d already present.\n", id);
                return false;
        }

        hid_t dspace, dset, grp;
        hsize_t bufsz = bufferSize(), maxbufsz = H5S_UNLIMITED, chunksz = chunkSize();
        if (recordLength > 0)
                bufsz = maxbufsz = recordLength;
        char groupName[GROUP_NAME_LEN];
        char datasetName[DATASET_NAME_LEN];

        // the name of the group (i.e., /Entities/0001)
        sprintf(groupName, "%s/%04d", ENTITIES_GROUP, id);
        if (!createGroup(groupName, &grp)) {
                Logger(Critical, "Unable to create group [%s].\n", groupName);
                return false;
        }
        Logger(Debug, "Group [%s] created.\n", groupName);

        // dataset for actual data (i.e., /Entities/0001/Data)
        sprintf(datasetName, "%s/%04d/%s", ENTITIES_GROUP, id, DATA_DATASET);
        if (!createUnlimitedDataset(datasetName, 1, &bufsz, &maxbufsz, &chunksz, &dspace, &dset)) {
                Logger(Critical, "Unable to create dataset [%s].\n", datasetName);
                return false;
        }
        Logger(Debug, "Dataset [%s] created.\n", datasetName);

        // save name and units as attributes of the group
        writeStringAttribute(grp, "Name", name);
        writeStringAttribute(grp, "Units", units);

        // save metadata
        if (metadata) {
                char label[LABEL_LEN];
                hsize_t hdims[2];
                for (int i=0; i<2; i++)
                        hdims[i] = metadataDims[i];
                sprintf(datasetName, "%s/%04d/%s", ENTITIES_GROUP, id, METADATA_DATASET);
                if (! writeData(datasetName, 2, hdims, metadata, label)) {
                        Logger(Critical, "Unable to create metadata dataset.\n");
                        return false;
                }
        } 
        
        // group for the parameters (i.e., /Entities/0001/Parameters)
        sprintf(groupName, "%s/%04d/%s", ENTITIES_GROUP, id, PARAMETERS_GROUP);
        if (!createGroup(groupName, &grp)) {
                Logger(Critical, "Unable to create group [%s].\n", groupName);
                return false;
        }
        Logger(Debug, "Group [%s] created.\n", groupName);

        double_dict::const_iterator it;
        for (it = parameters.begin(); it != parameters.end(); it++)
                writeScalarAttribute(grp, it->first.c_str(), it->second);

        m_ids.push_back(id);
        m_datasetSizes.push_back(0);
        m_writerThreads.push_back(new pthread_t);

        return true;
}

bool ChunkedH5Recorder::writeRecord(uint id, const double *data, size_t length)
{
        if (std::find(m_ids.begin(), m_ids.end(), id) == m_ids.end()) {
                Logger(Critical, "%d: no such ID.\n", id);
                return false;
        }
        thread_data *arg = new thread_data(this, id, data, length);
        //pthread_join(*m_writerThreads[id], NULL);
        if (pthread_create(m_writerThreads[id], NULL, ChunkedH5Recorder::writerThread, (void *) arg) != 0)
                return false;
        pthread_join(*m_writerThreads[id], NULL);
        return true;
}

bool ChunkedH5Recorder::writeMetadata(uint id, const double *metadata, size_t rows, size_t cols)
{
        char datasetName[DATASET_NAME_LEN], label[LABEL_LEN] = "Stimulus_Matrix";
        hsize_t dims[2] = {rows, cols};
        sprintf(datasetName, "%s/%04d/%s", ENTITIES_GROUP, id, METADATA_DATASET);
        if (! writeData(datasetName, 2, dims, metadata, label)) {
                Logger(Important, "Unable to create metadata dataset.\n");
                return false;
        }
        return true;
}

bool ChunkedH5Recorder::writeRecordingDuration(double duration)
{
        return writeScalarAttribute(m_infoGroup, "tend", duration);
}

bool ChunkedH5Recorder::writeTimeStep(double dt)
{
        return writeScalarAttribute(m_infoGroup, "dt", dt);
}

void* ChunkedH5Recorder::writerThread(void *arg)
{
        thread_data *data = static_cast<thread_data*>(arg);
        if (!data)
                pthread_exit(NULL);

        ChunkedH5Recorder *self = data->m_self;
        uint id = data->m_id;
        const double *buffer = data->m_data;
        hid_t filespace;
        herr_t status;
        hsize_t start = self->m_datasetSizes[id], count = data->m_length;
        self->m_datasetSizes[id] += count;
        delete data;

        // extend the dataset
        status = H5Dset_extent(self->m_datasets[id], &self->m_datasetSizes[id]);
        if (status < 0)
                throw "Unable to extend dataset.";
        else
                Logger(All, "Extended dataset to %d elements.\n", self->m_datasetSizes[id]);

        // get the filespace
        filespace = H5Dget_space(self->m_datasets[id]);
        if (filespace < 0)
                throw "Unable to get filespace.";
        else
                Logger(All, "Obtained filespace.\n");

        // select an hyperslab
        status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, &start, NULL, &count, NULL);
        if (status < 0) {
                H5Sclose(filespace);
                throw "Unable to select hyperslab.";
        }
        else {
                Logger(All, "Selected hyperslab.\n");
        }

        // define memory space
        self->m_dataspaces[id] = H5Screate_simple(ChunkedH5Recorder::rank, &count, NULL);
        if (self->m_dataspaces[id] < 0) {
                H5Sclose(filespace);
                throw "Unable to define memory space.";
        }
        else {
                Logger(All, "Memory space defined.\n");
        }

        // write data
        status = H5Dwrite(self->m_datasets[id], H5T_IEEE_F64LE, self->m_dataspaces[id], filespace, H5P_DEFAULT, buffer);
        if (status < 0) {
                H5Sclose(filespace);
                throw "Unable to write data.";
        }
        else {
                Logger(All, "Written data.\n");
        }
        H5Sclose(filespace);
        pthread_exit(NULL);
}

void ChunkedH5Recorder::waitForWriterThreads()
{
        for (int i=0; i<m_writerThreads.size(); i++)
                pthread_join(*m_writerThreads[i], NULL);
}

}

