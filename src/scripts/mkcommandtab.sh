#!/bin/sh
# Copyright (c) 1992 by David Moore.  All rights reserved.
# mkcommandtab.sh,v 2.4 1994/02/04 01:22:01 dmoore Exp

# This file generates a table which can be bsearched on mapping
# a command name to a function pointer.
# Comment char: ;


if [ $# -ne 2 ]; then
    echo "usage: $0 <commands-list> <table header file>"
    exit 1
fi

exec < $1 > /tmp/.mkcomm$$

sed -e 's/[ 	]*;.*$//' -e '/^[ 	]*$/d' | sort -u +0 -1 | awk '
BEGIN	{ name[0] = ""; len[0] = 0; fun[0] = ""; cnt = 0; }
	{
	    name[cnt] = $1; fun[cnt] = $2;
	    if (NF > 2) len[cnt] = length($1); else len[cnt] = 0;
	    cnt++;
	}
END	{
	    name[-1] = "";
	    name[cnt] = "";
	    for (j = 0; j < cnt; j++) {
	        if (!len[j]) {
	            curr = name[j];
	            prev = name[j-1];
	            follow = name[j+1];
	            currlen = length(curr);
	            prevlen = length(prev);
	            followlen = length(follow);
	            i = 0;
	            while ((i < currlen) && (i < prevlen) && (substr(curr, i+1, 1) == substr(prev, i+1, 1))) i++;
	            while ((i < currlen) && (i < followlen) && (substr(curr, i+1, 1) == substr(follow, i+1, 1))) i++;
                    i++;
                    if (i > currlen) len[j] = currlen;
	            else len[j] = i;
	        }
	    }
	    for (j = 0; j < cnt; j++) {
	        printf("    { \"%s\", %d, %s },\n", name[j], len[j], fun[j]);
	    }
	}
'

mv /tmp/.mkcomm$$ $2
touch $2
