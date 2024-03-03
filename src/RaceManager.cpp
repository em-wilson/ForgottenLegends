#include <string>
#include "Race.h"
#include "RaceManager.h"
#include "StringHelper.h"

using std::string;
using std::vector;

extern  const	struct  race_type	race_table	[];

RaceManager::RaceManager() { }

RaceManager::~RaceManager() { }

Race RaceManager::getRaceByName(string name) {
    for ( int race = 0; race_table[race].name != NULL; race++) {
        if (tolower(name.at(0)) == tolower(race_table[race].name[0]) &&  !StringHelper::str_prefix( name.c_str(),race_table[race].name)) {
            return this->buildRaceFromLegacyTable(race);
        }
   }

   throw new InvalidRaceException();
}

vector<Race> RaceManager::getAllRaces() {
    auto results = vector<Race>();
    for (int race = 1; race_table[race].name != NULL; race++)
    {
        results.push_back(this->buildRaceFromLegacyTable(race));
    }
    return results;
}

Race RaceManager::buildRaceFromLegacyTable(int race) {
    Race result = Race();
    result.setLegacyId(race);
    result.setName(race_table[race].name);
    result.setPlayerRace(race_table[race].pc_race);
    return result;
}