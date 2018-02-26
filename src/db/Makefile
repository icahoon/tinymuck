# This makefile inherits many flags from ../Makefile, don't directly
# run make in this directory.
# Makefile,v 2.9 1996/08/21 00:30:31 dmoore Exp

# db

CFLAGS	=	$(OPTIM) -I../include -I. -I..

SRC	=	boolexp.c \
		db.c \
		debug_malloc.c \
		dump_db.c \
		flags.c \
		match.c \
		muck_malloc.c \
		property.c \
		sane.c

OBJ	=	boolexp.o \
		db.o \
		debug_malloc.o \
		dump_db.o  \
		flags.o \
		match.o \
		muck_malloc.o \
		property.o \
		sane.o

GENHDR	=	

MKFLAGS	=	CC="$(CC)" OPTIM="$(OPTIM)" LDFLAGS="$(LDFLAGS)" \
		AR="$(AR)" RANLIB="$(RANLIB)"

LIB	=	../libdb.a

SCRIPTS	=	../scripts

$(LIB):	$(OBJ)
	$(AR) $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

depend:	$(GENHDR)
	makedepend -- ${CFLAGS} -- $(SRC)

clean:
	-rm -f $(OBJ) $(GENHDR)

# DO NOT DELETE THIS LINE -- make depend depends on it.
