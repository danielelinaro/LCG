#include <string.h>
#include "short.h"
#include "utils.h"

lcg::Entity* ShortFactory(string_dict& args)
{
        uint inputSubdevice, readChannel, inputRange, outputSubdevice, writeChannel, reference, id;
        std::string deviceFile, rangeStr, referenceStr, inputUnits, outputUnits;
        double inputConversionFactor, outputConversionFactor;
        bool resetOutput;

        id = lcg::GetIdFromDictionary(args);

        if ( ! lcg::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "inputSubdevice", &inputSubdevice) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "readChannel", &readChannel) ||
             ! lcg::CheckAndExtractDouble(args, "inputConversionFactor", &inputConversionFactor) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "outputSubdevice", &outputSubdevice) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "writeChannel", &writeChannel) ||
             ! lcg::CheckAndExtractDouble(args, "outputConversionFactor", &outputConversionFactor)) {
                lcg::Logger(lcg::Debug, "Short: missing parameter.\n");
                lcg::Logger(lcg::Critical, "Unable to build a short.\n");
                return NULL;
        }

        if (! lcg::CheckAndExtractValue(args, "inputRange", rangeStr)) {
                inputRange = PLUS_MINUS_TEN;
        }
        else {
                if (rangeStr.compare("PlusMinusTen") == 0 ||
                    rangeStr.compare("[-10,+10]") == 0 ||
                    rangeStr.compare("+-10") == 0) {
                        inputRange = PLUS_MINUS_TEN;
                }
                else if (rangeStr.compare("PlusMinusFive") == 0 ||
                         rangeStr.compare("[-5,+5]") == 0 ||
                         rangeStr.compare("+-5") == 0) {
                        inputRange = PLUS_MINUS_FIVE;
                }
                else if (rangeStr.compare("PlusMinusOne") == 0 ||
                         rangeStr.compare("[-1,+1]") == 0 ||
                         rangeStr.compare("+-1") == 0) {
                        inputRange = PLUS_MINUS_ONE;
                }
                else if (rangeStr.compare("PlusMinusZeroPointTwo") == 0 ||
                         rangeStr.compare("[-0.2,+0.2]") == 0 ||
                         rangeStr.compare("+-0.2") == 0) {
                        inputRange = PLUS_MINUS_ZERO_POINT_TWO;
                }
                else {
                        lcg::Logger(lcg::Critical, "Unknown input range: [%s].\n", rangeStr.c_str());
                        lcg::Logger(lcg::Critical, "Unable to build a short.\n");
                        return NULL;
                }
        }

        if (! lcg::CheckAndExtractValue(args, "reference", referenceStr)) {
                reference = GRSE;
        }
        else {
                if (referenceStr.compare("GRSE") == 0) {
                        reference = GRSE;
                }
                else if (referenceStr.compare("NRSE") == 0) {
                        reference = NRSE;
                }
                else {
                        lcg::Logger(lcg::Critical, "Unknown reference mode: [%s].\n", referenceStr.c_str());
                        lcg::Logger(lcg::Critical, "Unable to build a short.\n");
                        return NULL;
                }
        }

        if (! lcg::CheckAndExtractValue(args, "inputUnits", inputUnits)) {
                inputUnits = "mV";
        }

        if (! lcg::CheckAndExtractValue(args, "outputUnits", outputUnits)) {
                outputUnits = "pA";
        }

        if (! lcg::CheckAndExtractBool(args, "resetOutput", &resetOutput)) {
                resetOutput = (getenv("LCG_RESET_OUTPUT") == NULL ? false :
                        (strcmp(getenv("LCG_RESET_OUTPUT"),"yes") ? false : true));
        }

        return new lcg::Short(deviceFile.c_str(), inputSubdevice, outputSubdevice, readChannel,
                                writeChannel, inputConversionFactor, outputConversionFactor,
                                inputUnits, outputUnits, inputRange, reference, resetOutput, id);
}

namespace lcg {

Short::Short(const char *deviceFile, uint inputSubdevice, uint outputSubdevice, uint readChannel,
             uint writeChannel, double inputConversionFactor, double outputConversionFactor,
             const std::string& inputUnits, const std::string& outputUnits, uint inputRange,
             uint reference, bool resetOutput, uint id) : Entity(id),
#if defined(HAVE_LIBCOMEDI)
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor, inputRange, reference),
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor, reference)
#elif defined(HAVE_LIBANALOGY)
          m_input(deviceFile, inputSubdevice, &readChannel, 1, inputRange, reference),
          m_output(deviceFile, outputSubdevice, &writeChannel, 1, PLUS_MINUS_TEN, reference)
#endif
{
        setName("Short");
        setUnits(inputUnits + "/" + outputUnits);
}

Short::~Short()
{
        terminate();
}

bool Short::initialise()
{
        if (! m_output.initialise() || ! m_input.initialise())
                return false;
        m_data = m_input.read();
        m_output.write(m_data);
        return true;
}

void Short::terminate()
{
        if (m_resetOutput)
                m_output.write(0.0);
}

void Short::step()
{
        m_data = m_input.read();
        m_output.write(m_data);
}

double Short::output()
{
        return m_data;
}

} // namespace lcg

