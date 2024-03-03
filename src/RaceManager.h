#ifndef __RACEMANAGER_H__
#define __RACEMANAGER_H__

class Race;

class InvalidRaceException : public std::exception { };

class RaceManager {
    public:
        RaceManager();
        virtual ~RaceManager();
        std::vector<Race> getAllRaces();
        Race getRaceByName(std::string argument);

    private:
        Race buildRaceFromLegacyTable(int race);
};

#endif