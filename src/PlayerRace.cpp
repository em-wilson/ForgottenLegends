#include <string>
#include "PlayerRace.h"

using std::unordered_map;
using std::string;

PlayerRace::PlayerRace() { }

PlayerRace::~PlayerRace() { }

unordered_map<string, short int> PlayerRace::getClassMultipliers() { return _classMultiplier; }
void PlayerRace::setClassMultipliers(unordered_map<string, short int> value) { _classMultiplier = value; }
void PlayerRace::addClassMultiplier(std::string className, short int multiplier) {
    _classMultiplier.emplace(className, multiplier);
}

std::vector<short int> PlayerRace::getMaxStats() { return std::vector<short int>(_maxStats.begin(), _maxStats.end()); }
void PlayerRace::addMaxStat(short int value) { _maxStats.push_back(value); }

string PlayerRace::getName() { return _name; }
void PlayerRace::setName(string value) { _name = value; }

short int PlayerRace::getPoints() { return _points; }
void PlayerRace::setPoints(short int value) { _points = value; }

std::list<std::string> PlayerRace::getSkills() { return _skills; }
void PlayerRace::addSkill(std::string value) { _skills.push_back(value); }

int PlayerRace::getSize() { return _size; }
void PlayerRace::setSize(int value) { _size = value; }

std::vector<short int> PlayerRace::getStats() { return std::vector<short int>(_stats.begin(), _stats.end()); }
void PlayerRace::addStat(short int value) { _stats.push_back(value); }

std::string PlayerRace::getWhoName() { return _whoName; }
void PlayerRace::setWhoName(std::string value) { _whoName = value; }