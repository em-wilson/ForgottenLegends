#ifndef __ILOGGER_H__
#define __ILOGGER_H__

class ILogger {
    public:
        virtual void log_string( const char *str ) = 0;
        virtual void log_stringf(const char *fmt, ...) = 0;
};

#endif