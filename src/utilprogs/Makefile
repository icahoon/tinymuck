# This makefile inherits many flags from ../Makefile, don't directly
# run make in this directory.
# Makefile,v 2.7 1994/02/04 00:18:08 dmoore Exp

# utilprogs

CFLAGS	=	$(OPTIM) -I../include -I. -I..

SRC	=	announce.c \
		convert.c \
		decompress.c \
		sanity.c \
		stubs.c \
		testfork.c

OBJ	=	announce.o \
		convert.o \
		decompress.o \
		sanity.o \
		stubs.o \
		testfork.o

GENHDR	=	

MKFLAGS	=	CC="$(CC)" OPTIM="$(OPTIM)" LDFLAGS="$(LDFLAGS)" \
		AR="$(AR)" RANLIB="$(RANLIB)"

SCRIPTS	=	../scripts

../sanity:	stubs.o sanity.o ../libdb.a ../libutils.a
	$(CC) $(CFLAGS) -o ../sanity stubs.o sanity.o $(LDFLAGS) -L../ -ldb -lutils

../convert:	stubs.o convert.o ../libdb.a ../libutils.a
	$(CC) $(CFLAGS) -o ../convert stubs.o convert.o $(LDFLAGS) -L../ -ldb -lutils

../decompress:	decompress.o ../libutils.a
	$(CC) $(CFLAGS) -o ../decompress decompress.o $(LDFLAGS) -L../ -lutils

../announce:	announce.o
	$(CC) $(CFLAGS) -o ../announce announce.o $(LDFLAGS)

../testfork: testfork.o
	$(CC) $(CLFGAS) -o ../testfork testfork.o $(LDFLAGS)

depend:	$(GENHDR)
	makedepend -- $(CFLAGS) -- $(SRC)

clean:
	-rm -f $(OBJ)

realclean:
	-rm -f $(OBJ) $(GENHDR)

# DO NOT DELETE THIS LINE -- make depend depends on it.
