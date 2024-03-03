#ifndef __CLANREADER_H__
#define __CLANREADER_H__

#include <stdio.h>
#include "IClanReader.h"

class ClanReader : public IClanReader {
    public:
        ClanReader();
        virtual ~ClanReader();
        std::list<Clan *> loadClans();

    private:
        Clan * load_clan_file( char *clanfile );
        void fread_clan( Clan *clan, FILE *fp );
};

#endif