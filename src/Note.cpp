/* Includes for board system */
/* Version 2 of the board system, (c) 1995-96 erwin@pip.dknet.dk */
/* Version 3 of the board system, (c) 2011 Forgotten Legends */
#include <string.h>
#include <sys/types.h>
#include "merc.h"
#include "Note.h"


/* recycle a note */
Note::~Note()
{
	if (this->sender)
		free_string (this->sender);
	if (this->to_list)
		free_string (this->to_list);
	if (this->subject)
		free_string (this->subject);
	if (this->date) /* was note->datestamp for some reason */
		free_string (this->date);
}

/* allocate memory for a new note or recycle */
Note::Note()
{
	this->next = NULL;
	this->sender = NULL;		
	this->expire = 0;
	this->to_list = NULL;
	this->subject = NULL;
	this->date = NULL;
	this->date_stamp = 0;
}

void Note::setText(std::string value )
{
	_text = value;
}

std::string Note::getText()
{
	return _text;
}