#include <string.h>
#include "recorders.h"

dynclamp::Entity* ASCIIRecorderFactory(dictionary& args)
{
        uint id;
        std::string filename;
        id = dynclamp::GetIdFromDictionary(args);
        if (!dynclamp::CheckAndExtractValue(args, "filename", filename))
                return new dynclamp::recorders::ASCIIRecorder((const char *) NULL, id);
        return new dynclamp::recorders::ASCIIRecorder(filename.c_str(), id);
}

dynclamp::Entity* H5RecorderFactory(dictionary& args)
{       
        uint id;
        std::string filename;
        bool compress;
        id = dynclamp::GetIdFromDictionary(args);
        if (!dynclamp::CheckAndExtractBool(args, "compress", &compress)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build an H5 recorder.\n");
                return NULL;
        }
        if (!dynclamp::CheckAndExtractValue(args, "filename", filename))
                return new dynclamp::recorders::H5Recorder(compress, NULL, id);
        return new dynclamp::recorders::H5Recorder(compress, filename.c_str(), id);

}

namespace dynclamp {

namespace recorders {

Recorder::Recorder(uint id) : Entity(id)
{}

double Recorder::output() const
{
        return 0.0;
}

ASCIIRecorder::ASCIIRecorder(const char *filename, uint id)
        : Recorder(id), m_closeFile(false)
{
        if (filename == NULL) {
                m_makeFilename = true;
        }
        else {
                m_makeFilename = false;
                strncpy(m_filename, filename, FILENAME_MAXLEN);
        }
}

ASCIIRecorder::~ASCIIRecorder()
{
        closeFile();
}

void ASCIIRecorder::closeFile()
{
        if (m_closeFile)
                fclose(m_fid);
}

void ASCIIRecorder::openFile()
{
        m_fid = fopen(m_filename, "w");
        if (m_fid == NULL) {
                char msg[100];
                sprintf(msg, "Unable to open %s.\n", m_filename); 
                throw msg;
        }
        m_closeFile = true;
}

void ASCIIRecorder::initialise()
{       
        closeFile();
        if (m_makeFilename)
                MakeFilename(m_filename, "dat");
        openFile();
}

void ASCIIRecorder::step()
{
        uint i, n = m_inputs.size();
        if (n > 0) {
                fprintf(m_fid, "%14e", GetGlobalTime());
                for (i=0; i<n; i++)
                        fprintf(m_fid, " %14e", m_inputs[i]);
                fprintf(m_fid, "\n");
        }
}

const hsize_t H5Recorder::rank           = 1;
const hsize_t H5Recorder::unlimitedSize  = H5S_UNLIMITED;
const hsize_t H5Recorder::chunkSize      = 1024;
const uint    H5Recorder::numberOfChunks = 20;
const hsize_t H5Recorder::bufferSize     = 20480;    // This MUST be equal to chunkSize times numberOfChunks.
const double  H5Recorder::fillValue      = 0.0;

H5Recorder::H5Recorder(bool compress, const char *filename, uint id)
        : Recorder(id), m_fid(-1),
          m_data(), m_numberOfInputs(0),
          m_threadRun(false), m_numberOfBuffers(2),
          m_mutex(), m_cv(), 
          m_groups(), m_dataspaces(), m_datasets()
{
        if (filename == NULL) {
                m_makeFilename = true;
        }
        else {
                m_makeFilename = false;
                strncpy(m_filename, filename, FILENAME_MAXLEN);
        }

        if (compress && isCompressionAvailable() == OK)
                m_compress = true;
        else
                m_compress = false;

        m_bufferLengths = new hsize_t[m_numberOfBuffers];
}

H5Recorder::~H5Recorder()
{
        stopWriterThread();
        closeFile();
        uint i, j;
        for (i=0; i<m_numberOfInputs; i++) {
                for(j=0; j<m_numberOfBuffers; j++)
                        delete m_data[i][j];
                delete m_data[i];
        }
        delete m_bufferLengths;
}

void H5Recorder::initialise()
{
        stopWriterThread();
        closeFile();

        if (m_makeFilename)
                MakeFilename(m_filename, "h5");

        m_bufferInUse = m_numberOfBuffers-1;
        m_bufferPosition = 0;
        m_offset = 0;
        m_datasetSize = 0;
        m_groups.clear();
        m_dataspaces.clear();
        m_datasets.clear();

        if (openFile() != OK)
                throw "Unable to open H5 file.";
        else
                Logger(Debug, "Successfully opened file [%s].\n", m_filename);

        for (uint i=0; i<m_pre.size(); i++)
                allocateForEntity(m_pre[i]);

        startWriterThread();
}

void H5Recorder::startWriterThread()
{
        if (m_threadRun)
                return;
        m_threadRun = true;
        m_writerThread = boost::thread(&H5Recorder::buffersWriter, this); 
}


void H5Recorder::stopWriterThread()
{
        if (!m_threadRun)
                return;

        Logger(Debug, "H5Recorder::stopWriterThread() >> Terminating writer thread.\n");
        {
                boost::unique_lock<boost::mutex> lock(m_mutex);
                Logger(Debug, "H5Recorder::~H5Recorder() >> buffer position = %d.\n", m_bufferPosition);
                if (m_bufferPosition > 0)
                        m_dataQueue.push_back(m_bufferInUse);
                Logger(Debug, "H5Recorder::~H5Recorder() >> %d values left to save in buffer #%d.\n",
                                m_bufferLengths[m_bufferInUse], m_bufferInUse);
        }
        m_threadRun = false;
        m_cv.notify_all();
        m_writerThread.join();
        Logger(Debug, "H5Recorder::stopWriterThread() >> Writer thread has terminated.\n");
}

void H5Recorder::step()
{
        if (m_numberOfInputs == 0)
                return;

        if (m_bufferPosition == 0)
        {
                boost::unique_lock<boost::mutex> lock(m_mutex);
                while (m_dataQueue.size() == m_numberOfBuffers) {
                        Logger(Debug, "Main thread: the data queue is full.\n");
                        m_cv.wait(lock);
                }
                m_bufferInUse = (m_bufferInUse+1) % m_numberOfBuffers;
                m_bufferLengths[m_bufferInUse] = 0;
                Logger(Debug, "H5Recorder::step() >> Starting to write in buffer #%d @ t = %g.\n", m_bufferInUse, GetGlobalTime());
        }

        for (uint i=0; i<m_numberOfInputs; i++)
                m_data[i][m_bufferInUse][m_bufferPosition] = m_inputs[i];
        m_bufferLengths[m_bufferInUse]++;
        m_bufferPosition = (m_bufferPosition+1) % bufferSize;

        if (m_bufferPosition == 0) {
                Logger(Debug, "H5Recorder::step() >> Buffer #%d is full (it contains %d elements).\n",
                                m_bufferInUse, m_bufferLengths[m_bufferInUse]);
                {
                        Logger(Debug, "H5Recorder::step() >> Trying to acquire the mutex on the data queue.\n");
                        boost::unique_lock<boost::mutex> lock(m_mutex);
                        Logger(Debug, "H5Recorder::step() >> Acquired the mutex on the data queue.\n");
                        m_dataQueue.push_back(m_bufferInUse);
                }
                m_cv.notify_all();
                Logger(Debug, "H5Recorder::step() >> Released the mutex and notified all.\n");
        }
}

void H5Recorder::buffersWriter()
{
        Logger(Debug, "H5Recorder::buffersWriter >> Started.\n");
        while (m_threadRun || m_dataQueue.size() != 0) {
                {
                        boost::unique_lock<boost::mutex> lock(m_mutex);
                        while (m_dataQueue.size() == 0) {
                                Logger(Debug, "H5Recorder::buffersWriter >> The data queue is empty.\n");
                                m_cv.wait(lock);
                        }
                }
                uint bufferToSave = m_dataQueue.front();
                Logger(Debug, "H5Recorder::buffersWriter() >> Acquired lock: will save data in buffer #%d.\n", bufferToSave);

                hid_t filespace;
                herr_t status;

                if (m_bufferLengths[bufferToSave] > 0) {

                        m_datasetSize += m_bufferLengths[bufferToSave];

                        Logger(Debug, "Dataset size = %d.", m_datasetSize);
                        Logger(Debug, " Offset = %d.", m_offset);
                        Logger(Debug, " Time = %g sec.\n", m_datasetSize*GetGlobalDt());

                        for (uint i=0; i<m_numberOfInputs; i++) {

                                // extend the dataset
                                status = H5Dset_extent(m_datasets[i], &m_datasetSize);
                                if (status < 0)
                                        throw "Unable to extend dataset.";
                                else
                                        Logger(All, "Extended dataset.\n");

                                // get the filespace
                                filespace = H5Dget_space(m_datasets[i]);
                                if (filespace < 0)
                                        throw "Unable to get filespace.";
                                else
                                        Logger(All, "Obtained filespace.\n");

                                // select an hyperslab
                                status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, &m_offset, NULL, &m_bufferLengths[bufferToSave], NULL);
                                if (status < 0) {
                                        H5Sclose(filespace);
                                        throw "Unable to select hyperslab.";
                                }
                                else {
                                        Logger(All, "Selected hyperslab.\n");
                                }

                                // define memory space
                                m_dataspaces[i] = H5Screate_simple(rank, &m_bufferLengths[bufferToSave], NULL);
                                if (m_dataspaces[i] < 0) {
                                        H5Sclose(filespace);
                                        throw "Unable to define memory space.";
                                }
                                else {
                                        Logger(All, "Memory space defined.\n");
                                }

                                // write data
                                status = H5Dwrite(m_datasets[i], H5T_IEEE_F64LE, m_dataspaces[i], filespace, H5P_DEFAULT, m_data[i][bufferToSave]);
                                if (status < 0) {
                                        H5Sclose(filespace);
                                        throw "Unable to write data.";
                                }
                                else {
                                        Logger(All, "Written data.\n");
                                }
                        }
                        H5Sclose(filespace);
                        m_offset = m_datasetSize;
                        Logger(Debug, "H5Recorder::buffersWriter() >> Finished writing data.\n");
                }

                {
                        boost::unique_lock<boost::mutex> lock(m_mutex);
                        m_dataQueue.pop_front();
                }
                m_cv.notify_all();
        }
        Logger(Debug, "H5Recorder::buffersWriter() >> Writing thread has terminated.\n");
}

void H5Recorder::allocateForEntity(Entity *entity)
{
        herr_t status;

        // the name of the dataset
        char datasetName[DATASET_NAME_LEN];
        sprintf(datasetName, "/Data/Entity-%04d", entity->id());

        // create the dataspace with unlimited dimensions
        hid_t dspace = H5Screate_simple(rank, &bufferSize, &unlimitedSize);
        if (dspace < 0)
                throw "Unable to create dataspace.";
        else
                Logger(All, "Dataspace created.\n");

        // modify dataset creation properties, i.e. enable chunking.
        hid_t cparms = H5Pcreate(H5P_DATASET_CREATE);
        if (cparms < 0) {
                H5Sclose(dspace);
                throw "Unable to create dataset properties.";
        }
        else {
                Logger(All, "Dataset properties created.\n");
        }

        status = H5Pset_chunk(cparms, rank, &chunkSize);
        if (status < 0) {
                H5Sclose(dspace);
                H5Pclose(cparms);
                throw "Unable to set chunking.";
        }
        else {
                Logger(All, "Chunking set.\n");
        }

        status = H5Pset_fill_value(cparms, H5T_IEEE_F64LE, &fillValue);
        if (status < 0) {
                H5Sclose(dspace);
                H5Pclose(cparms);
                throw "Unable to set fill value.";
        }
        else {
                Logger(All, "Fill value set.\n");
        }

        // create a new dataset within the file using cparms creation properties.
        hid_t dset = H5Dcreate2(m_fid, datasetName,
                                H5T_IEEE_F64LE, dspace,
                                H5P_DEFAULT, cparms, H5P_DEFAULT);
        if (dset < 0) {
                H5Sclose(dspace);
                H5Pclose(cparms);
                throw "Unable to create a dataset.";
        }
        else {
                Logger(All, "Dataset created.\n");
        }

        H5Pclose(cparms);

        m_datasets.push_back(dset);
        m_dataspaces.push_back(dspace);

        // save parameters
        size_t npars = entity->numberOfParameters();
        if (npars > 0) {
                double *pars = new double[npars];
                for (uint i=0; i<npars; i++)
                        pars[i] = entity->parameter(i);
                sprintf(datasetName, "/Parameters/Entity-%04d", entity->id());
                if (!writeData(datasetName, pars, &npars, 1))
                        Logger(Critical, "Unable to save parameters for entity #%d.\n", entity->id());
                delete pars;
        }
        
        // save metadata
        size_t ndims;
        if (entity->hasMetadata(&ndims)) {
                char label[LABEL_LEN];
                size_t *dims = new size_t[ndims];
                const double *metadata = entity->metadata(dims, label);
                sprintf(datasetName, "/Metadata/Entity-%04d-%s", entity->id(), label);
                if (!writeData(datasetName, metadata, dims, ndims))
                        Logger(Critical, "Unable to write metadata for entity #%d.\n", entity->id());
                delete dims;
        }
}

void H5Recorder::addPre(Entity *entity)
{
        Entity::addPre(entity);

        Logger(All, "--- H5Recorder::addPre(Entity*, double) ---\n");

        m_numberOfInputs++;
        double **buffer = new double*[m_numberOfBuffers];
        for (uint i=0; i<m_numberOfBuffers; i++)
                buffer[i] = new double[bufferSize];
        m_data.push_back(buffer);
}

bool H5Recorder::writeData(const char *datasetName, const double *data, const size_t *dims, size_t ndims)
{
        herr_t status;
        hid_t dataspace, dataset;
        hsize_t *hdims;

        hdims = new hsize_t[ndims];
        for (int i=0; i<ndims; i++)
                hdims[i] = (hsize_t) dims[i];
        dataspace = H5Screate_simple(ndims, hdims, NULL);
        if (dataspace < 0) {
                delete hdims;
                return false;
        }

        dataset = H5Dcreate2(m_fid, datasetName, H5T_IEEE_F64LE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (dataset < 0) {
                H5Sclose(dataspace);
                delete hdims;
                return false;
        }

        status = H5Dwrite(dataset, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

        H5Dclose(dataset);
        H5Sclose(dataspace);
        delete hdims;

        if (status < 0)
                return false;

        return true;
}

int H5Recorder::isCompressionAvailable() const
{
        htri_t avail;
        herr_t status;
        uint filterInfo;
        
        Logger(All, "Checking whether GZIP compression is available...");
        // check if gzip compression is available
        avail = H5Zfilter_avail (H5Z_FILTER_DEFLATE);
        if (!avail) {
                Logger(All, "\nGZIP compression is not available on this system.\n");
                return H5_NO_GZIP_COMPRESSION;
        }
        Logger(All, " ok.\nGetting filter info...");
        status = H5Zget_filter_info (H5Z_FILTER_DEFLATE, &filterInfo);
        if (!(filterInfo & H5Z_FILTER_CONFIG_ENCODE_ENABLED)) {
                Logger(All, "\nUnable to get filter info: disabling compression.\n");
                return H5_NO_FILTER_INFO;
        }
        Logger(All, " ok.\nChecking whether the shuffle filter is available...");
        // check for availability of the shuffle filter.
        avail = H5Zfilter_avail(H5Z_FILTER_SHUFFLE);
        if (!avail) {
                Logger(All, "\nThe shuffle filter is not available on this system.\n");
                return H5_NO_SHUFFLE_FILTER;
        }
        Logger(All, " ok.\nGetting filter info...");
        status = H5Zget_filter_info (H5Z_FILTER_SHUFFLE, &filterInfo);
        if ( !(filterInfo & H5Z_FILTER_CONFIG_ENCODE_ENABLED) ) {
                Logger(All, "Unable to get filter info: disabling compression.\n");
                return H5_NO_FILTER_INFO;
        }
        Logger(All, " ok.\nCompression is enabled.\n");
        return OK;
}

int H5Recorder::openFile()
{
        Logger(All, "--- H5Recorder::openFile() ---\n");
        Logger(All, "Opening file %s.\n", m_filename);

        m_fid = H5Fcreate(m_filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        if(m_fid < 0)
                return H5_FILE_OPEN_ERROR;

        if (!createGroups())
                return H5_GROUPS_OPEN_ERROR;

        if (!writeMiscellanea())
                return H5_MISC_WRITE_ERROR;

        return checkCompression();
}

bool H5Recorder::createGroups()
{
        hid_t grp;
        grp = H5Gcreate2(m_fid, "/Data", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (grp < 0)
                return false;
        m_groups["data"] = grp;

        grp = H5Gcreate2(m_fid, "/Metadata", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (grp < 0)
                return false;
        m_groups["metadata"] = grp;

        grp = H5Gcreate2(m_fid, "/Parameters", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (grp < 0)
                return false;
        m_groups["parameters"] = grp;

        grp = H5Gcreate2(m_fid, "/Misc", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (grp < 0)
                return false;
        m_groups["misc"] = grp;

        return true;
}

int H5Recorder::checkCompression()
{
        if(m_compress) {
                // Create the dataset creation property list and add the shuffle
                // filter and the gzip compression filter.
                // The order in which the filters are added here is significant -
                // we will see much greater results when the shuffle is applied
                // first.  The order in which the filters are added to the property
                // list is the order in which they will be invoked when writing
                // data.
                m_datasetPropertiesList = H5Pcreate(H5P_DATASET_CREATE);
                if(m_datasetPropertiesList < 0)
                        return H5_DATASET_ERROR;
                if(H5Pset_shuffle(m_datasetPropertiesList) < 0)
                        return H5_SHUFFLE_ERROR;
                if(H5Pset_deflate(m_datasetPropertiesList, 9) < 0)
                        return H5_DEFLATE_ERROR;
        }
        return OK;
}
        
bool H5Recorder::writeMiscellanea()
{
        double dt = GetGlobalDt();
        herr_t status;
        hid_t dataspace, cparms, dataset, aid, attr;
        hsize_t dims = 0, chunk = 1;

        dataspace = H5Screate_simple(1, &dims, &unlimitedSize);
        if (dataspace < 0) {
                Logger(Critical, "Unable to create dataspace.\n");
                return false;
        }

        cparms = H5Pcreate(H5P_DATASET_CREATE);
        if (cparms < 0) {
                H5Sclose(dataspace);
                Logger(Critical, "Unable to create properties.\n");
                return false;
        }

        status = H5Pset_chunk(cparms, 1, &chunk);
        if (status < 0) {
                H5Pclose(cparms);
                H5Sclose(dataspace);
                Logger(Critical, "Unable to set chunking.\n");
                return false;
        }

        status = H5Pset_fill_value(cparms, H5T_IEEE_F64LE, &fillValue);
        if (status < 0) {
                H5Pclose(cparms);
                H5Sclose(dataspace);
                Logger(Critical, "Unable to set fill value.\n");
                return false;
        }

        dataset = H5Dcreate2(m_fid, "/Misc/Simulation_properties", H5T_IEEE_F64LE, dataspace, H5P_DEFAULT, cparms, H5P_DEFAULT);
        if (dataset < 0) {
                H5Pclose(cparms);
                H5Sclose(dataspace);
                Logger(Critical, "Unable to create dataset.\n");
                return false;
        }

        aid = H5Screate(H5S_SCALAR);
        if (aid < 0) {
                H5Dclose(dataset);
                H5Pclose(cparms);
                H5Sclose(dataspace);
                Logger(Critical, "Unable to create scalar.\n");
                return false;
        }

        attr = H5Acreate2(dataset, "dt", H5T_IEEE_F64LE, aid, H5P_DEFAULT, H5P_DEFAULT);
        if (attr < 0) {
                H5Sclose(aid);
                H5Dclose(dataset);
                H5Pclose(cparms);
                H5Sclose(dataspace);
                Logger(Critical, "Unable to create attribute.\n");
                return false;
        }

        status = H5Awrite(attr, H5T_IEEE_F64LE, &dt);

        H5Aclose(attr);
        H5Sclose(aid);
        H5Dclose(dataset);
        H5Pclose(cparms);
        H5Sclose(dataspace);

        if (status < 0) {
                Logger(Critical, "Unable to write attribute.\n");
                return false;
        }

        return true;
}

void H5Recorder::closeFile()
{
        Logger(All, "--- H5Recorder::closeFile() ---\n");
        Logger(All, "Closing file.\n");
        double dt = GetGlobalDt();
        if (m_fid != -1) {

                hid_t dataset = H5Dopen2(m_fid, "/Misc/Simulation_properties", H5P_DEFAULT);
                if (dataset >= 0) {
                        hid_t aid = H5Screate(H5S_SCALAR);
                        hid_t attr = H5Acreate2(dataset, "tend", H5T_IEEE_F64LE, aid, H5P_DEFAULT, H5P_DEFAULT);
                        double tend = GetGlobalTime() - dt;
                        H5Awrite(attr, H5T_IEEE_F64LE, &tend);
                        H5Aclose(attr);
                        H5Sclose(aid);
                        H5Dclose(dataset);
                }

                if(m_compress)
                        H5Pclose(m_datasetPropertiesList);

                for (uint i=0; i<m_numberOfInputs; i++) {
                        H5Dclose(m_datasets[i]);
                        H5Sclose(m_dataspaces[i]);
                }

                std::map<std::string,hid_t>::iterator it;
                for (it = m_groups.begin(); it != m_groups.end(); it++)
                        H5Gclose((*it).second);

                H5Fclose(m_fid);
                
                m_fid = -1;
        }
}

} // namespace recorders

} // namespace dynclamp

