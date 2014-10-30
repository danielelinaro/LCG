#include <sstream>
#include <time.h>
#include <sys/time.h>       // for adding timestamp to H5 files
#include "recorders.h"
#include "common.h"
#ifdef REALTIME_ENGINE
#include "engine.h"
#endif

/*
 * Taken from <https://gist.github.com/jbenet/1087739>.
 * START
 */
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif
 
void current_utc_time(struct timespec *ts) {
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
        clock_serv_t cclock;
        mach_timespec_t mts;
        host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
        clock_get_time(cclock, &mts);
        mach_port_deallocate(mach_task_self(), cclock);
        ts->tv_sec = mts.tv_sec;
        ts->tv_nsec = mts.tv_nsec;
#else
        clock_gettime(CLOCK_REALTIME, ts);
#endif
}
/* END */

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

Recorder::Recorder(uint id) : Entity(id)
{
        setName("Recorder");
}

Recorder::~Recorder()
{
}

double Recorder::output()
{
        return 0.0;
}

//~~~

BaseH5Recorder::BaseH5Recorder(bool compress, hsize_t bufferSize, const char *filename, uint id)
        : H5RecorderCore(compress, bufferSize, filename), Recorder(id), m_numberOfInputs(0)
{}

BaseH5Recorder::BaseH5Recorder(bool compress, hsize_t chunkSize, uint numberOfChunks, const char *filename, uint id)
        : H5RecorderCore(compress, chunkSize, numberOfChunks, filename), Recorder(id), m_numberOfInputs(0)
{}

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

        if (!initialiseFile())
                return false;

        Logger(Debug, "Successfully initialised file [%s].\n", m_filename);
        writeScalarAttribute(m_infoGroup, "dt", GetGlobalDt());

        return finaliseInit();
}

void BaseH5Recorder::terminate()
{
        Logger(Debug, "BaseH5Recorder::terminate()\n");
        writeScalarAttribute(m_infoGroup, "tend", GetGlobalTime() - GetGlobalDt());
	closeFile();
}

#ifdef REALTIME_ENGINE
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
#endif // REALTIME_ENGINE

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
        writeStringAttribute(grp, "Name", entity->name().c_str());
        writeStringAttribute(grp, "Units", entity->units().c_str());

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
                writeScalarAttribute(grp, it->first.c_str(), it->second);

        return true;
}

bool BaseH5Recorder::allocateEventsDatasets(int dataRank,
                                       const hsize_t *dataDims, const hsize_t *maxDataDims, const hsize_t *chunkDims)
{
        hid_t dspace, dset;
        char datasetName[DATASET_NAME_LEN];
        sprintf(datasetName, "%s/%s",EVENTS_GROUP, "Code");
        if (!createUnlimitedDataset(datasetName, dataRank, dataDims, maxDataDims, chunkDims, &dspace, &dset, H5T_STD_I32LE)) {
                Logger(Critical, "Unable to create dataset [%s].\n", datasetName);
                return false;
        }
        Logger(Debug, "Dataset [%s] created.\n", datasetName);
        
	sprintf(datasetName, "%s/%s",EVENTS_GROUP, "Sender");
	if (!createUnlimitedDataset(datasetName, dataRank, dataDims, maxDataDims, chunkDims, &dspace, &dset, H5T_STD_I32LE)) {
                Logger(Critical, "Unable to create dataset [%s].\n", datasetName);
                return false;
        }
        Logger(Debug, "Dataset [%s] created.\n", datasetName);
        
	sprintf(datasetName, "%s/%s",EVENTS_GROUP, "Timestamp");
	if (!createUnlimitedDataset(datasetName, dataRank, dataDims, maxDataDims, chunkDims, &dspace, &dset, H5T_STD_I32LE)) {
                Logger(Critical, "Unable to create dataset [%s].\n", datasetName);
                return false;
        }
        Logger(Debug, "Dataset [%s] created.\n", datasetName);
}
//~~~

const uint H5Recorder::numberOfBuffers = 2;
const int  H5Recorder::rank            = 1;

H5Recorder::H5Recorder(bool compress, const char *filename, uint id)
        : BaseH5Recorder(compress, 1024, 20, filename, id), // 1024 = chunkSize and 20 = numberOfChunks
          m_data(), m_eventsData(), m_threadRun(false), m_runCount(0)
{
        m_bufferLengths = new hsize_t[H5Recorder::numberOfBuffers];
        m_eventsBufferLengths = new hsize_t[H5Recorder::numberOfBuffers];
        setName("H5Recorder");
}

H5Recorder::~H5Recorder()
{
	closeFile();
        for (int i=0; i<m_numberOfDatasets; i++) {
                for(int j=0; j<H5Recorder::numberOfBuffers; j++)
                        delete m_data[i][j];
                delete m_data[i];
        }
        delete m_bufferLengths;
	// Delete events data buffers
        for (int i=0; i < m_numberOfEventsDatasets; i++) {
                for(int j=0; j<H5Recorder::numberOfBuffers; j++) 
                        delete m_eventsData[i][j];
                delete m_eventsData[i];
        }
        delete m_eventsBufferLengths;
}

bool H5Recorder::finaliseInit()
{
        Logger(Debug, "H5Recorder::finaliseInit()\n");
        int err;
        hsize_t bufsz = bufferSize(), maxbufsz = H5S_UNLIMITED, chunksz = chunkSize(), evbufsz = 1;

        stopWriterThread();
        if (m_runCount) {
                err = pthread_mutex_destroy(&m_mutex);
                if (err)
                        Logger(Critical, "pthread_mutex_destroy: %s.\n", strerror(err));
                else
                        Logger(Debug, "Successfully destroyed the mutex.\n");
                err = pthread_cond_destroy(&m_cv);
                if (err)
                        Logger(Critical, "pthread_cond_destroy: %s.\n", strerror(err));
                else
                        Logger(Debug, "Successfully destroyed the condition variable.\n");
        }

        m_bufferInUse = H5Recorder::numberOfBuffers-1;
        m_bufferPosition = 0;
        m_datasetSize = 0;
        m_eventsDatasetSize = 0;
	m_numberOfDatasets = 0;        
        for (int i=0; i<m_pre.size(); i++) {
		if (m_pre[i]->hasOutput()) {
                if (!allocateForEntity(m_pre[i], H5Recorder::rank, &bufsz, &maxbufsz, &chunksz))
		        	return false;
		m_numberOfDatasets += 1;
		}
        }
	// Initialize events datasets (Code,Origin,Timestamps)	
	#define NUMBER_OF_EVENTS_DATASETS 3        
        m_eventsBufferInUse = H5Recorder::numberOfBuffers-1;
        m_eventsBufferPosition = 0;
        if (!allocateEventsDatasets(H5Recorder::rank, &evbufsz, &maxbufsz, &chunksz))
        	return false;
        m_eventsDatasetSize = 0;
	m_numberOfEventsDatasets = 0;
	for (int j=0; j<NUMBER_OF_EVENTS_DATASETS; j++) {
        	int32_t **eventsBuffer = new int32_t*[H5Recorder::numberOfBuffers];
        	for (int i=0; i<H5Recorder::numberOfBuffers; i++)
                	eventsBuffer[i] = new int32_t[bufferSize()];
       		m_eventsData.push_back(eventsBuffer);
		m_numberOfEventsDatasets += 1;
        }
	err = pthread_mutex_init(&m_mutex, NULL);
        if (err) {
                Logger(Critical, "pthread_mutex_init: %s.", strerror(err));
                return false;
        }
        else {
                Logger(Debug, "Successfully initialized the mutex.\n");
        }

        err = pthread_cond_init(&m_cv, NULL);
        if (err) {
                Logger(Critical, "pthread_cond_init: %s.", strerror(err));
                return false;
        }
        else {
                Logger(Debug, "Successfully initialized the condition variable.\n");
        }


        startWriterThread();
        
        return true;
}

void H5Recorder::startWriterThread()
{
        pthread_attr_t attr;
        int err;
        if (m_threadRun)
                return;
        m_threadRun = true;
        // explicitly make the thread joinable, for portability
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        err = pthread_create(&m_writerThread, &attr, buffersWriter, this); 
        if (err)
                Logger(Critical, "pthread_create: %s.\n", strerror(err));
        else
                Logger(Debug, "Successfully created the writer thread.\n");
        pthread_attr_destroy(&attr);
}


void H5Recorder::stopWriterThread()
{
        if (!m_threadRun)
                return;

        Logger(Debug, "H5Recorder::stopWriterThread() >> Terminating writer thread.\n");
        pthread_mutex_lock(&m_mutex);
        Logger(Debug, "H5Recorder::stopWriterThread() >> Locked the mutex.\n");
        Logger(Debug, "H5Recorder::stopWriterThread() >> buffer position = %d.\n", m_bufferPosition);
        if (m_bufferPosition > 0)
                m_dataQueue.push_back(m_bufferInUse);
        Logger(Debug, "H5Recorder::stopWriterThread() >> events buffer position = %d.\n", m_eventsBufferPosition);
        if (m_eventsBufferPosition > 0)
                m_eventsDataQueue.push_back(m_eventsBufferInUse);
        Logger(Debug, "H5Recorder::stopWriterThread() >> %d values left to save in buffer #%d.\n",
                m_bufferLengths[m_bufferInUse], m_bufferInUse);
        Logger(Debug, "H5Recorder::stopWriterThread() >> %d events left to save in buffer #%d.\n",
                m_eventsBufferLengths[m_eventsBufferInUse], m_eventsBufferInUse);
			
        m_threadRun = false;
        Logger(Debug, "H5Recorder::stopWriterThread() >> before pthread_cond_broadcast.\n");
        pthread_cond_broadcast(&m_cv);
        Logger(Debug, "H5Recorder::stopWriterThread() >> after pthread_cond_broadcast.\n");
        pthread_mutex_unlock(&m_mutex);
        Logger(Debug, "H5Recorder::stopWriterThread() >> Unlocked the mutex.\n");
        pthread_join(m_writerThread, NULL);
        Logger(Debug, "H5Recorder::stopWriterThread() >> Writer thread has terminated.\n");
}

void H5Recorder::terminate()
{
        stopWriterThread();
        BaseH5Recorder::terminate();
        m_runCount++;
}

void H5Recorder::firstStep()
{
        struct timespec ts;
        current_utc_time(&ts);
        writeScalarAttribute(m_infoGroup, "startTimeSec", (long) ts.tv_sec);
        writeScalarAttribute(m_infoGroup, "startTimeNsec", ts.tv_nsec);
        step();
}

void H5Recorder::step()
{
        if (m_numberOfInputs == 0)
                return;

        if (m_bufferPosition == 0) {
                pthread_mutex_lock(&m_mutex);
                while (m_dataQueue.size() == H5Recorder::numberOfBuffers) {
                        //Logger(Debug, "H5Recorder::step() >> The data queue is full.\n");
                        pthread_cond_wait(&m_cv, &m_mutex);
                }
                pthread_mutex_unlock(&m_mutex);
                m_bufferInUse = (m_bufferInUse+1) % H5Recorder::numberOfBuffers;
                m_bufferLengths[m_bufferInUse] = 0;
                Logger(Debug, "H5Recorder::step() >> Starting to write in buffer #%d @ t = %g.\n", m_bufferInUse, GetGlobalTime());
        }
	int ent_idx = 0;
        for (int i=0; i<m_numberOfInputs; i++) {
		if (m_pre[i]->hasOutput()) {
               		m_data[ent_idx][m_bufferInUse][m_bufferPosition] = m_inputs[i];
			ent_idx += 1;
		}
	}
        m_bufferLengths[m_bufferInUse]++;
        m_bufferPosition = (m_bufferPosition+1) % bufferSize();

        if (m_bufferPosition == 0) {
                Logger(Debug, "H5Recorder::step() >> Buffer #%d is full (it contains %d elements).\n",
                                m_bufferInUse, m_bufferLengths[m_bufferInUse]);
                Logger(Debug, "H5Recorder::step() >> Trying to acquire the mutex on the data queue.\n");
                pthread_mutex_lock(&m_mutex);
                Logger(Debug, "H5Recorder::step() >> Acquired the mutex on the data queue.\n");
                m_dataQueue.push_back(m_bufferInUse);
                Logger(Debug, "H5Recorder::step() >> Pushed buffer number in the data queue.\n");
                pthread_cond_broadcast(&m_cv);
                Logger(Debug, "H5Recorder::step() >> Signalled the condition variable.\n");
                pthread_mutex_unlock(&m_mutex);
                Logger(Debug, "H5Recorder::step() >> Unlocked the mutex.\n");
        }
}

void H5Recorder::handleEvent(const Event *event) {

        if (m_eventsBufferPosition == 0) {
                pthread_mutex_lock(&m_mutex);
                while (m_eventsDataQueue.size() == H5Recorder::numberOfBuffers) {
                        Logger(Critical, "H5Recorder::handleEvent() >> The events data queue is full.\n");
                        pthread_cond_wait(&m_cv, &m_mutex);
                }
                pthread_mutex_unlock(&m_mutex);
                m_eventsBufferInUse = (m_eventsBufferInUse+1) % H5Recorder::numberOfBuffers;
                m_eventsBufferLengths[m_eventsBufferInUse] = 0;
        }
        m_eventsData[0][m_eventsBufferInUse][m_eventsBufferPosition] = (int32_t) event->type();
        m_eventsData[1][m_eventsBufferInUse][m_eventsBufferPosition] = (int32_t) event->sender()->id();
        m_eventsData[2][m_eventsBufferInUse][m_eventsBufferPosition] = (int32_t) (event->time()/GetGlobalDt());
	
        m_eventsBufferLengths[m_eventsBufferInUse]++;
        m_eventsBufferPosition = (m_eventsBufferPosition+1) % bufferSize();
	if (m_eventsBufferPosition == 0) {
                Logger(Debug, "H5Recorder::handleEvents() >> Buffer #%d is full (it contains %d elements).\n",
                                m_eventsBufferInUse, m_eventsBufferLengths[m_eventsBufferInUse]);
                Logger(Debug, "H5Recorder::handleEvents() >> Trying to acquire the mutex on the data queue.\n");
                pthread_mutex_lock(&m_mutex);
                Logger(Debug, "H5Recorder::handleEvents() >> Acquired the mutex on the data queue.\n");
                m_eventsDataQueue.push_back(m_eventsBufferInUse);
                Logger(Debug, "H5Recorder::handleEvents() >> Pushed buffer number in the data queue.\n");
                pthread_cond_broadcast(&m_cv);
                Logger(Debug, "H5Recorder::handleEvents() >> Signalled the condition variable.\n");
                pthread_mutex_unlock(&m_mutex);
                Logger(Debug, "H5Recorder::handleEvents() >> Unlocked the mutex.\n");
        }
}

void* H5Recorder::buffersWriter(void *arg)
{
        Logger(Debug, "H5Recorder::buffersWriter() >> Started.\n");
        H5Recorder *self = static_cast<H5Recorder*>(arg);
        if (self == NULL) {
                Logger(Critical, "buffers writer terminating.\n");
                goto endBuffersWriter;
        }

#ifdef REALTIME_ENGINE
        //reducePriority();
#endif

        uint bufferToSave;
        while (self->m_threadRun || self->m_dataQueue.size() != 0) {
                pthread_mutex_lock(&self->m_mutex);
                while (self->m_dataQueue.size() == 0) {
                        //Logger(Debug, "H5Recorder::buffersWriter() >> The data queue is empty.\n");
                        pthread_cond_wait(&self->m_cv, &self->m_mutex);
                }
                bufferToSave = self->m_dataQueue.front();
                pthread_mutex_unlock(&self->m_mutex);
                Logger(Debug, "H5Recorder::buffersWriter() >> Acquired lock: will save data in buffer #%d.\n", bufferToSave);

                hid_t filespace;
                herr_t status;
                hsize_t offset;

                if (self->m_bufferLengths[bufferToSave] > 0) {
                        offset = self->m_datasetSize;
                        self->m_datasetSize += self->m_bufferLengths[bufferToSave];

                        Logger(Debug, "Dataset size = %d.\n", self->m_datasetSize);
                        Logger(Debug, "Offset = %d.\n", offset);
                        Logger(Debug, "Time = %g sec.\n", self->m_datasetSize*GetGlobalDt());

                        for (int i=0; i<self->m_numberOfDatasets; i++) {

                                // extend the dataset
                                status = H5Dset_extent(self->m_datasets[i], &self->m_datasetSize);
                                if (status < 0)
                                        throw "Unable to extend dataset.";
                                else
                                        Logger(All, "Extended dataset.\n");

                                // get the filespace
                                filespace = H5Dget_space(self->m_datasets[i]);
                                if (filespace < 0)
                                        throw "Unable to get filespace.";
                                else
                                        Logger(All, "Obtained filespace.\n");

                                // select an hyperslab
                                status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, &offset, NULL, &self->m_bufferLengths[bufferToSave], NULL);
                                if (status < 0) {
                                        H5Sclose(filespace);
                                        throw "Unable to select hyperslab.";
                                }
                                else {
                                        Logger(All, "Selected hyperslab.\n");
                                }

                                // define memory space
                                self->m_dataspaces[i] = H5Screate_simple(H5Recorder::rank, &self->m_bufferLengths[bufferToSave], NULL);
                                if (self->m_dataspaces[i] < 0) {
                                        H5Sclose(filespace);
                                        throw "Unable to define memory space.";
                                }
                                else {
                                        Logger(All, "Memory space defined.\n");
                                }

                                // write data
                                status = H5Dwrite(self->m_datasets[i], H5T_IEEE_F64LE, self->m_dataspaces[i], filespace, H5P_DEFAULT, self->m_data[i][bufferToSave]);
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
                if (self->m_eventsDataQueue.size() > 0) {
                
                	bufferToSave = self->m_dataQueue.front();
                	pthread_mutex_unlock(&self->m_mutex);
                	Logger(Debug, "H5Recorder::buffersWriter() >> Acquired lock: will save data in buffer #%d.\n", bufferToSave);

                	hid_t filespace;
                	herr_t status;
                	hsize_t offset;

                	if (self->m_eventsBufferLengths[bufferToSave] > 0) {

                        	offset = self->m_eventsDatasetSize;
                        	self->m_eventsDatasetSize += self->m_eventsBufferLengths[bufferToSave];

                        	Logger(Debug, "Events dataset size = %d.\n", self->m_eventsDatasetSize);
                        	Logger(Debug, "Events offset = %d.\n", offset);

                        for (int i=self->m_numberOfDatasets; i<(self->m_numberOfDatasets + self->m_numberOfEventsDatasets) ; i++) {
                                // extend the dataset
                                status = H5Dset_extent(self->m_datasets[i], &self->m_eventsDatasetSize);
                                if (status < 0)
                                        throw "Unable to extend dataset.";
                                else
                                        Logger(All, "Extended dataset [%d].\n",i);

                                // get the filespace
                                filespace = H5Dget_space(self->m_datasets[i]);
                                if (filespace < 0)
                                        throw "Unable to get filespace.";
                                else
                                        Logger(All, "Obtained filespace.\n");

                                // select an hyperslab
                                status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, &offset, NULL, &self->m_eventsBufferLengths[bufferToSave], NULL);
                                if (status < 0) {
                                        H5Sclose(filespace);
                                        throw "Unable to select hyperslab.";
                                }
                                else {
                                        Logger(All, "Selected hyperslab [%d].\n",i);
                                }

                                // define memory space
                                self->m_dataspaces[i] = H5Screate_simple(H5Recorder::rank, &self->m_eventsBufferLengths[bufferToSave], NULL);
                                if (self->m_dataspaces[i] < 0) {
                                        H5Sclose(filespace);
                                        throw "Unable to define memory space.";
                                }
                                else {
                                        Logger(All, "Memory space defined [%d].\n",i-self->m_numberOfDatasets);
                                }

                                // write data
                                status = H5Dwrite(self->m_datasets[i], 
					H5T_STD_I32LE, self->m_dataspaces[i], filespace, H5P_DEFAULT, 
					self->m_eventsData[i-self->m_numberOfDatasets][bufferToSave]);
                                if (status < 0) {
                                        H5Sclose(filespace);
                                        throw "Unable to write data.";
                                }
                                else {
                                        Logger(All, "Written data.\n");
					self->setHasEvents();
                                }
                        }
                        H5Sclose(filespace);
                        Logger(Debug, "H5Recorder::buffersWriter() >> Finished writing data.\n");
                	}
		}
                pthread_mutex_lock(&self->m_mutex);
                Logger(Debug, "H5Recorder::buffersWriter() >> Locked the mutex.\n");
                self->m_dataQueue.pop_front();
                Logger(Debug, "H5Recorder::buffersWriter() >> Removed data from the queue.\n");
                pthread_cond_broadcast(&self->m_cv);
                Logger(Debug, "H5Recorder::buffersWriter() >> Signalled the condition variable.\n");
                pthread_mutex_unlock(&self->m_mutex);
                Logger(Debug, "H5Recorder::buffersWriter() >> Unlocked the mutex.\n");
        }
endBuffersWriter:
        Logger(Debug, "H5Recorder::buffersWriter() >> Writing thread has terminated.\n");
        pthread_exit(NULL);
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

// for passing data to the writer thread
struct thread_data {
        thread_data(TriggeredH5Recorder *rec, uint buffer, uint position)
                : self(rec), buffer_to_save(buffer), buffer_position(position) {}
        TriggeredH5Recorder *self;
        uint buffer_to_save, buffer_position;
};

TriggeredH5Recorder::TriggeredH5Recorder(double before, double after, bool compress, const char *filename, uint id)
        : BaseH5Recorder(compress, ceil((before+after)/GetGlobalDt()), filename, id),
          m_recording(false), m_data(), m_bufferPosition(0), m_bufferInUse(0),
          m_maxSteps(ceil(after/GetGlobalDt())), m_nSteps(0)
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
        thread_data *arg;
        // store the data
        for (int i=0; i<m_numberOfInputs; i++)
                m_data[i][m_bufferInUse][m_bufferPosition] = m_inputs[i];
        m_bufferPosition = (m_bufferPosition + 1) % bufferSize();
        if (m_recording) {
                m_nSteps++;
                if (m_nSteps == m_maxSteps) {
                        m_recording = false;
                        // start the writing thread
                        arg = new thread_data(this, m_bufferInUse, m_bufferPosition);
                        pthread_create(&m_writerThread, NULL, buffersWriter, (void *) arg);
                        m_bufferInUse = (m_bufferInUse + 1) % TriggeredH5Recorder::numberOfBuffers;
                        m_bufferPosition = 0;
                }
        }
}

void TriggeredH5Recorder::terminate()
{
        thread_data *arg;
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
                arg = new thread_data(this, m_bufferInUse, m_bufferPosition);
                pthread_create(&m_writerThread, NULL, buffersWriter, (void *) arg);
        }
        pthread_join(m_writerThread, NULL);
        BaseH5Recorder::terminate();
}

void TriggeredH5Recorder::handleEvent(const Event *event)
{
        if (event->type() == TRIGGER) {
                if (!m_recording) {
                        // wait for the writing thread to finish
                        pthread_join(m_writerThread, NULL);
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
        pthread_join(m_writerThread, NULL);
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

void* TriggeredH5Recorder::buffersWriter(void *arg)
{
        thread_data *data = static_cast<thread_data*>(arg);
        TriggeredH5Recorder *self = data->self;
        uint bufferToSave = data->buffer_to_save;
        uint bufferPosition = data->buffer_position;
        delete data;

        Logger(Debug, "TriggeredH5Recorder::buffersWriter: will save buffer #%d starting from pos %d.\n",
                        bufferToSave, bufferPosition);

#ifdef REALTIME_ENGINE
        //reducePriority();
#endif

        hid_t filespace;
        herr_t status;

        hsize_t start[TriggeredH5Recorder::rank], count[TriggeredH5Recorder::rank];
        start[0] = 0;
        start[1] = self->m_datasetSize[1];
        count[0] = self->bufferSize();
        count[1] = 1;
        self->m_datasetSize[1]++;

        Logger(Debug, "Dataset size = (%dx%d).\n", self->m_datasetSize[0], self->m_datasetSize[1]);
        Logger(Debug, "Offset = (%d,%d).\n", start[0], start[1]);

        uint offset = self->bufferSize() - bufferPosition;
        Logger(Debug, "buffer size = %d.\n", self->bufferSize());
        Logger(Debug, "buffer position = %d.\n", bufferPosition);
        Logger(Debug, "offset = %d\n", offset);
        for (int i=0; i<self->m_numberOfInputs; i++) {

                // extend the dataset
                status = H5Dset_extent(self->m_datasets[i], self->m_datasetSize);
                if (status < 0)
                        throw "Unable to extend dataset.";
                else
                        Logger(All, "Extended dataset.\n");

                // get the filespace
                filespace = H5Dget_space(self->m_datasets[i]);
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
                self->m_dataspaces[i] = H5Screate_simple(TriggeredH5Recorder::rank, count, NULL);
                if (self->m_dataspaces[i] < 0) {
                        H5Sclose(filespace);
                        throw "Unable to define memory space.";
                }
                else {
                        Logger(All, "Memory space defined.\n");
                }

                memcpy(self->m_tempData, self->m_data[i][bufferToSave]+bufferPosition, offset*sizeof(double));
                memcpy(self->m_tempData+offset, self->m_data[i][bufferToSave], bufferPosition*sizeof(double));
                // write data
                status = H5Dwrite(self->m_datasets[i], H5T_IEEE_F64LE, self->m_dataspaces[i], filespace, H5P_DEFAULT, self->m_tempData);
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
        pthread_exit(NULL);
}

} // namespace recorders

} // namespace lcg

