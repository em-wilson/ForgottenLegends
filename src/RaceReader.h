#ifndef __RACEREADER_H__
#define __RACEREADER_H__

#include <list>

class RaceNotFoundInFileException : public std::runtime_error {
    public:
        RaceNotFoundInFileException(std::string msg) : std::runtime_error(msg) {};
};

class PcRace;
class Race;

class RaceReader {
    public:
        RaceReader(std::string raceDir);
        virtual ~RaceReader();
        std::vector<Race *> loadRaces();

    private:
        Race * loadRace(std::string path);
        Race * parseRace(std::ifstream &file);
        PcRace * parsePlayerRace(std::ifstream &file);
        bool readBoolean(std::stringstream &ss);
        std::unordered_map<std::string, short int> readClassMultipliers(std::stringstream &ss);
        long readFlags(const struct flag_type source[], std::stringstream &ss);
        int readNumber(std::stringstream &ss);
        std::string readString(std::stringstream &ss);
        std::string _raceDir;
};

#endif