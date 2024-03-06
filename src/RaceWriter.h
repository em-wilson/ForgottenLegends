#ifndef __RACEWRITER_H__
#define __RACEWRITER_H__

class PlayerRace;
class Race;

class WriteRaceException : public std::exception { };

class RaceWriter {
    public:
        RaceWriter(const std::string raceDir);
        void saveRace(Race * race);
    
    private:
        std::string generateClassMultipliers(PlayerRace * race);
        std::string generateFlagString(const struct flag_type source[], long flag);
        std::string generateSkillList(PlayerRace *race);
        std::string generateNumberList(std::vector<short int> values);
        std::string _raceDir;
};

#endif