# This makefile inherits many flags from ../Makefile, don't directly
# run make in this directory.
# Makefile,v 2.4 1994/02/04 00:17:58 dmoore Exp

# game

CFLAGS	=	$(OPTIM) -I../include -I. -I..

SRC	=	connect.c \
		disconnect.c \
		move.c \
		predicates.c

OBJ	=	connect.o \
		disconnect.o \
		move.o \
		predicates.o

GENHDR	=	

MKFLAGS	=	CC="$(CC)" OPTIM="$(OPTIM)" LDFLAGS="$(LDFLAGS)" \
		AR="$(AR)" RANLIB="$(RANLIB)"

LIB	=	../libgame.a

SCRIPTS	=	../scripts

$(LIB):	$(OBJ)
	$(AR) $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

depend:	$(GENHDR)
	makedepend -- ${CFLAGS} -- $(SRC)

clean:
	-rm -f $(OBJ) $(GENHDR)

# DO NOT DELETE THIS LINE -- make depend depends on it.
