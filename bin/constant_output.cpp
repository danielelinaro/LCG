#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace po = boost::program_options;
namespace fs = boost::filesystem;

#include "types.h"
#include "utils.h"
#include "analog_io.h"
using namespace dynclamp;

int main(int argc, char *argv[])
{
#ifdef HAVE_LIBCOMEDI

        po::options_description description(
                        "\nThis program outputs a constant value to a specified channel of the DAQ board.\n\n"
                        "Allowed options are");
        po::variables_map options;
        std::string deviceFile, refStr;
        uint subdevice, channel, ref;
        double value, conversionFactor;

        try {
                description.add_options()
                        ("help,h", "print help message")
                        ("device,d", po::value<std::string>(&deviceFile)->default_value("/dev/comedi0"),
                         "specify the path of the device")
                        ("subdevice,s", po::value<uint>(&subdevice)->default_value(1),
                         "specify output subdevice")
                        ("channel,c", po::value<uint>(&channel)->default_value(0),
                         "specify output channel")
                        ("value,v", po::value<double>(&value)->default_value(0.0),
                         "specify value to output (in pA)")
                        ("factor,f", po::value<double>(&conversionFactor)->default_value(0.001),
                         "specify conversion factor (in V/pA)")
                        ("reference,r", po::value<std::string>(&refStr)->default_value("grse"),
                         "specify reference type: allowed values are "
                         "'grse' for ground-referenced single ended or "
                         "'nrse' for non-referenced single ended.");

                po::store(po::parse_command_line(argc, argv, description), options);
                po::notify(options);    

                if (options.count("help")) {
                        std::cout << description << "\n";
                        exit(0);
                }

                if (!fs::exists(deviceFile)) {
                        std::cout << "Device file \"" << deviceFile << "\" not found. Aborting...\n";
                        exit(1);
                }

                if (boost::iequals(refStr, "grse")) {
                        ref = GRSE;
                }
                else if (boost::iequals(refStr, "nrse")) {
                        ref = NRSE;
                }
                else {
                        Logger(Critical, "Unknown reference type [%s].\n", refStr.c_str());
                        exit(1);
                }

        } catch (std::exception e) {
                std::cout << e.what() << std::endl;
                exit(1);
        }

        Logger(Important, "Outputting %g on device [%s], subdevice [%d], channel [%d].\n",
                value, deviceFile.c_str(), subdevice, channel);
        dynclamp::ComediAnalogOutputSoftCal output(deviceFile.c_str(), subdevice, channel, conversionFactor, ref);
        output.write(value);
        FILE *fid = fopen("/tmp/last_value.out","w");
        if (fid != NULL) {
                fprintf(fid, "%e", value);
                fclose(fid);
                Logger(Important, "Saved output value to /tmp/last_value.out.\n");
        }
        else {
                Logger(Important, "Unable to save output value to /tmp/last_value.out.\n");
        }

#else

        std::cout << "This program requires the Comedi library." << std::endl;

#endif
        return 0;
}

