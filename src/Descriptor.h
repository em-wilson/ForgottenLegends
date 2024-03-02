#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#define MAX_INPUT_LENGTH	  256

typedef struct	descriptor_data		DESCRIPTOR_DATA;

class Character;

/*
 * Descriptor (channel) structure.
 */
struct	descriptor_data
{
    DESCRIPTOR_DATA *	next;
    DESCRIPTOR_DATA *	snoop_by;
    Character *		    character;
    Character *		    original;
    bool		        valid;
    char *		        host;
    short int		    descriptor;
    short int		    connected;
    bool		        fcommand;
    unsigned char		inbuf		[4 * MAX_INPUT_LENGTH];
    char		        incomm		[MAX_INPUT_LENGTH];
    char		        inlast		[MAX_INPUT_LENGTH];
    int			        repeat;
    char *		        outbuf;
    int			        outsize;
    int			        outtop;
    char *		        showstr_head;
    char *		        showstr_point;
    void *              pEdit;		/* OLC */
    char **             pString;	/* OLC */
    int			        editor;		/* OLC */
};

#endif