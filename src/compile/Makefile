# This makefile inherits many flags from ../Makefile, don't directly
# run make in this directory.
# Makefile,v 2.6 1997/08/29 20:58:50 dmoore Exp

# compile

CFLAGS	=	$(OPTIM) -I../include -I../interp -I. -I..

SRC	=	compile.c \
		macro.c \
		optimize.c

OBJ	=	compile.o \
		macro.o \
		optimize.o

GENHDR	=	../interp/prim_offsets.h

MKFLAGS	=	CC="$(CC)" OPTIM="$(OPTIM)" LDFLAGS="$(LDFLAGS)" \
		AR="$(AR)" RANLIB="$(RANLIB)"

LIB	=	../libcompile.a

SCRIPTS	=	../scripts

$(LIB):	$(OBJ)
	$(AR) $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

depend:	$(GENHDR)
	makedepend -- ${CFLAGS} -- $(SRC)

clean:
	-rm -f $(OBJ) #$(GENHDR)

../interp/prim_offsets.h:
	cd ../interp; make $(MKFLAGS) prim_offsets.h

# Some default dependancies in case they don't have makedepend.

compile.o:	compile.c $(GENHDR)
optimize.o:	optimize.c $(GENHDR)

# DO NOT DELETE THIS LINE -- make depend depends on it.
