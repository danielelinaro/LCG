#include "configuration.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
using boost::property_tree::ptree;

#include "utils.h"
using namespace lcg;

#define AREF_GROUND 0
#define AREF_COMMON 1

#define CONFIG_FILE ".lcg-non-rt"

int parse_configuration_file(const char *filename, io_options *input, io_options *output)
{
        ptree pt;
        std::string str;
        struct stat buf;
        char *home, configFile[FILENAME_MAXLEN] = {0};
        char channels[128];

        if (filename != NULL) {
                strncpy(configFile, filename, FILENAME_MAXLEN);
        }
        else {
                home = getenv("HOME");
                if (home == NULL) {
                        Logger(Critical, "Unable to get HOME environment variable.\n");
                        return -1;
                }
                sprintf(configFile, "%s/%s", home, CONFIG_FILE);
        }

        if (stat(configFile, &buf) == -1) {
                Logger(Critical, "%s: %s\n", configFile, strerror(errno));
                return -1;
        }

        read_ini(configFile, pt);

        //// INPUT ////
        
        if (pt.count("AnalogInput") == 0) {
                Logger(Critical, "The configuration file [%s] does not contain a 'AnalogInput' record. "
                                 "Aborting...\n", filename);
                input->nchan = 0;
                return -1;
        }

        try {
                str = pt.get<std::string>("AnalogInput.device");
                strncpy(input->device, str.c_str(), FILENAME_MAXLEN);
        } catch(...) {
                strncpy(input->device, getenv("COMEDI_DEVICE"), FILENAME_MAXLEN);
        }
        Logger(Debug, "input device: %s\n", input->device);
        try {
                input->subdevice = pt.get<int>("AnalogInput.subdevice");
        } catch(...) {
                input->subdevice = atoi(getenv("AI_SUBDEVICE"));
        }
        Logger(Debug, "input subdevice: %d\n", input->subdevice);
        try {
                input->conversionFactor = pt.get<double>("AnalogInput.conversionFactor");
        } catch(...) {
                input->conversionFactor = atof(getenv("AI_CONVERSION_FACTOR"));
        }
        Logger(Debug, "input conversion factor: %g\n", input->conversionFactor);
        try {
                str = pt.get<std::string>("AnalogInput.range");
        } catch(...) {
                str = getenv("RANGE");
        }
        if (str.compare("PlusMinusTen") == 0 ||
            str.compare("[-10,+10]") == 0 ||
            str.compare("+-10") == 0) {
                input->range = PLUS_MINUS_TEN;
        }
        else if (str.compare("PlusMinusFive") == 0 ||
                 str.compare("[-5,+5]") == 0 ||
                 str.compare("+-5") == 0) {
                input->range = PLUS_MINUS_FIVE;
        }
        else if (str.compare("PlusMinusOne") == 0 ||
                 str.compare("[-1,+1]") == 0 ||
                 str.compare("+-1") == 0) {
                input->range = PLUS_MINUS_ONE;
        }
        else if (str.compare("PlusMinusZeroPointTwo") == 0 ||
                 str.compare("[-0.2,+0.2]") == 0 ||
                 str.compare("+-0.2") == 0) {
                input->range = PLUS_MINUS_ZERO_POINT_TWO;
        }
        else {
                Logger(Critical, "[%s]: Unknown input range.\n", str.c_str());
                return -1;
        }
        try {
                str = pt.get<std::string>("AnalogInput.reference");
        } catch(...) {
                str = getenv("GROUND_REFERENCE");
        }
        if (str.compare("GRSE") == 0) {
                input->reference = AREF_GROUND;
        }
        else if (str.compare("NRSE") == 0) {
                input->reference = AREF_COMMON;
        }
        else {
                Logger(Critical, "[%s]: Unknown ground reference.\n", str.c_str());
                return -1;
        }
        try {
                str = pt.get<std::string>("AnalogInput.units");
                strncpy(input->units, str.c_str(), 10);
        } catch(...) {
                strncpy(input->units, getenv("INPUT_UNITS"), 10);
        }
        Logger(Debug, "input units: %s\n", input->units);
        try {
                str = pt.get<std::string>("AnalogInput.channels");
                strncpy(channels, str.c_str(), 128);
                if (channels[strlen(channels)-1] == ',')
                        channels[strlen(channels)-1] = '\0';
                char *cur, *next = channels;
                input->nchan = 0;
                int channels[1024];
                while ((cur = strsep(&next, ",")) != NULL)
                        channels[input->nchan++] = atoi(cur);
                input->channels = (uint *) malloc(input->nchan * sizeof(uint));
                for (int i=0; i<input->nchan; i++)
                        input->channels[i] = channels[i];
        } catch(...) {
                input->nchan = 1;
                input->channels = (uint *) malloc(sizeof(uint));
                input->channels[0] = atoi(getenv("AI_CHANNEL"));
        }
        Logger(Debug, "number of input channels: %d (", input->nchan);
        for (int i=0; i<input->nchan-1; i++)
                Logger(Debug, "%d,", input->channels[i]);
        Logger(Debug, "%d).\n", input->channels[input->nchan-1]);
                
        //// OUTPUT ////

        if (pt.count("AnalogOutput") == 0) {
                Logger(Info, "The configuration file [%s] does not contain a 'AnalogOutput' record: "
                             "will only record spontaneous activity.\n", filename);
                output->nchan = 0;
                return 0;
        }

        output->range = PLUS_MINUS_TEN;
        try {
                str = pt.get<std::string>("AnalogOutput.device");
                strncpy(output->device, str.c_str(), FILENAME_MAXLEN);
        } catch(...) {
                strncpy(output->device, getenv("COMEDI_DEVICE"), FILENAME_MAXLEN);
        }
        Logger(Debug, "output device: %s\n", output->device);
        try {
                output->subdevice = pt.get<int>("AnalogOutput.subdevice");
        } catch(...) {
                output->subdevice = atoi(getenv("AO_SUBDEVICE"));
        }
        Logger(Debug, "output subdevice: %d\n", output->subdevice);
        try {
                output->conversionFactor = pt.get<double>("AnalogOutput.conversionFactor");
        } catch(...) {
                output->conversionFactor = atof(getenv("AO_CONVERSION_FACTOR"));
        }
        Logger(Debug, "output conversion factor: %g\n", output->conversionFactor);
        try {
                str = pt.get<std::string>("AnalogOutput.reference");
        } catch(...) {
                str = getenv("GROUND_REFERENCE");
        }
        if (str.compare("GRSE") == 0) {
                output->reference = AREF_GROUND;
        }
        else if (str.compare("NRSE") == 0) {
                output->reference = AREF_COMMON;
        }
        else {
                Logger(Critical, "[%s]: Unknown ground reference.\n", str.c_str());
                return -1;
        }
        try {
                str = pt.get<std::string>("AnalogOutput.units");
                strncpy(output->units, str.c_str(), 10);
        } catch(...) {
                strncpy(output->units, getenv("INPUT_UNITS"), 10);
        }
        Logger(Debug, "output units: %s\n", output->units);
        try {
                str = pt.get<std::string>("AnalogOutput.channels");
                strncpy(channels, str.c_str(), 128);
                if (channels[strlen(channels)-1] == ',')
                        channels[strlen(channels)-1] = '\0';
                char *cur, *next = channels;
                output->nchan = 0;
                int channels[1024];
                while ((cur = strsep(&next, ",")) != NULL)
                        channels[output->nchan++] = atoi(cur);
                output->channels = (uint *) malloc(output->nchan * sizeof(uint));
                for (int i=0; i<output->nchan; i++)
                        output->channels[i] = channels[i];
        } catch(...) {
                output->nchan = 1;
                output->channels = (uint *) malloc(sizeof(uint));
                output->channels[0] = atoi(getenv("AO_CHANNEL"));
        }
        Logger(Debug, "number of output channels: %d (", output->nchan);
        for (int i=0; i<output->nchan-1; i++)
                Logger(Debug, "%d,", output->channels[i]);
        Logger(Debug, "%d).\n", output->channels[output->nchan-1]);
                
        Logger(Debug, "Successfully parsed configuration file [%s].\n", configFile);

        return 0;
}

