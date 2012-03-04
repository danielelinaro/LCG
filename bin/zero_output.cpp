#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

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
                        "\nThis program resets to zero the output of the DAQ board.\n\n"
                        "Allowed options are");
        po::variables_map options;
        std::string deviceFile, refStr;
        uint subdevice, channel, ref;

        try {
                description.add_options()
                        ("help,h", "print help message")
                        ("device,d", po::value<std::string>(&deviceFile)->default_value("/dev/comedi0"),
                         "specify the path of the device")
                        ("subdevice,s", po::value<uint>(&subdevice)->default_value(1),
                         "specify output subdevice")
                        ("channel,c", po::value<uint>(&channel)->default_value(-1),
                         "specify write channel (if this option is not specified, all channels are set to 0)")
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

                if (refStr.compare("grse")) {
                        ref = GRSE;
                }
                else if (refStr.compare("nrse")) {
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

        if (channel == -1) {
                comedi_t *device;
                device = comedi_open(deviceFile.c_str());
                if (device == NULL) {
                        Logger(Critical, "Unable to open device [%s].\n", deviceFile.c_str());
                        exit(1);
                }

                int nChannels = comedi_get_n_channels(device, subdevice);

                comedi_close(device);

                for (int ch=0; ch<nChannels; ch++) {
                        Logger(Info, "Outputting 0.0 on device [%s], subdevice [%d], channel [%d].\n",
                                        deviceFile.c_str(), subdevice, ch);
                        dynclamp::ComediAnalogOutput output(deviceFile.c_str(), subdevice, ch, 0.0, ref);
                        output.write(0.0);
                }
        }
        else {
                Logger(Info, "Outputting 0.0 on device [%s], subdevice [%d], channel [%d].\n",
                                deviceFile.c_str(), subdevice, channel);
                dynclamp::ComediAnalogOutput output(deviceFile.c_str(), subdevice, channel, 0.0, ref);
                output.write(0.0);
        }

#else

        std::cout << "This program requires the Comedi library." << std::endl;

#endif
        return 0;
}

