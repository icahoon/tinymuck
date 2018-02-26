# This makefile inherits many flags from ../Makefile, don't directly
# run make in this directory.
# Makefile,v 2.5 1994/02/04 00:18:11 dmoore Exp

# utils

CFLAGS	=	$(OPTIM) -I../include -I. -I..

SRC	=	buffer.c \
		compress.c \
		hashtab.c \
		help.c \
		intern.c \
		log.c \
		muck_strftime.c \
		muck_time.c \
		player.c \
		stringutil.c \
		utils.c

OBJ	=	buffer.o \
		compress.o \
		hashtab.o \
		help.o \
		intern.o \
		log.o \
		muck_strftime.o \
		muck_time.o \
		player.o \
		stringutil.o \
		utils.o

GENHDR	=	

MKFLAGS	=	CC="$(CC)" OPTIM="$(OPTIM)" LDFLAGS="$(LDFLAGS)" \
		AR="$(AR)" RANLIB="$(RANLIB)"

LIB	=	../libutils.a

SCRIPTS	=	../scripts

$(LIB):	$(OBJ)
	$(AR) $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

depend:	$(GENHDR)
	makedepend -- ${CFLAGS} -- $(SRC)

clean:
	-rm -f $(OBJ) $(GENHDR)

# DO NOT DELETE THIS LINE -- make depend depends on it.
