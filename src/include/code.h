/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* code.h,v 2.9 1997/08/30 06:43:53 dmoore Exp */
#ifndef MUCK_CODE_H
#define MUCK_CODE_H

#include "params.h"		/* For *_STACK_SIZE. */

struct func_addr;
struct code;

#define INST_STRING		1
#define INST_INTEGER		2
#define INST_OBJECT		3
#define INST_ADDRESS		4
#define INST_CONNECTION		5
#define INST_VARIABLE		6
#define INST_OFFSET		7
#define INST_PRIMITIVE		8
#define INST_IF			9
#define INST_NOT_IF		10
#define INST_EXECUTE		11
#define INST_LINENO		12 /* Compiler embedded line number. */
#define INST_BADVAR		13 /* Used to mark uninitialized vars. */
#define INST_WIZCHECK		14 /* Check that the program has wizperms. */
#define INST_STKCHECK		15 /* Check the stack as given primitive */
#define INST_TYPECHECK		16 /* Check types as given primitive */
#define INST_PICK		17 /* 'n INST_PICK' == 'n+1 PRIM_pick' */
#define INST_EXIT		18 /* this is called a lot */


union inst_un {
    struct shared_string *string;
    long integer;
    dbref object;
    struct func_addr *address;
    int connection;
    int variable;
    long offset;		/* IF/JMP - offset in current function */
    int primitive;
    int lineno;			/* Line number in original source. */
    struct {
        unsigned long min_depth : 16;
        unsigned long min_space : 16;
    } depth;
};

typedef struct inst {
    signed int type : 8;	 /* Type of instruction.  Negs for compiler. */

    /* these two can only be 0 for INST_PRIMITIVE */
    unsigned int stack_safe : 1; /* True when inst can't under/over-flow */
    unsigned int type_safe : 1;	 /* True when all arg types known correct */

    union inst_un un;		 /* Type specific data for each instruction. */
} inst;


#define dup_inst_p(x) \
    ((x)->type == INST_STRING || (x)->type == INST_ADDRESS)

#define dup_inst_interp(x) \
    do { \
        if ((x)->type == INST_STRING) \
            dup_shared_string((x)->un.string); \
        else if ((x)->type == INST_ADDRESS) \
            dup_code((x)->un.address->code); \
    } while (0)

#define clear_inst_interp(x) \
    do { \
        if ((x)->type == INST_STRING) \
            clear_shared_string((x)->un.string); \
        else if ((x)->type == INST_ADDRESS) \
            clear_code((x)->un.address->code); \
    } while (0)

#define clear_inst_code(x) \
    do { \
        if ((x)->type == INST_STRING) \
            clear_shared_string((x)->un.string); \
    } while (0)

#define true_inst(x) \
    ((x)->type == INST_INTEGER ? ((x)->un.integer != 0) : \
     ((x)->type == INST_STRING ? ((x)->un.string != NULL) : \
      ((x)->type == INST_OBJECT ? ((x)->un.object != NOTHING) : \
       1)))

#define DoAnonymous(x)		((x) ? (x) : "-anon-")

typedef struct func_addr {
    struct code *code;		/* Pointer to code struct this func is on. */
    const char *name;		/* The name of this function. */
    int num_inst;		/* Length of instruction array. */
    struct inst *bytecode;
} func_addr;


struct code {
    int refcount;		/* Number of pointers in use to this struct. */
    dbref object;		/* Object this program in is on. */
    int num_funcs;		/* Length of function array. */
    func_addr *main_func;	/* Offset to main entry point. */
    func_addr **funcs;		/* Array of functions for this program. */
    struct code *next;
    struct code **prev;
};

#ifdef DEBUG_INTERP_STACK
#define push_stack(st, what) \
    (full_stack(st, DATA_STACK_SIZE) ? \
     (log_status("INTERP: push to full stack: %s %i", __FILE__, __LINE__), \
      (what)->type = INST_BADVAR, (what)->un.integer = 0) : \
     (void) (*(--(st)->top) = *(what)))
#define pop_stack(st, result) \
    (empty_stack(st, DATA_STACK_SIZE) ? \
     (log_status("INTERP: pop on empty stack: %s %i", __FILE__, __LINE__), \
      (result)->type = INST_BADVAR, (result)->un.integer = 0) : \
     (void) (*(result) = *((st)->top++)))
#else
#define push_stack(st, what)	(*(--(st)->top) = *(what))
#define pop_stack(st, result)	(*(result) = *((st)->top++))
#endif
#define empty_stack(st, size)	((st)->top >= (st)->data + (size))
#define full_stack(st, size)	((st)->top <= (st)->data)
#define depth_stack(st, size)	(((st)->data + (size)) - (st)->top)
#define push_stack_nocheck(st, what)	(*(--(st)->top) = *(what))
#define pop_stack_nocheck(st, result)	(*(result) = *((st)->top++))

typedef struct data_stack {
    inst data[DATA_STACK_SIZE];
    inst *top;
} data_stack;


typedef void (*continue_func)(struct frame *, struct data_stack *, void *);

/* Addresses stored onto the address stack look are usually just a pointer
   to the structure holding the compiled muf function, and an offset into
   that function.  Or, it can be a pointer to a C function, and some data
   to pass to that C function.  When one of these continuation functions
   is hit going back down the address stack, it's called with the data
   it had stored at the time, as well as the _current_ state of the frame.
   This allows a primitive coded in C to go ahead and call more muf code,
   perform various operations and then continue on with those operations
   using results of muf computations.  If a contination function is called
   with a frame of NULL, then it means that the program failed some point
   after that routine got started.  What it needs to do is free up it's
   data and such, and just quietly exit. */
#define JUMP_MUF_FUNC		0
#define JUMP_CONTINUATION	1
#define JUMP_OBJECT		2
#define JUMP_FINISHED		3

#define JumpType(x)		((x)->type)
#define JumpFunc(x)		((x)->un.muf_func.func)
#define JumpCurrInst(x)		((x)->un.muf_func.curr_inst)
#define JumpLineno(x)		((x)->un.muf_func.lineno)
#define JumpContinuation(x)	((x)->un.cont_func.continuation)
#define JumpContData(x)		((x)->un.cont_func.cont_data)
#define JumpObject(x)		((x)->un.object)
typedef struct jump_addr {
    int type;			/* muf function, or continuation */
    union {
	struct {
	    func_addr *func;	/* Function to jump to. */
	    struct inst *curr_inst; /* current instruction in said func */
	    int lineno;		/* Compiler info for debugging purposes. */
	} muf_func;
	struct {
	    /* C function to jump to. */
	    continue_func continuation;
	    void *cont_data;	/* Data for that function. */
	} cont_func;
	dbref object;
    } un;
} jump_addr;


typedef struct addr_stack {
    jump_addr data[ADDR_STACK_SIZE];
    jump_addr *top;
} addr_stack;


typedef struct frame {
    struct frame *next;		/* Pointer to next frame. */

    addr_stack addr_stack;	/* System stack, holds return addresses. */
    data_stack data_stack;	/* User stack, holds user data. */
    inst variables[MAX_VAR];	/* Global user variables. */

    unsigned int writeonly : 1;	/* Program can't call 'read'. */
    unsigned int swap : 1;	/* Pop out of interpreter, keeping frame. */
    unsigned int error : 1;	/* True if an error occured. */
    unsigned int cache_dirty : 1; /* True if we've changed jump_addrs */
    unsigned int max_create : 8; /* Number of objects you can create. */
    unsigned int no_inst_limit : 1; /* Wiz programs can avoid limits. */

    dbref orig_program;         /* Original program which was started. */
    dbref player;		/* Who is running the program. */
    dbref program;		/* dbref of current program running. */

    unsigned long instructions_run;
    unsigned long max_instructions;
} frame;

#define frame_wizzed(fr) \
    (Wizard((fr)->program) && TrueWizard(GetOwner((fr)->program)))

#define frame_euid(fr) \
    (HasFlag((fr)->program, SETUID) ? GetOwner((fr)->program) : (fr)->player)



/* It takes a decent amount of time on many processors to pass all all of
   these common arguments to the primitive functions.  So instead we use
   global variables.  This works fine as long as only one frame can be
   being *run* at a time, which is true even with read, backgrounded
   processes, etc.  The only time this wouldn't be true is if you added
   support for unix level threads to muck, and tried to run programs at
   the same time. */
#ifdef INTERP_PASS_FRAME
#define MUF_PRIM(name) void name(frame *curr_frame, data_stack *curr_data_st)
#else
#define MUF_PRIM(name) void name(void)
extern frame *curr_frame;
extern data_stack *curr_data_st;
#endif

typedef MUF_PRIM((*prim_func));

#endif /* MUCK_CODE_H */


