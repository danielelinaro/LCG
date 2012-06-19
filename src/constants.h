#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "entity.h"

namespace dynclamp
{

class Constant : public Entity
{
public:
        Constant(double value, const std::string& units = "N/A", uint id = GetId());
        double value() const;
        void setValue(double value);
        virtual void step();
        virtual double output() const;
        virtual bool initialise();
private:
        double m_value;
};

class ConstantFromFile : public Constant
{
public:
        ConstantFromFile(const std::string& filename, const std::string& units = "N/A", uint id = GetId());
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

dynclamp::Entity* ConstantFactory(dictionary& args);
dynclamp::Entity* ConstantFromFileFactory(dictionary& args);
	
#ifdef __cplusplus
}
#endif

#endif

