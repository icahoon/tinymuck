# This makefile inherits many flags from ../Makefile, don't directly
# run make in this directory.
# Makefile,v 2.5 1994/02/04 00:17:55 dmoore Exp

# editor

CFLAGS	=	$(OPTIM) -I../include -I. -I..

SRC	=	edit.c \
		text.c 

OBJ	=	edit.o \
		text.o

GENHDR	=	editor.h editortab.h

MKFLAGS	=	CC="$(CC)" OPTIM="$(OPTIM)" LDFLAGS="$(LDFLAGS)" \
		AR="$(AR)" RANLIB="$(RANLIB)"

LIB	=	../libeditor.a

SCRIPTS	=	../scripts

$(LIB):	$(OBJ)
	$(AR) $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

depend:	$(GENHDR)
	makedepend -- ${CFLAGS} -- $(SRC)

clean:
	-rm -f $(OBJ) $(GENHDR)

editor.h:	${SCRIPTS}/mkcommands.sh editor
	${SCRIPTS}/mkcommands.sh editor editor.h "static" "(const dbref, const dbref, const char **)"

editortab.h:	${SCRIPTS}/mkcommandtab.sh editor
	${SCRIPTS}/mkcommandtab.sh editor editortab.h

# Some defaults in case you don't have makedepend.
edit.o:		edit.c $(GENHDR)

# DO NOT DELETE THIS LINE -- make depend depends on it.
