#if !defined(WIZNET_H)
#define WIZNET_H

class Wiznet
{
	public:
		static Wiznet *instance();
		void report(const char *string, Character *ch, OBJ_DATA *obj, long flag, long flag_skip, int min_level);
};
#endif