#!/bin/sh
# Copyright (c) 1992 by David Moore.  All rights reserved.
# mkexterns.sh,v 2.8 1997/08/29 21:04:58 dmoore Exp

# This script is designed to generate a header of prototypes for all
# of the given C source files.  Using it can help make sure that
# everyone is being honest about function definitions.

if [ $# -lt 2 ]; then
    echo "usage: $0 <externs header file> <file1> ... <fileN>"
    exit 1
fi

output=$1
shift

exec > /tmp/.mkexterns$$

echo '#ifndef MUCK_EXTERNS_H'
echo '#define MUCK_EXTERNS_H'
echo '#ifdef USE_EXTERNS'
echo '#include "config.h"'
echo '#include "db.h"'
echo '#include <stdio.h>'
echo '#include <time.h>'
echo '#include "ansify.h"'
echo '#include "buffer.h"'
#echo '#include "builtins.h"'
echo '#include "match.h"'
echo '#include "hostname.h"'
echo '#include "hashtab.h"'
echo '#include "code.h"'
echo '#include "muf_con.h"'
echo '#define MATH_PRIM(a, b) MUF_PRIM(prim_##a)'
echo '#define MATH0_PRIM(a, b) MUF_PRIM(prim_##a)'
echo '#define GET_STR_PRIM(a, b) MUF_PRIM(prim_##a)'
echo '#define SET_STR_PRIM(a, b) MUF_PRIM(prim_set##a)'
echo '#define GET_DBREF_PRIM(a, b) MUF_PRIM(prim_##a)'
echo '#define GET_INT_PRIM(a, b) MUF_PRIM(prim_##a)'
echo '#define TRY_INTERN_PROPS(a) MUF_PRIM(prim_##a##_i)'

for f in $*; do
    if [ "$f" != "utils/compress.c" -a "$f" != "utils/buffer.c" -a "$f" != "utils/muck_time.c" ]; then
        echo
        echo "/* From: $f */"

        sed -e '/^[ #\/"	]/d' -e '/^[^(]*$/d' -e '/;/d' \
            -e '/^char \*alloc_string(const char \*string)/d' \
            -e '/^static /d' -e '/^extern /d' -e '/^typedef /d' \
	    -e '/^int main(/d' \
            $f | \
        awk '{ print "extern " $0 ";" }'
    fi
done

echo '#undef MATH_PRIM'
echo '#undef MATH0_PRIM'
echo '#undef GET_STR_PRIM'
echo '#undef SET_STR_PRIM'
echo '#undef GET_DBREF_PRIM'
echo '#undef GET_INT_PRIM'
echo '#undef TRY_INTERN_PROPS'

echo '#endif /* USE_EXTERNS */'
echo '#endif /* MUCK_EXTERNS_H */'


# now copy it to the right place
mv /tmp/.mkexterns$$ $output
touch $output
