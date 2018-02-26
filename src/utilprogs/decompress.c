#include "copyright.h"
/* decompress.c,v 2.4 1993/12/10 04:35:53 dmoore Exp */

#include "config.h"

#include <stdio.h>

#include "externs.h"

void main()
{
    char buf[16384];

    while(gets(buf)) {
	puts(uncompress(buf));
    }
    exit(0);
}
