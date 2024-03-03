#include <string>
#include "StringHelper.h"

using std::tolower;

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return TRUE if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
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