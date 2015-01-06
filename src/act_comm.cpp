/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 **************************************************************************/

/***************************************************************************
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@efn.org)				   *
*	    Gabrielle Taylor						   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "Wiznet.h"
#include "PlayerCharacter.h"

/* command procedures needed */
DECLARE_DO_FUN(do_quit	);
void chatperform(Character *ch,Character *victim,char* msg);

/* RT code to delete yourself */

void do_delet( Character *ch, char *argument)
{
    send_to_char("You must type the full command to delete yourself.\n\r",ch);
}

void do_delete( Character *ch, char *argument)
{
   char strsave[MAX_INPUT_LENGTH];

   if (IS_NPC(ch))
	return;
  
   if (ch->pcdata->confirm_delete)
   {
	if (argument[0] != '\0')
	{
	    send_to_char("Delete status removed.\n\r",ch);
	    ch->pcdata->confirm_delete = FALSE;
	    return;
	}
	else
	{
    	    sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( ch->getName() ) );
	    Wiznet::instance()->report("$N turns $Mself into line noise.",ch,NULL,0,0,0);
	    stop_fighting(ch,TRUE);
	    if (ch->pcdata->clan)
	    {
		ch->pcdata->clan->members--;
		save_clan(ch->pcdata->clan);
	    }
	    do_quit(ch,(char*)"");
	    unlink(strsave);
	    return;
 	}
    }

    if (argument[0] != '\0')
    {
	send_to_char("Just type delete. No argument.\n\r",ch);
	return;
    }

    send_to_char("Type delete again to confirm this command.\n\r",ch);
    send_to_char("WARNING: this command is irreversible.\n\r",ch);
    send_to_char("Typing delete with an argument will undo delete status.\n\r",
	ch);
    ch->pcdata->confirm_delete = TRUE;
    Wiznet::instance()->report("$N is contemplating deletion.",ch,NULL,0,0,get_trust(ch));
}
	    

/* RT code to display channel status */

void do_channels( Character *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    /* lists all channels and their status */
    send_to_char("   channel     status\n\r",ch);
    send_to_char("---------------------\n\r",ch);
 
    send_to_char("{cgossip{x         ",ch);
    if (!IS_SET(ch->comm,COMM_NOGOSSIP))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    send_to_char("auction        ",ch);
    if (!IS_SET(ch->comm,COMM_NOAUCTION))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    send_to_char("music          ",ch);
    if (!IS_SET(ch->comm,COMM_NOMUSIC))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    send_to_char("info           ",ch);
    if (!IS_SET(ch->comm,COMM_NOINFO))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    send_to_char("Q/A            ",ch);
    if (!IS_SET(ch->comm,COMM_NOQUESTION))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    send_to_char("Quote          ",ch);
    if (!IS_SET(ch->comm,COMM_NOQUOTE))
	send_to_char("ON\n\r",ch);
    else
	send_to_char("OFF\n\r",ch);

    send_to_char("{ygrats{x          ",ch);
    if (!IS_SET(ch->comm,COMM_NOGRATS))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    if (IS_IMMORTAL(ch))
    {
      send_to_char("god channel    ",ch);
      if(!IS_SET(ch->comm,COMM_NOWIZ))
        send_to_char("ON\n\r",ch);
      else
        send_to_char("OFF\n\r",ch);
    }

    send_to_char("shouts         ",ch);
    if (!IS_SET(ch->comm,COMM_SHOUTSOFF))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    send_to_char("{Bt{Ce{Bl{Cl{Bs{x          ",ch);
    if (!IS_SET(ch->comm,COMM_DEAF))
	send_to_char("ON\n\r",ch);
    else
	send_to_char("OFF\n\r",ch);

    send_to_char("quiet mode     ",ch);
    if (IS_SET(ch->comm,COMM_QUIET))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);

    if (IS_SET(ch->comm,COMM_AFK))
	send_to_char("You are AFK.\n\r",ch);

    if (IS_SET(ch->comm,COMM_SNOOP_PROOF))
	send_to_char("You are immune to snooping.\n\r",ch);
   
    if (ch->lines != PAGELEN)
    {
	if (ch->lines)
	{
	    sprintf(buf,"You display %d lines of scroll.\n\r",ch->lines+2);
	    send_to_char(buf,ch);
 	}
	else
	    send_to_char("Scroll buffering is off.\n\r",ch);
    }

    if (ch->prompt != NULL)
    {
	sprintf(buf,"Your current prompt is: %s\n\r",ch->prompt);
	send_to_char(buf,ch);
    }

    if (IS_SET(ch->comm,COMM_NOSHOUT))
      send_to_char("You cannot shout.\n\r",ch);
  
    if (IS_SET(ch->comm,COMM_NOTELL))
      send_to_char("You cannot use tell.\n\r",ch);
 
    if (IS_SET(ch->comm,COMM_NOCHANNELS))
     send_to_char("You cannot use channels.\n\r",ch);

    if (IS_SET(ch->comm,COMM_NOEMOTE))
      send_to_char("You cannot show emotions.\n\r",ch);

}

/* RT deaf blocks out all shouts */

void do_deaf( Character *ch, char *argument)
{
    
   if (IS_SET(ch->comm,COMM_DEAF))
   {
     send_to_char("You can now hear tells again.\n\r",ch);
     REMOVE_BIT(ch->comm,COMM_DEAF);
   }
   else 
   {
     send_to_char("From now on, you won't hear tells.\n\r",ch);
     SET_BIT(ch->comm,COMM_DEAF);
   }
}

/* RT quiet blocks out all communication */

void do_quiet ( Character *ch, char * argument)
{
    if (IS_SET(ch->comm,COMM_QUIET))
    {
      send_to_char("Quiet mode removed.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_QUIET);
    }
   else
   {
     send_to_char("From now on, you will only hear says and emotes.\n\r",ch);
     SET_BIT(ch->comm,COMM_QUIET);
   }
}

/* afk command */

void do_afk ( Character *ch, char * argument)
{
    if (IS_SET(ch->comm,COMM_AFK))
    {
      send_to_char("AFK mode removed. Type 'replay' to see tells.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_AFK);
    }
   else
   {
     send_to_char("You are now in AFK mode.\n\r",ch);
     SET_BIT(ch->comm,COMM_AFK);
   }
}

void do_replay (Character *ch, char *argument)
{
    if (IS_NPC(ch))
    {
	send_to_char("You can't replay.\n\r",ch);
	return;
    }

    if (buf_string(ch->pcdata->buffer)[0] == '\0')
    {
	send_to_char("You have no tells to replay.\n\r",ch);
	return;
    }

    page_to_char(buf_string(ch->pcdata->buffer),ch);
    clear_buf(ch->pcdata->buffer);
}

void talk_auction( char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

      for ( d = descriptor_list; d != NULL; d = d->next )
      {
	Character *victim;

	victim = d->original ? d->original : d->character;

	if ( d->connected == CON_PLAYING &&
	     !IS_SET(victim->comm,COMM_NOAUCTION) &&
	     !IS_SET(victim->comm,COMM_QUIET) )
	{
		sprintf(buf, "{BAUCTION: {c%s{x", argument);
		send_to_char(buf,victim);
		send_to_char("\n\r",victim);
 	}
      }
}

/* RT chat replaced with ROM gossip */
void do_gossip( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
 
    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOGOSSIP))
      {
        send_to_char("Gossip channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOGOSSIP);
      }
      else
      {
        send_to_char("Gossip channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOGOSSIP);
      }
    }
    else  /* gossip message sent, turn gossip on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET))
        {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
 
       	}

      REMOVE_BIT(ch->comm,COMM_NOGOSSIP);
 
      sprintf( buf, "{cYou gossip '%s{c'{x\n\r", argument );
      send_to_char( buf, ch );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        Character *victim;
 
        victim = d->original ? d->original : d->character;
 
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_NOGOSSIP) &&
             !IS_SET(victim->comm,COMM_QUIET) )
        {
          act_new( "{c$n gossips '$t{c'{x", 
		   ch,argument, d->character, TO_VICT,POS_SLEEPING );
        }
      }
    }
}

void do_grats( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
 
    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOGRATS))
      {
        send_to_char("Grats channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOGRATS);
      }
      else
      {
        send_to_char("Grats channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOGRATS);
      }
    }
    else  /* grats message sent, turn grats on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET))
        {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
 
        }
 
      REMOVE_BIT(ch->comm,COMM_NOGRATS);
 
      sprintf( buf, "{yYou grats '%s{y'{x\n\r", argument );
      send_to_char( buf, ch );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        Character *victim;
 
        victim = d->original ? d->original : d->character;
 
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_NOGRATS) &&
             !IS_SET(victim->comm,COMM_QUIET) )
        {
          act_new( "{y$n grats '$t{y'{x",
                   ch,argument, d->character, TO_VICT,POS_SLEEPING );
        }
      }
    }
}

void do_quote( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
 
    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOQUOTE))
      {
        send_to_char("Quote channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOQUOTE);
      }
      else
      {
        send_to_char("Quote channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOQUOTE);
      }
    }
    else  /* quote message sent, turn quote on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET))
        {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
 
        }
 
      REMOVE_BIT(ch->comm,COMM_NOQUOTE);
 
      sprintf( buf, "You quote '%s'\n\r", argument );
      send_to_char( buf, ch );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        Character *victim;
 
        victim = d->original ? d->original : d->character;
 
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_NOQUOTE) &&
             !IS_SET(victim->comm,COMM_QUIET) )
        {
          act_new( "$n quotes '$t'",
                   ch,argument, d->character, TO_VICT,POS_SLEEPING );
        }
      }
    }
}

/* RT question channel */
void do_question( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
 
    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOQUESTION))
      {
        send_to_char("Q/A channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOQUESTION);
      }
      else
      {
        send_to_char("Q/A channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOQUESTION);
      }
    }
    else  /* question sent, turn Q/A on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET))
        {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
	}
 
        REMOVE_BIT(ch->comm,COMM_NOQUESTION);
 
      sprintf( buf, "You question '%s'\n\r", argument );
      send_to_char( buf, ch );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        Character *victim;
 
        victim = d->original ? d->original : d->character;
 
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_NOQUESTION) &&
             !IS_SET(victim->comm,COMM_QUIET) )
        {
	  act_new("$n questions '$t'",
	 	  ch,argument,d->character,TO_VICT,POS_SLEEPING);
        }
      }
    }
}

/* RT answer channel - uses same line as questions */
void do_answer( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
 
    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOQUESTION))
      {
        send_to_char("Q/A channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOQUESTION);
      }
      else
      {
        send_to_char("Q/A channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOQUESTION);
      }
    }
    else  /* answer sent, turn Q/A on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET))
        {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
	}
 
        REMOVE_BIT(ch->comm,COMM_NOQUESTION);
 
      sprintf( buf, "You answer '%s'\n\r", argument );
      send_to_char( buf, ch );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        Character *victim;
 
        victim = d->original ? d->original : d->character;
 
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_NOQUESTION) &&
             !IS_SET(victim->comm,COMM_QUIET) )
        {
	  act_new("$n answers '$t'",
		  ch,argument,d->character,TO_VICT,POS_SLEEPING);
        }
      }
    }
}

/* RT music channel */
void do_music( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
 
    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOMUSIC))
      {
        send_to_char("Music channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOMUSIC);
      }
      else
      {
        send_to_char("Music channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOMUSIC);
      }
    }
    else  /* music sent, turn music on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET))
        {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }
 
        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
	}
 
        REMOVE_BIT(ch->comm,COMM_NOMUSIC);
 
      sprintf( buf, "You MUSIC: '%s'\n\r", argument );
      send_to_char( buf, ch );
      sprintf( buf, "$n MUSIC: '%s'", argument );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        Character *victim;
 
        victim = d->original ? d->original : d->character;
 
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_NOMUSIC) &&
             !IS_SET(victim->comm,COMM_QUIET) )
        {
	    act_new("$n MUSIC: '$t'",
		    ch,argument,d->character,TO_VICT,POS_SLEEPING);
        }
      }
    }
}

/* clan channels */
void do_clantalk( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (!IS_CLANNED(ch))
    {
	send_to_char("You aren't in a clan.\n\r",ch);
	return;
    }
    if ( argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOCLAN))
      {
        send_to_char("Clan channel is now ON\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOCLAN);
      }
      else
      {
        send_to_char("Clan channel is now OFF\n\r",ch);
        SET_BIT(ch->comm,COMM_NOCLAN);
      }
      return;
    }

        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
         send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
        }

        REMOVE_BIT(ch->comm,COMM_NOCLAN);

      sprintf( buf, "You clan '%s'\n\r", argument );
      send_to_char( buf, ch );
      sprintf( buf, "$n clans '%s'", argument );
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
	     is_same_clan(ch,d->character) &&
             !IS_SET(d->character->comm,COMM_NOCLAN) &&
	     !IS_SET(d->character->comm,COMM_QUIET) )
        {
            act_new("$n clans '$t'",ch,argument,d->character,TO_VICT,POS_DEAD);
        }
    }

    return;
}

void do_immtalk( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if ( argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOWIZ))
      {
	send_to_char("Immortal channel is now ON\n\r",ch);
	REMOVE_BIT(ch->comm,COMM_NOWIZ);
      }
      else
      {
	send_to_char("Immortal channel is now OFF\n\r",ch);
	SET_BIT(ch->comm,COMM_NOWIZ);
      } 
      return;
    }

    REMOVE_BIT(ch->comm,COMM_NOWIZ);

    sprintf( buf, "$n: %s", argument );
    act_new("$n: $t",ch,argument,NULL,TO_CHAR,POS_DEAD);
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
	if ( d->connected == CON_PLAYING && 
	     IS_IMMORTAL(d->character) && 
             !IS_SET(d->character->comm,COMM_NOWIZ) )
	{
	    act_new("$n: $t",ch,argument,d->character,TO_VICT,POS_DEAD);
	}
    }

    return;
}



void do_say( Character *ch, char *argument )
{
    if ( argument[0] == '\0' )
    {
	send_to_char( "Say what?\n\r", ch );
	return;
    }

    act( "{g$n says '$T{g'{x", ch, NULL, argument, TO_ROOM );
    act( "{gYou say '$T{g'{x", ch, NULL, argument, TO_CHAR );

    if ( !IS_NPC(ch) )
    {
	Character *mob, *mob_next;
	for ( mob = ch->in_room->people; mob != NULL; mob = mob_next )
	{
	    mob_next = mob->next_in_room;
	    if ( IS_NPC(mob) && HAS_TRIGGER( mob, TRIG_SPEECH )
	    &&   mob->position == mob->pIndexData->default_pos )
		mp_act_trigger( argument, mob, ch, NULL, NULL, TRIG_SPEECH );
	}
    }
    return;
}



void do_shout( Character *ch, char *argument )
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0' )
    {
      	if (IS_SET(ch->comm,COMM_SHOUTSOFF))
      	{
            send_to_char("You can hear shouts again.\n\r",ch);
            REMOVE_BIT(ch->comm,COMM_SHOUTSOFF);
      	}
      	else
      	{
            send_to_char("You will no longer hear shouts.\n\r",ch);
            SET_BIT(ch->comm,COMM_SHOUTSOFF);
      	}
      	return;
    }

    if ( IS_SET(ch->comm, COMM_NOSHOUT) )
    {
        send_to_char( "You can't shout.\n\r", ch );
        return;
    }
 
    REMOVE_BIT(ch->comm,COMM_SHOUTSOFF);

    WAIT_STATE( ch, 12 );

    act( "You shout '$T'", ch, NULL, argument, TO_CHAR );
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
	Character *victim;

	victim = d->original ? d->original : d->character;

	if ( d->connected == CON_PLAYING &&
	     d->character != ch &&
	     !IS_SET(victim->comm, COMM_SHOUTSOFF) &&
	     !IS_SET(victim->comm, COMM_QUIET) ) 
	{
	    act("$n shouts '$t'",ch,argument,d->character,TO_VICT);
	}
    }

    return;
}



void do_tell( Character *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    Character *victim;

    if ( IS_SET(ch->comm, COMM_NOTELL) || IS_SET(ch->comm,COMM_DEAF))
    {
	send_to_char( "Your message didn't get through.\n\r", ch );
	return;
    }

    if ( IS_SET(ch->comm, COMM_QUIET) )
    {
	send_to_char( "You must turn off quiet mode first.\n\r", ch);
	return;
    }

    if (IS_SET(ch->comm,COMM_DEAF))
    {
	send_to_char("You must turn off deaf mode first.\n\r",ch);
	return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	send_to_char( "Tell whom what?\n\r", ch );
	return;
    }

    /*
     * Can tell to PC's anywhere, but NPC's only in same room.
     * -- Furey
     */
    if ( (victim = get_char_world(ch,arg) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim->desc == NULL && !IS_NPC(victim))
    {
	act("$N seems to have misplaced $S link...try again later.",
	    ch,NULL,victim,TO_CHAR);
        sprintf(buf,"{B%s{C tried to tell you '%s' while you were {GLINKDEAD{C.{x\n\r",PERS(ch,victim),argument);
        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer,buf);
	return;
    }

    if ( IS_NPC(victim) && victim->in_room != ch->in_room )
    {
	send_to_char("You may only send tells to mobiles in the same room sas you.\n\r",ch);
	return;
    }

    if ( !(IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL) && !IS_AWAKE(victim) )
    {
	act( "$E can't hear you.", ch, 0, victim, TO_CHAR );
	return;
    }
  
    if ((IS_SET(victim->comm,COMM_QUIET) || IS_SET(victim->comm,COMM_DEAF))
    && !IS_IMMORTAL(ch))
    {
	act( "$E is not receiving tells.", ch, 0, victim, TO_CHAR );
  	return;
    }

    if (IS_SET(victim->comm,COMM_AFK))
    {
	if (IS_NPC(victim))
	{
	    act("$E is AFK, and not receiving tells.",ch,NULL,victim,TO_CHAR);
	    return;
	}

	act("$E is {RAFK{x, but your tell will go through when $E returns.",
	    ch,NULL,victim,TO_CHAR);
	sprintf(buf,"{B%s {Ctried to tell you '%s' while you were {RAFK{x.\n\r",PERS(ch,victim),argument);
	buf[0] = UPPER(buf[0]);
	add_buf(victim->pcdata->buffer,buf);
	return;
    }

    act( "{CYou tell {B$N{C '$t{C'{x", ch, argument, victim, TO_CHAR );
    act_new("{B$n {Ctells you '$t{C'{x",ch,argument,victim,TO_VICT,POS_DEAD);
    victim->reply	= ch;

    if ( !IS_NPC(ch) && IS_NPC(victim) && HAS_TRIGGER(victim,TRIG_SPEECH) )
	mp_act_trigger( argument, victim, ch, NULL, NULL, TRIG_SPEECH );

    return;
}



void do_reply( Character *ch, char *argument )
{
    Character *victim;
    char buf[MAX_STRING_LENGTH];

    if ( IS_SET(ch->comm, COMM_NOTELL) )
    {
	send_to_char( "Your message didn't get through.\n\r", ch );
	return;
    }

    if ( ( victim = ch->reply ) == NULL ||
		(!IS_NPC(victim) && victim->desc->connected != CON_PLAYING))
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    sprintf(buf,"%s %s", ch->reply->getName(), argument );
    do_tell(ch,buf);
    return;
}



void do_yell( Character *ch, char *argument )
{
    DESCRIPTOR_DATA *d;

    if ( IS_SET(ch->comm, COMM_NOSHOUT) )
    {
        send_to_char( "You can't yell.\n\r", ch );
        return;
    }
 
    if ( argument[0] == '\0' )
    {
	send_to_char( "Yell what?\n\r", ch );
	return;
    }


    act("You yell '$t'",ch,argument,NULL,TO_CHAR);
    
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
	if ( d->connected == CON_PLAYING
	&&   d->character != ch
	&&   d->character->in_room != NULL
	&&   d->character->in_room->area == ch->in_room->area 
        &&   !IS_SET(d->character->comm,COMM_QUIET) )
	{
	    act("$n yells '$t'",ch,argument,d->character,TO_VICT);
	}
    }

    return;
}


void do_emote( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
    {
        send_to_char( "You can't show your emotions.\n\r", ch );
        return;
    }
 
    if ( argument[0] == '\0' )
    {
        send_to_char( "Emote what?\n\r", ch );
        return;
    }
 
    while (isspace(argument[0]))
	argument++;

    MOBtrigger = FALSE;
    sprintf(buf, "$n %s", argument);
    act( buf, ch, NULL, NULL, TO_ROOM );
    act( buf, ch, NULL, NULL, TO_CHAR );
    MOBtrigger = TRUE;
    return;
}


void do_pmote( Character *ch, char *argument )
{
    Character *vch;
    char *letter,*name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    unsigned int matches = 0;

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
    {
        send_to_char( "You can't show your emotions.\n\r", ch );
        return;
    }
 
    if ( argument[0] == '\0' )
    {
        send_to_char( "Emote what?\n\r", ch );
        return;
    }
 
    act( "$n $t", ch, argument, NULL, TO_CHAR );

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
	if (vch->desc == NULL || vch == ch)
	    continue;

	if ((letter = strstr(argument,vch->getName())) == NULL)
	{
	    MOBtrigger = FALSE;
	    act("$N $t",vch,argument,ch,TO_CHAR);
	    MOBtrigger = TRUE;
	    continue;
	}

	strcpy(temp,argument);
	temp[strlen(argument) - strlen(letter)] = '\0';
   	last[0] = '\0';
 	name = vch->getName();
	
	for (; *letter != '\0'; letter++)
	{ 
	    if (*letter == '\'' && matches == strlen(vch->getName()))
	    {
		strcat(temp,"r");
		continue;
	    }

	    if (*letter == 's' && matches == strlen(vch->getName()))
	    {
		matches = 0;
		continue;
	    }
	    
 	    if (matches == strlen(vch->getName()))
	    {
		matches = 0;
	    }

	    if (*letter == *name)
	    {
		matches++;
		name++;
		if (matches == strlen(vch->getName()))
		{
		    strcat(temp,"you");
		    last[0] = '\0';
		    name = vch->getName();
		    continue;
		}
		strncat(last,letter,1);
		continue;
	    }

	    matches = 0;
	    strcat(temp,last);
	    strncat(temp,letter,1);
	    last[0] = '\0';
	    name = vch->getName();
	}

	MOBtrigger = FALSE;
	act("$N $t",vch,temp,ch,TO_CHAR);
	MOBtrigger = TRUE;
    }
	
    return;
}


/*
 * All the posing stuff.
 */
struct	pose_table_type
{
    const char *	message[2*MAX_CLASS];
};

const	struct	pose_table_type	pose_table	[]	=
{
    {
	{
	    "You sizzle with energy.",
	    "$n sizzles with energy.",
	    "You feel very holy.",
	    "$n looks very holy.",
	    "You perform a small card trick.",
	    "$n performs a small card trick.",
	    "You show your bulging muscles.",
	    "$n shows $s bulging muscles."
	}
    },

    {
	{
	    "You turn into a butterfly, then return to your normal shape.",
	    "$n turns into a butterfly, then returns to $s normal shape.",
	    "You nonchalantly turn wine into water.",
	    "$n nonchalantly turns wine into water.",
	    "You wiggle your ears alternately.",
	    "$n wiggles $s ears alternately.",
	    "You crack nuts between your fingers.",
	    "$n cracks nuts between $s fingers."
	}
    },

    {
	{
	    "Blue sparks fly from your fingers.",
	    "Blue sparks fly from $n's fingers.",
	    "A halo appears over your head.",
	    "A halo appears over $n's head.",
	    "You nimbly tie yourself into a knot.",
	    "$n nimbly ties $mself into a knot.",
	    "You grizzle your teeth and look mean.",
	    "$n grizzles $s teeth and looks mean."
	}
    },

    {
	{
	    "Little red lights dance in your eyes.",
	    "Little red lights dance in $n's eyes.",
	    "You recite words of wisdom.",
	    "$n recites words of wisdom.",
	    "You juggle with daggers, apples, and eyeballs.",
	    "$n juggles with daggers, apples, and eyeballs.",
	    "You hit your head, and your eyes roll.",
	    "$n hits $s head, and $s eyes roll."
	}
    },

    {
	{
	    "A slimy green monster appears before you and bows.",
	    "A slimy green monster appears before $n and bows.",
	    "Deep in prayer, you levitate.",
	    "Deep in prayer, $n levitates.",
	    "You steal the underwear off every person in the room.",
	    "Your underwear is gone!  $n stole it!",
	    "Crunch, crunch -- you munch a bottle.",
	    "Crunch, crunch -- $n munches a bottle."
	}
    },

    {
	{
	    "You turn everybody into a little pink elephant.",
	    "You are turned into a little pink elephant by $n.",
	    "An angel consults you.",
	    "An angel consults $n.",
	    "The dice roll ... and you win again.",
	    "The dice roll ... and $n wins again.",
	    "... 98, 99, 100 ... you do pushups.",
	    "... 98, 99, 100 ... $n does pushups."
	}
    },

    {
	{
	    "A small ball of light dances on your fingertips.",
	    "A small ball of light dances on $n's fingertips.",
	    "Your body glows with an unearthly light.",
	    "$n's body glows with an unearthly light.",
	    "You count the money in everyone's pockets.",
	    "Check your money, $n is counting it.",
	    "Arnold Schwarzenegger admires your physique.",
	    "Arnold Schwarzenegger admires $n's physique."
	}
    },

    {
	{
	    "Smoke and fumes leak from your nostrils.",
	    "Smoke and fumes leak from $n's nostrils.",
	    "A spot light hits you.",
	    "A spot light hits $n.",
	    "You balance a pocket knife on your tongue.",
	    "$n balances a pocket knife on your tongue.",
	    "Watch your feet, you are juggling granite boulders.",
	    "Watch your feet, $n is juggling granite boulders."
	}
    },

    {
	{
	    "The light flickers as you rap in magical languages.",
	    "The light flickers as $n raps in magical languages.",
	    "Everyone levitates as you pray.",
	    "You levitate as $n prays.",
	    "You produce a coin from everyone's ear.",
	    "$n produces a coin from your ear.",
	    "Oomph!  You squeeze water out of a granite boulder.",
	    "Oomph!  $n squeezes water out of a granite boulder."
	}
    },

    {
	{
	    "Your head disappears.",
	    "$n's head disappears.",
	    "A cool breeze refreshes you.",
	    "A cool breeze refreshes $n.",
	    "You step behind your shadow.",
	    "$n steps behind $s shadow.",
	    "You pick your teeth with a spear.",
	    "$n picks $s teeth with a spear."
	}
    },

    {
	{
	    "A fire elemental singes your hair.",
	    "A fire elemental singes $n's hair.",
	    "The sun pierces through the clouds to illuminate you.",
	    "The sun pierces through the clouds to illuminate $n.",
	    "Your eyes dance with greed.",
	    "$n's eyes dance with greed.",
	    "Everyone is swept off their foot by your hug.",
	    "You are swept off your feet by $n's hug."
	}
    },

    {
	{
	    "The sky changes color to match your eyes.",
	    "The sky changes color to match $n's eyes.",
	    "The ocean parts before you.",
	    "The ocean parts before $n.",
	    "You deftly steal everyone's weapon.",
	    "$n deftly steals your weapon.",
	    "Your karate chop splits a tree.",
	    "$n's karate chop splits a tree."
	}
    },

    {
	{
	    "The stones dance to your command.",
	    "The stones dance to $n's command.",
	    "A thunder cloud kneels to you.",
	    "A thunder cloud kneels to $n.",
	    "The Grey Mouser buys you a beer.",
	    "The Grey Mouser buys $n a beer.",
	    "A strap of your armor breaks over your mighty thews.",
	    "A strap of $n's armor breaks over $s mighty thews."
	}
    },

    {
	{
	    "The heavens and grass change colour as you smile.",
	    "The heavens and grass change colour as $n smiles.",
	    "The Burning Man speaks to you.",
	    "The Burning Man speaks to $n.",
	    "Everyone's pocket explodes with your fireworks.",
	    "Your pocket explodes with $n's fireworks.",
	    "A boulder cracks at your frown.",
	    "A boulder cracks at $n's frown."
	}
    },

    {
	{
	    "Everyone's clothes are transparent, and you are laughing.",
	    "Your clothes are transparent, and $n is laughing.",
	    "An eye in a pyramid winks at you.",
	    "An eye in a pyramid winks at $n.",
	    "Everyone discovers your dagger a centimeter from their eye.",
	    "You discover $n's dagger a centimeter from your eye.",
	    "Mercenaries arrive to do your bidding.",
	    "Mercenaries arrive to do $n's bidding."
	}
    },

    {
	{
	    "A black hole swallows you.",
	    "A black hole swallows $n.",
	    "Valentine Michael Smith offers you a glass of water.",
	    "Valentine Michael Smith offers $n a glass of water.",
	    "Where did you go?",
	    "Where did $n go?",
	    "Four matched Percherons bring in your chariot.",
	    "Four matched Percherons bring in $n's chariot."
	}
    },

    {
	{
	    "The world shimmers in time with your whistling.",
	    "The world shimmers in time with $n's whistling.",
	    "The great god Mota gives you a staff.",
	    "The great god Mota gives $n a staff.",
	    "Click.",
	    "Click.",
	    "Atlas asks you to relieve him.",
	    "Atlas asks $n to relieve him."
	}
    }
};



void do_pose( Character *ch, char *argument )
{
    unsigned int level;
    int pose;

    if ( IS_NPC(ch) )
	return;

    level = UMIN( (unsigned int)ch->level, sizeof(pose_table) / sizeof(pose_table[0]) - 1 );
    pose  = number_range(0, level);

    act( pose_table[pose].message[2*ch->class_num+0], ch, NULL, NULL, TO_CHAR );
    act( pose_table[pose].message[2*ch->class_num+1], ch, NULL, NULL, TO_ROOM );

    return;
}



void do_bug( Character *ch, char *argument )
{
    append_file( ch, BUG_FILE, argument );
    send_to_char( "Bug logged.\n\r", ch );
    return;
}

void do_typo( Character *ch, char *argument )
{
    append_file( ch, TYPO_FILE, argument );
    send_to_char( "Typo logged.\n\r", ch );
    return;
}

void do_rent( Character *ch, char *argument )
{
    send_to_char( "There is no rent here.  Just save and quit.\n\r", ch );
    return;
}


void do_qui( Character *ch, char *argument )
{
    send_to_char( "If you want to QUIT, you have to spell it out.\n\r", ch );
    return;
}



void do_quit( Character *caller, char *argument )
{
    DESCRIPTOR_DATA *d,*d_next;
    int id;

    if ( caller->isNPC() )
	return;

    PlayerCharacter *ch = (PlayerCharacter*)caller;

    if ( ch->position == POS_FIGHTING )
    {
	send_to_char( "No way! You are fighting.\n\r", ch );
	return;
    }

    if ( ch->getAdrenaline() > 0 )
    {
	send_to_char("You are too excited from the chase.\n\r", ch );
	return;
    }

    if ( ch->position  < POS_STUNNED  )
    {
	send_to_char( "You're not DEAD yet.\n\r", ch );
	return;
    }

    if (IS_CLANNED(ch))
    {
	ch->pcdata->clan->played += current_time - ch->logon;
	save_clan(ch->pcdata->clan);
    }

/*
    send_to_char( 
"In the beginning, there was George.\n\r\
And {RHe{x said: \"Let there be {YLight{x\"\n\r\
And there {Rw{ra{Rs{x {CIndustrial {WLight{x and {MMagic{x\n\r"
, ch);
-OLD!  Taken out march 30
*/

    send_to_char("'Good, bad, I'm the guy with the gun.' -Ash\n\r",ch);
    act( "$n has left the game.", ch, NULL, NULL, TO_ROOM );
    sprintf( log_buf, "%s has quit.", ch->getName() );
    log_string( log_buf );
     Wiznet::instance()->report("$N rejoins the real world.",ch,NULL,WIZ_LOGINS,0,get_trust(ch));

    /*
     * After extract_char the ch is no longer valid!
     */
    save_char_obj( ch );

    /* Free note that might be there somehow */
    if (ch->pcdata->in_progress) {
       delete ch->pcdata->in_progress;
       ch->pcdata->in_progress = NULL;
     }

    id = ch->id;
    d = ch->desc;
    extract_char( ch, TRUE );
    if ( d != NULL )
	close_socket( d );

    /* toast evil cheating bastards */
    for (d = descriptor_list; d != NULL; d = d_next)
    {
	Character *tch;

	d_next = d->next;
	tch = d->original ? d->original : d->character;
	if (tch && tch->id == id)
	{
	    extract_char(tch,TRUE);
	    close_socket(d);
	} 
    }

    return;
}



void do_save( Character *ch, char *argument )
{
    if ( IS_NPC(ch) )
	return;

    save_char_obj( ch );
    send_to_char("Saving. Remember that ROM has automatic saving now.\n\r", ch);
//    WAIT_STATE(ch,4 * PULSE_VIOLENCE);
    return;
}



void do_follow( Character *ch, char *argument )
{
/* RT changed to allow unlimited following and follow the NOFOLLOW rules */
    char arg[MAX_INPUT_LENGTH];
    Character *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Follow whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL )
    {
	act( "But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR );
	return;
    }

    if ( victim == ch )
    {
	if ( ch->master == NULL )
	{
	    send_to_char( "You already follow yourself.\n\r", ch );
	    return;
	}
	stop_follower(ch);
	return;
    }

    if (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOFOLLOW) && !IS_IMMORTAL(ch))
    {
	act("$N doesn't seem to want any followers.\n\r",
             ch,NULL,victim, TO_CHAR);
        return;
    }

    REMOVE_BIT(ch->act,PLR_NOFOLLOW);
    
    if ( ch->master != NULL )
	stop_follower( ch );

    add_follower( ch, victim );
    return;
}


void add_follower( Character *ch, Character *master )
{
    if ( ch->master != NULL )
    {
	bug( "Add_follower: non-null master.", 0 );
	return;
    }

    ch->master        = master;
    ch->leader        = NULL;

    if ( can_see( master, ch ) )
	act( "$n now follows you.", ch, NULL, master, TO_VICT );

    act( "You now follow $N.",  ch, NULL, master, TO_CHAR );

    return;
}



void stop_follower( Character *ch )
{
    if ( ch->master == NULL )
    {
	bug( "Stop_follower: null master.", 0 );
	return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) )
    {
	REMOVE_BIT( ch->affected_by, AFF_CHARM );
	affect_strip( ch, gsn_charm_person );
    }

    if ( can_see( ch->master, ch ) && ch->in_room != NULL)
    {
	act( "$n stops following you.",     ch, NULL, ch->master, TO_VICT    );
    	act( "You stop following $N.",      ch, NULL, ch->master, TO_CHAR    );
    }
    if (ch->master->pet == ch)
	ch->master->pet = NULL;

    ch->master = NULL;
    ch->leader = NULL;
    return;
}

/* nukes charmed monsters and pets */
void nuke_pets( Character *ch )
{    
    Character *pet;

    if ((pet = ch->pet) != NULL)
    {
    	stop_follower(pet);
    	if (pet->in_room != NULL)
    	    act("$N slowly fades away.",ch,NULL,pet,TO_NOTVICT);
    	extract_char(pet,TRUE);
    }
    ch->pet = NULL;

    return;
}



void die_follower( Character *ch )
{
    Character *fch;

    if ( ch->master != NULL )
    {
    	if (ch->master->pet == ch)
    	    ch->master->pet = NULL;
	stop_follower( ch );
    }

    ch->leader = NULL;

    for (std::list<Character*>::iterator it = char_list.begin(); it != char_list.end(); it++)
    {
      fch = *it;

      if ( fch->master == ch )
        stop_follower( fch );
	    if ( ch->leader == ch )
	       fch->leader = fch;
    }

    return;
}



void do_order( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    Character *victim;
    Character *och;
    Character *och_next;
    bool found;
    bool fAll;

    argument = one_argument( argument, arg );
    one_argument(argument,arg2);

    if (!str_cmp(arg2,"delete") || !str_cmp(arg2,"mob"))
    {
        send_to_char("That will NOT be done.\n\r",ch);
        return;
    }

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	send_to_char( "Order whom to do what?\n\r", ch );
	return;
    }

    if ( IS_AFFECTED( ch, AFF_CHARM ) )
    {
	send_to_char( "You feel like taking, not giving, orders.\n\r", ch );
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
	fAll   = TRUE;
	victim = NULL;
    }
    else
    {
	fAll   = FALSE;
	if ( ( victim = get_char_room( ch, arg ) ) == NULL )
	{
	    send_to_char( "They aren't here.\n\r", ch );
	    return;
	}

	if ( victim == ch )
	{
	    send_to_char( "Aye aye, right away!\n\r", ch );
	    return;
	}

	if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch 
	||  (IS_IMMORTAL(victim) && victim->trust >= ch->trust))
	{
	    send_to_char( "Do it yourself!\n\r", ch );
	    return;
	}
    }

    found = FALSE;
    for ( och = ch->in_room->people; och != NULL; och = och_next )
    {
	och_next = och->next_in_room;

	if ( IS_AFFECTED(och, AFF_CHARM)
	&&   och->master == ch
	&& ( fAll || och == victim ) )
	{
	    found = TRUE;
	    sprintf( buf, "$n orders you to '%s'.", argument );
	    act( buf, ch, NULL, och, TO_VICT );
	    interpret( och, argument );
	}
    }

    if ( found )
    {
	WAIT_STATE(ch,PULSE_VIOLENCE);
	send_to_char( "Ok.\n\r", ch );
    }
    else
	send_to_char( "You have no followers here.\n\r", ch );
    return;
}



void do_group( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Character *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	Character *gch;
	Character *leader;

	leader = (ch->leader != NULL) ? ch->leader : ch;
	sprintf( buf, "%s's group:\n\r", PERS(leader, ch) );
	send_to_char( buf, ch );

    for (std::list<Character*>::iterator it = char_list.begin(); it != char_list.end(); it++)
	{
   gch = *it;
	    if ( is_same_group( gch, ch ) )
	    {
		sprintf( buf,
		"[%2d %s] %-16s %4d/%4d hp %4d/%4d mana %4d/%4d mv %5d xp\n\r",
		    gch->level,
		    IS_NPC(gch) ? "Mob" : class_table[gch->class_num].who_name,
		    capitalize( PERS(gch, ch) ),
		    gch->hit,   gch->max_hit,
		    gch->mana,  gch->max_mana,
		    gch->move,  gch->max_move,
		    gch->exp    );
		send_to_char( buf, ch );
	    }
	}
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( ch->master != NULL || ( ch->leader != NULL && ch->leader != ch ) )
    {
	send_to_char( "But you are following someone else!\n\r", ch );
	return;
    }

    if ( victim->master != ch && ch != victim )
    {
	act_new("$N isn't following you.",ch,NULL,victim,TO_CHAR,POS_SLEEPING);
	return;
    }
    
    if (IS_AFFECTED(victim,AFF_CHARM))
    {
        send_to_char("You can't remove charmed mobs from your group.\n\r",ch);
        return;
    }
    
    if (IS_AFFECTED(ch,AFF_CHARM))
    {
    	act_new("You like your master too much to leave $m!",
	    ch,NULL,victim,TO_VICT,POS_SLEEPING);
    	return;
    }

    if ( is_same_group( victim, ch ) && ch != victim )
    {
	victim->leader = NULL;
	act_new("$n removes $N from $s group.",
	    ch,NULL,victim,TO_NOTVICT,POS_RESTING);
	act_new("$n removes you from $s group.",
	    ch,NULL,victim,TO_VICT,POS_SLEEPING);
	act_new("You remove $N from your group.",
	    ch,NULL,victim,TO_CHAR,POS_SLEEPING);
	return;
    }

    victim->leader = ch;
    act_new("$N joins $n's group.",ch,NULL,victim,TO_NOTVICT,POS_RESTING);
    act_new("You join $n's group.",ch,NULL,victim,TO_VICT,POS_SLEEPING);
    act_new("$N joins your group.",ch,NULL,victim,TO_CHAR,POS_SLEEPING);
    return;
}



/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    Character *gch;
    int members;
    int amount_gold = 0, amount_silver = 0;
    int share_gold, share_silver;
    int extra_gold, extra_silver;

    argument = one_argument( argument, arg1 );
	       one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
	send_to_char( "Split how much?\n\r", ch );
	return;
    }
    
    amount_silver = atoi( arg1 );

    if (arg2[0] != '\0')
	amount_gold = atoi(arg2);

    if ( amount_gold < 0 || amount_silver < 0)
    {
	send_to_char( "Your group wouldn't like that.\n\r", ch );
	return;
    }

    if ( amount_gold == 0 && amount_silver == 0 )
    {
	send_to_char( "You hand out zero coins, but no one notices.\n\r", ch );
	return;
    }

    if ( ch->gold <  amount_gold || ch->silver < amount_silver)
    {
	send_to_char( "You don't have that much to split.\n\r", ch );
	return;
    }
  
    members = 0;
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
	if ( is_same_group( gch, ch ) && !IS_AFFECTED(gch,AFF_CHARM))
	    members++;
    }

    if ( members < 2 )
    {
	send_to_char( "Just keep it all.\n\r", ch );
	return;
    }
	    
    share_silver = amount_silver / members;
    extra_silver = amount_silver % members;

    share_gold   = amount_gold / members;
    extra_gold   = amount_gold % members;

    if ( share_gold == 0 && share_silver == 0 )
    {
	send_to_char( "Don't even bother, cheapskate.\n\r", ch );
	return;
    }

    ch->silver	-= amount_silver;
    ch->silver	+= share_silver + extra_silver;
    ch->gold 	-= amount_gold;
    ch->gold 	+= share_gold + extra_gold;

    if (share_silver > 0)
    {
	sprintf(buf,
	    "You split %d silver coins. Your share is %d silver.\n\r",
 	    amount_silver,share_silver + extra_silver);
	send_to_char(buf,ch);
    }

    if (share_gold > 0)
    {
	sprintf(buf,
	    "You split %d gold coins. Your share is %d gold.\n\r",
	     amount_gold,share_gold + extra_gold);
	send_to_char(buf,ch);
    }

    if (share_gold == 0)
    {
	sprintf(buf,"$n splits %d silver coins. Your share is %d silver.",
		amount_silver,share_silver);
    }
    else if (share_silver == 0)
    {
	sprintf(buf,"$n splits %d gold coins. Your share is %d gold.",
		amount_gold,share_gold);
    }
    else
    {
	sprintf(buf,
"$n splits %d silver and %d gold coins, giving you %d silver and %d gold.\n\r",
	 amount_silver,amount_gold,share_silver,share_gold);
    }

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
	if ( gch != ch && is_same_group(gch,ch) && !IS_AFFECTED(gch,AFF_CHARM))
	{
	    act( buf, ch, NULL, gch, TO_VICT );
	    gch->gold += share_gold;
	    gch->silver += share_silver;
	}
    }

    return;
}



void do_gtell( Character *ch, char *argument )
{
    Character *gch = NULL;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Tell your group what?\n\r", ch );
	return;
    }

    if ( IS_SET( ch->comm, COMM_NOTELL ) )
    {
	send_to_char( "Your message didn't get through!\n\r", ch );
	return;
    }

    for (std::list<Character*>::iterator it = char_list.begin(); it != char_list.end(); it++)
    {
      gch = *it;
	if ( is_same_group( gch, ch ) )
	    act_new("{W$n tells the group '$t{W'{x",
		ch,argument,gch,TO_VICT,POS_SLEEPING);
    }
    act_new("{WYou tell the group '$t{W'{x", ch, argument,
		gch, TO_CHAR, POS_SLEEPING);
    return;
}



/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group( Character *ach, Character *bch )
{
    if ( ach == NULL || bch == NULL)
	return FALSE;

    if ( ach->leader != NULL ) ach = ach->leader;
    if ( bch->leader != NULL ) bch = bch->leader;
    return ach == bch;
}

/*
 * Colour setting and unsetting, way cool, Lope Oct '94
 */
void do_colour( Character *ch, char *argument )
{
    char       arg[ MAX_STRING_LENGTH ];

    argument = one_argument( argument, arg );

    if( !*arg )
    {
       if( !IS_SET( ch->act, PLR_COLOUR ) )
       {
           SET_BIT( ch->act, PLR_COLOUR );
           send_to_char( "{bC{ro{yl{co{mu{gr{x is now {rON{x, Way Cool!\n\r", ch );
       }
       else
       {
           send_to_char_bw( "Colour is now OFF, <sigh>\n\r", ch );
           REMOVE_BIT( ch->act, PLR_COLOUR );
       }
       return;
    }
    else
    {
       send_to_char_bw( "Colour Configuration is unavailable in this\n\r", ch );
       send_to_char_bw( "version of colour, sorry\n\r", ch );
    }

    return;
}

void do_noauction( Character *ch, char *argument )
{
    if (IS_SET(ch->comm, COMM_NOAUCTION) )
    {
	REMOVE_BIT(ch->comm, COMM_NOAUCTION);
	send_to_char("You will no longer see auctions.\n\r",ch);
    }
    else
    {
	SET_BIT(ch->comm, COMM_NOAUCTION);
	send_to_char("You will now see auctions.\n\r",ch);
    }
}

void do_reclass( Character *caller, char *argument)
{
    if ( caller->isNPC() ) {
        send_to_char("Only players may reclass\n\r", caller);
        return;
    }

    PlayerCharacter *ch = (PlayerCharacter*)caller;

    char buf[MAX_STRING_LENGTH];
    int iClass;
    int count = 0;

    if (ch->level < LEVEL_HERO)
    {
	send_to_char("Only heroes may reclass.\n\r",ch);
	return;
    }

    /* First check to make sure there's a class available! */
    for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
    {
	if (!IS_SET(ch->done, class_table[iClass].flag)
		&& can_be_class( ch, iClass ) )
	    count++;
    }

    if (count < 1)
    {
	send_to_char("There are no classes for you to chose at this time.\n\r",ch);
	return;
    }

    /* Confirm first */
    if (!ch->confirm_reclass)
    {
	send_to_char("You are about to reclass.  This will cut your stats in half and drop you\n\r",ch);
	send_to_char("to level 25.  Once done, this cannot be reversed.\n\r",ch);
	send_to_char("Type 'reclass' again to begin the process.\n\r",ch);
	ch->confirm_reclass = TRUE;
	return;
    }

    /* Now do the dirty work */
    send_to_char("Reclassing process has begun.\n\r",ch);
    ch->max_hit			/=	2;
    ch->hit			/=	2;
    ch->pcdata->perm_hit	/=	2;
    ch->max_mana		/=	2;
    ch->pcdata->perm_mana	/=	2;
    ch->mana			/=	2;
    ch->max_move		/=	2;
    ch->pcdata->perm_move	/=	2;
    ch->move			/=	2;
    ch->level			 =	25;
    ch->train 			+=	5;
    ch->practice		+=	5;
    ch->reclass_num		+=	1;

    strcpy( buf, "Select a class [" );
    for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
    {
	if (IS_SET(ch->done, class_table[iClass].flag) ||
		!can_be_class( ch, iClass ) )
	    continue;

        if ( iClass > 0)
            strcat( buf, " " );
        strcat( buf, class_table[iClass].name );
    }
    strcat( buf, "]: " );
    send_to_char(buf,ch);

    /* If any messages get sent to them, mud will crash, so we'll
     * prevent that right here.
     */
    ch->setWasNoteRoom(ch->in_room);
    char_from_room( ch );
    char_to_room( ch, get_room_index(ROOM_VNUM_NOTE));

    ch->desc->connected = CON_GET_RECLASS;
}

void do_info( Character *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    /* Only imms may send over this channel.  Everyone else just
       is set and unset. */
    if (argument[0] == '\0' || !IS_IMMORTAL(ch))
    {
      if (IS_SET(ch->comm,COMM_NOINFO))
      {
        send_to_char("Info channel is now ON.\n\r",ch);
       REMOVE_BIT(ch->comm,COMM_NOINFO);
      }
      else
      {
        send_to_char("Info channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOINFO);
      }
    }
    else  /* info sent, turn info on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET))
        {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }

        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
        }

        REMOVE_BIT(ch->comm,COMM_NOINFO);

/* Old format, re-integrated
      sprintf( buf, "{YYou INFO: {C'%s{C'{x\n\r", argument );
      send_to_char( buf, ch );
      sprintf( buf, "{Y$n INFO: {C'%s{C'{x", argument );
*/

      sprintf( buf, "{YINFO: {C'%s{C'{x\n\r", argument );
      send_to_char( buf, ch );
      sprintf( buf, "{YINFO: {C'%s{C'{x", argument );

      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        Character *victim;

        victim = d->original ? d->original : d->character;

        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_NOINFO) &&
             !IS_SET(victim->comm,COMM_QUIET) )
        {
            act_new("{Y$n INFO: {C'$t{C'{x",
                    ch,argument,d->character,TO_VICT,POS_SLEEPING);
        }
      }
    }
}

