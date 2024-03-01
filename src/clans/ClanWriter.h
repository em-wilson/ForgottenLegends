#ifndef CLANWRITER_H
#define CLANWRITER_H

#include "IClanWriter.h"

class ClanWriter : public IClanWriter {
    public:
        ClanWriter(const std::string clanDir, const std::string clanListFile);
        void write_clan_list(vector<Clan *> clans);
        void save_clan( Clan *clan );
        void delete_clan( Clan *clan );

    private:
        std::string _clanDir;
        std::string _clanListFile;
};
#endif