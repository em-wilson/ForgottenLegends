#ifndef CLANMANAGER_H
#define CLANMANAGER_H

#include "../Character.h"
#include "Clan.h"
#include "ClanWriter.h"
#include <list>
#include <vector>

using std::list;
using std::unordered_map;
using std::vector;

class ClanManager
{
    public:
        ClanManager(IClanWriter *writer);
        ~ClanManager();
        Clan *get_clan( const char *name );
        void write_clan_list( );
        void save_clan(Clan *clan);
        Clan * delete_clan(Clan *clan);
        void handle_player_delete(Character *ch);
        void handle_kill(Character *killer, Character *victim);
        void handle_death(Character *killer, Character *victim);
        void add_clan(Clan *clan);
        vector<Clan *> get_all_clans();
        void add_player(Character *ch, Clan *clan);
        void add_playtime(Clan *ch, int seconds);
        void remove_player(Character *ch);
        unordered_map<std::string, long> list_clan_nw();

    private:
        IClanWriter *_writer;
        list<Clan *> _clan_list;
};
#endif