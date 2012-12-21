#include <sstream>
#include <string.h>
#include <time.h>       // for adding timestamp to H5 files
#include "recorders.h"
#include "engine.h"

dynclamp::Entity* ASCIIRecorderFactory(string_dict& args)
{
        uint id;
        std::string filename;
        id = dynclamp::GetIdFromDictionary(args);
        if (!dynclamp::CheckAndExtractValue(args, "filename", filename))
                return new dynclamp::recorders::ASCIIRecorder((const char *) NULL, id);
        return new dynclamp::recorders::ASCIIRecorder(filename.c_str(), id);
}

dynclamp::Entity* H5RecorderFactory(string_dict& args)
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
{
        setName("Recorder");
}

double Recorder::output()
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
        setName("ASCIIRecorder");
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

bool ASCIIRecorder::initialise()
{       
        closeFile();
        if (m_makeFilename)
                MakeFilename(m_filename, "dat");
        openFile();
        return true;
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

void ASCIIRecorder::terminate()
{
        closeFile();
}

//~~~

BaseH5Recorder::BaseH5Recorder(uint id)
        : Recorder(id)
{}

//~~~

const hsize_t H5Recorder::rank            = 1;
const hsize_t H5Recorder::unlimitedSize   = H5S_UNLIMITED;
const uint    H5Recorder::numberOfBuffers = 2;
const hsize_t H5Recorder::chunkSize       = 1024;
const uint    H5Recorder::numberOfChunks  = 20;
const hsize_t H5Recorder::bufferSize      = 20480;    // This MUST be equal to chunkSize times numberOfChunks.
const double  H5Recorder::fillValue       = 0.0;

H5Recorder::H5Recorder(bool compress, const char *filename, uint id)
        : BaseH5Recorder(id), m_fid(-1),
          m_data(), m_numberOfInputs(0),
          m_threadRun(false),
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

        if (compress && isCompressionAvailable())
                m_compress = true;
        else
                m_compress = false;

        m_bufferLengths = new hsize_t[numberOfBuffers];
        setName("H5Recorder");
}

H5Recorder::~H5Recorder()
{
        stopWriterThread();
        closeFile();
        uint i, j;
        for (i=0; i<m_numberOfInputs; i++) {
                for(j=0; j<numberOfBuffers; j++)
                        delete m_data[i][j];
                delete m_data[i];
        }
        delete m_bufferLengths;
}

bool H5Recorder::initialise()
{
        if (m_inputs.size() == 0) {
                Logger(Critical, "H5Recorder::initialise() >> There are no entities connected to this H5Recorder. Probably you don't want this.\n");
                return false;
        }

        stopWriterThread();
        closeFile();

        if (m_makeFilename)
                MakeFilename(m_filename, "h5");

        m_bufferInUse = numberOfBuffers-1;
        m_bufferPosition = 0;
        m_offset = 0;
        m_datasetSize = 0;
        m_groups.clear();
        m_dataspaces.clear();
        m_datasets.clear();

        if (!openFile())
                return false;
        Logger(Debug, "Successfully opened file [%s].\n", m_filename);

        if (!createGroup(INFO_GROUP, &m_infoGroup)) {
                Logger(Critical, "Unable to create Info group.\n");
                return false;
        }
        Logger(Debug, "Successfully created Info group.\n");
        writeScalarAttribute(m_infoGroup, "version", H5_FILE_VERSION);
        writeScalarAttribute(m_infoGroup, "dt", GetGlobalDt());

        hid_t grp;
        if (!createGroup(ENTITIES_GROUP, &grp)) {
                Logger(Critical, "Unable to create Entities group.\n");
                return false;
        }
        Logger(Debug, "Successfully created Entities group.\n");

        for (uint i=0; i<m_pre.size(); i++) {
                if (!allocateForEntity(m_pre[i]))
                        return false;
        }

        startWriterThread();
        
        return true;
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

void H5Recorder::terminate()
{
        stopWriterThread();
        closeFile();
}

void H5Recorder::step()
{
        if (m_numberOfInputs == 0)
                return;

        if (m_bufferPosition == 0)
        {
                boost::unique_lock<boost::mutex> lock(m_mutex);
                while (m_dataQueue.size() == numberOfBuffers) {
                        Logger(Debug, "Main thread: the data queue is full.\n");
                        m_cv.wait(lock);
                }
                m_bufferInUse = (m_bufferInUse+1) % numberOfBuffers;
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

bool H5Recorder::writeData(const std::string& datasetName, int rank, const hsize_t *dims, const double *data, const std::string& label)
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

        dset = H5Dcreate2(m_fid, datasetName.c_str(), H5T_IEEE_F64LE, dspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (dset < 0) {
                Logger(Critical, "Unable to create dataset [%s].\n", datasetName.c_str());
                return false;
        }
        Logger(Debug, "Successfully created dataset [%s].\n", datasetName.c_str());

        status = H5Dwrite(dset, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
        if (status < 0) {
                Logger(Critical, "Unable to write data to dataset [%s].\n", datasetName.c_str());
                H5Dclose(dset);
                H5Sclose(dspace);
                return false;
        }
        Logger(Debug, "Successfully written data.\n");

        if (label.size() > 0)
                writeStringAttribute(dset, "Label", label);

        H5Dclose(dset);
        H5Sclose(dspace);

        return true;
}

bool H5Recorder::createUnlimitedDataset(const std::string& datasetName, hid_t *dspace, hid_t *dset)
{
        herr_t status;
        
        // create the dataspace with unlimited dimensions
        *dspace = H5Screate_simple(rank, &bufferSize, &unlimitedSize);
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
                H5Sclose(*dspace);
                Logger(Critical, "Unable to create dataset properties.\n");
                return false;
        }
        else {
                Logger(Debug, "Dataset properties created.\n");
        }

        status = H5Pset_chunk(cparms, rank, &chunkSize);
        if (status < 0) {
                H5Sclose(*dspace);
                H5Pclose(cparms);
                Logger(Critical, "Unable to set chunking.\n");
                return false;
        }
        else {
                Logger(Debug, "Chunking set.\n");
        }

        status = H5Pset_fill_value(cparms, H5T_IEEE_F64LE, &fillValue);
        if (status < 0) {
                H5Sclose(*dspace);
                H5Pclose(cparms);
                Logger(Critical, "Unable to set fill value.\n");
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
                        Logger(Debug, "Successfully enabled ompression.\n");
        }

        // create a new dataset within the file using cparms creation properties.
        *dset = H5Dcreate2(m_fid, datasetName.c_str(),
                           H5T_IEEE_F64LE, *dspace,
                           H5P_DEFAULT, cparms, H5P_DEFAULT);
        if (*dset < 0) {
                H5Sclose(*dspace);
                H5Pclose(cparms);
                Logger(Critical, "Unable to create dataset.\n");
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

bool H5Recorder::allocateForEntity(Entity *entity)
{
        hid_t dspace, dset, grp;
        char groupName[GROUP_NAME_LEN];
        char datasetName[DATASET_NAME_LEN];

        // the name of the group (i.e., /Entities/0001)
        sprintf(groupName, "%s/%04d", ENTITIES_GROUP, entity->id());
        if (!createGroup(groupName, &grp)) {
                Logger(Critical, "Unable to create group [%s].\n", groupName);
                return false;
        }
        Logger(Debug, "Group [%s] created.\n", groupName);

        // dataset for actual data (i.e., /Entities/0001/Data)
        sprintf(datasetName, "%s/%04d/%s", ENTITIES_GROUP, entity->id(), DATA_DATASET);
        if (!createUnlimitedDataset(datasetName, &dspace, &dset)) {
                Logger(Critical, "Unable to create dataset [%s].\n", datasetName);
                return false;
        }
        Logger(Debug, "Dataset [%s] created.\n", datasetName);

        // save name and units as attributes of the group
        writeStringAttribute(grp, "Name", entity->name());
        writeStringAttribute(grp, "Units", entity->units());

        // save metadata
        size_t ndims;
        if (entity->hasMetadata(&ndims)) {
                char label[LABEL_LEN];
                size_t *dims = new size_t[ndims];
                hsize_t *hdims = new hsize_t[ndims];
                const double *metadata = entity->metadata(dims, label);
                for (int i=0; i<ndims; i++)
                        hdims[i] = dims[i];
                sprintf(datasetName, "%s/%04d/%s", ENTITIES_GROUP, entity->id(), METADATA_DATASET);
                if (! writeData(datasetName, (int) ndims, hdims, metadata, label)) {
                        Logger(Critical, "Unable to create metadata dataset.\n");
                        delete hdims;
                        delete dims;
                        return false;
                }
                delete hdims;
                delete dims;
        } 
        
        // group for the parameters (i.e., /Entities/0001/Parameters)
        sprintf(groupName, "%s/%04d/%s", ENTITIES_GROUP, entity->id(), PARAMETERS_GROUP);
        if (!createGroup(groupName, &grp)) {
                Logger(Critical, "Unable to create group [%s].\n", groupName);
                return false;
        }
        Logger(Debug, "Group [%s] created.\n", groupName);


        double_dict::const_iterator it;
        for (it = entity->parameters().begin(); it != entity->parameters().end(); it++)
                writeScalarAttribute(grp, it->first, it->second);

        return true;
}

void H5Recorder::addPre(Entity *entity)
{
        Entity::addPre(entity);

        Logger(All, "--- H5Recorder::addPre(Entity*, double) ---\n");

        m_numberOfInputs++;
        double **buffer = new double*[numberOfBuffers];
        for (uint i=0; i<numberOfBuffers; i++)
                buffer[i] = new double[bufferSize];
        m_data.push_back(buffer);
}

bool H5Recorder::writeArrayAttribute(hid_t dataset, const std::string& attrName,
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

        attr = H5Acreate2(dataset, attrName.c_str(), H5T_IEEE_F64LE, dspace, H5P_DEFAULT, H5P_DEFAULT);
        if (status < 0) {
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

bool H5Recorder::writeStringAttribute(hid_t dataset,
                                      const std::string& attrName,
                                      const std::string& attrValue)
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

        status = H5Tset_size(atype, attrValue.size() + 1);
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

        attr = H5Acreate2(dataset, attrName.c_str(), atype, dspace, H5P_DEFAULT, H5P_DEFAULT);
        if (status < 0) {
                Logger(Critical, "Error in H5Acreate2.\n");
                H5Tclose(atype);
                H5Sclose(dspace);
                return false;
        }
        Logger(Debug, "Successfully created attribute.\n");

        status = H5Awrite(attr, atype, attrValue.c_str());
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


bool H5Recorder::isCompressionAvailable() const
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

bool H5Recorder::openFile()
{
        Logger(All, "--- H5Recorder::openFile() ---\n");
        Logger(All, "Opening file %s.\n", m_filename);

        m_fid = H5Fcreate(m_filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        if(m_fid < 0)
                return false;

        time_t now;
        char buf[26];
        std::stringstream timestamp;
        time(&now);
        ctime_r(&now, buf);
        // to remove newline at the end of the string returned by ctime
        buf[24] = 0;
        timestamp << "File created on " << buf << ".";
        writeStringAttribute(m_fid, "Timestamp", timestamp.str());

        return true;
}

bool H5Recorder::createGroup(const std::string& groupName, hid_t *grp)
{
        *grp = H5Gcreate2(m_fid, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (*grp < 0)
                return false;
        m_groups.push_back(*grp);
        return true;
}

bool H5Recorder::writeScalarAttribute(hid_t dataset, const std::string& attrName, double attrValue)
{
        hid_t aid, attr;
        herr_t status;

        aid = H5Screate(H5S_SCALAR);
        if (aid < 0) {
                Logger(Critical, "Unable to create scalar.\n");
                return false;
        }

        attr = H5Acreate2(dataset, attrName.c_str(), H5T_IEEE_F64LE, aid, H5P_DEFAULT, H5P_DEFAULT);
        if (attr < 0) {
                H5Sclose(aid);
                Logger(Critical, "Unable to create attribute.\n");
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

void H5Recorder::closeFile()
{
        Logger(All, "--- H5Recorder::closeFile() ---\n");
        Logger(All, "Closing file.\n");
        if (m_fid != -1) {
                writeScalarAttribute(m_infoGroup, "tend", GetGlobalTime() - GetGlobalDt());
                for (uint i=0; i<m_numberOfInputs; i++) {
                        H5Dclose(m_datasets[i]);
                        H5Sclose(m_dataspaces[i]);
                }
                for (int i=0; i<m_groups.size(); i++)
                        H5Gclose(m_groups[i]);
                H5Fclose(m_fid);
                m_fid = -1;
        }
}

} // namespace recorders

} // namespace dynclamp

