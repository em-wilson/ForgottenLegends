#include "Character.h"
#include "clans/Clan.h"

class IRaceManager;

class PlayerCharacter : public Character {
public:
    PlayerCharacter(IRaceManager * race_manager);
    ~PlayerCharacter();

    virtual void gain_exp(int gain) override;
    ROOM_INDEX_DATA * getWasNoteRoom();
    void setWasNoteRoom( ROOM_INDEX_DATA *room );
    virtual bool isNPC() override;
    virtual int hit_gain() override;
    virtual int mana_gain( ) override;
    virtual int move_gain( ) override;

    void setClanCust(int i);

    void setJoin(Clan * clan);

    void incrementPlayerKills( int amount );

    void incrementMobKills(int amount);

    int getAdrenaline();

    int getPlayerKillCount();

    int getKilledByPlayersCount();

    int getMobKillCount();

    int getKilledByMobCount();

    virtual int getRange() override;

    void incrementRange(int amount);

    void incrementKilledByPlayerCount(int amount);

    void setJustKilled(int tickTimer);

    void incrementKilledByMobCount(int amount);

    void writeToFile(FILE *fp, IRaceManager *race_manager);

    bool wantsToJoinClan( Clan *clan );

private:
    /* clan stuff */
    Clan *		join;
    int			    pkills;
    int			    pkilled;
    int			    mkills;
    int		    	mkilled;
    int			    range;
    int		    	clan_cust;
    int			    adrenaline;
    int			    jkilled; // If they were just killed

    ROOM_INDEX_DATA *	was_note_room;
public:
    virtual bool didJustDie() override;

    void setAdrenaline(int value);

    long get_pc_id();

    void setKills(int pkills, int pkilled, int mkills, int mkilled);

    void setRange(int range);

    virtual void update() override;
};