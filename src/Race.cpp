#include <string>
#include "PcRace.h"
#include "Race.h"

using std::string;

Race::Race() {
    _pcRace = nullptr;
}

Race::~Race() {
    // if (_pcRace != nullptr) {
    //     delete _pcRace;
    //     _pcRace = nullptr;
    // }
}

long Race::getActFlags() { return _actFlags; }
void Race::setActFlags(long value) { _actFlags = value; }

long Race::getAffectFlags() { return _affectFlags; }
void Race::setAffectFlags(long value) { _affectFlags = value; }

long Race::getForm() { return _form; }
void Race::setForm(long value) { _form = value; }

long Race::getImmunityFlags() { return _immunityFlags; }
void Race::setImmunityFlags(long value) { _immunityFlags = value; }

short int Race::getLegacyId() { return _legacyId; }
void Race::setLegacyId(short int value) { _legacyId = value; }

string Race::getName() { return _name; }

void Race::setName(string value) { _name = value; }

long Race::getOffensiveFlags() { return _offensiveFlags; }
void Race::setOffensiveFlags(long value) { _offensiveFlags = value; }

long Race::getParts() { return _parts; }
void Race::setParts(long value) { _parts = value; }

PcRace * Race::getPlayerRace() { return _pcRace; }
void Race::setPlayerRace(PcRace *value) { _pcRace = value; }
bool Race::isPlayerRace() { return _pcRace != nullptr; }

long Race::getResistanceFlags() { return _resistanceFlags; }
void Race::setResistanceFlags(long value) { _resistanceFlags = value; }

long Race::getVulnerabilityFlags() { return _vulnerabilityFlags; }
void Race::setVulnerabilityFlags(long value) { _vulnerabilityFlags = value; }

bool Race::hasPart(long flag) {
    return (_parts & flag) > 0;
}
