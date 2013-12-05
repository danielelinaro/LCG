#include <dlfcn.h>
#include "stream.h"

namespace lcg {

Stream::Stream(uint id)
        : m_id(id), m_pre(), m_name("Stream"), m_units("N/A")
{}

Stream::~Stream()
{
        terminate();
}

uint Stream::id() const
{
        return m_id;
}

size_t Stream::numberOfParameters() const
{
        return m_parameters.size();
}

const std::map<std::string,double>& Stream::parameters() const
{
        return m_parameters;
}

double& Stream::parameter(const std::string name)
{
        if (m_parameters.count(name) == 0)
                throw "No parameter with such name";
        return m_parameters[name];
}

void Stream::connect(Stream *stream)
{
        Logger(All, "--- Stream::connect(Stream*) ---\n");

        if (stream == this) {
                Logger(Critical, "Can't connect a stream to itself (stream #%d).\n", stream->id());
                throw "Tried to connect stream to itself.";
        }

        stream->addPre(this);
}

void Stream::terminate()
{}

const std::vector<Stream*>& Stream::pre() const
{
        return m_pre;
}

void Stream::addPre(Stream *stream)
{
        Logger(All, "--- Stream::addPre(Stream*, double) ---\n");
        for (int i=0; i<m_pre.size(); i++) {
                if (m_pre[i]->id() == stream->id()) {
                        Logger(Important, "Trying to connect streams #%d and #%d more than once.\n");
                        return;
                }
        }
        m_pre.push_back(stream);
}

bool Stream::hasMetadata(size_t *ndims) const
{
        *ndims = 0;
        return false;
}

const double* Stream::metadata(size_t *dims, char *label) const
{
        return NULL;
}

const std::string& Stream::name() const
{
        return m_name;
}

const std::string& Stream::units() const
{
        return m_units;
}

void Stream::setName(const std::string& name)
{
        m_name = name;
}

void Stream::setUnits(const std::string& units)
{
        m_units = units;
}

Stream* StreamFactory(const char *streamName, string_dict& args)
{
        Stream *stream = NULL;
        StrmFactory builder;
        void *library, *addr;
        char symbol[50] = {0};

        library = dlopen(STREAMS_LIBNAME, RTLD_LAZY);
        if (library == NULL) {
                Logger(Critical, "Unable to open library %s.\n", STREAMS_LIBNAME);
                return NULL;
        }
        Logger(Debug, "Successfully opened library %s.\n", STREAMS_LIBNAME);

        sprintf(symbol, "%sFactory", streamName);

        addr = dlsym(library, symbol);
        if (addr == NULL) {
                Logger(Critical, "Unable to find symbol %s.\n", symbol);
                goto close_lib;
        }
        else {
                Logger(Debug, "Successfully found symbol %s.\n", symbol);
        }

        builder = (StrmFactory) addr;
        stream = builder(args);

close_lib:
        if (dlclose(library) == 0) {
                Logger(Debug, "Successfully closed library %s.\n", STREAMS_LIBNAME);
        }
        else {
                Logger(Critical, "Unable to close library %s: %s.\n", STREAMS_LIBNAME, dlerror());
        }

        return stream;
}

}

