#include "constants.h"
#include "events.h"
#include "engine.h"
#include <stdio.h>
#include <string>

dynclamp::Entity* ConstantFactory(dictionary& args)
{
        uint id;
        double value;
        std::string units;
        id = dynclamp::GetIdFromDictionary(args);
        if (!dynclamp::CheckAndExtractDouble(args, "value", &value)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build a Constant.\n");
                return NULL;
        }
        if (!dynclamp::CheckAndExtractValue(args, "units", units))
                units = "N/A";
        return new dynclamp::Constant(value, units, id);
}

dynclamp::Entity* ConstantFromFileFactory(dictionary& args)
{
        uint id;
        std::string filename, units;
        id = dynclamp::GetIdFromDictionary(args);
        if (!dynclamp::CheckAndExtractValue(args, "filename", filename))
                filename = LOGFILE;
        if (!dynclamp::CheckAndExtractValue(args, "units", units))
                units = "N/A";
        dynclamp::Logger(dynclamp::Info, "ConstantFromFile: using file [%s].\n", filename.c_str());
        return new dynclamp::ConstantFromFile(filename, units, id);
}

namespace dynclamp {

Constant::Constant(double value, const std::string& units, uint id)
        : Entity(id), m_value(value)
{
        m_parameters.push_back(m_value);
        m_parametersNames.push_back("value");
        setName("Constant");
        setUnits(units);
}

void Constant::setValue(double value) 
{
        m_value = value;
}

double Constant::value() const
{
        return m_value;
}

bool Constant::initialise()
{
        return true;
}

double Constant::output() const
{
        return m_value;
}

void Constant::step()
{}

ConstantFromFile::ConstantFromFile(const std::string& filename, const std::string& units, uint id)
        : Constant(0.0, units, id), m_filename(filename)
{
        setName("ConstantFromFile");
}

void ConstantFromFile::setFilename(const std::string& filename) 
{
        m_filename = filename;
}

std::string ConstantFromFile::filename() const
{
        return m_filename;
}

bool ConstantFromFile::initialise()
{
        double value;
        FILE *fid = fopen(m_filename.c_str(),"r");
        if (fid != NULL) {
                fscanf(fid, "%lf", &value);
                fclose(fid);
        }
        else {
                Logger(Important, "ConstantFromFile: File [%s] not found.\n", m_filename.c_str());
                value = 0.0;
        }
        setValue(value);
}

} // namespace dynclamp

