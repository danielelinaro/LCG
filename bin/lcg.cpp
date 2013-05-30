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
#include "entity.h"
#include "utils.h"
#include "waveform.h"
#include "engine.h"
#include "recorders.h"

#include "sha1.h"

#define DCLAMP_DIR    ".lcg"
#define TMP_DIR       ".tmp"
#define REPLAY_SCRIPT "replay"
#define HASHES_FILE   "hashes.sha"

using namespace lcg;
using namespace lcg::generators;
using namespace lcg::recorders;

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

bool sha1(const char *filename, unsigned *messageDigest)
{
        FILE *fp = fopen(filename, "rb");
        if (fp == NULL) {
                Logger(Important, "Unable to open [%s] for computing the SHA-1 digest.\n", filename);
                return false;
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
            return false;
        }
        return true;
}

bool Store(int argc, char *argv[], const std::vector<Entity*>& entities)
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
                        return false;
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
                if (errno == EEXIST)
                        Logger(Debug, "mkdir: [%s]: directory exists.\n", directory);
                else
                        Logger(Important, "mkdir: [%s]: %s.\n", directory, strerror(errno));
                //return false;
        }

        // write a very simple script that runs lcg again with the options used for calling it now
        sprintf(path, "%s/%s", directory, REPLAY_SCRIPT);
        FILE *fid = fopen(path, "w");
        if (fid == NULL) {
                Logger(Important, "Unable to create file [%s].\n", path);
                return false;
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
                                return false;
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
                                return false;
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
                return false;
        }
        else {
                Logger(Debug, "Successfully created hashes file [%s].\n", path);
        }

        if (rec != NULL) {
                if (sha1(rec->filename(), md))
                        fprintf(fid, "%08x%08x%08x%08x%08x  %s\n", md[0], md[1], md[2], md[3], md[4], rec->filename());
                else
                        Logger(Important, "Unable to compute the SHA-1 message digest for [%s].\n", rec->filename());
        }

        DIR *dirp = opendir(directory);
        if (dirp == NULL) {
                Logger(Important, "Unable to open directory [%s] for traversing the files.\n", directory);
                return false;
        }
        else {
                Logger(Debug, "Traversing the files in directory [%s].\n", directory);
        }

        struct dirent *dp;
        while ((dp = readdir(dirp)) != NULL) {
                if (dp->d_name[0] != '.' && strcmp(dp->d_name, HASHES_FILE) != 0) {
                        sprintf(path, "%s/%s", directory, dp->d_name);
                        if (sha1(path, md))
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

        return true;
}

int main(int argc, char *argv[])
{
        CommandLineOptions opt;

        if (!SetupSignalCatching()) {
                Logger(Critical, "Unable to setup signal catching functionalities. Aborting.\n");
                exit(1);
        }

        ParseCommandLineOptions(argc, argv, &opt);

        if (opt.configFile.compare("") == 0) {
                Logger(Critical, "No configuration file specified. Aborting.\n");
                exit(1);
        }

        double tend, dt;
        std::vector<Entity*> entities;
        lcg::generators::Waveform *stimulus;

        if (!ParseConfigurationFile(opt.configFile, entities, &tend, &dt)) {
                Logger(Critical, "Error while parsing configuration file. Aborting.\n");
                exit(1);
        }

        Logger(Debug, "opt.tend = %g sec.\n", opt.tend);
        if (opt.tend != -1)
                // the duration specified in the command line has precedence over the one in the configuration file
                tend = opt.tend;

        if (opt.dt != -1)
                // the timestep specified in the command line has precedence over the one in the configuration file
                dt = opt.dt;
        if (dt == -1) {
                dt = 1.0 / 20000;
                Logger(Debug, "Using default timestep (%g sec -> %g Hz).\n", dt, 1.0/dt);
        }

        SetGlobalDt(dt);

        Logger(Debug, "Number of trials: %d.\n", opt.nTrials);
        Logger(Debug, "Inter-trial interval: %g sec.\n", (double) opt.iti * 1e-6);

        bool success;
        if (opt.stimulusFiles.size() > 0) {
                for (int i=0; i<entities.size(); i++) {
                        if ((stimulus = dynamic_cast<lcg::generators::Waveform*>(entities[i])) != NULL)
                                break;
                }

                if (stimulus == NULL) {
                        Logger(Critical, "You need to have at least one Stimulus in your configuration "
                                         "file if you specify a stimulus file. Aborting.\n");
                        goto endMain;
                }

                Logger(Debug, "Number of batches: %d.\n", opt.nBatches);
                Logger(Debug, "Inter-batch interval: %g sec.\n", (double) opt.ibi * 1e-6);

                for (int i=0; i<opt.nBatches; i++) {
                        for (int j=0; j<opt.stimulusFiles.size(); j++) {
                                stimulus->setStimulusFile(opt.stimulusFiles[j].c_str());
                                for (int k=0; k<opt.nTrials; k++) {
                                        Logger(Debug, "Batch: %d, stimulus: %d, trial: %d. (of %d,%d,%d).\n",
                                                i+1, j+1, k+1, opt.nBatches, opt.stimulusFiles.size(), opt.nTrials);
                                        ResetGlobalTime();
                                        success = Simulate(entities,stimulus->duration());
                                        if (!success || KILL_PROGRAM())
                                                goto endMain;
                                        if (k != opt.nTrials-1)
                                                usleep(opt.iti);
                                        if (KILL_PROGRAM())
                                                goto endMain;
                                }
                                if (j != opt.stimulusFiles.size()-1)
                                        usleep(opt.iti);
                                if (KILL_PROGRAM())
                                        goto endMain;
                        }
                        if (i != opt.nBatches-1)
                                usleep(opt.ibi);
                        if (KILL_PROGRAM())
                                goto endMain;
                }

        }
        else {
                if (tend == -1) {
                        Logger(Critical, "The duration of the simulation was not specified. Aborting.\n");
                        exit(1);
                }
                for (int i=0; i<opt.nTrials; i++) {
                        Logger(Important, "Trial: %d of %d.\n", i+1, opt.nTrials);
                        ResetGlobalTime();
                        success = Simulate(entities,tend);
                        if (!success || KILL_PROGRAM())
                                goto endMain;
                        if (i != opt.nTrials-1)
                                usleep(opt.iti);
                        if (KILL_PROGRAM())
                                goto endMain;
                        if (opt.enableReplay)
                                Store(argc, argv, entities);
                }
        }

endMain:
        for (int i=0; i<entities.size(); i++)
                delete entities[i];

        return 0;
}

