# This makefile inherits many flags from ../Makefile, don't directly
# run make in this directory.
# Makefile,v 2.5 1994/02/04 00:18:01 dmoore Exp

# interface

CFLAGS	=	$(OPTIM) -I../include -I. -I..

SRC	=	clilib.c \
		hostname.c \
		interface.c \
		lockout.c \
		main.c
OBJ	=	clilib.o \
		hostname.o \
		interface.o \
		lockout.o \
		main.o

GENHDR	=	

LIBS	=	../libbuiltins.a ../libcompile.a ../libdb.a ../libeditor.a \
		../libgame.a ../libinterp.a ../libutils.a

LDLIBS	=	-lbuiltins -lcompile -ldb -leditor -lgame -linterp -lutils

MKFLAGS	=	CC="$(CC)" OPTIM="$(OPTIM)" LDFLAGS="$(LDFLAGS)" \
		AR="$(AR)" RANLIB="$(RANLIB)"

BIN	=	../netmuck

SCRIPTS	=	../scripts

$(BIN):	$(OBJ) $(LIBS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ) $(LDFLAGS) -L../ $(LDLIBS)


depend:	$(GENHDR)
	makedepend -- $(CFLAGS) -- $(SRC)

clean:
	-rm -f $(OBJ)

realclean:
	-rm -f $(OBJ) $(GENHDR)

# DO NOT DELETE THIS LINE -- make depend depends on it.
