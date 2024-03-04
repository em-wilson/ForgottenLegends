#include <time.h>
#include "merc.h"
#include "clans/ClanManager.h"
#include "clans/ClanReader.h"
#include "clans/ClanWriter.h"

using std::unordered_map;
using std::string;
using std::vector;

/*
 * Get pointer to clan structure from clan name.
 */
Clan * ClanManager::get_clan( const char *name )
{
    for (std::list<Clan*>::iterator it = _clan_list.begin(); it != _clan_list.end(); it++)
    {
        Clan *clan = *it;
        if ( !str_prefix( name, clan->getName().c_str() ) ) {
         return clan;
        }
    }

    return NULL;
}

void ClanManager::write_clan_list( )
{
    _writer->write_clan_list(this->get_all_clans());
}

ClanManager::ClanManager(IClanReader *reader, IClanWriter *writer) {
    _reader = reader;
    _writer = writer;
}

ClanManager::~ClanManager() {
    if (_reader) {
        delete _reader;
        _reader = nullptr;
    }
    if (_writer) {
        delete _writer;
        _writer = nullptr;
    }
}

void ClanManager::handle_player_delete(Character *ch) {
    remove_player(ch);
}

void ClanManager::add_player(Character *ch, Clan *clan) {
    // Remove from their existing clan
    remove_player(ch);

    if (clan) {
        clan->addMember();
        ch->pcdata->clan = clan;
        _writer->save_clan(clan);
    }
}

void ClanManager::remove_player(Character *ch) {
    if (ch->pcdata->clan)
    {
		ch->pcdata->clan->removeMember();
        _writer->save_clan(ch->pcdata->clan);
        ch->pcdata->clan = NULL;
    }
}

void ClanManager::add_playtime(Clan *clan, int seconds) {
    clan->addPlayTime(seconds);
    _writer->save_clan(clan);
}

unordered_map<string, long> ClanManager::list_clan_nw() {
    unordered_map<string, long> clan_nw;
    for (auto clan : _clan_list) {
        if (!clan->hasFlag(CLAN_NOSHOW)) {
            clan_nw.insert(std::make_pair<string, long>(clan->getName(), clan->calculateNetWorth()));
        }
    }
    return clan_nw;
}

void ClanManager::handle_kill(Character *killer, Character *victim) {
    if (killer->pcdata && killer->pcdata->clan) {
        if (IS_NPC(victim)) {
            killer->pcdata->clan->incrementMobKills();
        } else if (victim->pcdata && victim->pcdata->clan != killer->pcdata->clan) {
            killer->pcdata->clan->incrementPlayerKills();
        }
        _writer->save_clan(killer->pcdata->clan);
    }
}

void ClanManager::handle_death(Character *killer, Character *victim) {
    if (victim->pcdata && victim->pcdata->clan) {
        if (IS_NPC(killer)) {
            killer->pcdata->clan->incrementMobDeaths();
        } else if (killer->pcdata && killer->pcdata->clan != victim->pcdata->clan) {
            killer->pcdata->clan->incrementPlayerDeaths();
        }
        _writer->save_clan(victim->pcdata->clan);
    }
}

void ClanManager::add_clan(Clan *clan) {
    _clan_list.push_back(clan);
}

void ClanManager::save_clan(Clan *clan) {
    _writer->save_clan(clan);
}

Clan * ClanManager::delete_clan(Clan *clan) {
    _clan_list.remove(clan);
    _writer->delete_clan(clan);
    this->write_clan_list();
    delete clan;
    clan = NULL;
    return clan;
}

vector<Clan *> ClanManager::get_all_clans() {
    vector<Clan *> v{ std::begin(_clan_list), std::end(_clan_list) };
    return v;
}

/*
 * Load in all the clan files.
 */
void ClanManager::load_clans( )
{
    _clan_list = _reader->loadClans();
}