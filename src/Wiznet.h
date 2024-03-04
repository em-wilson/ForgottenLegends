#if !defined(WIZNET_H)
#define WIZNET_H

class Character;
typedef struct obj_data OBJ_DATA;

#ifndef ASCII_FLAG_CONVERSIONS
#define ASCII_FLAG_CONVERSIONS

#define A		  	1
#define B			2
#define C			4
#define D			8
#define E			16
#define F			32
#define G			64
#define H			128

#define I			256
#define J			512
#define K		        1024
#define L		 	2048
#define M			4096
#define N		 	8192
#define O			16384
#define P			32768

#define Q			65536
#define R			131072
#define S			262144
#define FLAG_T			524288
#define U			1048576
#define FLAG_V			2097152
#define W			4194304
#define X			8388608

#define Y			16777216
#define Z			33554432
#define aa			67108864 	/* doubled due to conflicts */
#define bb			134217728
#define cc			268435456    
#define dd			536870912
#define ee			1073741824

#endif

/* WIZnet flags */
#define WIZ_ON			(A)
#define WIZ_TICKS		(B)
#define WIZ_LOGINS		(C)
#define WIZ_SITES		(D)
#define WIZ_LINKS		(E)
#define WIZ_DEATHS		(F)
#define WIZ_RESETS		(G)
#define WIZ_MOBDEATHS	(H)
#define WIZ_FLAGS		(I)
#define WIZ_PENALTIES	(J)
#define WIZ_SACCING		(K)
#define WIZ_LEVELS		(L)
#define WIZ_SECURE		(M)
#define WIZ_SWITCHES	(N)
#define WIZ_SNOOPS		(O)
#define WIZ_RESTORE		(P)
#define WIZ_LOAD		(Q)
#define WIZ_NEWBIE		(R)
#define WIZ_PREFIX		(S)
#define WIZ_SPAM		(FLAG_T)
#define WIZ_LOG			(U)
#define WIZ_CLANS		(FLAG_V)

class Wiznet
{
	public:
		static Wiznet *instance();
		void report(const char *string, Character *ch, OBJ_DATA *obj, long flag, long flag_skip, int min_level);
};
#endif