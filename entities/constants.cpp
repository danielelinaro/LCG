#include "constants.h"
#include "events.h"
#include "utils.h"
#include <stdio.h>
#include <string>

lcg::Entity* ConstantFactory(string_dict& args)
{
        uint id;
        double value;
        std::string units;
        id = lcg::GetIdFromDictionary(args);
        if (!lcg::CheckAndExtractDouble(args, "value", &value)) {
                lcg::Logger(lcg::Critical, "Unable to build a Constant.\n");
                return NULL;
        }
        if (!lcg::CheckAndExtractValue(args, "units", units))
                units = "N/A";
        return new lcg::Constant(value, units, id);
}

lcg::Entity* ConstantFromFileFactory(string_dict& args)
{
        uint id;
        std::string filename, units;
        id = lcg::GetIdFromDictionary(args);
        if (!lcg::CheckAndExtractValue(args, "filename", filename))
                filename = LOGFILE;
        if (!lcg::CheckAndExtractValue(args, "units", units))
                units = "N/A";
        lcg::Logger(lcg::Info, "ConstantFromFile: using file [%s].\n", filename.c_str());
        return new lcg::ConstantFromFile(filename, units, id);
}

namespace lcg {

Constant::Constant(double value, const std::string& units, uint id)
        : Entity(id)
{
        m_parameters["value"] = value;
        setName("Constant");
        setUnits(units);
}

void Constant::setValue(double value) 
{
        m_parameters["value"] = value;
}

bool Constant::initialise()
{
        return true;
}

double Constant::output()
{
        return m_parameters["value"];
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
        return true;
}

} // namespace lcg

