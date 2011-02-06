/* Includes for board system */
/* Version 2 of the board system, (c) 1995-96 erwin@pip.dknet.dk */
/* Version 3 of the board system, (c) 2011 Forgotten Legends */
#if !defined(NOTE_H)
#define NOTE_H

#include <string>

/*
 * Data structure for notes.
 */
class Note
{
public:
    Note *next;
    bool 	valid;
    sh_int	type;
    char *	sender;
    char *	date;
    char *	to_list;
    char *	subject;
    time_t  	date_stamp;
    time_t      expire;

    Note();
    ~Note();
    std::string getText();
    void setText(std::string value);

private:
    std::string _text;
};

#endif