/* params.h,v 2.12 1997/08/29 21:00:19 dmoore Exp */
#ifndef MUCK_PARAMS_H
#define MUCK_PARAMS_H

#define MUCK_VERSION "TinyMUCK 2.4"

/* Location of files used by tinymuck */

#define WELC_FILE "data/welcome.txt" /* For the opening screen */
#define HELP_FILE "data/help.txt"    /* For the 'help' command */
#define NEWS_FILE "data/news.txt"    /* For the 'news' command */
#define MAN_FILE  "data/forth.ref"   /* For the 'man' command */
#define EDITOR_HELP_FILE "data/edit-help.txt" /* editor help file */
#define LOCKOUT_FILE "data/lockout.sites" /* Registration/lockouts */

#define MACRO_FILE "muf/macros"

#define GRIPE_LOG   "logs/gripes"       /* Gripes Log */
#define STATUS_LOG  "logs/status"       /* System errors and stats */
#define MUF_LOG     "logs/muf-errors"   /* Muf compiler errors and warnings. */
#define COMMAND_LOG "logs/commands"     /* Player commands */
#define INFO_LOG    "logs/info"         /* Hashtable performance info, etc. */


/* penny related stuff */

/* amount of object endowment, based on cost */

#define MAX_OBJECT_ENDOWMENT 100
#define MAX_OBJECT_DEPOSIT 504
#define OBJECT_ENDOWMENT(cost) (((cost)-5)/5)
#define OBJECT_DEPOSIT(endow) ((endow)*5+4)

/* minimum costs for various things */

#define OBJECT_COST 10		/* Amount it costs to make an object	*/
#define EXIT_COST 1		/* Amount it costs to make an exit	*/
#define LINK_COST 1		/* Amount it costs to make a link	*/
#define ROOM_COST 10		/* Amount it costs to dig a room 	*/
#define LOOKUP_COST 1		/* cost to do a full db scan            */
#define PAGE_COST 0		/* cost to page someone			*/
#define MAX_PENNIES 10000	/* amount at which temple gets cheap 	*/
#define PENNY_RATE 5		/* 1/chance of getting a penny per room */

/* costs of kill command */

#define KILL_BASE_COST 100	/* prob = expenditure/KILL_BASE_COST	*/
#define KILL_MIN_COST 10	/* minimum amount needed to kill	*/
#define KILL_BONUS 50		/* amount of "insurance" paid to victim */


/* timing stuff */

#define TIME_MINUTE(x)	(60 * (x))		/* 60 seconds */
#define TIME_HOUR(x)	(60 * (TIME_MINUTE(x)))	/* 60 minutes */
#define TIME_DAY(x)	(24 * (TIME_HOUR(x)))	/* 24 hours   */

#define COMMAND_TIME_MSEC 1000	/* time slice length in milliseconds	*/
#define COMMAND_BURST_SIZE 100	/* commands allowed per user in a burst */
#define COMMANDS_PER_TIME 1	/* commands per time slice after burst	*/


/* max length of command taken as socket input. */
/* Remember BUFFER_LEN has to be >= MAX_COMMAND_LEN!!!!! */
#define MAX_COMMAND_LEN 2048	/* Originally 512. */


/* maximum amount of queued output */
#define MAX_OUTPUT 16384

#define QUIT_COMMAND "QUIT"
#define WHO_COMMAND "WHO"

#define PREFIX_COMMAND "OUTPUTPREFIX"
#define SUFFIX_COMMAND "OUTPUTSUFFIX"

/* Change to 'home' if you don't want it to be too confusing to novices. */

#define BREAK_COMMAND "@Q"

/* maximum number of arguments */
#define MAX_ARG  2

/* Usage comments:
   Line numbers start from 1, so when an argument variable is equal to 0, it
   means that it is non existent.

   I've chosen to put the parameters before the command, because this should
   more or less make the players get used to the idea of forth coding..     */

#define EXIT_INSERT "."		/* character to exit from the editor	*/
#define EXIT_DELIMITER ';'	/* delimiter for lists of exit aliases	*/

#define MAX_LINKS 50		/* maximum # destinations for an exit */
#define MAX_BOOLEXP_NEST 50	/* Maximum nesting of ('s and !'s in locks. */
#define MAX_BOOLEXP_NODE 2048	/* Maximum number of leafs/connects in lock. */
#define MAX_IF_STMT_NEST 200	/* Maximum nesting of if-elses in muf. */
#define MAX_ANON_DEPTH 50
#define MAX_INTERP_CREATE 5	/* Maximum object creation per program run. */

/* magic cookies (not chocolate chip) :) */

#define NOT_TOKEN '!'
#define AND_TOKEN '&'
#define OR_TOKEN '|'
#define LOOKUP_TOKEN '*'
#define NUMBER_TOKEN '#'
#define ARG_DELIMITER '='
#define PROP_DELIMITER ':'
#define PROP_RDONLY '_'
#define PROP_PRIVATE '.'

/* magic command cookies (oh me, oh my!) */

#define SAY_TOKEN '"'
#define POSE_TOKEN ':'

/* compiler stuff. */
#define BEGINCOMMENT '('
#define ENDCOMMENT ')'
#define BEGINSTRING '"'
#define ENDSTRING '"'
#define BEGINMACRO '.'

#define MAX_MACRO_SUBS 20  /* How many nested macros will we allow? */

/* Sizes of various hash tables. */
/* #### this should be made more dynamic */

#define PROP_HASH_SIZE     ((1 << 16) - 0) /* dbref+prop name -> prop value */
#define INTERN_HASH_SIZE   ((1 << 15) - 0) /* Property names. */
#define PLAYER_HASH_SIZE   ((1 << 12) - 0) /* player name -> dbref */
#define MACRO_HASH_SIZE	   ((1 << 10) - 0) /* macro name -> macro data */
#define HOST_HASH_SIZE	   ((1 << 10) - 0) /* IP addr -> hostname. */
#define SYMTAB_HASH_SIZE   ((1 << 10) - 0) /* Symbol table for compiler. */
#define CODE_HASH_SIZE	   ((1 << 10) - 0) /* prog dbref -> compiled code */
#define EDIT_HASH_SIZE     ((1 << 5)  - 0) /* Player -> text */
#define FRAME_HASH_SIZE	   ((1 << 5)  - 0) /* player -> program being run */

/* Stuff for interpreter. */
#define MAX_VAR		53
#define DATA_STACK_SIZE	512
#define ADDR_STACK_SIZE	512
#define MAX_INSTRUCTIONS	1000000

#endif /* MUCK_PARAMS_H */
