#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "entity.h"
#include "common.h"

namespace lcg
{

class Constant : public Entity
{
public:
        Constant(double value, const std::string& units = "N/A", uint id = GetId());
        void setValue(double value);
        virtual void step();
        virtual double output();
        virtual bool initialise();
};

class ConstantFromFile : public Constant
{
public:
        ConstantFromFile(const std::string& filename = LOGFILE,
                         const std::string& units = "N/A",
                         uint id = GetId());
        virtual bool initialise();
        std::string filename() const;
        void setFilename(const std::string& filename);
private:
        std::string m_filename;
};

}

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* ConstantFactory(string_dict& args);
lcg::Entity* ConstantFromFileFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif

