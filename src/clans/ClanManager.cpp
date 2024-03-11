#include <time.h>
#include "merc.h"
#include "Character.h"
#include "clans/ClanManager.h"
#include "clans/ClanReader.h"
#include "clans/ClanWriter.h"
#include "PlayerCharacter.h"

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
    if (!ch->isNPC()) {
        remove_player((PlayerCharacter*)ch);
    }
}

void ClanManager::add_player(PlayerCharacter *ch, Clan *clan) {
    // Remove from their existing clan
    remove_player(ch);

    if (clan) {
        clan->addMember();
        ch->setClan(clan);
        _writer->save_clan(clan);
    }
}

void ClanManager::remove_player(PlayerCharacter *ch) {
    if (ch->getClan())
    {
		ch->getClan()->removeMember();
        _writer->save_clan(ch->getClan());
        ch->setClan(nullptr);
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
    if (killer->isClanned()) {
        if (IS_NPC(victim)) {
            killer->getClan()->incrementMobKills();
        } else if (!isSameClan(killer, victim)) {
            killer->getClan()->incrementPlayerKills();
        }
        _writer->save_clan(killer->getClan());
    }
}

void ClanManager::handle_death(Character *killer, Character *victim) {
    if (victim->isClanned()) {
        if (IS_NPC(killer)) {
            killer->getClan()->incrementMobDeaths();
        } else if (!isSameClan(killer, victim)) {
            killer->getClan()->incrementPlayerDeaths();
        }
        _writer->save_clan(victim->getClan());
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

bool ClanManager::isClanLeader(Character *ch) {
    Clan * clan;
    
    if (!(clan = ch->getClan()) ) {
        return false;
    }

    if (clan->getLeader() == ch->getName()) {
        return true;
    }

    if (clan->getFirstOfficer() == ch->getName()) {
        return true;
    }

    if (clan->getSecondOfficer() == ch->getName()) {
        return true;
    }

    return false;
}

bool ClanManager::isSameClan(Character *ch, Character *wch) {
    if (!ch->isClanned()) {
        return false;
    }

    if (!wch->isClanned()) {
        return false;
    }

    return ((PlayerCharacter *)ch)->getClan() == ((PlayerCharacter *)wch)->getClan();
}