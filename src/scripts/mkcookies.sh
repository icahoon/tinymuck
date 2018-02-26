#!/bin/sh
# Copyright (c) 1992 by David Moore.  All rights reserved.
# mkcookies.sh,v 2.4 1994/02/04 01:22:02 dmoore Exp

# This file simply generates case statements to implement expanding
# magic cookies into their full form.
# Comment char: ;;

if [ $# -ne 2 ]; then
    echo "usage: $0 <cookies-list> <header file>"
    exit 1
fi

exec < $1 > /tmp/.mkcook$$

sed -e 's/[ 	]*;;.*$//' -e '/^[ 	]*$/d' | sort -u | awk '
{
    print "    case "$1" :";
    print "        Bufsprint(&buf, \"%s %s\", \""$2"\", str+1);";
    print "        break;";
}'

mv /tmp/.mkcook$$ $2
touch $2
