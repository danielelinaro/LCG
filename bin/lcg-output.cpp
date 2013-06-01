#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace po = boost::program_options;
namespace fs = boost::filesystem;

#include <comedilib.h>
#include "common.h"
#include "types.h"
#include "utils.h"
using namespace lcg;

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
                        ("device,d", po::value<std::string>(&deviceFile)->default_value(getenv("COMEDI_DEVICE")),
                         "specify the path of the device")
                        ("subdevice,s", po::value<uint>(&subdevice)->default_value(atoi(getenv("AO_SUBDEVICE"))),
                         "specify output subdevice")
                        ("channel,c", po::value<uint>(&channel)->default_value(atoi(getenv("AO_CHANNEL"))),
                         "specify output channel")
                        ("value,v", po::value<double>(&value)->default_value(0.0),
                         "specify value to output (in pA)")
                        ("factor,f", po::value<double>(&conversionFactor)->default_value(atof(getenv("AO_CONVERSION_FACTOR"))),
                         "specify conversion factor (in V/pA)")
                        ("reference,r", po::value<std::string>(&refStr)->default_value(getenv("GROUND_REFERENCE")),
                         "specify reference type: allowed values are "
                         "'GRSE' for ground-referenced single ended or "
                         "'NRSE' for non-referenced single ended.");

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

                if (boost::iequals(refStr, "GRSE")) {
                        ref = AREF_GROUND;
                }
                else if (boost::iequals(refStr, "NRSE")) {
                        ref = AREF_COMMON;
                }
                else {
                        Logger(Critical, "Unknown reference type [%s].\n", refStr.c_str());
                        exit(1);
                }

        } catch (std::exception e) {
                std::cout << e.what() << std::endl;
                exit(1);
        }

        uint range = 0;
        comedi_t *device;
        char *calibrationFile;
        comedi_calibration_t *calibration;
        comedi_polynomial_t converter;

        Logger(Info, "Outputting %g on device [%s], subdevice [%d], channel [%d].\n", value, deviceFile.c_str(), subdevice, channel);

        device = comedi_open(deviceFile.c_str());
        calibrationFile = comedi_get_default_calibration_path(device);
        calibration = comedi_parse_calibration_file(calibrationFile);
        comedi_get_softcal_converter(subdevice, channel, range, COMEDI_FROM_PHYSICAL, calibration, &converter);
        comedi_data_write(device, subdevice, channel, range, ref, comedi_from_physical(value*conversionFactor, &converter));

        comedi_cleanup_calibration(calibration);
        free(calibrationFile);
        comedi_close(device);

#else

        std::cout << "This program requires the Comedi library." << std::endl;

#endif

#ifdef ANALOG_IO
        FILE *fid = fopen(LOGFILE,"w");
        if (fid != NULL) {
                fprintf(fid, "%lf", value);
                fclose(fid);
                Logger(Info, "Saved output value to [%s].\n", LOGFILE);
        }
        else {
                Logger(Critical, "Unable to save output value to [%s].\n", LOGFILE);
        }
#endif
        return 0;
}
