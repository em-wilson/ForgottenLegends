#include <iostream>
#include <sstream>
#include <string>
#include "merc.h"
#include "ILogger.h"
#include "PlayerRace.h"
#include "Race.h"
#include "RaceManager.h"
#include "RaceReader.h"
#include "RaceWriter.h"
#include "StringHelper.h"

using std::string;
using std::vector;

extern	const	struct	class_type	class_table	[MAX_CLASS];
extern  const	struct  race_type	race_table	[];

RaceManager::RaceManager(RaceReader *reader, RaceWriter *writer) {
    _reader = reader;
    _writer = writer;
}

RaceManager::~RaceManager() {
    if (_reader) {
        delete _reader;
        _reader = nullptr;
    }

    if (_writer) {
        delete _writer;
        _writer = nullptr;
    }
}

Race * RaceManager::getRaceByLegacyId(int legacy_id) {
    for (auto race : _races) {
        if (race->getLegacyId() == legacy_id) {
            return race;
        }
    }
    
    std::stringstream ss;
    ss << "Invalid race with legacy ID " << legacy_id;
    throw InvalidRaceException(ss.str());
}

Race * RaceManager::getRaceByName(string name) {
//     for ( int race = 0; race_table[race].name != NULL; race++) {
//         if (tolower(name.at(0)) == tolower(race_table[race].name[0]) &&  !StringHelper::str_prefix( name.c_str(),race_table[race].name)) {
//             return this->buildRaceFromLegacyTable(race);
//         }
//    }
    for (auto race : _races) {
        if (tolower(name.at(0)) == tolower(race->getName().at(0)) &&  !StringHelper::str_prefix( name.c_str(), race->getName().c_str())) {
            return race;
        }
    }
    
    std::stringstream ss;
    ss << "Invalid race with name: " << name;
    throw InvalidRaceException(ss.str());
}

vector<Race *> RaceManager::getAllRaces() {
    return _races;
    // auto results = vector<Race *>();
    // for (int race = 1; race_table[race].name != NULL; race++)
    // {
    //     results.push_back(this->buildRaceFromLegacyTable(race));
    // }
    // return results;
}

// Race * RaceManager::buildRaceFromLegacyTable(int race) {
//     Race * result = new Race();
//     result->setLegacyId(race);
//     result->setActFlags(race_table[race].act);
//     result->setAffectFlags(race_table[race].aff);
//     result->setImmunityFlags(race_table[race].imm);
//     result->setName(race_table[race].name);
//     result->setParts(race_table[race].parts);
//     result->setResistanceFlags(race_table[race].res);
//     result->setVulnerabilityFlags(race_table[race].vuln);

//     if (race_table[race].pc_race) {
//         PlayerRace *pcRace = new PlayerRace();
//         pcRace->setName(pc_race_table[race].name);
//         pcRace->setPoints(pc_race_table[race].points);
//         pcRace->setWhoName(pc_race_table[race].who_name);
//         for (int classNum = 0; classNum < MAX_CLASS; classNum++) {
//             pcRace->addClassMultiplier(class_table[classNum].name, pc_race_table[race].class_mult[classNum]);
//         }
//         for (int skillNum = 0; pc_race_table[race].skills[skillNum] != NULL; skillNum++ ) {
//             pcRace->addSkill(pc_race_table[race].skills[skillNum]);
//         }
//         for (int stat = 0; stat < MAX_STATS; stat++) {
//             pcRace->addMaxStat(pc_race_table[race].max_stats[stat]);
//             pcRace->addStat(pc_race_table[race].stats[stat]);
//         }
//         pcRace->setSize(pc_race_table[race].size);
//         result->setPlayerRace(pcRace);
//     }


//     return result;
// }

void RaceManager::loadRaces() {
    logger->log_string("Loading races");
    _races = _reader->loadRaces();
    char buf[500];
    snprintf(buf, sizeof(buf), "Found %zu races", _races.size());
    logger->log_string(buf);
}

void RaceManager::saveRaces() {
    logger->log_string("Saving races...");
    for (auto race : getAllRaces() ) {
        _writer->saveRace(race);
    }
}