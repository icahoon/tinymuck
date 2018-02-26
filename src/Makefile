# You will NEED a compiler which is at least marginally ANSI C.

# The actual source (excluding 'interface/' portion) should be ansi C
# clean, making it easy to port to any machine w/ a decent compiler.
# A standalone version (stdin/stdout interface) works fine on PCs, Macs,
# etc.

# It should also compile pretty well on most unix boxes w/ gcc.  The
# biggest problem that happens w/ gcc is that someone improperly
# installs it.  Usually the problems arise with header files.  If the
# code doesn't compile due to this, you can tweak the code some, or yell
# at your sysadmin to fix the installation.

# Makefile,v 2.27 1997/08/29 21:05:34 dmoore Exp

# Please choose the appropriate compiler and flags for your system:

# IMPORTANT: Read the file PORT to get per os specific information.

# If you don't have ranlib, use touch.
RANLIB	=	ranlib
#RANLIB	=	touch

# GCC
CC	=	gcc
OPTIM	=	-g -ansi -O -Wall
LDFLAGS	=

# Server development
#CC	=	anal-gcc
OPTIM	=	-g -O2 -pg -ansi -Wall
LDFLAGS	=

# Solaris 2.x
LDFLAGS	=	-lsocket -lnsl -lelf -ldl
RANLIB	=	touch

# IBM AIX compiler
#CC	=	cc
#OPTIM	=	-g -qlanglvl=ansi
#LDFLAGS	=	-lbsd

# HPUX - edit ansify.h, follow directions for hpux.
#CC	=	gcc
#OPTIM	=	-g -ansi -O -D_INCLUDE_HPUX_SOURCE -D_INCLUDE_XOPEN_SOURCE -D_INCLUDE_POSIX_SOURCE -D_INCLUDE_AES_SOURCE

# Add v if you want it noisier.
AR	=	ar rc
#AR	=	ar rcv


# You shouldn't need to touch anything below this point.

SHELL= /bin/sh

SCRIPTS= ./scripts

CFLAGS	=	${OPTIM} -I./include -I.

MKFLAGS	=	CC="$(CC)" OPTIM="$(OPTIM)" LDFLAGS="$(LDFLAGS)" \
		AR="$(AR)" RANLIB="$(RANLIB)"

GENHDRS	=	externs.h

LIBS	=	libbuiltins.a libcompile.a libdb.a libeditor.a \
		libgame.a libinterp.a libutils.a

BIN	=	netmuck decompress sanity convert


default:	netmuck sanity

all:	$(BIN)

netmuck:	FORCE include/config.h externs.h $(LIBS)
	cd interface; make $(MKFLAGS) ../netmuck

sanity:		FORCE include/config.h externs.h libdb.a libutils.a
	cd utilprogs; make $(MKFLAGS) ../sanity

convert:	FORCE include/config.h externs.h libdb.a libutils.a
	cd utilprogs; make $(MKFLAGS) ../convert

decompress:	FORCE include/config.h externs.h libutils.a
	cd utilprogs; make $(MKFLAGS) ../decompress

announce:
	cd utilprogs; make $(MKFLAGS) ../announce

testfork:
	cd utilprogs; make $(MKFLAGS) ../testfork

depend:	include/config.h externs.h
	cd builtins; make $(MKFLAGS) depend
	cd compile; make $(MKFLAGS) depend
	cd db; make $(MKFLAGS) depend
	cd editor; make $(MKFLAGS) depend
	cd game; make $(MKFLAGS) depend
	cd interface; make $(MKFLAGS) depend
	cd interp; make $(MKFLAGS) depend
	cd utilprogs; make $(MKFLAGS) depend
	cd utils; make $(MKFLAGS) depend

clean:
	cd builtins; make $(MKFLAGS) clean
	cd compile; make $(MKFLAGS) clean
	cd db; make $(MKFLAGS) clean
	cd editor; make $(MKFLAGS) clean
	cd game; make $(MKFLAGS) clean
	cd interface; make $(MKFLAGS) clean
	cd interp; make $(MKFLAGS) clean
	cd utilprogs; make $(MKFLAGS) clean
	cd utils; make $(MKFLAGS) clean
	-rm -f $(LIBS) $(GENHDRS)

realclean:	clean
	-rm -f TAGS *~ core core.netmuck
	-rm -f stamp.*
	-rm -f */*~ */Makefile.bak
	-rm -f $(BIN)

libbuiltins.a:	FORCE include/config.h externs.h
	cd builtins; make $(MKFLAGS) ../libbuiltins.a

libcompile.a:	FORCE include/config.h externs.h
	cd compile; make $(MKFLAGS) ../libcompile.a

libdb.a:	FORCE include/config.h externs.h
	cd db; make $(MKFLAGS) ../libdb.a

libeditor.a:	FORCE include/config.h externs.h
	cd editor; make $(MKFLAGS) ../libeditor.a

libgame.a:	FORCE include/config.h externs.h
	cd game; make $(MKFLAGS) ../libgame.a

libinterp.a:	FORCE include/config.h externs.h
	cd interp; make $(MKFLAGS) ../libinterp.a

libutils.a:	FORCE include/config.h externs.h
	cd utils; make $(MKFLAGS) ../libutils.a

externs.h:	${SCRIPTS}/mkexterns.sh
	${SCRIPTS}/mkexterns.sh externs.h builtins/*.c compile/*.c db/*.c editor/*.c game/*.c interface/*.c interp/*.c utils/*.c

include/config.h:
	@echo
	@echo Please cp the file include/config.h-dist to include/config.h.
	@echo Then edit this file to reflect your particular system.
	@echo
	@sh -c 'exit 1'

FORCE:

# The stuff below here is mostly for me.  It might give you an idea
# about how to maintain one source tree for many servers you maintain
# which have different configuration options.
temp:	stamp.temp
	make $(MKFLAGS) netmuck sanity
	cp netmuck $(home)/mud/temp/newnetmuck
	cp sanity $(home)/mud/temp/sanity


tt:	stamp.tt
	make $(MKFLAGS) netmuck sanity
	cp netmuck /tmp/tt/newnetmuck
	cp sanity /tmp/tt/sanity

druid:	stamp.druid
	make $(MKFLAGS) netmuck sanity
	cp netmuck /export/home/druid/tmuck/newnetmuck
	cp sanity /export/home/druid/tmuck/sanity

prof:	stamp.druid
	make $(MKFLAGS) netmuck sanity
	cp netmuck /tmp/dm/newnetmuck
	cp sanity /tmp/dm/sanity

debug:	stamp.druid2
	make $(MKFLAGS) netmuck sanity
	cp netmuck /tmp/dm/newnetmuck
	cp sanity /tmp/dm/sanity

stamp.temp: include/config.h-temp
	make realclean
	cp include/config.h-temp include/config.h
	touch stamp.temp

stamp.druid: include/config.h-druid
	make realclean
	cp include/config.h-druid include/config.h
	touch stamp.druid

stamp.druid2: include/config.h-druid2
	make realclean
	cp include/config.h-druid2 include/config.h
	touch stamp.druid2

stamp.tt: include/config.h-tt
	make realclean
	cp include/config.h-tt include/config.h
	touch stamp.tt
