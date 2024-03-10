#ifndef __EXTRADESCRIPTION_H__
#define __EXTRADESCRIPTION_H__

#include <string>
#include <vector>

class ExtraDescription {
    public:
        ExtraDescription * clone();
        bool isValid();
        std::string getKeyword();
        void setKeyword(std::string value);
        std::string getDescription();
        void setDescription(std::string value);
        static std::string getByKeyword(std::string keyword, std::vector<ExtraDescription *> list);

    private:
        bool _valid;
        std::string _keyword;
        std::string _description;
};

#endif