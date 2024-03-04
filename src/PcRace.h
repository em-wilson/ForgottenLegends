#ifndef __PCRACE_H__
#define __PCRACE_H__

#include <list>

class PcRace {
    public:
        PcRace();
        virtual ~PcRace();

        std::unordered_map<std::string, short int> getClassMultipliers();
        void setClassMultipliers(std::unordered_map<std::string, short int> value);
        void addClassMultiplier(std::string className, short int multiplier);

        std::vector<short int> getMaxStats();
        void addMaxStat(short int value);

        std::string getName();
        void setName(std::string value);

        short int getPoints();
        void setPoints(short int value);

        std::list<std::string> getSkills();
        void addSkill(std::string value);

        int getSize();
        void setSize(int value);

        std::vector<short int> getStats();
        void addStat(short int value);

        std::string getWhoName();
        void setWhoName(std::string value);

    private:
        std::string _name;
        std::string _whoName;
        short int _points; /* cost in points of the race */
        std::unordered_map<std::string, short int> _classMultiplier; /* exp multiplier for class, * 100 */
        std::list<std::string> _skills; /* bonus skills for the race */
        std::list<short int> _stats; /* starting stats */
        std::list<short int> _maxStats; /* maximum stats */
        short int _size; /* aff bits for the race */
};

        // short int	class_mult[MAX_CLASS];	
        // const char *	skills[5];		
        // short int 	stats[MAX_STATS];	
        // short int	max_stats[MAX_STATS];	
        // short int	size;			

#endif