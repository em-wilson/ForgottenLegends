#include "Character.h"

class PlayerCharacter : public Character {
public:
    PlayerCharacter();
    ~PlayerCharacter();

    virtual void gain_exp(int gain) override;
    ROOM_INDEX_DATA * getWasNoteRoom();
    void setWasNoteRoom( ROOM_INDEX_DATA *room );
    virtual bool isNPC() override;

    void setClanCust(int i);

    void setJoin(CLAN_DATA * clan);

    void incrementPlayerKills( int amount );

    void incrementMobKills(int amount);

    int getAdrenaline();

    int getPlayerKillCount();

    int getKilledByPlayersCount();

    int getMobKillCount();

    int getKilledByMobCount();

    int getRange();

    void incrementRange(int amount);

    void incrementKilledByPlayerCount(int amount);

    void setJustKilled(int tickTimer);

    void incrementKilledByMobCount(int amount);

    void writeToFile(FILE *fp);

    bool wantsToJoinClan( CLAN_DATA *clan );

private:
    /* clan stuff */
    bool		confirm_pk;
    bool		makeclan;
    CLAN_DATA *		join;
    int			pkills;
    int			pkilled;
    int			mkills;
    int			mkilled;
    int			range;
    int			clan_cust;
    int			adrenaline;
    int			jkilled; // If they were just killed

    ROOM_INDEX_DATA *	was_note_room;
public:
    virtual bool didJustDie() override;

    void setAdrenaline(int value);

    long get_pc_id();

    void setKills(int pkills, int pkilled, int mkills, int mkilled);

    void setRange(int range);

    virtual void update() override;
};