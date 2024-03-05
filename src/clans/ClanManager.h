#ifndef CLANMANAGER_H
#define CLANMANAGER_H

#include <list>
#include <unordered_map>
#include <vector>

class Character;
class Clan;
class IClanReader;
class IClanWriter;

class ClanManager
{
    public:
        ClanManager(IClanReader *reader, IClanWriter *writer);
        ~ClanManager();
        Clan *get_clan( const char *name );
        void write_clan_list( );
        void save_clan(Clan *clan);
        Clan * delete_clan(Clan *clan);
        void handle_player_delete(Character *ch);
        void handle_kill(Character *killer, Character *victim);
        void handle_death(Character *killer, Character *victim);
        void load_clans();
        void add_clan(Clan *clan);
        std::vector<Clan *> get_all_clans();
        void add_player(Character *ch, Clan *clan);
        void add_playtime(Clan *ch, int seconds);
        void remove_player(Character *ch);
        bool isClanLeader(Character *ch);
        std::unordered_map<std::string, long> list_clan_nw();

    private:
        IClanReader *_reader;
        IClanWriter *_writer;
        std::list<Clan *> _clan_list;
};
#endif