/* Blizzard's banking code begins.  Hehe :) */

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "merc.h"
#include "interp.h"

DECLARE_DO_FUN( do_bank_change	);
DECLARE_DO_FUN( do_bank_clan	);

void do_bank( CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );

    if (!IS_SET(ch->in_room->room_flags, ROOM_BANK) )
    {
	send_to_char("You must be in a bank first.\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	send_to_char("Bank commands are: balance close deposit open withdraw\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "balance"))
    {
	do_bank_balance(ch,(char*)"");
	return;
    }

    if (!str_cmp(arg, "close"))
    {
	do_bank_close(ch,(char*)"");
	return;
    }

    if (!str_cmp(arg, "clan"))
    {
	do_bank_clan(ch,argument);
	return;
    }

    if (!str_cmp(arg, "deposit") || !str_cmp(arg, "put"))
    {
	do_bank_deposit(ch,argument);
	return;
    }

    if (!str_cmp(arg, "open"))
    {
	do_bank_open(ch,(char*)"");
	return;
    }

    if (!str_cmp(arg, "withdraw") || !str_cmp(arg, "get"))
    {
	do_bank_withdraw(ch,argument);
	return;
    }

    send_to_char("Bank commands are: balance close deposit open withdraw\n\r", ch);
    return;
}

void do_bank_balance(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;

    if ( ( obj = get_obj_carry( ch, (char*)"bank note", ch ) ) == NULL )
    {
	send_to_char( "You do not own a bank note.\n\r", ch );
	return;
    }

    sprintf(buf,"Balance on this note is %d gold and %d silver.\n\r", obj->value[0], obj->value[1]);
    send_to_char(buf,ch);
}

void do_bank_close(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;

    if ( ( obj = get_obj_carry( ch, (char*)"bank note", ch ) ) == NULL )
    {
	send_to_char( "You do not own a bank note.\n\r", ch );
	return;
    }

    ch->gold += obj->value[0];
    ch->silver += obj->value[1];
    extract_obj( obj );
    send_to_char("You withdraw all the money from your bank note and return it.\n\r",ch);
    return;
}

void do_bank_open(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    if ( ( obj = get_obj_carry( ch, (char*)"bank note", ch ) ) != NULL )
    {
	send_to_char( "You already own a bank note.\n\r", ch );
	return;
    }

    if (ch->gold < 10)
    {
	send_to_char("It costs 10 gold to open an account.\n\r", ch);
	return;
    }

    ch->gold -= 10;
    obj_to_char(create_object(get_obj_index(OBJ_VNUM_BANK_NOTE),0),ch);
    send_to_char("Enjoy your bank note.\n\r", ch);
    return;
}

void do_bank_withdraw( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    int amount;
    OBJ_DATA *obj;


    argument = one_argument( argument, arg );


    if ( IS_NPC(ch) )
    {
        send_to_char( "Mobs don't have bank accounts!\n\r", ch );
        return;
    }

    if ( ( obj = get_obj_carry( ch, (char*)"bank note", ch ) ) == NULL )
    {
        send_to_char( "You do not own a bank note.\n\r", ch );
        return;
    }

    if ( arg[0] == '\0' )
    {
        send_to_char( "Withdraw how much?\n\r", ch );
        return;
    }

    if(!str_cmp(argument, "silver"))
    {
   if ( !is_number( arg ) )
    {
        send_to_char( "Withdraw how much?\n\r", ch );
            return;
        }


        amount = atoi(arg);
        if ( obj->value[1] < amount ) /* is balance - than amount */
        {
            send_to_char( "Your account does not have that much silver in it!\n\r", ch );
            return;
        }    /* else withdraw silver */


        ch->silver += amount;  
        obj->value[1] -= amount;
        act("$n withdraws some silver.", ch, NULL, NULL, TO_ROOM);
    return;
    }


    if(!str_cmp(argument, "gold"))
    {
        if ( !is_number( arg ) )
        {
            send_to_char( "Withdraw how much?\n\r", ch );
            return;
        }
                         
        amount = atoi(arg);
        if ( obj->value[0] < amount ) /* is balance - than amount */
        {
            send_to_char( "Your account does not have that much gold in it!\n\r", ch );
            return;
        }    /* else withdraw gold */


        ch->gold += amount;  
        obj->value[0] -= amount;
        sprintf(buf,"You withdraw %d gold.\n\r", amount);
	send_to_char(buf,ch);
        act("$n withdraws some gold.", ch, NULL, NULL, TO_ROOM );
    return;
    }

       send_to_char("You want to withdraw what?\n\r", ch );
       return;
}   

void do_bank_deposit( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    int amount;
    OBJ_DATA *obj;


    argument = one_argument( argument, arg1 );

    if ( IS_NPC(ch) )
    {
        send_to_char( "Mobs don't have bank accounts!\n\r", ch );
        return;
    }

    if ( ( obj = get_obj_carry( ch, (char*)"bank note", ch ) ) == NULL )
    {
        send_to_char( "You do not own a bank note.\n\r", ch );
        return;
    }

    if (!str_cmp( arg1, "all"))
    {
	if (ch->gold <= 0 && ch->silver <= 0)
	{
	    send_to_char("You have no money to deposit!\n\r",ch);
	    return;
	}

	/* Lets convert our silver first */
	obj->value[1] += ch->silver;
        sprintf( buf, "%d", obj->value[1] );
        do_bank_change(ch, buf );
	sprintf(buf,"You deposit %ld silver and %ld gold into your bank note.\n\r", ch->silver, ch->gold );
	send_to_char(buf,ch);
	obj->value[0] += ch->gold;
	ch->gold = 0;
	ch->silver = 0;
	return;
    }

    if(!str_cmp(argument, "silver"))
    {

    if ( !is_number( arg1 ) )
    {
        send_to_char( "Deposit how much?\n\r", ch );
            return;
        }

        amount = atoi(arg1);
	if (ch->silver < amount || amount <= 0)
	{
	    send_to_char("You don't have that ammount of silver.\n\r",ch);
	    return;
	}

        ch->silver -= amount;  
        obj->value[1] += amount;
	sprintf(buf,"You deposit %d silver.\n\r", amount);
	send_to_char(buf,ch);
	sprintf( buf, "%d", obj->value[1] );
        do_bank_change(ch, buf );
        act("$n deposits some silver.", ch, NULL, NULL, TO_ROOM);
    return;
    }


    if(!str_cmp(argument, "gold"))
    {
        if ((amount = atoi(arg1)) <= 0)
	{
	    send_to_char("You must deposit at least one gold.\n\r",ch);
	    return;
        }

	if (ch->gold < amount || amount <= 0 )
	{
	    send_to_char("You can't put that much gold in the bank.\n\r",ch);
	    return;
	}

        ch->gold -= amount;  
        obj->value[0] += amount;
	sprintf(buf, "You deposit %d gold.\n\r", amount);
	send_to_char(buf,ch);
        act("$n deposits some gold.", ch, NULL, NULL, TO_ROOM );
    return;
    }

       send_to_char("Syntax: bank deposit <ammount> <gold/silver>\n\r", ch );
	send_to_char("Or:     bank deposit all\n\r",ch);
       return;
}   

void do_bank_change( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    long amount;
    int change;
    change = 0;
  
    argument = one_argument( argument, arg );

    amount = atoi(arg);
  
    obj = get_obj_carry( ch, (char*)"bank note", ch );

	
	if ( amount < 0 )
    if ( obj->value[1] < amount )
        return;

    if ( amount < 100 )
    {
        return;
    }

    change = amount/100;
    obj->value[0] += change;
    obj->value[1] -= (change * 100);
    return;
}

void do_bank_clan(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    arg1[0] = '\0';
    arg2[0] = '\0';
    long amount;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if (!IS_CLANNED(ch) && !IS_IMMORTAL(ch))
    {
	send_to_char("You are not in a clan.\n\r",ch);
	return;
    }

    if (IS_IMMORTAL(ch))
    {
	CLAN_DATA *clan;
	int debit;

	if ((clan = get_clan(arg1)) == NULL)
	{
	    send_to_char("Syntax: bank clan <clan name> <amount to withdraw (in k)>\n\r",ch);
	    return;
	}

	if ((debit = atoi(arg2)) <= 0)
	{
	    send_to_char("Syntax: bank clan <clan name> <amount to withdraw (in k)>\n\r",ch);
	    return;
	}

	if (clan->money < debit * 100000)
	{
	    send_to_char("The clan doesn't have that much money!\n\r",ch);
	    return;
	}

	clan->money -= debit * 100000;
	ch->gold += debit * 1000;
	append_file( ch, UPGRADE_FILE, argument ); 
	save_clan(clan);
	return;
    }
    else
    {
	if (str_cmp(ch->pcdata->clan->leader, ch->getName() ))
	{
	    send_to_char("Only your clan leader may access the clan account.\n\r", ch);
	    return;
	}

	if (arg2[0] == '\0' || (str_cmp(arg2, "gold") && str_cmp(arg2, "silver")))
	{
	    send_to_char("Syntax: bank clan (+/- amount) (gold/silver)\n\r",ch);
	    return;
	}

	if (!str_cmp(arg2, "gold"))
	    amount = atoi(arg1) * 100;
	else
	    amount = atoi(arg1);

	if ( amount == 0 )
	{
	    send_to_char("Nothing much happens.\n\r", ch);
	    return;
	}

	if ( amount < 0 && amount * -1 > ch->pcdata->clan->money )
	{
	    send_to_char("You cannot withdraw more than your clan has.\n\r", ch);
	    return;
	}

	if ((ch->gold * 100) + ch->silver < amount && amount > 0)
	{
	    send_to_char("You cannot afford to deposit that much.\n\r",ch);
	    return;
	}

	if (amount > 0)
	{
	    if (ch->silver < amount)
	    {
	        ch->gold -= ((amount - ch->silver + 99) / 100);
	        ch->silver -= amount - (100 * ((amount - ch->silver + 99) / 100));
	    }
	    else
		ch->silver -= amount;

	}
	
	ch->pcdata->clan->money += amount;
	save_clan(ch->pcdata->clan);

	if ( amount < 0 )
	{
	    amount = amount * -1;

	    ch->gold += amount / 100;
	    ch->silver += amount - (amount / 100);

	    send_to_char("You withdraw from your clan fund.\n\r",ch);
	    return;
	}
	else
	{
	    send_to_char("You add to your clan fund.\n\r",ch);
	    return;
	}
    }
}
