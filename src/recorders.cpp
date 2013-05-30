#include <sstream>
#include <string.h>
#include <time.h>       // for adding timestamp to H5 files
#include "recorders.h"
#include "engine.h"
#include "common.h"

lcg::Entity* ASCIIRecorderFactory(string_dict& args)
{
        uint id;
        std::string filename;
        id = lcg::GetIdFromDictionary(args);
        if (!lcg::CheckAndExtractValue(args, "filename", filename))
                return new lcg::recorders::ASCIIRecorder((const char *) NULL, id);
        return new lcg::recorders::ASCIIRecorder(filename.c_str(), id);
}

lcg::Entity* H5RecorderFactory(string_dict& args)
{       
        uint id;
        std::string filename;
        bool compress;
        id = lcg::GetIdFromDictionary(args);
        if (!lcg::CheckAndExtractBool(args, "compress", &compress))
                compress = true;
        if (!lcg::CheckAndExtractValue(args, "filename", filename))
                return new lcg::recorders::H5Recorder(compress, NULL, id);
        return new lcg::recorders::H5Recorder(compress, filename.c_str(), id);
}

lcg::Entity* TriggeredH5RecorderFactory(string_dict& args)
{       
        uint id;
        double before, after;
        std::string filename;
        bool compress;
        id = lcg::GetIdFromDictionary(args);
        if (!lcg::CheckAndExtractDouble(args, "before", &before) ||
            !lcg::CheckAndExtractDouble(args, "after", &after)) {
                lcg::Logger(lcg::Critical, "Unable to build a TriggeredH5Recorder.\n");
                return NULL;
        }
        if (!lcg::CheckAndExtractBool(args, "compress", &compress))
                compress = true;
        if (!lcg::CheckAndExtractValue(args, "filename", filename))
                return new lcg::recorders::TriggeredH5Recorder(before, after, compress, NULL, id);
        return new lcg::recorders::TriggeredH5Recorder(before, after, compress, filename.c_str(), id);
}

namespace lcg {

namespace recorders {

Recorder::Recorder(uint id) : Entity(id), m_comments()
{
        setName("Recorder");
}

Recorder::~Recorder()
{
        deleteComments();
}

void Recorder::deleteComments()
{
        std::deque<Comment*>::iterator it;
        for (it=m_comments.begin(); it!=m_comments.end(); it++)
                delete *it;
}

double Recorder::output()
{
        return 0.0;
}

void Recorder::addComment(const char *message, const time_t *timestamp)
{
        m_comments.push_back(new Comment(message, timestamp));
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

const hsize_t BaseH5Recorder::unlimitedSize = H5S_UNLIMITED;
const double  BaseH5Recorder::fillValue     = 0.0;

BaseH5Recorder::BaseH5Recorder(bool compress, hsize_t bufferSize, const char *filename, uint id)
        : Recorder(id), m_fid(-1), m_bufferSize(bufferSize),
          m_numberOfInputs(0), m_groups(), m_dataspaces(), m_datasets()
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

BaseH5Recorder::BaseH5Recorder(bool compress, hsize_t chunkSize, uint numberOfChunks, const char *filename, uint id)
        : Recorder(id), m_fid(-1), m_bufferSize(chunkSize*numberOfChunks),
          m_chunkSize(chunkSize), m_numberOfChunks(numberOfChunks),
          m_numberOfInputs(0), m_groups(), m_dataspaces(), m_datasets()
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

}

BaseH5Recorder::~BaseH5Recorder()
{}

hsize_t BaseH5Recorder::bufferSize() const
{
        return m_bufferSize;
}

hsize_t BaseH5Recorder::chunkSize() const
{
        return m_chunkSize;
}

uint BaseH5Recorder::numberOfChunks() const
{
        return m_numberOfChunks;
}

bool BaseH5Recorder::isCompressionAvailable() const
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

bool BaseH5Recorder::initialise()
{
        Logger(Debug, "BaseH5Recorder::initialise()\n");

        if (m_inputs.size() == 0) {
                Logger(Critical, "BaseH5Recorder::initialise() >> There are no entities "
                                 "connected to this BaseH5Recorder. Probably you don't want this.\n");
                return false;
        }

        closeFile();

        if (m_makeFilename)
                MakeFilename(m_filename, "h5");

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

        return finaliseInit();
}

bool BaseH5Recorder::openFile()
{
        Logger(All, "--- BaseH5Recorder::openFile() ---\n");
        Logger(All, "Opening file %s.\n", m_filename);

        m_fid = H5Fcreate(m_filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        if(m_fid < 0)
                return false;

        addComment("Opened file.");
        return true;
}

void BaseH5Recorder::closeFile()
{
        Logger(All, "--- BaseH5Recorder::closeFile() ---\n");
        if (m_fid != -1) {
                writeScalarAttribute(m_infoGroup, "tend", GetGlobalTime() - GetGlobalDt());
                addComment("Closed file.");
                writeComments();
                for (int i=0; i<m_numberOfInputs; i++) {
                        H5Dclose(m_datasets[i]);
                        H5Sclose(m_dataspaces[i]);
                }
                for (int i=0; i<m_groups.size(); i++)
                        H5Gclose(m_groups[i]);
                //H5Gclose(m_infoGroup);
                //H5Gclose(m_commentsGroup);
                H5Fclose(m_fid);
                m_fid = -1;
        }
}

void BaseH5Recorder::terminate()
{
        Logger(Debug, "BaseH5Recorder::terminate()\n");
        closeFile();
}

const char* BaseH5Recorder::filename() const
{
        return m_filename;
}

void BaseH5Recorder::writeComments()
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

#if defined(HAVE_LIBRT)
void BaseH5Recorder::reducePriority() const
{
        int priority;
        struct sched_param schedp;
        priority = sched_get_priority_max(SCHEDULER);
        if (priority > 0) {
                Logger(Debug, "The maximum priority is %d.\n", priority);
	        memset(&schedp, 0, sizeof(schedp));
	        schedp.sched_priority = priority-1;
                if (sched_setscheduler(0, SCHEDULER, &schedp) == 0) {
                        Logger(Debug, "Successfully set the priority of the writing thread to %d.\n", priority-1);
                }
                else {
                        Logger(Info, "Unable to set the priority of the writing thread to %d: "
                                "it will run at the same priority of the parent thread.\n", priority-1);
                }
        }
        else {
                Logger(Info, "Unable to get maximum priority: "
                        "the writing thread will run at the same priority of the parent thread.\n");
        }
}
#endif

void BaseH5Recorder::addPre(Entity *entity)
{
        Entity::addPre(entity);
        Logger(All, "--- BaseH5Recorder::addPre(Entity*) ---\n");
        m_numberOfInputs++;
        finaliseAddPre(entity);
}

bool BaseH5Recorder::allocateForEntity(Entity *entity, int dataRank,
                                       const hsize_t *dataDims, const hsize_t *maxDataDims, const hsize_t *chunkDims)
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
        if (!createUnlimitedDataset(datasetName, dataRank, dataDims, maxDataDims, chunkDims, &dspace, &dset)) {
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

bool BaseH5Recorder::createGroup(const std::string& groupName, hid_t *grp)
{
        *grp = H5Gcreate2(m_fid, groupName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (*grp < 0)
                return false;
        m_groups.push_back(*grp);
        return true;
}

bool BaseH5Recorder::createUnlimitedDataset(const std::string& datasetName,
                                            int rank, const hsize_t *dataDims, const hsize_t *maxDataDims, const hsize_t *chunkDims,
                                            hid_t *dspace, hid_t *dset)
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
                        Logger(Debug, "Successfully enabled ompression.\n");
        }

        // create a new dataset within the file using cparms creation properties.
        *dset = H5Dcreate2(m_fid, datasetName.c_str(),
                           H5T_IEEE_F64LE, *dspace,
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

bool BaseH5Recorder::writeStringAttribute(hid_t dataset,
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
        if (attr < 0) {
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

bool BaseH5Recorder::writeScalarAttribute(hid_t dataset, const std::string& attrName, double attrValue)
{
        Logger(Debug, "BaseH5Recorder::writeScalarAttribute(%d, %s, %g)\n", dataset, attrName.c_str(), attrValue);
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

bool BaseH5Recorder::writeArrayAttribute(hid_t dataset, const std::string& attrName,
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

bool BaseH5Recorder::writeData(const std::string& datasetName, int rank, const hsize_t *dims,
                               const double *data, const std::string& label)
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

//~~~

const uint H5Recorder::numberOfBuffers = 2;
const int  H5Recorder::rank            = 1;

H5Recorder::H5Recorder(bool compress, const char *filename, uint id)
        : BaseH5Recorder(compress, 1024, 20, filename, id), // 1024 = chunkSize and 20 = numberOfChunks
          m_data(),
          m_threadRun(false),
          m_mutex(), m_cv()
{
        m_bufferLengths = new hsize_t[H5Recorder::numberOfBuffers];
        setName("H5Recorder");
}

H5Recorder::~H5Recorder()
{
        terminate();
        for (int i=0; i<m_numberOfInputs; i++) {
                for(int j=0; j<H5Recorder::numberOfBuffers; j++)
                        delete m_data[i][j];
                delete m_data[i];
        }
        delete m_bufferLengths;
}

bool H5Recorder::finaliseInit()
{
        Logger(Debug, "H5Recorder::finaliseInit()\n");
        hsize_t bufsz = bufferSize(), maxbufsz = H5S_UNLIMITED, chunksz = chunkSize();

        stopWriterThread();

        m_bufferInUse = H5Recorder::numberOfBuffers-1;
        m_bufferPosition = 0;
        m_datasetSize = 0;
        
        for (int i=0; i<m_pre.size(); i++) {
                if (!allocateForEntity(m_pre[i], H5Recorder::rank, &bufsz, &maxbufsz, &chunksz))
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
        BaseH5Recorder::terminate();
}

void H5Recorder::step()
{
        if (m_numberOfInputs == 0)
                return;

        if (m_bufferPosition == 0)
        {
                boost::unique_lock<boost::mutex> lock(m_mutex);
                while (m_dataQueue.size() == H5Recorder::numberOfBuffers) {
                        Logger(Debug, "Main thread: the data queue is full.\n");
                        m_cv.wait(lock);
                }
                m_bufferInUse = (m_bufferInUse+1) % H5Recorder::numberOfBuffers;
                m_bufferLengths[m_bufferInUse] = 0;
                Logger(Debug, "H5Recorder::step() >> Starting to write in buffer #%d @ t = %g.\n", m_bufferInUse, GetGlobalTime());
        }

        for (int i=0; i<m_numberOfInputs; i++)
                m_data[i][m_bufferInUse][m_bufferPosition] = m_inputs[i];
        m_bufferLengths[m_bufferInUse]++;
        m_bufferPosition = (m_bufferPosition+1) % bufferSize();

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

#if defined(HAVE_LIBRT)
        //reducePriority();
#endif

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
                hsize_t offset;

                if (m_bufferLengths[bufferToSave] > 0) {

                        offset = m_datasetSize;
                        m_datasetSize += m_bufferLengths[bufferToSave];

                        Logger(Debug, "Dataset size = %d.\n", m_datasetSize);
                        Logger(Debug, "Offset = %d.\n", offset);
                        Logger(Debug, "Time = %g sec.\n", m_datasetSize*GetGlobalDt());

                        for (int i=0; i<m_numberOfInputs; i++) {

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
                                status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, &offset, NULL, &m_bufferLengths[bufferToSave], NULL);
                                if (status < 0) {
                                        H5Sclose(filespace);
                                        throw "Unable to select hyperslab.";
                                }
                                else {
                                        Logger(All, "Selected hyperslab.\n");
                                }

                                // define memory space
                                m_dataspaces[i] = H5Screate_simple(H5Recorder::rank, &m_bufferLengths[bufferToSave], NULL);
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

void H5Recorder::finaliseAddPre(Entity *entity)
{
        Logger(All, "--- H5Recorder::finaliseAddPre(Entity*) ---\n");
        double **buffer = new double*[H5Recorder::numberOfBuffers];
        for (int i=0; i<H5Recorder::numberOfBuffers; i++)
                buffer[i] = new double[bufferSize()];
        m_data.push_back(buffer);
}

//~~~

const int TriggeredH5Recorder::rank = 2;
const uint TriggeredH5Recorder::numberOfBuffers = 2;

TriggeredH5Recorder::TriggeredH5Recorder(double before, double after, bool compress, const char *filename, uint id)
        : BaseH5Recorder(compress, ceil((before+after)/GetGlobalDt()), filename, id),
          m_recording(false), m_data(), m_bufferPosition(0), m_bufferInUse(0),
          m_maxSteps(ceil(after/GetGlobalDt())), m_nSteps(0),
          m_writerThread()
{
        setName("TriggeredH5Recorder");
        m_tempData = new double[bufferSize()];
}

TriggeredH5Recorder::~TriggeredH5Recorder()
{
        Logger(Debug, "TriggeredH5Recorder::~TriggeredH5Recorder()\n");
        terminate();
        for (int i=0; i<m_numberOfInputs; i++) {
                for (int j=0; j<TriggeredH5Recorder::numberOfBuffers; j++)
                        delete m_data[i][j];
                delete m_data[i];
        }
        delete m_tempData;
}

void TriggeredH5Recorder::step()
{
        // store the data
        for (int i=0; i<m_numberOfInputs; i++)
                m_data[i][m_bufferInUse][m_bufferPosition] = m_inputs[i];
        m_bufferPosition = (m_bufferPosition + 1) % bufferSize();
        if (m_recording) {
                m_nSteps++;
                if (m_nSteps == m_maxSteps) {
                        m_recording = false;
                        // start the writing thread
                        m_writerThread = boost::thread(&TriggeredH5Recorder::buffersWriter, this, m_bufferInUse, m_bufferPosition);
                        m_bufferInUse = (m_bufferInUse + 1) % TriggeredH5Recorder::numberOfBuffers;
                        m_bufferPosition = 0;
                }
        }
}

void TriggeredH5Recorder::terminate()
{
        Logger(Debug, "TriggeredH5Recorder::terminate()\n");
        if (m_recording) { // we were recording when the experiment ended, damn it!
                Logger(Info, "TriggeredH5Recorder::terminate() called while recording.\n");
                // fill the remaining part of the buffers with NaNs
                for ( ; m_nSteps<m_maxSteps; m_nSteps++) {
                        for (int i=0; i<m_numberOfInputs; i++)
                                m_data[i][m_bufferInUse][m_bufferPosition] = NOT_A_NUMBER;
                        m_bufferPosition = (m_bufferPosition + 1) % bufferSize();
                }
                m_recording = false;
                // start the writing thread, to save the remaining data.
                m_writerThread = boost::thread(&TriggeredH5Recorder::buffersWriter, this, m_bufferInUse, m_bufferPosition);
        }
        m_writerThread.join();
        BaseH5Recorder::terminate();
}

void TriggeredH5Recorder::handleEvent(const Event *event)
{
        if (event->type() == TRIGGER) {
                if (!m_recording) {
                        // wait for the writing thread to finish
                        m_writerThread.join();
                        Logger(Debug, "TriggeredH5Recorder::handleEvent >> started recording.\n");
                        m_recording = true;
                        m_nSteps = 0;
                }
                else {
                        // we're already recording, this CAN'T happen
                        Logger(Critical, "TriggeredH5Recorder: received a Trigger event while already recording: "
                                         "ignoring it, but probably there's something wrong here.\n");
                }
        }
}

bool TriggeredH5Recorder::finaliseInit()
{
        Logger(Debug, "TriggeredH5Recorder::finaliseInit()\n");
        hsize_t dataDims[TriggeredH5Recorder::rank] = {bufferSize(), 1};
        hsize_t maxDataDims[TriggeredH5Recorder::rank] = {bufferSize(), H5S_UNLIMITED};
        hsize_t chunkDims[TriggeredH5Recorder::rank] = {chunkSize(), 1};
        m_writerThread.join();
        m_bufferPosition = 0;
        m_bufferInUse = 0;
        m_nSteps = 0;
        m_datasetSize[0] = bufferSize();
        m_datasetSize[1] = 0;
        for (int i=0; i<m_pre.size(); i++) {
                if (!allocateForEntity(m_pre[i], TriggeredH5Recorder::rank, dataDims, maxDataDims, chunkDims))
                        return false;
        }
        return true;
}

void TriggeredH5Recorder::finaliseAddPre(Entity *entity)
{
        Logger(Debug, "TriggeredH5Recorder::finaliseAddPre(Entity*)\n");
        double **buffer = new double*[TriggeredH5Recorder::numberOfBuffers];
        for (int i=0; i<TriggeredH5Recorder::numberOfBuffers; i++)
                buffer[i] = new double[bufferSize()];
        m_data.push_back(buffer);
}

void TriggeredH5Recorder::buffersWriter(uint bufferToSave, uint bufferPosition)
{

        Logger(Debug, "TriggeredH5Recorder::buffersWriter: will save buffer #%d starting from pos %d.\n",
                        bufferToSave, bufferPosition);

#if defined(HAVE_LIBRT)
        reducePriority();
#endif

        hid_t filespace;
        herr_t status;

        hsize_t start[TriggeredH5Recorder::rank], count[TriggeredH5Recorder::rank];
        start[0] = 0;
        start[1] = m_datasetSize[1];
        count[0] = bufferSize();
        count[1] = 1;
        m_datasetSize[1]++;

        Logger(Debug, "Dataset size = (%dx%d).\n", m_datasetSize[0], m_datasetSize[1]);
        Logger(Debug, "Offset = (%d,%d).\n", start[0], start[1]);

        uint offset = bufferSize() - bufferPosition;
        Logger(Debug, "buffer size = %d.\n", bufferSize());
        Logger(Debug, "buffer position = %d.\n", bufferPosition);
        Logger(Debug, "offset = %d\n", offset);
        for (int i=0; i<m_numberOfInputs; i++) {

                // extend the dataset
                status = H5Dset_extent(m_datasets[i], m_datasetSize);
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
                status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start, NULL, count, NULL);
                if (status < 0) {
                        H5Sclose(filespace);
                        throw "Unable to select hyperslab.";
                }
                else {
                        Logger(All, "Selected hyperslab.\n");
                }

                // define memory space
                m_dataspaces[i] = H5Screate_simple(TriggeredH5Recorder::rank, count, NULL);
                if (m_dataspaces[i] < 0) {
                        H5Sclose(filespace);
                        throw "Unable to define memory space.";
                }
                else {
                        Logger(All, "Memory space defined.\n");
                }

                memcpy(m_tempData, m_data[i][bufferToSave]+bufferPosition, offset*sizeof(double));
                memcpy(m_tempData+offset, m_data[i][bufferToSave], bufferPosition*sizeof(double));
                // write data
                status = H5Dwrite(m_datasets[i], H5T_IEEE_F64LE, m_dataspaces[i], filespace, H5P_DEFAULT, m_tempData);
                if (status < 0) {
                        H5Sclose(filespace);
                        throw "Unable to write data.";
                }
                else {
                        Logger(All, "Written data.\n");
                }
        }
        H5Sclose(filespace);
        
        Logger(Debug, "TriggeredH5Recorder::buffersWriter terminated.\n");
}

} // namespace recorders

} // namespace lcg

