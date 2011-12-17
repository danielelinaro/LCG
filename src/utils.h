#ifndef UTILS_H
#define UTILS_H

namespace dynclamp
{

typedef enum
{
        All = 0, Debug, Info, Critical
} LogLevel;

void SetLoggingLevel(LogLevel level);
void Logger(LogLevel level, const char *fmt, ...);

} // namespace dynclamp

#endif
