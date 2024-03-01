#ifndef CLAN_H
#define CLAN_H

using std::string;

class Clan {
    public:
        Clan();
        ~Clan();
        string      getName();
        void        setName(string value);
        string      getLeader();
        void        setLeader(string value);
        string      getWhoname();
        void        setWhoname(string value);
        string      getFilename();
        void        setFilename(string value);
        int         getDeathRoomVnum();
        void        setDeathRoomVnum(int value);
        string      getFirstOfficer();
        void        setFirstOfficer(string value);
        void        removeFirstOfficer();
        string      getSecondOfficer();
        void        setSecondOfficer(string value);
        void        removeSecondOfficer();
        string      getMotto();
        void        setMotto(string value);
        long        getMoney();
        void        creditMoney(long amount);
        void        setMoney(long amount);
        void        debitMoney(long amount);
        void        addMember();
        void        removeMember();
        int         countMembers();
        void        setMemberCount(int value);
        int         getPlayTime();
        void        setPlayTime(int seconds);
        void        addPlayTime(int seconds);
        long        getFlags();
        void        setFlags(long value);
        bool        hasFlag(long flag);
        void        toggleFlag(long flag);
        long        calculateNetWorth();
        int         getPkills();
        int         getPdeaths();
        int         getMkills();
        int         getMdeaths();
        void        setPkills(int value);
        void        setPdeaths(int value);
        void        setMkills(int value);
        void        setMdeaths(int value);
        void        incrementMobKills();
        void        incrementMobDeaths();
        void        incrementPlayerDeaths();
        void        incrementPlayerKills();

    private:
        string      _filename;      /* Clan filename                        */
        string      _name;          /* Clan name                            */
        string      _motto;         /* Clan motto                           */
        string      _description;   /* A brief description of the clan      */
        string      _whoname;       /* How the clan appears in the wholist  */
        string      _leader;        /* Head clan leader                     */
        string      _number1;       /* First officer                        */
        string      _number2;       /* Second officer                       */
        int         _pkills;        /* Number of pkills on behalf of clan   */
        int         _pdeaths;       /* Number of pkills against clan        */
        int         _mkills;        /* Number of mkills on behalf of clan   */
        int         _mdeaths;       /* Number of clan deaths due to mobs    */
        long	    _money;		    /* How much money do they have?		    */
        short int   _strikes;       /* Number of strikes against the clan   */
        short int   _members;       /* Number of clan members               */
        int         _death;         /* Where players end up when they die   */
        long	    _flags;		    /* Clan flags				            */
        int		    _played;        /* How long has the clan been in?	    */
};
#endif