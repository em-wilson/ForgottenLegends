#include <fstream>
#include <sstream>
#include <string>
#include "tables.h"
#include "PlayerRace.h"
#include "Race.h"
#include "RaceWriter.h"

using std::endl;
using std::ofstream;
using std::ostringstream;
using std::string;
using std::stringstream;

RaceWriter::RaceWriter(const string raceDir) {
    _raceDir = raceDir;
}

string RaceWriter::generateFlagString(const struct flag_type source[], long flag) {
    ostringstream flags;
    for (int i = 0; source[i].name != NULL; i++) {
        if (flag & source[i].bit) {
            if (flags.str().length() > 0) {
                flags << " ";
            }
            flags << source[i].name;
        }
    }
    return flags.str();
}

void RaceWriter::saveRace(Race * race) {

    string filename = _raceDir + race->getName() + ".dat";
    filename.erase(remove_if(filename.begin(), filename.end(), isspace));
    ofstream file( filename );
    if( file.fail() )
    {
        throw new WriteRaceException();
    }

    file << "#RACE" << endl;
    file << "Name " << race->getName() << endl;
    file << "IsPlayerRace " << race->isPlayerRace() << endl;
    if (race->getActFlags() > 0) {
        file << "Act " << generateFlagString(act_flags, race->getActFlags()) << endl;
    }

    if (race->getAffectFlags() > 0) {
        file << "Aff " << generateFlagString(affect_flags, race->getAffectFlags()) << endl;
    }
    if (race->getForm() > 0) {
        file << "Form " << generateFlagString(form_flags, race->getForm()) << endl;
    }
    if (race->getImmunityFlags() > 0) {
        file << "Imm " << generateFlagString(imm_flags, race->getImmunityFlags()) << endl;
    }
    if (race->getOffensiveFlags() > 0) {
        file << "Off " << generateFlagString(off_flags, race->getOffensiveFlags()) << endl;
    }
    if (race->getParts() > 0) {
        file << "Parts " << generateFlagString(part_flags, race->getParts()) << endl;
    }

    if (race->getResistanceFlags() > 0) {
        file << "Res " << generateFlagString(part_flags, race->getResistanceFlags()) << endl;
    }

    if (race->getVulnerabilityFlags() > 0) {
        file << "Vuln " << generateFlagString(vuln_flags, race->getVulnerabilityFlags()) << endl;
    }
    file << "End" << endl << endl;

    if (race->isPlayerRace()) {
        PlayerRace *player_race = race->getPlayerRace();
        file << "#PCRACE" << endl;
        file << "Name " << player_race->getName() << endl;
        file << "ClassMultipliers " << generateClassMultipliers(player_race) << endl;
        file << "MaxStats " << generateNumberList(player_race->getMaxStats()) << endl;
        file << "Points " << player_race->getPoints() << endl;
        file << "Size " << player_race->getSize() << endl;
        if (player_race->getSkills().size() > 0) {
            file << "Skills " << generateSkillList(player_race) << endl;
        }
        file << "Stats " << generateNumberList(player_race->getStats()) << endl;
        file << "WhoName " << player_race->getWhoName() << endl;
        file << "End" << endl << endl;
    }

    file << "#$" << endl;
}

string RaceWriter::generateClassMultipliers(PlayerRace * race) {
    stringstream result;
    for (auto multiplier : race->getClassMultipliers()) {
        if (result.str().size() > 0) {
            result << " ";
        }
        result << multiplier.first << " " << multiplier.second;
    }
    return result.str();
}

string RaceWriter::generateNumberList(std::vector<short int> values) {
    stringstream result;
    for (auto num : values) {
        if (result.str().size() > 0) {
            result << " ";
        }
        result << num;
    }
    return result.str();
}

string RaceWriter::generateSkillList(PlayerRace *race) {
    stringstream result;
    for (auto skill : race->getSkills()) {
        if (result.str().size() > 0) {
            result << " ";
        }
        result << skill;
    }
    return result.str();
}