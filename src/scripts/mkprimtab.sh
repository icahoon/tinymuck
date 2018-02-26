#!/bin/sh
# Copyright (c) 1992 by David Moore.  All rights reserved.
# mkprimtab.sh,v 2.3 1993/04/08 20:24:43 dmoore Exp

# I'd like to say that this shell script was as readable as the source
# code.  However, it's clearly not.  The basic gist is that this script
# converts the very pretty (human readable) primitives file and
# generates two files.  The one file is a table of the types that each
# primitive takes and returns, in the form of a static C array.  The
# other file is a header file providing a unique number for every
# primitive.  The compiler and interpreter use this for ''special'' case
# primitives such as 'IF', etc.

# Comment char: #

if [ $# -ne 3 ]; then
    echo "usage: $0 <primitives-list> <table file> <offset file>"
    exit 1
fi

exec < $1 

sed -e 's/[ 	]*#.*$//' -e '/^[ 	]*$/d' | sort -u +0 -1 | awk '
BEGIN	{ table = "'"$2"'"; offset = "'"$3"'"; counter = 0; }
	{
	    print "#define PRIM_" $2 "		" counter++ >offset;

	    if ($3 == "w") wizonly = 1;
	    else wizonly = 0;

	    if ($3 == "i") prim_func = "NULL";
	    else prim_func = "prim_" $2;

	    totcount = NF - 4;
	    argcount = int($4);
	    rtncount = totcount - argcount;

	    if ((argcount < 0) || (rtncount < 0)) {
		print "Entry: " $1 " has bad argument counts.";
	    }

	    if (argcount == 0) argtypes = "{ INST_T_NONE }";
	    else {
		argtypes = "";
		for (i = 0+5; i < argcount+5; i++) {
		    arg = "";
		    while ($i ~ /^L/) {
			arg = arg "INST_T_LIST, ";
			$i = substr($i, 2, length($i)-1);
		    }
		    if ($i == "*") $i = "ANY";
		    arg = arg "INST_T_" $i;
		    argtypes = arg ", " argtypes;
		}
		argtypes = "{ " argtypes "}";
	    }
	    if (rtncount == 0) rtntypes = "{ INST_T_NONE }";
	    else {
		rtntypes = "";
		for (i = argcount+5; i < totcount+5; i++) {
		    arg = "";
		    while ($i ~ /^L/) {
			arg = arg "INST_T_LIST, ";
			$i = substr($i, 2, length($i)-1);
		    }
		    if ($i == "*") $i = "ANY";
		    arg = arg "INST_T_" $i;
		    rtntypes = arg ", " rtntypes;
		}
		rtntypes = "{ " rtntypes "}";
	    }

	    printf("{ \"%s\",	%s,	%d, %d, %d, %s, %s },\n", \
		   $1, prim_func, wizonly, argcount, \
		    rtncount, argtypes, rtntypes) >table;
}
'

echo '#define PRIM_NONE		-1' >> $3 
echo '#define PRIM_PUSH		-2' >> $3

