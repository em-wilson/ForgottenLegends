#include <string>
#include "Race.h"

using std::string;

Race::Race() { }

Race::~Race() { }

long Race::getAffectFlags() { return _affectFlags; }
void Race::setAffectFlags(long value) { _affectFlags = value; }

long Race::getForm() { return _form; }
void Race::setForm(long value) { _form = value; }

long Race::getImmunityFlags() { return _immunityFlags; }
void Race::setImmunityFlags(long value) { _immunityFlags = value; }

short int Race::getLegacyId() { return _legacyId; }
void Race::setLegacyId(short int value) { _legacyId = value; }

string Race::getName() { return _name; }

bool Race::isPlayerRace() { return _isPlayerRace; }

void Race::setName(string value) { _name = value; }
void Race::setPlayerRace(bool value) { _isPlayerRace = value; }

long Race::getParts() { return _parts; }
void Race::setParts(long value) { _parts = value; }

long Race::getResistanceFlags() { return _resistanceFlags; }
void Race::setResistanceFlags(long value) { _resistanceFlags = value; }

long Race::getVulnerabilityFlags() { return _vulnerabilityFlags; }
void Race::setVulnerabilityFlags(long value) { _vulnerabilityFlags = value; }