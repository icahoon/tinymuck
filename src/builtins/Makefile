# This makefile inherits many flags from ../Makefile, don't directly
# run make in this directory.
# Makefile,v 2.5 1994/02/04 00:17:45 dmoore Exp

# builtins

CFLAGS	=	$(OPTIM) -I../include -I. -I..

SRC	=	action.c \
		attach.c \
		boot.c \
		builtins.c \
		create.c \
		chown.c \
		desc.c \
		dig.c \
		drop.c \
		dropobj.c \
		dump.c \
		edit.c \
		entrances.c \
		examine.c \
		extract.c \
		fail.c \
		find.c \
		force.c \
		get.c \
		give.c \
		gripe.c \
		inv.c \
		kill.c \
		list.c \
		lock.c \
		look.c \
		move.c \
		name.c \
		newpass.c \
		odrop.c \
		ofail.c \
		open.c \
		osucc.c \
		owned.c \
		page.c \
		pass.c \
		pcreate.c \
		pose.c \
		prog.c \
		purge.c \
		recycle.c \
		rob.c \
		say.c \
		score.c \
		set.c \
		shutdown.c \
		stats.c \
		succ.c \
		tele.c \
		toad.c \
		trace.c \
		unlink.c \
		unlock.c \
		version.c \
		wall.c \
		whisper.c

OBJ	=	action.o \
		attach.o \
		boot.o \
		builtins.o \
		create.o \
		chown.o \
		desc.o \
		dig.o \
		drop.o \
		dropobj.o \
		dump.o \
		edit.o \
		entrances.o \
		examine.o \
		extract.o \
		fail.o \
		find.o \
		force.o \
		get.o \
		give.o \
		gripe.o \
		inv.o \
		kill.o \
		link.o \
		list.o \
		lock.o \
		look.o \
		move.o \
		name.o \
		newpass.o \
		odrop.o \
		ofail.o \
		open.o \
		osucc.o \
		owned.o \
		page.o \
		pass.o \
		pcreate.o \
		pose.o \
		prog.o \
		purge.o \
		recycle.o \
		rob.o \
		say.o \
		score.o \
		set.o \
		shutdown.o \
		stats.o \
		succ.o \
		tele.o \
		toad.o \
		trace.o \
		unlink.o \
		unlock.o \
		version.o \
		wall.o \
		whisper.o

GENHDR	=	builtins.h builtintab.h cookies.h

MKFLAGS	=	CC="$(CC)" OPTIM="$(OPTIM)" LDFLAGS="$(LDFLAGS)" \
		AR="$(AR)" RANLIB="$(RANLIB)"

LIB	=	../libbuiltins.a

SCRIPTS	=	../scripts

$(LIB):	$(OBJ)
	$(AR) $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

depend:	$(GENHDR)
	makedepend -- ${CFLAGS} -- $(SRC)

clean:
	-rm -f $(OBJ) $(GENHDR)

builtins.h:	$(SCRIPTS)/mkcommands.sh commands
	$(SCRIPTS)/mkcommands.sh commands builtins.h "extern" "(const dbref, const char *, const char *, const char *)"

builtintab.h:	$(SCRIPTS)/mkcommandtab.sh commands
	$(SCRIPTS)/mkcommandtab.sh commands builtintab.h

cookies.h:	$(SCRIPTS)/mkcookies.sh cookies
	$(SCRIPTS)/mkcookies.sh cookies cookies.h

# Some default rules in case they don't have makedepend.
builtins.o:	builtins.c $(GENHDR)

# DO NOT DELETE THIS LINE -- make depend depends on it.
