/* flags.h,v 2.10 1994/02/21 08:32:54 dmoore Exp */
#ifndef MUCK_FLAGS_H
#define MUCK_FLAGS_H

typedef long object_flag_type;

#define TYPE_ROOM 	0x0
#define TYPE_THING 	0x1
#define TYPE_EXIT 	0x2
#define TYPE_PLAYER 	0x3
#define TYPE_PROGRAM    0x4
#define TYPE_GARBAGE	0x6
#define NOTYPE		0x7
#define TYPE_MASK 	0x7
#define TYPE_ANY	TYPE_MASK
#define NOFLAGS		0x0	/* Used for default flags. */
#define WIZARD		0x10	/* gets automatic control */
#define LINK_OK		0x20	/* anybody can link to this room */
#define DARK		0x40	/* contents of room are not printed */
#define LINE_NUMBERS	0x80	/* Show the player line numbers. */
#define STICKY		0x100	/* this object goes home when dropped */
#define BUILDER		0x200	/* this player can use construction commands */
#define	CHOWN_OK	0x400	/* this player can be @chowned to */
#define JUMP_OK		0x800
#define DEBUG		0x1000
#define SETUID		0x2000
#define HAVEN           0x10000	/* can't kill here */
#define ABODE           0x20000	/* can set home here */
#define MUCKER          0x40000	/* programmer */
#define MURKY           MUCKER  /* for rooms.  show no contents */
#define QUELL		0x80000 /* Quelled wizard */
#define IN_EDITOR	0x100000 /* Player is in editor. */
#define IN_PROGRAM	0x200000 /* Player is in program read. */
#define INSERT_MODE     0x400000 /* Player is inserting in the editor. */
#define EDIT_LOCK	0x800000 /* Program is being edited. */
#define BEING_RECYCLED  0x1000000 /* Object has been mark()'d for recycle */
#define VISITED		0x2000000 /* Used in sanity checker. */

#ifdef GOD_FLAGS
#define GOD		0x4000000
#endif

#ifdef DRUID_MUCK
#define INVISIBLE	0x8000000
#endif

#define BAD_FLAG	((object_flag_type) -1)

#endif /* MUCK_FLAGS_H */
