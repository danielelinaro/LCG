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

#include "sha1.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using boost::property_tree::ptree;

#define DCLAMP_DIR    ".lcg"
#define TMP_DIR       ".tmp"
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

const char lcg_vcclamp_usage_string[] =
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
        printf("%s\n", lcg_vcclamp_usage_string);
}

void parse_args(int argc, char *argv[], options *opts)
{
        int ch;
        struct stat buf;
        double iti = -1;
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
                        strncmp(opts->configFile, optarg, FILENAME_MAXLEN);
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

int parse_configuration_file(const std::string& filename, std::vector<Entity*>& entities, double *tend, double *dt)
{
        ptree pt;
        uint id;
        std::string name, conn;
        std::map< uint, std::vector<uint> > connections;
        std::map< uint, Entity* > ntts;

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

                /*** entities ***/
                BOOST_FOREACH(ptree::value_type &ntt, pt.get_child("lcg.entities")) {
                        string_dict args;
                        name = ntt.second.get<std::string>("name");
                        id = ntt.second.get<uint>("id");
                        if (ntts.count(id) == 1) {
                                Logger(Critical, "Duplicate ID in configuration file: [%d].\n", id);
                                entities.clear();
                                return -1;
                        }
                        args["id"] = ntt.second.get<std::string>("id");
                        BOOST_FOREACH(ptree::value_type &pars, ntt.second.get_child("parameters")) {
                                if (pars.first.substr(0,12).compare("<xmlcomment>") != 0)
                                        args[pars.first] = std::string(pars.second.data());
                        }
                        try {
                                conn = ntt.second.get<std::string>("connections");
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

                EntitySorter sorter;
                std::sort(entities.begin(), entities.end(), sorter);

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

        } catch(std::exception e) {
                Logger(Critical, "Error while parsing configuration file: %s.\n", e.what());
                entities.clear();
                return -1;
        }

        return 0;
}

int cp(const char *to, const char *from)
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

int sha1(const char *filename, unsigned *messageDigest)
{
        FILE *fp = fopen(filename, "rb");
        if (fp == NULL) {
                Logger(Important, "Unable to open [%s] for computing the SHA-1 digest.\n", filename);
                return -1;
        }
        char c;
        SHA1 sha;
        sha.Reset();
        c = fgetc(fp);
        while (!feof(fp)) {
            sha.Input(c);
            c = fgetc(fp);
        }
        fclose(fp);
        if (!sha.Result(messageDigest)) {
            Logger(Important, "Could not compute SHA-1 digest for [%s]\n", filename);
            return -1;
        }
        return 0;
}

int store(int argc, char *argv[], const std::vector<Entity*>& entities)
{
        int flag, i;
        char directory[1024] = {0}, path[1024] = {0}, configFile[1024] = {0};

        // create an invisible directory where all data will be stored
        flag = mkdir(DCLAMP_DIR, 0755);
        if (flag == 0) {
                Logger(Debug, "Created directory [%s].\n", DCLAMP_DIR);
        }
        else {
                if (errno == EEXIST) {
                        Logger(Debug, "mkdir: [%s]: directory exists.\n", DCLAMP_DIR);
                }
                else {
                        Logger(Important, "mkdir: [%s]: %s.\n", DCLAMP_DIR, strerror(errno));
                        return -1;
                }
        }
        
        // find the H5 recorder that was used: this is necessary
        // because we will create a directory inside DCLAMP_DIR with
        // the same name (except the .h5 suffix) as the one saved
        // by the H5 recorder
        BaseH5Recorder *rec = NULL;
        for (i=0; i<entities.size(); i++) {
                rec = dynamic_cast<BaseH5Recorder*>(entities[i]);
                if (rec != NULL) {
                        sprintf(directory, "%s/%s", DCLAMP_DIR, rec->filename());
                        directory[strlen(directory)-3] = 0;
                        break;
                }
        }

        flag = mkdir(directory, 0755);
        if (flag == 0) {
                Logger(Debug, "Created directory [%s].\n", directory);
        }
        else {
                // either the directory exists already or there's been an error: in both
                // cases, we stop here.
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
                fprintf(fid, "%s ", argv[i]);
                if (strncmp(argv[i], "-c", 2) == 0) { // switch for the configuration file: the next argument is the name of the XML configuration file
                        sprintf(configFile, "%s/%s", directory, argv[i+1]);
                        if (cp(configFile, argv[i+1]) == 0) {
                                Logger(Debug, "Copied file [%s] to directory [%s].\n",
                                                argv[i+1], directory);
                        }
                        else {
                                Logger(Important, "Unable to copy configuration file [%s] to directory [%s].\n",
                                                argv[i+1], directory);
                                return -1;
                        }
                }
        }
        fprintf(fid, "-r 0\n"); // turn enable replay off, so that a .lcg directory is not created when replaying the experiment
        fclose(fid);
        chmod(path, 0755);
        Logger(Debug, "Written file [%s/%s].\n", directory, REPLAY_SCRIPT);

        // look for Waveform objects and copy their stimulation files into the directory
        for (i=0; i<entities.size(); i++) {
                Waveform *wave = dynamic_cast<Waveform*>(entities[i]);
                if (wave != NULL) {
                        sprintf(path, "%s/%s", directory, wave->stimulusFile());
                        if (cp(path, wave->stimulusFile()) == 0) {
                                Logger(Debug, "Copied file [%s] to directory [%s].\n",
                                                wave->stimulusFile(), directory);
                        }
                        else {
                                Logger(Important, "Unable to copy file [%s] to directory [%s].\n",
                                                wave->stimulusFile(), directory);
                                return -1;
                        }
                        continue;
                }
        }

        // compute the hash for the H5 file and for all files in the directory
        unsigned md[5];
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

        if (rec != NULL) {
                if (sha1(rec->filename(), md) == 0)
                        fprintf(fid, "%08x%08x%08x%08x%08x  %s\n", md[0], md[1], md[2], md[3], md[4], rec->filename());
                else
                        Logger(Important, "Unable to compute the SHA-1 message digest for [%s].\n", rec->filename());
        }

        DIR *dirp = opendir(directory);
        if (dirp == NULL) {
                Logger(Important, "Unable to open directory [%s] for traversing the files.\n", directory);
                fclose(fid);
                return -1;
        }
        else {
                Logger(Debug, "Traversing the files in directory [%s].\n", directory);
        }

        struct dirent *dp;
        while ((dp = readdir(dirp)) != NULL) {
                if (dp->d_name[0] != '.' && strcmp(dp->d_name, HASHES_FILE) != 0) {
                        sprintf(path, "%s/%s", directory, dp->d_name);
                        if (sha1(path, md) == 0)
                                fprintf(fid, "%08x%08x%08x%08x%08x  %s\n", md[0], md[1], md[2], md[3], md[4], dp->d_name);
                        else
                                Logger(Important, "Unable to compute the SHA-1 message digest for [%s].\n", path);
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

        bool success;
        double tend, dt;
        std::vector<Entity*> entities;
        lcg::generators::Waveform *stimulus;

        if (parse_configuration_file(opts.configFile, entities, &tend, &dt) != 0) {
                Logger(Critical, "Error while parsing configuration file. Aborting.\n");
                exit(1);
        }

        SetGlobalDt(dt);

        Logger(Debug, "Number of trials: %d.\n", opts.nTrials);
        Logger(Debug, "Inter-trial interval: %g sec.\n", (double) opts.iti * 1e-6);

        for (int i=0; i<opts.nTrials; i++) {
                Logger(Info, "Trial: %d of %d.\n", i+1, opts.nTrials);
                ResetGlobalTime();
                success = Simulate(entities,tend);
                if (!success || KILL_PROGRAM())
                        goto endMain;
                if (opts.enableReplay)
                        store(argc, argv, entities);
                if (i != opts.nTrials-1)
                        usleep(opts.iti);
        }

endMain:
        for (int i=0; i<entities.size(); i++)
                delete entities[i];

        return 0;
}

