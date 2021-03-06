#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <dirent.h>
#include <unistd.h>
#include <getopt.h>
#include "entity.h"
#include "utils.h"
#include "waveform.h"
#include "engine.h"
#include "recorders.h"
#include "stream.h"
#include "neurons.h"

#include "sha1.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#ifdef __APPLE__
#include <libgen.h>
#endif

using boost::property_tree::ptree;

#define LCG_DIR    ".lcg"
#define REPLAY_SCRIPT "replay"
#define HASHES_FILE   "hashes.sha"

using namespace lcg;
using namespace lcg::generators;
using namespace lcg::recorders;

struct options {
        options() : iti(0), nTrials(0), enableReplay(true) {
                configFile[0] = '\0';
        }
        useconds_t iti;
        uint nTrials;
        char configFile[FILENAME_MAXLEN];
        bool enableReplay;
};

static struct option longopts[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"verbosity", required_argument, NULL, 'V'},
        {"frequency", required_argument, NULL, 'F'},
        {"iti", required_argument, NULL, 'i'},
        {"ntrials", required_argument, NULL, 'n'},
        {"disable-replay", no_argument, NULL, 'r'},
        {"config-file", required_argument, NULL, 'c'},
        {NULL, 0, NULL, 0}
};

const char lcg_experiment_usage_string[] =
        "This program performs closed loop, hybrid, dynamic-clamp experiments: you name it, we got it...\n\n"
        "Usage: lcg experiment [<options> ...]\n"
        "where options are:\n"
        "   -h, --help            Print this help message.\n"
        "   -v, --version         Print the program version.\n"
        "   -V, --verbosity       Verbosity level (0 for maximum, 4 for minimum verbosity).\n"
        "   -i, --iti             Inter-trial interval.\n"
        "   -n, --ntrials         Number of trials (how many times a stimulus is repeated, default 1).\n"
        "   -c, --config-file     Configuration file.\n"
        "   -r, --disable-replay  Disable metadata writing in the directory .lcg.\n";

static void usage()
{
        printf("%s\n", lcg_experiment_usage_string);
}

void parse_args(int argc, char *argv[], options *opts)
{
        int ch;
        struct stat buf;
        double iti = -1;
        // default values
        opts->nTrials = 1;
        while ((ch = getopt_long(argc, argv, "hvV:c:n:i:r", longopts, NULL)) != -1) {
                switch(ch) {
                case 'h':
                        usage();
                        exit(0);
                case 'v':
                        printf("lcg experiment version %s.\n", VERSION);
                        exit(0);
                case 'V':
                        if (atoi(optarg) < All || atoi(optarg) > Critical) {
                                Logger(Important, "The verbosity level must be between %d and %d.\n", All, Critical);
                                exit(1);
                        }
                        SetLoggingLevel(static_cast<LogLevel>(atoi(optarg)));
                        break;
                case 'n':
                        opts->nTrials = atoi(optarg);
                        if (opts->nTrials <= 0) {
                                Logger(Critical, "The number of trials must be greater than zero.\n");
                                exit(1);
                        }
                        break;
                case 'i':
                        iti = atof(optarg);
                        if (iti < 0) {
                                Logger(Critical, "The inter-trial interval must be non-negative.\n");
                                exit(1);
                        }
                        break;
                case 'c':
                        if (stat(optarg, &buf) == -1) {
                                Logger(Critical, "%s: %s.\n", optarg, strerror(errno));
                                exit(1);
                        }
                        strncpy(opts->configFile, optarg, FILENAME_MAXLEN);
                        break;
                case 'r':
                        opts->enableReplay = false;
                        break;
                default:
                        Logger(Critical, "Enter 'lcg help experiment' for help on how to use this program.\n");
                        exit(1);
                }
        }
        if (strlen(opts->configFile) == 0) {
                Logger(Critical, "You must specify a configuration file.\n");
                exit(1);
        }
        if (opts->nTrials > 1 && iti < 0) {
                Logger(Critical, "You must specify the duration of the inter-trial interval.\n");
                exit(1);
        }
        opts->iti = (useconds_t) (1e6 * iti);
}

int parse_configuration_file(const std::string& filename,
                             std::vector<Entity*>& entities, std::vector<Stream*>& streams,
                             double *tend, double *dt, std::string& outfilename, struct trigger_data *trigger)
{
        ptree pt;
        uint id;
        std::string name, conn;
        std::map< uint, std::vector<uint> > connections;
        std::map< uint, Entity* > ntts;
        std::map< uint, Stream* > strms;

        try {
                read_xml(filename, pt);

                /*** simulation time and time step ***/
                try {
                        *tend = pt.get<double>("lcg.simulation.tend");
                } catch(...) {
                        *tend = -1;
                }

                try {
                        *dt = pt.get<double>("lcg.simulation.dt");
                } catch(...) {
                        *dt = -1;
                        try {
                                *dt = 1.0 / pt.get<double>("lcg.simulation.rate");
                        } catch(...) {
                                Logger(Info, "dt = %g sec.\n", *dt);
                        }
                }

                SetGlobalDt(*dt); // So that the entities are loaded with the proper sampling rate.
                SetRunTime(*tend);
		
                /*** integration algorithm, in case ionic currents are present ***/
                try {
                        std::string algo = pt.get<std::string>("lcg.simulation.algorithm");
                        if (ToUpper(algo).compare("EULER") == 0) {
                                Logger(Info, "Using Euler integration method.\n");
                                lcg::SetIntegrationMethod(lcg::EULER);
                        }
                        else if (ToUpper(algo).compare("RK4") == 0) {
                                Logger(Info, "Using Runge-Kutta integration method.\n");
                                lcg::SetIntegrationMethod(lcg::RK4);
                        }
                        else {
                                Logger(Important, "Unknown integration method [%s]: will use default.\n", algo.c_str());
                        }
                } catch(...) {}

                /*** output file name (makes sense only for streams) ***/
                try {
                        outfilename = pt.get<std::string>("lcg.simulation.outfile");
                } catch(...) {
                        outfilename = "";
                }

		/*** trigger subdevice and channel***/
		trigger->use = false;	
                try {
                        trigger->device = pt.get<std::string>("lcg.simulation.trigger.device").c_str();
			trigger->use = true;
                } catch(...) {
                        trigger->device = "/dev/comedi0";
                }
                try {
                        trigger->subdevice = pt.get<uint>("lcg.simulation.trigger.subdevice");
			trigger->use = true;
                } catch(...) {
                        trigger->subdevice = 0;
                }
                try {
                        trigger->channel = pt.get<uint>("lcg.simulation.trigger.channel");
			trigger->use = true;
                } catch(...) {
                        trigger->channel = 0;
                }

                /*** entities ***/
                const char *children[2] = {"lcg.entities","lcg.streams"};
                for (int i=0; i<2; i++) {
                        try {
                                BOOST_FOREACH(ptree::value_type &vt, pt.get_child(children[i])) {
                                        string_dict args;
                                        name = vt.second.get<std::string>("name");
                                        id = vt.second.get<uint>("id");
                                        if (ntts.count(id) == 1 || strms.count(id) == 1) {
                                                Logger(Critical, "Duplicate ID in configuration file: [%d].\n", id);
                                                entities.clear();
                                                return -1;
                                        }
                                        args["id"] = vt.second.get<std::string>("id");
                                        BOOST_FOREACH(ptree::value_type &pars, vt.second.get_child("parameters")) {
                                                if (pars.first.substr(0,12).compare("<xmlcomment>") != 0)
                                                        args[pars.first] = std::string(pars.second.data());
                                        }
                                        try {
                                                conn = vt.second.get<std::string>("connections");
                                                // this test allows to have <connections></connections> in the configuration file
                                                if (conn.length() > 0) {
                                                        connections[id] = std::vector<uint>();
                                                        size_t start=0, stop;
                                                        int post;
                                                        Logger(Debug, "Entity #%d is connected to entities", id);
                                                        while ((stop = conn.find(",",start)) != conn.npos) {
                                                                std::stringstream ss(conn.substr(start,stop-start));
                                                                ss >> post;
                                                                connections[id].push_back(post);
                                                                start = stop+1;
                                                                Logger(Debug, " #%d", post);
                                                        }
                                                        std::stringstream ss(conn.substr(start,stop-start));
                                                        ss >> post;
                                                        connections[id].push_back(post);
                                                        Logger(Debug, " #%d.\n", post);
                                                }
                                        } catch(std::exception e) {
                                                Logger(Debug, "No connections for entity #%d.\n", id);
                                        }
                                        if (i == 0) {
                                                Entity *entity;
                                                try {
                                                        entity = EntityFactory(name.c_str(), args);
                                                        if (entity == NULL)
                                                                throw "Entity factory is missing";
                                                } catch(const char *err) {
                                                        Logger(Critical, "Unable to create entity [%s]: %s.\n", name.c_str(), err);
                                                        for (int i=0; i<entities.size(); i++)
                                                                delete entities[i];
                                                        entities.clear();
                                                        return -1;
                                                }
                                                entities.push_back(entity);
                                                ntts[id] = entity;
                                        }
                                        else {
                                                Stream *stream;
                                                try {
                                                        stream = StreamFactory(name.c_str(), args);
                                                        if (stream == NULL)
                                                                throw "Stream factory is missing";
                                                } catch(const char *err) {
                                                        Logger(Critical, "Unable to create stream [%s]: %s.\n", name.c_str(), err);
                                                        for (int i=0; i<entities.size(); i++)
                                                                delete entities[i];
                                                        entities.clear();
                                                        for (int i=0; i<streams.size(); i++)
                                                                delete streams[i];
                                                        streams.clear();
                                                        return -1;
                                                }
                                                streams.push_back(stream);
                                                strms[id] = stream;
                                        }
                                }
        
                                if (i == 0) {
                                        EntitySorter sorter;
                                        std::sort(entities.begin(), entities.end(), sorter);
                                }
                                else {
                                        StreamSorter sorter;
                                        std::sort(streams.begin(), streams.end(), sorter);
                                }
                        } catch (...) {
                                Logger(Debug, "No children %s.\n", children[i]);
                        }
                }

                /*** connections ***/
                uint idPre, idPost;
                for (int i=0; i<entities.size(); i++) {
                        idPre = entities[i]->id();
                        Logger(Debug, "Id = %d.\n", idPre);
                        for (int j=0; j<connections[idPre].size(); j++) {
                                idPost = connections[idPre][j];
                                entities[i]->connect(ntts[idPost]);
                                Logger(Debug, "Connecting entity #%d to entity #%d.\n", idPre, idPost);
                        }
                }
                for (int i=0; i<streams.size(); i++) {
                        idPre = streams[i]->id();
                        Logger(Debug, "Id = %d.\n", idPre);
                        for (int j=0; j<connections[idPre].size(); j++) {
                                idPost = connections[idPre][j];
                                streams[i]->connect(strms[idPost]);
                                Logger(Debug, "Connecting stream #%d to stream #%d.\n", idPre, idPost);
                        }
                }

        } catch(std::exception e) {
                Logger(Critical, "Error while parsing configuration file: %s.\n", e.what());
                entities.clear();
                return -1;
        }

        return 0;
}

int cp(const char *to, const char *from, bool use_relative_paths = false)
{
        int fd_to, fd_from;
        char buf[4096];
        ssize_t nread;
        int saved_errno;
        
        fd_from = open(from, O_RDONLY);
        if (fd_from < 0)
                return -1;
        
        fd_to = open(to, O_WRONLY | O_CREAT, 0666);
        if (fd_to < 0)
                goto out_error;
        
        while (nread = read(fd_from, buf, sizeof buf), nread > 0) {
                char *out_ptr = buf;
                ssize_t nwritten;

                if (use_relative_paths) {
                        int start;
                        for (int i=0; i<nread-strlen(".stim"); i++) {
                                if (buf[i] == '>')
                                        start = i;
                                if (!strncmp(buf+i,".stim",5)) {
                                        bool del = false;
                                        for (int j=i; j>start; j--) {
                                                if (buf[j] == '/')
                                                        del = true;
                                                if (del)
                                                        buf[j] = ' ';
                                        }
                                }
                        }
                }
        
                do {
                        nwritten = write(fd_to, out_ptr, nread);
        
                        if (nwritten >= 0) {
                                nread -= nwritten;
                                out_ptr += nwritten;
                        }
                        else if (errno != EINTR) {
                                goto out_error;
                        }
                } while (nread > 0);
        }
        
        if (nread == 0) {
                if (close(fd_to) < 0) {
                        fd_to = -1;
                        goto out_error;
                }
                close(fd_from);
        
                /* Success! */
                return 0;
        }
        
out_error:
        saved_errno = errno;

        close(fd_from);
        if (fd_to >= 0)
                close(fd_to);

        errno = saved_errno;
        return -1;
}

#define SHA1_BUFSIZE 1048576    // 1 Megabyte

int sha1(const char *filename, uint8_t *messageDigest)
{
        struct sha1_ctx ctx;
        uint8_t buf[SHA1_BUFSIZE];
        ssize_t nbytes;
        int fd;
        fd = open(filename, O_RDONLY);
        if (fd < -1) {
                Logger(Important, "Unable to open [%s] for computing the SHA-1 digest.\n", filename);
                return -1;
        }
        sha1_init((void *) &ctx);
        while ((nbytes = read(fd, (void *) buf, SHA1_BUFSIZE)) > 0)
                sha1_update((void *) &ctx, (uint8_t *) buf, nbytes);
        close(fd);
        if (nbytes == -1) {
                Logger(Important, "Unable to properly read the file [%s].\n", filename);
                return -1;
        }
        sha1_final((void* ) &ctx, messageDigest);
        return 0;
}

int store(int argc, char *argv[])
{
        ptree pt;

        // find the most recent H5 file
        DIR *dirp = opendir(".");
        if (dirp == NULL) {
                Logger(Important, "Unable to open current directory for traversing the files.\n");
                return -1;
        }
        else {
                Logger(Debug, "Traversing the files in the current directory.\n");
        }

        struct dirent *dp;
        char latest_h5_file[32] = {0};
        while ((dp = readdir(dirp)) != NULL) {
                if (dp->d_name[0] != '.' && !strcmp(dp->d_name+strlen(dp->d_name)-3, ".h5")) {
                        if (!strlen(latest_h5_file))
                                strcpy(latest_h5_file, dp->d_name);
                        else if (strcmp(dp->d_name, latest_h5_file) > 0)
                                strcpy(latest_h5_file, dp->d_name);
                }
        }
        Logger(Debug, "Most recent file: %s\n", latest_h5_file);
        closedir(dirp);
        
        int flag, i;
        char directory[128], path[128] = {0}, config_file[128] = {0}, *orig_config_file;

        sprintf(directory, "%s/%s", LCG_DIR, latest_h5_file);
        directory[strlen(directory)-3] = 0;

        // create an invisible directory where all data will be stored
        flag = mkdir(LCG_DIR, 0755);
        if (flag == 0) {
                Logger(Debug, "Created directory [%s].\n", LCG_DIR);
        }
        else {
                if (errno == EEXIST) {
                        Logger(Debug, "mkdir: [%s]: directory exists.\n", LCG_DIR);
                }
                else {
                        Logger(Important, "mkdir: [%s]: %s.\n", LCG_DIR, strerror(errno));
                        return -1;
                }
        }
        
        flag = mkdir(directory, 0755);
        if (flag == 0) {
                Logger(Debug, "Created directory [%s].\n", directory);
        }
        else {
                // either the directory exists already or there's been an error: in the first
                // case we continue, otherwise we stop here.
                if (errno == EEXIST) {
                        Logger(Debug, "mkdir: [%s]: directory exists.\n", directory);
                }
                else {
                        Logger(Important, "mkdir: [%s]: %s.\n", directory, strerror(errno));
                        return -1;
                }
        }

        // write a very simple script that runs lcg again with the options used for calling it now
        sprintf(path, "%s/%s", directory, REPLAY_SCRIPT);
        FILE *fid = fopen(path, "w");
        if (fid == NULL) {
                Logger(Important, "Unable to create file [%s].\n", path);
                return -1;
        }
        fprintf(fid, "#!/bin/bash\n");
        for (i=0; i<argc; i++) {
                if (strncmp(argv[i], "-c", 2) == 0) { // switch for the configuration file: the next argument is the name of the XML configuration file
                        orig_config_file = argv[++i];
                        fprintf(fid, "-c %s ", basename(orig_config_file));
                }
                else {
                        fprintf(fid, "%s ", argv[i]);
                }
        }
        fprintf(fid, "-r 0\n"); // turn enable replay off, so that a .lcg directory is not created when replaying the experiment
        fclose(fid);
        chmod(path, 0755);
        Logger(Debug, "Written file [%s/%s].\n", directory, REPLAY_SCRIPT);

        // copy the stim files to the directory 
        try {
                read_xml(orig_config_file, pt);
                const char *children[2] = {"lcg.entities","lcg.streams"};
                const char *parameter_names[2] = {"parameters.filename","parameters.stimfile"};
                for (int i=0; i<2; i++) {
                        try {
                                BOOST_FOREACH (ptree::value_type &vt, pt.get_child(children[i])) {
                                        if (vt.second.get<std::string>("name").compare("Waveform") == 0 ||
                                            vt.second.get<std::string>("name").compare("OutputChannel") == 0) {
                                                char src[128] = {0}, dest[128] = {0};
                                                strcpy(src, vt.second.get<std::string>(parameter_names[i]).c_str());
                                                sprintf(dest, "%s/%s", directory, basename(src));
                                                if (cp(dest, src) == 0)
                                                        Logger(Debug, "Copied stimulus file [%s] to directory [%s].\n",
                                                                        src, directory);
                                                else
                                                        Logger(Important, "Unable to copy stimulus file [%s] to directory [%s].\n",
                                                                        src, directory);
                                        }
                                }
                        } catch (...) {}
                }
        } catch (...) {
                Logger(Critical, "Unable to read the configuration file [%s].\n", orig_config_file);
        }

        // copy the configuration file to the destination directory
        sprintf(config_file, "%s/%s", directory, basename(orig_config_file));
        if (cp(config_file, orig_config_file, true) == 0) {
                Logger(Debug, "Copied file [%s] to directory [%s].\n",
                                orig_config_file, directory);
        }
        else {
                Logger(Important, "Unable to copy configuration file [%s] to directory [%s].\n",
                                orig_config_file, directory);
                return -1;
        }

        // compute the hash for the H5 file and for all files in the directory
        uint8_t md[20];
        sprintf(path, "%s/%s", directory, HASHES_FILE);

        struct stat buf;
        if (stat(path, &buf) == 0) {
                // the file already exists, we need to change its access mode
                chmod(path, 0644);
                Logger(Debug, "[%s] already exists, changing its access mode to 0644\n", path);
        }

        fid = fopen(path, "w");
        if (fid == NULL) {
                Logger(Important, "Unable to create hashes file [%s].\n", path);
                return -1;
        }
        else {
                Logger(Debug, "Successfully created hashes file [%s].\n", path);
        }

        if (sha1(latest_h5_file, md) == 0) {
                for (int k=0; k<20; k++)
                        fprintf(fid, "%02x", md[k]);
                fprintf(fid, " *../../%s\n", latest_h5_file);
        }
        else {
                Logger(Important, "Unable to compute the SHA-1 message digest for [%s].\n", latest_h5_file);
        }

        dirp = opendir(directory);
        if (dirp == NULL) {
                Logger(Important, "Unable to open directory [%s] for traversing the files.\n", directory);
                fclose(fid);
                return -1;
        }
        else {
                Logger(Debug, "Traversing the files in directory [%s].\n", directory);
        }

        while ((dp = readdir(dirp)) != NULL) {
                if (dp->d_name[0] != '.' && strcmp(dp->d_name, HASHES_FILE) != 0) {
                        sprintf(path, "%s/%s", directory, dp->d_name);
                        if (sha1(path, md) == 0) {
                                for (int k=0; k<20; k++)
                                        fprintf(fid, "%02x", md[k]);
                                fprintf(fid, "  %s\n", dp->d_name);
                        }
                        else {
                                Logger(Important, "Unable to compute the SHA-1 message digest for [%s].\n", path);
                        }
                }
        }

        closedir(dirp);
        fclose(fid);

        // change the access mode of the hashes file to read-only
        sprintf(path, "%s/%s", directory, HASHES_FILE);
        chmod(path, 0444);

        return 0;
}

int main(int argc, char *argv[])
{
        options opts;

        if (!SetupSignalCatching()) {
                Logger(Critical, "Unable to setup signal catching functionalities. Aborting.\n");
                exit(1);
        }

        parse_args(argc, argv, &opts);

        int success;
        double tend, dt;
        std::string outfilename;
	struct trigger_data trigger;
        std::vector<Entity*> entities;
        std::vector<Stream*> streams;

        if (parse_configuration_file(opts.configFile, entities, streams, &tend, &dt, outfilename, &trigger) != 0) {
                Logger(Critical, "Error while parsing configuration file. Aborting.\n");
                exit(1);
        }

        SetGlobalDt(dt);

        Logger(Debug, "Number of trials: %d.\n", opts.nTrials);
        Logger(Debug, "Inter-trial interval: %g sec.\n", (double) opts.iti * 1e-6);

        for (int i=0; i<opts.nTrials; i++) {
                Logger(Info, "Trial: %d of %d.\n", i+1, opts.nTrials);
                ResetGlobalTime();
                if (!entities.empty())
                        success = Simulate(&entities,tend,trigger);
                else
                        success = Simulate(&streams,tend,outfilename.size() ? outfilename : MakeFilename("h5"));
                if (success!=0 || KILL_PROGRAM())
                        goto endMain;
                if (opts.enableReplay)
                        store(argc, argv);
                if (i != opts.nTrials-1)
                        usleep(opts.iti);
        }

endMain:
        for (int i=0; i<entities.size(); i++)
                delete entities[i];
        for (int i=0; i<streams.size(); i++)
                delete streams[i];

        return 0;
}

