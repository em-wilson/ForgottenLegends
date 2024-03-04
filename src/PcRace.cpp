#include <string>
#include "PcRace.h"

using std::unordered_map;
using std::string;

PcRace::PcRace() { }

PcRace::~PcRace() { }

unordered_map<string, short int> PcRace::getClassMultipliers() { return _classMultiplier; }
void PcRace::setClassMultipliers(unordered_map<string, short int> value) { _classMultiplier = value; }
void PcRace::addClassMultiplier(std::string className, short int multiplier) {
    _classMultiplier.emplace(className, multiplier);
}

std::vector<short int> PcRace::getMaxStats() { return std::vector<short int>(_maxStats.begin(), _maxStats.end()); }
void PcRace::addMaxStat(short int value) { _maxStats.push_back(value); }

string PcRace::getName() { return _name; }
void PcRace::setName(string value) { _name = value; }

short int PcRace::getPoints() { return _points; }
void PcRace::setPoints(short int value) { _points = value; }

std::list<std::string> PcRace::getSkills() { return _skills; }
void PcRace::addSkill(std::string value) { _skills.push_back(value); }

int PcRace::getSize() { return _size; }
void PcRace::setSize(int value) { _size = value; }

std::vector<short int> PcRace::getStats() { return std::vector<short int>(_stats.begin(), _stats.end()); }
void PcRace::addStat(short int value) { _stats.push_back(value); }

std::string PcRace::getWhoName() { return _whoName; }
void PcRace::setWhoName(std::string value) { _whoName = value; }