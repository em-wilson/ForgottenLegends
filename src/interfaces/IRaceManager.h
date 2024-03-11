#ifndef __IRACEMANAGER_H__
#define __IRACEMANAGER_H__

#include <string>

class Race;

class IRaceManager {
    public:
        virtual Race * getRaceByName(std::string argument) = 0;
};

#endif