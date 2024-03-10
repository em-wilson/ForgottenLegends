#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <iostream>
#include <string>
#include "FileLogger.h"
#include "Wiznet.h"

using std::endl;
using std::string;

extern time_t current_time;			  /* time of this pulse */
/*
 * Writes a string to the log.
 */
void FileLogger::log_string( const string str )
{
    char *strtime;

    strtime                    = ctime( &current_time );
    strtime[strlen(strtime)-1] = '\0';
    std::cerr << strtime << " :: " << str << endl;
    snprintf(strtime, sizeof(strtime), "%s", str.c_str());
    Wiznet::instance()->report( strtime, 0, 0, WIZ_LOG, WIZ_SECURE, 0);
    return;
}

void FileLogger::log_stringf(const string fmt, ...)
{
	char buf[2 * 4608];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt.c_str(), args);
	va_end(args);

	log_string(buf);
}