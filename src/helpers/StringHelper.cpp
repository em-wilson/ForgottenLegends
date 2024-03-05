#include <string>
#include "StringHelper.h"

using std::string;
using std::tolower;
using std::toupper;

string StringHelper::capitalize( string str ) {
    str[0] = toupper(str[0]);
    return str;
}

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return TRUE if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool StringHelper::str_prefix( string astr, string bstr ) { return str_prefix(astr.c_str(), bstr.c_str()); }
bool StringHelper::str_prefix( const char *astr, const char *bstr )
{
    if ( !astr || !bstr ) {
        return true;
    }

    for ( ; *astr; astr++, bstr++ ) {
        if ( tolower(*astr) != tolower(*bstr) )
            return true;
    }

    return false;
}