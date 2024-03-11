#ifndef __RACEMANAGER_H__
#define __RACEMANAGER_H__

#include "IRaceManager.h"

class Race;
class RaceReader;
class RaceWriter;

class InvalidRaceException : public std::runtime_error {
    public:
        InvalidRaceException(std::string msg) : std::runtime_error(msg) {};
};

class RaceManager : public IRaceManager {
    public:
        RaceManager(RaceReader *reader, RaceWriter *writer);
        virtual ~RaceManager();
        std::vector<Race *> getAllRaces();
        Race * getRaceByName(std::string argument);
        Race * getRaceByLegacyId(int legacy_id);
        void loadRaces();
        void saveRaces();

    private:
        // Race * buildRaceFromLegacyTable(int race);
        RaceReader * _reader;
        RaceWriter * _writer;
        std::vector<Race *> _races;
};

#endif