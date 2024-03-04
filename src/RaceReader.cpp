#include <filesystem>
#include <fstream>
#include <list>
#include <string>
#include <sstream>
#include <vector>
#include "tables.h"
#include "PcRace.h"
#include "Race.h"
#include "RaceReader.h"

namespace fs = std::filesystem;
using std::ifstream;
using std::ios;
using std::list;
using std::unordered_map;
using std::string;
using std::stringstream;
using std::vector;

void log_string( const char *str );

RaceReader::RaceReader(string raceDir) {
    _raceDir = raceDir;
}

RaceReader::~RaceReader() { }

vector<Race *> RaceReader::loadRaces() {
    vector<Race *> races;
    for (const auto & entry : fs::directory_iterator(_raceDir)) {
        races.push_back(loadRace(entry.path()));
    }
    return races;
}

Race * RaceReader::loadRace(std::string path) {
    Race * race = nullptr;
    ifstream file (path);
    string line;

    if (file.is_open()) {
        while ( file ) {
            std::getline(file, line);
            if (line == "#RACE") { race = parseRace(file); }
            if (line == "#PCRACE") {
                PcRace *pc_race = parsePlayerRace(file);
                race->setPlayerRace(pc_race);
                log_string("read player race");
            }
        }
    }

    if (!race) {
        throw RaceNotFoundInFileException(path);
    }
    return race;
}

Race * RaceReader::parseRace(std::ifstream &file) {
    Race * race = new Race();
    string line, word;
    while ( file ) {
        std::getline(file, line);
        std::stringstream ss(line);
        ss >> word;
        if (word == "End") { break; }
        else if (word == "Act") { race->setActFlags(readFlags(act_flags, ss)); }
        else if (word == "Aff") { race->setAffectFlags(readFlags(affect_flags, ss)); }
        else if (word == "Form") { race->setForm(readFlags(form_flags, ss)); }
        else if (word == "Imm") { race->setImmunityFlags(readFlags(imm_flags, ss)); }
        else if (word == "IsPlayerRace") { race->setActFlags(readBoolean(ss)); }
        else if (word == "Name") {race->setName(readString(ss)); }
        else if (word == "Off") { race->setOffensiveFlags(readFlags(off_flags, ss)); }
        else if (word == "Parts") { race->setParts(readFlags(part_flags, ss)); }
        else if (word == "Res") { race->setResistanceFlags(readFlags(res_flags, ss)); }
        else if (word == "Vuln") { race->setVulnerabilityFlags(readFlags(vuln_flags, ss)); }
    }
    return race;
}

PcRace * RaceReader::parsePlayerRace(std::ifstream &file) {
    PcRace * race = new PcRace();
    string line, word;
    while ( file ) {
        std::getline(file, line);
        std::stringstream ss(line);
        ss >> word;
        if (word == "End") { break; }
        else if (word == "Name") { race->setName(readString(ss)); }
        else if (word == "ClassMultipliers") { race->setClassMultipliers(readClassMultipliers(ss)); }
        else if (word == "MaxStats") {
            string stat;
            while (ss >> stat) race->addMaxStat(std::stoi(stat));
        }
        else if (word == "Points") { race->setPoints(readNumber(ss)); }
        else if (word == "Size") { race->setSize(readNumber(ss)); }
        else if (word == "Skills") {
            string skill;
            while (ss >> skill) race->addSkill(skill);
        }
        else if (word == "Stats") {
            string stat;
            while (ss >> stat) race->addStat(std::stoi(stat));
        }
        else if (word == "WhoName") { race->setWhoName(readString(ss)); }
    }
    return race;
}

bool RaceReader::readBoolean(std::stringstream &ss) {
    string result;
    ss >> result;
    return result == "1";
}

unordered_map<string, short int> RaceReader::readClassMultipliers(stringstream &ss) {
    unordered_map<string, short int> results;
    string className, multiplier;
    while (ss >> className) {
        ss >> multiplier;
        results.emplace(className, std::stoi(multiplier));
    }
    return results;
}

long RaceReader::readFlags(const struct flag_type source[], stringstream &ss) {
    long flags = 0;
    string word;
    while (ss >> word) {
        for (int i = 0; source[i].name != NULL; i++) {
            if (word == source[i].name) {
                flags |= source[i].bit;
            }
        }
    }
    return flags;
}

int RaceReader::readNumber(stringstream &ss) {
    string result;
    ss >> result;
    return std::stoi(result);
}

std::string RaceReader::readString(std::stringstream &ss) {
    string result, word;
    while (ss >> word) {
        if (result.size() > 0 ) {
            result += " ";
        }
        result += word;
    }
    return result;
}