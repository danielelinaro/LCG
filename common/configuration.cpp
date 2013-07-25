#include "configuration.h"
#include "daq_io.h"

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

#define CONFIG_FILE ".lcg-non-rt"

std::vector<std::string> split_string(const std::string &source, const char *delimiter = " ", bool keepEmpty = false)
{
    std::vector<std::string> results;
    size_t prev=0, next=0;
    while ((next = source.find_first_of(delimiter, prev)) != std::string::npos) {
        if (keepEmpty || (next - prev != 0))
            results.push_back(source.substr(prev, next - prev));
        prev = next + 1;
    }
    if (prev < source.size())
        results.push_back(source.substr(prev));
    return results;
}

int parse_configuration_file(const char *filename, std::vector<InputChannel*>& input_channels, std::vector<OutputChannel*>& output_channels)
{
        ptree pt;
        std::string str, device;
        std::vector<std::string> channels_v, factors_v, units_v, filenames_v;
        uint subdevice, range, reference;
        int i;
        struct stat buf;
        char *home, configFile[FILENAME_MAXLEN] = {0};
        char chan[128];

        if (!input_channels.empty()) {
                for (i=0; i<input_channels.size(); i++)
                        delete input_channels[i];
                input_channels.clear();
        }
        if (!output_channels.empty()) {
                for (i=0; i<output_channels.size(); i++)
                        delete output_channels[i];
                output_channels.clear();
        }

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
        
        if (pt.count("AnalogInput") != 0) {

                // let's parse first what's common among all channels...
                try {
                        device = pt.get<std::string>("AnalogInput.device");
                } catch(...) {
                        device = getenv("COMEDI_DEVICE");
                }
                Logger(Debug, "input device: %s\n", device.c_str());

                try {
                        subdevice = pt.get<int>("AnalogInput.subdevice");
                } catch(...) {
                        subdevice = atoi(getenv("AI_SUBDEVICE"));
                }
                Logger(Debug, "input subdevice: %d\n", subdevice);

                try {
                        str = pt.get<std::string>("AnalogInput.range");
                } catch(...) {
                        str = getenv("RANGE");
                }
                if (str.compare("PlusMinusTen") == 0 ||
                    str.compare("[-10,+10]") == 0 ||
                    str.compare("+-10") == 0) {
                        range = PLUS_MINUS_TEN;
                }
                else if (str.compare("PlusMinusFive") == 0 ||
                         str.compare("[-5,+5]") == 0 ||
                         str.compare("+-5") == 0) {
                        range = PLUS_MINUS_FIVE;
                }
                else if (str.compare("PlusMinusOne") == 0 ||
                         str.compare("[-1,+1]") == 0 ||
                         str.compare("+-1") == 0) {
                        range = PLUS_MINUS_ONE;
                }
                else if (str.compare("PlusMinusZeroPointTwo") == 0 ||
                         str.compare("[-0.2,+0.2]") == 0 ||
                         str.compare("+-0.2") == 0) {
                        range = PLUS_MINUS_ZERO_POINT_TWO;
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
                        reference = AREF_GROUND;
                }
                else if (str.compare("NRSE") == 0) {
                        reference = AREF_COMMON;
                }
                else {
                        Logger(Critical, "[%s]: Unknown ground reference.\n", str.c_str());
                        return -1;
                }

                // ... and then what can be different across channels.
                try {
                        channels_v = split_string(pt.get<std::string>("AnalogInput.channels"), ",");
                } catch(...) {
                        channels_v.push_back(getenv("AI_CHANNEL"));
                }
                Logger(Debug, "number of input channels: %d.\n", channels_v.size());
                 
                try {
                        factors_v = split_string(pt.get<std::string>("AnalogInput.conversionFactors"), ",");
                } catch(...) {
                        factors_v.push_back(getenv("AI_CONVERSION_FACTOR"));
                }
                if (factors_v.size() == 1) {
                        for (i=1; i<channels_v.size(); i++)
                                factors_v.push_back(factors_v[0]);
                }
                else if (factors_v.size() != channels_v.size()) {
                        Logger(Critical, "Number of channels != Number of conversion factors.\n");
                        return -1;
                }

                try {
                        units_v = split_string(pt.get<std::string>("AnalogInput.units"), ",");
                } catch(...) {
                        units_v.push_back(getenv("INPUT_UNITS"));
                }
                if (units_v.size() == 1) {
                        for (i=1; i<channels_v.size(); i++)
                                units_v.push_back(units_v[0]);
                }
                else if (units_v.size() != channels_v.size()) {
                        Logger(Critical, "Number of channels != Number of units.\n");
                        return -1;
                }
                
                for (i=0; i<channels_v.size(); i++) {
                        input_channels.push_back(new InputChannel(device.c_str(), subdevice, range, reference,
                                                atoi(channels_v[i].c_str()), atof(factors_v[i].c_str()), 20000, units_v[i].c_str()));
                        //print_channel(input_channels[i]);
                }
        }
        
        //// OUTPUT ////

        channels_v.clear();
        factors_v.clear();
        units_v.clear();

        if (pt.count("AnalogOutput") == 0) {
                Logger(Info, "The configuration file [%s] does not contain a 'AnalogOutput' record: "
                             "will only record spontaneous activity.\n", filename);
                return 0;
        }

        range = PLUS_MINUS_TEN;
        try {
                device = pt.get<std::string>("AnalogOutput.device");
        } catch(...) {
                device = getenv("COMEDI_DEVICE");
        }
        Logger(Debug, "output device: %s\n", device.c_str());

        try {
                subdevice = pt.get<int>("AnalogOutput.subdevice");
        } catch(...) {
                subdevice = atoi(getenv("AO_SUBDEVICE"));
        }
        Logger(Debug, "output subdevice: %d\n", subdevice);

        try {
                str = pt.get<std::string>("AnalogOutput.reference");
        } catch(...) {
                str = getenv("GROUND_REFERENCE");
        }
        if (str.compare("GRSE") == 0) {
                reference = AREF_GROUND;
        }
        else if (str.compare("NRSE") == 0) {
                reference = AREF_COMMON;
        }
        else {
                Logger(Critical, "[%s]: Unknown ground reference.\n", str.c_str());
                goto dealloc_and_fail;
        }

        try {
                channels_v = split_string(pt.get<std::string>("AnalogOutput.channels"), ",");
        } catch(...) {
                channels_v.push_back(getenv("AO_CHANNEL"));
        }
        Logger(Debug, "number of output channels: %d.\n", channels_v.size());
         
        try {
                factors_v = split_string(pt.get<std::string>("AnalogOutput.conversionFactors"), ",");
        } catch(...) {
                factors_v.push_back(getenv("AO_CONVERSION_FACTOR"));
        }
        if (factors_v.size() == 1) {
                for (i=1; i<channels_v.size(); i++)
                        factors_v.push_back(factors_v[0]);
        }
        else if (factors_v.size() != channels_v.size()) {
                Logger(Critical, "Number of channels != Number of conversion factors.\n");
                goto dealloc_and_fail;
        }

        try {
                units_v = split_string(pt.get<std::string>("AnalogOutput.units"), ",");
        } catch(...) {
                units_v.push_back(getenv("OUTPUT_UNITS"));
        }
        if (units_v.size() == 1) {
                for (i=1; i<channels_v.size(); i++)
                        units_v.push_back(units_v[0]);
        }
        else if (units_v.size() != channels_v.size()) {
                Logger(Critical, "Number of channels != Number of units.\n");
                goto dealloc_and_fail;
        }

        try {
                filenames_v = split_string(pt.get<std::string>("AnalogOutput.stimfiles"), ",");
        } catch(...) {
                for (i=0; i<channels_v.size(); i++)
                        filenames_v.push_back("");
        }
        if (filenames_v.size() == 1) {
                for (i=1; i<channels_v.size(); i++)
                        filenames_v.push_back(filenames_v[0]);
        }
        else if (filenames_v.size() != channels_v.size()) {
                Logger(Critical, "Number of channels != Number of stimfiles.\n");
                goto dealloc_and_fail;
        }

        for (i=0; i<channels_v.size(); i++) {
                output_channels.push_back(new OutputChannel(device.c_str(), subdevice, range, reference,
                                        atoi(channels_v[i].c_str()), atof(factors_v[i].c_str()), 20000,
                                        units_v[i].c_str(), filenames_v[i].c_str()));
                //print_channel(output_channels[i]);
        }
        
        Logger(Debug, "Successfully parsed configuration file [%s].\n", configFile);

        return 0;

dealloc_and_fail:

        for (i=0; i<input_channels.size(); i++)
                delete input_channels[i];
        input_channels.clear();
        for (i=0; i<output_channels.size(); i++)
                delete output_channels[i];
        output_channels.clear();

        Logger(Debug, "Error while parsing configuration file [%s].\n", configFile);

        return -1;
}

void print_channel(Channel* chan)
{
        OutputChannel *out = dynamic_cast<OutputChannel*>(chan);
        if (out)
                printf("Device type: OUTPUT\n");
        else
                printf("Device type: INPUT\n");
        printf("Device: %s\n", chan->device());
        printf("Subdevice: %d\n", chan->subdevice());
        printf("Range: %d\n", chan->range());
        printf("Reference: %d\n", chan->reference());
        printf("Conversion factor: %g\n", chan->conversionFactor());
        printf("Channel: %d\n", chan->channel());
        printf("Units: %s\n", chan->units());
        if (out)
                printf("Stimfile: %s\n", out->stimulus()->stimulusFile());
}

