#include "converter.h"

namespace dynclamp {

Converter::Converter(std::string propertyName, uint id)
        : Entity(id), m_propertyName(propertyName)
{
        setName("Converter");
}

void Converter::step()
{
        if (!m_first && m_inputs[0] != m_previousInput) {
                // call the function to change the property on the post entity
                m_first = false;
        }
        m_previousInput = m_inputs[0];
}

double Converter::output()
{
        return 0.0;
}

bool Converter::initialise()
{
        m_first = true;
        m_previousInput = 0.0;
        return true;
}

} // namespace dynclamp

