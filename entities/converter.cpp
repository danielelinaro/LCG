#include "converter.h"

lcg::Entity* ConverterFactory(string_dict& args)
{
        uint id;
        std::string parameterName;
        id = lcg::GetIdFromDictionary(args);
        if ( ! lcg::CheckAndExtractValue(args, "parameterName", parameterName)) {
                lcg::Logger(lcg::Critical, "Unable to build a Converter.\n");
                return NULL;
        }
        return new lcg::Converter(parameterName, id);
        
}

namespace lcg {

Converter::Converter(std::string parameterName, uint id)
        : Entity(id), m_parameterName(parameterName) 
{
        setName("Converter");
}

void Converter::step()
{
        if (!m_first && m_inputs[0] != m_previousInput) {
                //Logger(Critical, "%s >> %g -> %g\n", m_parameterName.c_str(), m_previousInput, m_inputs[0]);
                try {
                        // call the function to change the property of the post entity
                        m_post[0]->parameter(m_parameterName) = m_inputs[0];
                } catch (const char *err) {
                        Logger(Critical, "Unable to change the parameter [%s]. Error: %s.\n", m_parameterName.c_str(), err);
                }
        }
        m_first = false;
        m_previousInput = m_inputs[0];
}

double Converter::output()
{
        return 0.0;
}

bool Converter::initialise()
{
        m_first = true;
        m_previousInput = 0.0; // just for completeness, it is not used on the first iteration
        return true;
}

} // namespace lcg

