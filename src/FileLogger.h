#ifndef __FILE_LOGGER_H__
#define __FILE_LOGGER_H__

#include "interfaces/ILogger.h"

class FileLogger : public ILogger {
    public:
        void log_string( const std::string str );
        void log_stringf(const std::string fmt, ...);
};

#endif