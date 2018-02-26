#!/bin/sh
# Copyright (c) 1992 by David Moore.  All rights reserved.
# mkcommands.sh,v 2.4 1994/02/04 01:22:00 dmoore Exp

# This script generates a header file giving prototypes for a list of
# commands.
# Comment char: ;

if [ $# -ne 4 ]; then
    echo "usage: $0 <builtins-list> <externs header file> <extern|static> <function form...>"
    exit 1
fi

exec < $1 > /tmp/.mkcomm$$

echo "#define COMMAND_FUNC(func) $3 void func$4"

sed -e 's/[ 	]*;.*$//' -e '/^[ 	]*$/d' | sort -u +0 -1 | \
    awk '{ print "COMMAND_FUNC(" $2 ");" }'

echo "#undef COMMAND_FUNC"

mv /tmp/.mkcomm$$ $2
touch $2

