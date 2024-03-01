#include <string>
#include "Clan.h"

using std::string;

Clan::Clan() {
    _pdeaths = 0;
    _pkills = 0;
    _mkills = 0;
    _mdeaths = 0;
}

Clan::~Clan() {

}

string Clan::getName() {
    return _name;
}
void Clan::setName(string value) {
    _name = value;
}

string Clan::getWhoname() {
    return _whoname;
}
void Clan::setWhoname(string value) {
    _whoname = value;
}

string Clan::getLeader() {
    return _leader;
}
void Clan::setLeader(string value) {
    _leader = value;
}

string Clan::getFilename() {
    return _filename;
}

void Clan::setFilename(string value) {
    _filename = value;
}

string Clan::getMotto() {
    return _motto;
}

void Clan::setMotto(string value) {
    _motto = value;
}

string Clan::getFirstOfficer() {
    return _number1;
}

void Clan::setFirstOfficer(string value) {
    _number1 = value;
}

void Clan::removeFirstOfficer() {
    _number1.clear();
}

string Clan::getSecondOfficer() {
    return _number2;
}

void Clan::setSecondOfficer(string value) {
    _number2 = value;
}

void Clan::removeSecondOfficer() {
    _number2.clear();
}

long Clan::getFlags() {
    return _flags;
}

void Clan::setFlags(long value) {
    _flags = value;
}

void Clan::toggleFlag(long flag) {
    _flags ^= flag;
}

int Clan::getPlayTime() {
    return _played;
}

void Clan::setMkills(int value) {
    _mkills = value;
}

void Clan::setMdeaths(int value) {
    _mdeaths = value;
}

void Clan::setPkills(int value) {
    _pkills = value;
}

void Clan::setPdeaths(int value) {
    _pdeaths = value;
}

int Clan::getPkills() {
    return _pkills;
}

int Clan::getPdeaths() {
    return _pdeaths;
}

int Clan::getMkills() {
    return _mkills;
}

int Clan::getMdeaths() {
    return _mdeaths;
}

long Clan::getMoney() {
    return _money;
}

void Clan::debitMoney(long amount) {
    _money -= amount;
}

void Clan::creditMoney(long amount) {
    _money += amount;
}

void Clan::setMoney(long amount) {
    _money = amount;
}

void Clan::setMemberCount(int value) {
    _members = value;
}

int Clan::countMembers() {
    return _members;
}

void Clan::addMember() {
    _members += 1;
}

void Clan::removeMember() {
    _members -= 1;
}

void Clan::incrementMobKills() {
    _mkills += 1;
}
void Clan::incrementMobDeaths() {
    _mdeaths += 1;
}
void Clan::incrementPlayerDeaths() {
    _pdeaths += 1;
}
void Clan::incrementPlayerKills() {
    _pkills += 1;
}

int Clan::getDeathRoomVnum() {
    return _death;
}

void Clan::setDeathRoomVnum(int value) {
    _death = value;
}


void Clan::addPlayTime(int seconds) {
    _played += seconds;
}

void Clan::setPlayTime(int seconds) {
    _played = seconds;
}

bool Clan::hasFlag(long flag) {
    return _flags & flag;
}

long Clan::calculateNetWorth( )
{
    long nw = 0;

    /* First, monetary value */
    nw += _money * 10;

    // Pkills are worth +/- 1000 gold, mkills are worth +/- 50 gold
    nw += 10000 * (_pkills * 10);
    nw -= 100 * (_pdeaths * 10);
    nw += 5000 * (_mkills * 5);
    nw -= 50 * (_mdeaths * 5);

    /* Members are worth 250 each */
    nw += _members * 25000;

    return nw / 100;
}
