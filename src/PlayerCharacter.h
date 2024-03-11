#include "Character.h"
#include "clans/Clan.h"

#define MAX_ALIAS		    5

class IRaceManager;
class Note;

typedef struct board_data BOARD_DATA;
typedef struct 	buf_type	 	BUFFER;

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
    void advance_level( bool hide );
    void gain_condition( int iCond, int value ) override;

    bool isClanned() override;
    Clan * getClan() override;
    void setClan(Clan *value);

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

    void perform_autoloot() override;

        std::string getPassword();
        void setPassword(std::string value);

    // from pcdata

        BUFFER * 		buffer;
        bool		valid;
        char *		bamfin;
        char *		bamfout;
        char *		title;
        short int		perm_hit;
        short int		perm_mana;
        short int		perm_move;
        short int		true_sex;
        int			last_level;
        short int		condition	[4];
        std::unordered_map<int, short int> learned;
        std::unordered_map<int, short int> sk_level;
        std::unordered_map<int, short int> sk_rating;
        std::unordered_map<int, bool> group_known;
        short int		points;
        short int       min_reclass_points; // Minimum points we add in order to reclass
        bool              	confirm_delete;
        char *		alias[MAX_ALIAS];
        char * 		alias_sub[MAX_ALIAS];
        BOARD_DATA *        board;                  /* The current board        */
        std::unordered_map<int, time_t> last_note_read;   /* last note for the boards */
        Note *         in_progress;

private:
    /* clan stuff */
    Clan *		    join;
    Clan *		    _clan;
    int			    pkills;
    int			    pkilled;
    int			    mkills;
    int		    	mkilled;
    int			    range;
    int		    	clan_cust;
    int			    adrenaline;
    int			    jkilled; // If they were just killed
    std::string     _password;

    ROOM_INDEX_DATA *	was_note_room;

    std::string _pwd;

public:
    virtual bool didJustDie() override;

    void setAdrenaline(int value);

    long get_pc_id();

    void setKills(int pkills, int pkilled, int mkills, int mkilled);

    void setRange(int range);

    virtual void update() override;
};