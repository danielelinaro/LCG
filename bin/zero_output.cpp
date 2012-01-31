#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace po = boost::program_options;
namespace fs = boost::filesystem;

#include "types.h"
#include "analog_io.h"

int main(int argc, char *argv[])
{
#ifdef HAVE_LIBCOMEDI

        po::options_description description(
                        "\nThis program resets to zero the output of the DAQ board.\n\n"
                        "Allowed options are");
        po::variables_map options;
        std::string deviceFile;
        uint subdevice, channel;
        double factor = 0.0;

        try {
                description.add_options()
                        ("help,h", "print help message")
                        ("device,d", po::value<std::string>(&deviceFile)->default_value("/dev/comedi0"),
                         "specify the path of the device")
                        ("subdevice,s", po::value<uint>(&subdevice)->default_value(1),
                         "specify output subdevice")
                        ("channel,c", po::value<uint>(&channel)->default_value(1),
                         "specify write channel");

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

        } catch (std::exception e) {
                std::cout << e.what() << std::endl;
                exit(1);
        }

        dynclamp::ComediAnalogOutput output(deviceFile.c_str(), subdevice, channel, factor);
        output.write(0.0);

#else

        std::cout << "This program requires the Comedi library." << std::endl;

#endif
        return 0;
}

