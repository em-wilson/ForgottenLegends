#ifndef __STRINGHELPER_H__
#define __STRINGHELPER_H__

class StringHelper {
    public:
        static std::string capitalize( std::string str );
        static bool str_prefix( const char *astr, const char *bstr );
        static bool str_prefix( std::string astr, std::string bstr );
};

#endif