# This makefile inherits many flags from ../Makefile, don't directly
# run make in this directory.
# Makefile,v 2.7 1994/02/04 00:18:04 dmoore Exp

# interp

CFLAGS	=	$(OPTIM) -I../include -I. -I..

SRC	=	code.c \
		con_prims.c \
		db_prims.c \
		disassem.c \
		flow_prims.c \
		frames.c \
		inst.c \
		interp.c \
		math_prims.c \
		stack_prims.c \
		string_prims.c \
		typechk.c

OBJ	=	code.o \
		con_prims.o \
		db_prims.o \
		disassem.o \
		flow_prims.o \
		frames.o \
		inst.o \
		interp.o \
		math_prims.o \
		stack_prims.o \
		string_prims.o \
		typechk.o

GENHDR	=	prim_offsets.h primtab.h

MKFLAGS	=	CC="$(CC)" OPTIM="$(OPTIM)" LDFLAGS="$(LDFLAGS)" \
		AR="$(AR)" RANLIB="$(RANLIB)"

LIB	=	../libinterp.a

SCRIPTS	=	../scripts

$(LIB):	$(OBJ)
	$(AR) $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

depend:	$(GENHDR)
	makedepend -- ${CFLAGS} -- $(SRC)

clean:
	-rm -f $(OBJ) $(GENHDR)

prim_offsets.h:	primtab.h
	touch prim_offsets.h

primtab.h:	$(SCRIPTS)/mkprimtab.sh primitives
	$(SCRIPTS)/mkprimtab.sh primitives primtab.h prim_offsets.h

# Some defaults in case there is no makedepend.
con_prims.o:    con_prims.c     prim_offsets.h
db_prims.o:     db_prims.c      prim_offsets.h
flow_prims.o:   flow_prims.c    prim_offsets.h
inst.o:         inst.c          prim_offsets.h primtab.h ../include/typechk.h
interp.o:       interp.c        prim_offsets.h
math_prims.o:   math_prims.c    prim_offsets.h
stack_prims.o:  stack_prims.c   prim_offsets.h
string_prims.o: string_prims.c  prim_offsets.h
typechk.o:      typechk.c       ../include/typechk.h

# DO NOT DELETE THIS LINE -- make depend depends on it.
