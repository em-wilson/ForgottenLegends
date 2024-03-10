#include "ExtraDescription.h"

using std::string;
using std::vector;

ExtraDescription * ExtraDescription::clone() {
    auto clone = new ExtraDescription();
    clone->_keyword = _keyword;
    clone->_valid = _valid;
    clone->_description = _description;
    return clone;
}

/* Global functions */
bool is_name ( const char *str, const char *namelist );

bool ExtraDescription::isValid() { return _valid; }
string ExtraDescription::getKeyword() { return _keyword; }
void ExtraDescription::setKeyword(string value) { _keyword = value; }

string ExtraDescription::getDescription() { return _description; }
void ExtraDescription::setDescription(string value) { _description = value; }

/*
 * Get an extra description from a list.
 */
string ExtraDescription::getByKeyword(string keyword, vector<ExtraDescription *> list) {
    for ( auto ed : list ) {
        if ( is_name( keyword.c_str(), ed->getKeyword().c_str() ) ) {
            return ed->getDescription();
        }
    }
    return string();
}