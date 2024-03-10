#ifndef __ILOGGER_H__
#define __ILOGGER_H__

class ILogger {
    public:
        virtual void log_string( const std::string str ) = 0;
        virtual void log_stringf(const std::string fmt, ...) = 0;
};

#endif