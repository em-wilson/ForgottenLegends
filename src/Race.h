#ifndef __RACE_H__
#define __RACE_H__

#include <string>

#ifndef MAX_CLASS
#define MAX_CLASS		   12           // Temporary - if updating this, also update race.h
#endif

#ifndef MAX_STATS
#define MAX_STATS 	        5       // Temporary - if updating this, also update race.h
#endif

class PlayerRace;

// struct race_type
// {
//     const char *	name;			/* call name of the race */
//     bool	pc_race;		/* can be chosen by pcs */
//     long	act;			/* act bits for the race */
//     long	aff;			/* aff bits for the race */
//     long	off;			/* off bits for the race */
//     long	imm;			/* imm bits for the race */
//     long        res;			/* res bits for the race */
//     long	vuln;			/* vuln bits for the race */
//     long	form;			/* default form flag for the race */
//     long	parts;			/* default parts for the race */
// };

// struct pc_race_type  /* additional data for pc races */
// {
//     const char *	name;			/* MUST be in race_type */
//     char 	who_name[6];
//     short int	points;			/* cost in points of the race */
//     short int	class_mult[MAX_CLASS];	/* exp multiplier for class, * 100 */
//     const char *	skills[5];		/* bonus skills for the race */
//     short int 	stats[MAX_STATS];	/* starting stats */
//     short int	max_stats[MAX_STATS];	/* maximum stats */
//     short int	size;			/* aff bits for the race */
// };


struct morph_race_type  /* additional data for pc races */
{
    const char *	name;
    char 	who_name[6];
    const char *	skills[5];
};

class Race {
    public:
        Race();
        virtual ~Race();

        bool hasPart(long flag);

        long getActFlags();
        void setActFlags(long value);

        long getAffectFlags();
        void setAffectFlags(long value);

        long getForm();
        void setForm(long value);

        long getImmunityFlags();
        void setImmunityFlags(long value);

        short int getLegacyId();
        void setLegacyId(short int value);

        std::string getName();
        void setName(std::string arugment);


        long getOffensiveFlags();
        void setOffensiveFlags(long arugment);

        long getParts();
        void setParts(long value);

        long getResistanceFlags();
        void setResistanceFlags(long value);

        long getVulnerabilityFlags();
        void setVulnerabilityFlags(long value);

        PlayerRace * getPlayerRace();
        bool isPlayerRace();
        void setPlayerRace(PlayerRace *value);
    
    private:
        long _actFlags;
        long _parts;
        long _form;
        long _affectFlags;
        long _immunityFlags;
        long _offensiveFlags;
        long _resistanceFlags;
        long _vulnerabilityFlags;
        short int _legacyId;
        std::string _name;
        PlayerRace * _pcRace;
};

extern	const	struct	pc_race_type	pc_race_table	[];

#endif