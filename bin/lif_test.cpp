#include <vector>
#include <string>
#include <cstdio>
#include "utils.h"
#include "neurons.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using boost::property_tree::ptree;
using namespace dynclamp;
using namespace dynclamp::neurons;

bool ParseConfigFile(const std::string& filename, std::vector<Entity*>& entities)
{
        ptree pt;
        std::string key, value;
        int i;
        try {
                read_xml(filename, pt);
                i = 0;
                BOOST_FOREACH(ptree::value_type &v,
                              pt.get_child("dynamicclamp.entities")) {
                        dictionary args;
                        args["id"] = v.second.get<std::string>("id");
                        BOOST_FOREACH(ptree::value_type &vv,
                                        v.second.get_child("parameters")) {
                                key = vv.first;
                                value = vv.second.data();
                                if (args.count(key) == 1)
                                        Logger(Critical, "Duplicate parameter: [%s].\n", key.c_str());
                                else
                                        args.insert(std::pair<std::string,std::string>(key,value));
                        }
                        entities.push_back( EntityFactory(v.second.get<std::string>("name").c_str(), args) );
                }
        } catch(std::exception e) {
                Logger(Critical, "Error while parsing configuration file: %s.\n", e.what());
                return false;
        }
        return true;
}

int main()
{
        double t, tend = 2;

        std::vector<Entity*> entities;
        ParseConfigFile("configs/config.xml", entities);

        //LIFNeuron lif(0.08, 0.0075, 0.0014, -65.2, -70, -50, 220);
        LIFNeuron *lif = dynamic_cast<LIFNeuron*>(entities[0]);

        while ((t = GetGlobalTime()) <= tend) {
                printf("%e %e\n", t, lif->output());
                IncreaseGlobalTime();
                lif->step();
        }

        return 0;
}

