#include <string.h>
#include "recorders.h"

dynclamp::Entity* ASCIIRecorderFactory(dictionary& args)
{
        uint id;
        double dt;
        std::string filename;
        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);
        if (!dynclamp::CheckAndExtractValue(args, "filename", filename))
                return new dynclamp::recorders::ASCIIRecorder((const char *) NULL, id, dt);
        return new dynclamp::recorders::ASCIIRecorder(filename.c_str(), id, dt);
}

dynclamp::Entity* H5RecorderFactory(dictionary& args)
{       
        uint id;
        double dt;
        std::string filename;
        bool compress;
        dynclamp::GetIdAndDtFromDictionary(args, &id, &dt);
        if (!dynclamp::CheckAndExtractBool(args, "compress", &compress))
                return NULL;
        if (!dynclamp::CheckAndExtractValue(args, "filename", filename))
                return new dynclamp::recorders::H5Recorder(compress, NULL, id, dt);
        return new dynclamp::recorders::H5Recorder(compress, filename.c_str(), id, dt);

}

namespace dynclamp {

namespace recorders {

Recorder::Recorder(uint id, double dt)
        : Entity(id, dt)
{}

double Recorder::output() const
{
        return 0.0;
}

ASCIIRecorder::ASCIIRecorder(const char *filename, uint id, double dt)
        : Recorder(id, dt)
{
        char fname[FILENAME_MAXLEN];
        if (filename == NULL)
                MakeFilename(fname, "dat");
        else
                strncpy(fname, filename, FILENAME_MAXLEN);

        m_fid = fopen(fname, "w");
        if (m_fid == NULL) {
                char msg[100];
                sprintf(msg, "Unable to open %s.\n", filename); 
                throw msg;
        }
        m_closeFile = true;
}

ASCIIRecorder::ASCIIRecorder(FILE *fid, uint id, double dt)
        : Recorder(id, dt), m_fid(fid), m_closeFile(false)
{}

ASCIIRecorder::~ASCIIRecorder()
{
        if (m_closeFile)
                fclose(m_fid);
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
const hsize_t H5Recorder::chunkSize      = 100;
const uint    H5Recorder::numberOfChunks = 200;
const hsize_t H5Recorder::bufferSize     = 20000;    // This MUST be equal to chunkSize times numberOfChunks.
const double  H5Recorder::fillValue      = 0.0;

H5Recorder::H5Recorder(bool compress, const char *filename, uint id, double dt)
        : Recorder(id, dt),
          m_data(), m_numberOfInputs(0),
          m_bufferPosition(0), m_numberOfBuffers(2),
          m_mutex(), m_cv(), m_threadRun(true),
          m_dataspaces(), m_datasets(),
          m_offset(0), m_datasetSize(0)
{
        char fname[FILENAME_MAXLEN];
        if (filename == NULL)
                MakeFilename(fname, "h5");
        else
                strncpy(fname, filename, FILENAME_MAXLEN);

        if (open(fname, compress) != 0)
                throw "Unable to open H5 file.";

        m_bufferLengths = new hsize_t[m_numberOfBuffers];
        m_bufferInUse = m_numberOfBuffers-1;
        m_writerThread = boost::thread(&H5Recorder::buffersWriter, this); 
}

H5Recorder::~H5Recorder()
{
        Logger(Debug, "H5Recorder::~H5Recorder() >> Terminating writer thread.\n");
        {
                Logger(Debug, "H5Recorder::~H5Recorder() >> %d values left to save.\n", m_bufferLengths[m_bufferInUse]);
                boost::unique_lock<boost::mutex> lock(m_mutex);
                m_dataQueue.push_back(m_bufferInUse);
        }
        m_threadRun = false;
        m_cv.notify_all();
        m_writerThread.join();
        Logger(Debug, "H5Recorder::~H5Recorder() >> Writer thread has terminated.\n");
        close();
        uint i, j;
        for (i=0; i<m_numberOfInputs; i++) {
                for(j=0; j<m_numberOfBuffers; j++)
                        delete m_data[i][j];
                delete m_data[i];
        }
        delete m_bufferLengths;
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
        }

        for (uint i=0; i<m_numberOfInputs; i++)
                m_data[i][m_bufferInUse][m_bufferPosition] = m_inputs[i];
        m_bufferLengths[m_bufferInUse]++;
        m_bufferPosition = (m_bufferPosition+1) % bufferSize;

        if (m_bufferPosition == 0) {
                Logger(Debug, "         H5Recorder::step() >> Buffer #%d is full.\n", m_bufferInUse);
                {
                        boost::unique_lock<boost::mutex> lock(m_mutex);
                        m_dataQueue.push_back(m_bufferInUse);
                }
                m_cv.notify_all();
        }
}

void H5Recorder::buffersWriter()
{
        while (m_threadRun || m_dataQueue.size() != 0) {
                {
                        boost::unique_lock<boost::mutex> lock(m_mutex);
                        while (m_dataQueue.size() == 0)
                                m_cv.wait(lock);
                }
                uint bufferToSave = m_dataQueue.front();
                Logger(Debug, "H5Recorder::buffersWriter() >> Acquired lock: will save data in buffer #%d.\n", bufferToSave);

                hid_t filespace;
                herr_t status;

                if (m_bufferLengths[bufferToSave] > 0) {

                        m_datasetSize += m_bufferLengths[bufferToSave];

                        Logger(Debug, "Dataset size = %d. Offset = %d. Time = %g sec.\n", m_datasetSize, m_offset, m_datasetSize*GetGlobalDt());

                        for (uint i=0; i<m_numberOfInputs; i++) {

                                // extend the dataset
                                status = H5Dset_extent(m_datasets[i], &m_datasetSize);
                                if (status < 0)
                                        throw "Unable to extend dataset.";
                                else
                                        Logger(Debug, "Extended dataset.\n");

                                // get the filespace
                                filespace = H5Dget_space(m_datasets[i]);
                                if (filespace < 0)
                                        throw "Unable to get filespace.";
                                else
                                        Logger(Debug, "Obtained filespace.\n");

                                // select an hyperslab
                                status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, &m_offset, NULL, &m_bufferLengths[bufferToSave], NULL);
                                if (status < 0) {
                                        H5Sclose(filespace);
                                        throw "Unable to select hyperslab.";
                                }
                                else {
                                        Logger(Debug, "Selected hyperslab.\n");
                                }

                                // define memory space
                                m_dataspaces[i] = H5Screate_simple(rank, &m_bufferLengths[bufferToSave], NULL);
                                if (m_dataspaces[i] < 0) {
                                        H5Sclose(filespace);
                                        throw "Unable to define memory space.";
                                }
                                else {
                                        Logger(Debug, "Memory space defined.\n");
                                }

                                // write data
                                status = H5Dwrite(m_datasets[i], H5T_IEEE_F64LE, m_dataspaces[i], filespace, H5P_DEFAULT, m_data[i][bufferToSave]);
                                if (status < 0) {
                                        H5Sclose(filespace);
                                        throw "Unable to write data.";
                                }
                                else {
                                        Logger(Debug, "Written data.\n");
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

void H5Recorder::addPre(Entity *entity)
{
        Entity::addPre(entity);

        Logger(Debug, "--- H5Recorder::addPre(Entity*, double) ---\n");

        hid_t cparms;

        m_numberOfInputs++;
        uint k = m_numberOfInputs-1;
        double **buffer = new double*[m_numberOfBuffers];
        for (uint i=0; i<m_numberOfBuffers; i++)
                buffer[i] = new double[bufferSize];
        m_data.push_back(buffer);

        herr_t status;

        // the name of the dataset
        char datasetName[DATASET_NAME_LEN];
        sprintf(datasetName, "/Data/Entity-%04d", m_numberOfInputs, entity->id());

        // create the dataspace with unlimited dimensions
        m_dataspaces.push_back(H5Screate_simple(rank, &bufferSize, &unlimitedSize));
        if (m_dataspaces[k] < 0)
                throw "Unable to create dataspace.";
        else
                Logger(Debug, "Dataspace created\n");

        // modify dataset creation properties, i.e. enable chunking.
        cparms = H5Pcreate(H5P_DATASET_CREATE);
        if (cparms < 0) {
                H5Sclose(m_dataspaces[k]);
                throw "Unable to create dataset properties.";
        }
        else {
                Logger(Debug, "Dataset properties created.\n");
        }

        status = H5Pset_chunk(cparms, rank, &chunkSize);
        if (status < 0) {
                H5Sclose(m_dataspaces[k]);
                H5Pclose(cparms);
                throw "Unable to set chunking.";
        }
        else {
                Logger(Debug, "Chunking set.\n");
        }

        status = H5Pset_fill_value(cparms, H5T_IEEE_F64LE, &fillValue);
        if (status < 0) {
                H5Sclose(m_dataspaces[k]);
                H5Pclose(cparms);
                throw "Unable to set fill value.";
        }
        else {
                Logger(Debug, "Fill value set.\n");
        }

        // create a new dataset within the file using cparms creation properties.
        m_datasets.push_back(H5Dcreate2(m_fid, datasetName,
                                        H5T_IEEE_F64LE, m_dataspaces[k],
                                        H5P_DEFAULT, cparms, H5P_DEFAULT));
        if (m_datasets[k] < 0) {
                H5Sclose(m_dataspaces[k]);
                H5Pclose(cparms);
                throw "Unable to create a dataset.";
        }
        else {
                Logger(Debug, "Dataset created.\n");
        }

        H5Pclose(cparms);

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
        
        Logger(Debug, "Checking whether GZIP compression is available...");
        // check if gzip compression is available
        avail = H5Zfilter_avail (H5Z_FILTER_DEFLATE);
        if (!avail) {
                Logger(Debug, "\nGZIP compression is not available on this system.\n");
                return H5_NO_GZIP_COMPRESSION;
        }
        Logger(Debug, " ok.\nGetting filter info...");
        status = H5Zget_filter_info (H5Z_FILTER_DEFLATE, &filterInfo);
        if (!(filterInfo & H5Z_FILTER_CONFIG_ENCODE_ENABLED)) {
                Logger(Debug, "\nUnable to get filter info: disabling compression.\n");
                return H5_NO_FILTER_INFO;
        }
        Logger(Debug, " ok.\nChecking whether the shuffle filter is available...");
        // check for availability of the shuffle filter.
        avail = H5Zfilter_avail(H5Z_FILTER_SHUFFLE);
        if (!avail) {
                Logger(Debug, "\nThe shuffle filter is not available on this system.\n");
                return H5_NO_SHUFFLE_FILTER;
        }
        Logger(Debug, " ok.\nGetting filter info...");
        status = H5Zget_filter_info (H5Z_FILTER_SHUFFLE, &filterInfo);
        if ( !(filterInfo & H5Z_FILTER_CONFIG_ENCODE_ENABLED) ) {
                Logger(Debug, "Unable to get filter info: disabling compression.\n");
                return H5_NO_FILTER_INFO;
        }
        Logger(Debug, " ok.\nCompression is enabled.\n");
        return OK;
}

int H5Recorder::open(const char *filename, bool compress)
{
        Logger(Debug, "--- H5Recorder::open(const char*, bool) ---\n");
        Logger(Debug, "Opening file %s.\n", filename);

        if(!compress || isCompressionAvailable() != OK)
                m_compressed = false;
        else
                m_compressed = true;

        m_fid = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
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
        if(m_compressed) {
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

        status = H5Awrite(attr, H5T_IEEE_F64LE, &m_dt);

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

void H5Recorder::close()
{
        Logger(Debug, "--- H5Recorder::close() ---\n");
        Logger(Debug, "Closing file.\n");
        if (m_fid != -1) {

                hid_t dataset = H5Dopen2(m_fid, "/Misc/Simulation_properties", H5P_DEFAULT);
                if (dataset >= 0) {
                        hid_t aid = H5Screate(H5S_SCALAR);
                        hid_t attr = H5Acreate2(dataset, "tend", H5T_IEEE_F64LE, aid, H5P_DEFAULT, H5P_DEFAULT);
                        double tend = GetGlobalTime() - m_dt;
                        H5Awrite(attr, H5T_IEEE_F64LE, &tend);
                        H5Aclose(attr);
                        H5Sclose(aid);
                        H5Dclose(dataset);
                }

                if(m_compressed)
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

