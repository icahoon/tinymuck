/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* typechk.h,v 2.4 1997/08/29 21:00:22 dmoore Exp */
#ifndef MUCK_TYPECHK_H
#define MUCK_TYPECHK_H

#define permissions(fr, object) \
    (frame_wizzed(fr) || (GetOwner(object) == frame_euid(fr)))

/* Unique id's for each of the various types allowed for muf primitives
   listed in the primitives file. */

#define INST_T_NONE	-1
#define INST_T_ANY	0
#define INST_T_a	1
#define INST_T_c	2
#define INST_T_n	3
#define INST_T_N	4
#define INST_T_nt	5
#define INST_T_o	6
#define INST_T_O	7
#define INST_T_Oh	8
#define INST_T_P	9
#define INST_T_s	10
#define INST_T_S	11
#define INST_T_t	12
#define INST_T_v	13
#define INST_T_LIST	14

#endif /* MUCK_TYPECHK_H */
