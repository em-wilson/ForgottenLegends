#ifndef ICLANWRITER_H
#define ICLANWRITER_H

#include "Clan.h"

using std::vector;

class IClanWriter {
    public:
        virtual ~IClanWriter() { }
        virtual void write_clan_list(vector<Clan *> clans) = 0;
        virtual void save_clan( Clan *clan ) = 0;
};
#endif