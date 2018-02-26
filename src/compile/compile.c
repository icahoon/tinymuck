/* Copyright (c) 1992 by Ben Jackson.  All rights reserved. */
/* compile.c,v 2.17 1997/08/30 07:09:30 dmoore Exp */
#include "config.h"

#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "db.h"
#include "code.h"
#include "prim_offsets.h"
#include "text.h"
#include "buffer.h"
#include "hashtab.h"
#include "externs.h"

#define TOK_END       -1
#define TOK_UNKNOWN   -2
#define TOK_ANON      -3   /* ': foo ; */
#define TOK_LINENUMBER -4

struct symbol_entry {
    const char *name;
    struct inst data;
};


/*  this is the struct that everything passes around.
 *  Besides making the compiler easier to write, it makes it fully
 *  re-entrant.  If you wanted (and no, I don't know why you'd want
 *  to), you could suspend a compile at any point and resume it later.
 *  (presuming you return to the same place in the code)
 */
typedef struct compile_state {
    const char *str;         /* pointer to pos in current line */
    struct text *text;       /* pointer to our text block */
    const char *ours;        /* pointer to base of str if we own it. */

    int remain;              /* remaining in of current line */
    int where;               /* current line number */
    int apparentline;        /* line that we use to error */
    unsigned int needline : 1;	/* need to emit a line number? */

    dbref who;               /* the compiling person */
    dbref which;             /* the program being compiled */

    unsigned short errors;   /* we must needs count them goofs */
    unsigned short macrosubs;/* how many macro expansions this line? */
    unsigned short anondepth;/* how deep are we in anon lambdas? */

    struct code *code;       /* our result */

    union inst_un temp;      /* for the tokenizer to generate values */
    Buffer buf;              /* for next_word, also used to name 'prim, '.macro */

    unsigned short nextvar;  /* next available variable num */
    hash_tab *symbol_table;  /* funcs and vars */
} compile_state;

/*  notice we return a TAB at the end of a line, since `end of line'
 *  counts as whitespace
 */

/* this is a function now.  it only gets called once a line, not a big hit
#define next_line(x) (clear_our_str(s),\
    (x)->str = text_line((x)->text, ++(x)->where, &(x)->remain))
*/
/*
#define next_char(x) FORCE_RHS((--(x)->remain)? (*(++(x)->str)) :\
    (next_line(x)?  '\t' : '\0'))
*/

/* FIX FIX FIX: this text_line in next_char should probably just
   compare where to text_total_lines to see if it's at the end of
   the input.  Would save much overhead, since text_line is expensive
   and it gets called immediately again in next_line. */
/* FIX FIX FIX: also does it really hurt to return '\t' followed by
   a '\0'?  That'd save a lot of these. */
#define next_char(x) FORCE_RHS((--(x)->remain > 0)? (*(++(x)->str)) :\
    ((x)->remain < 0) ? next_line(x) :\
    ((x)->str++, text_line((x)->text, (x)->where + 1, NULL) ? '\t' : '\0'))
#define look_char(x) FORCE_RHS(((x)->remain > 1) ? (*((x)->str + 1)) :\
    text_line((x)->text, (x)->where + 1, NULL) ? '\t' : '\0')
#define curr_char(x) FORCE_RHS(((x)->remain) ? (*((x)->str)) :\
    text_line((x)->text, (x)->where + 1, NULL) ? '\t' : '\0')

/*  this stack is used to resolve if/else/then and repeat/until.  */

#define NEST_IF        1
#define NEST_ELSE      2
#define NEST_REPEAT    3

struct nest {
    struct nest *next;
    unsigned int type : 2;       /* what flavor is it? */
    unsigned int offset : 30;    /* where is it in the bytecode? */
};

/*  prototypes  */

void init_compiler(void);
static char next_line(compile_state *state);
static const char *next_word(compile_state *state);
static void skip_white(compile_state *state);
static void make_symbol(compile_state *state, const char *name, const int type, union inst_un un);
static struct symbol_entry *lookup_symbol(compile_state *state, const char *name);
static void delete_symbol_entry(void *sym);
struct func_addr *make_func_addr(struct code *code, int num_inst, const char *name);
void clear_func_addr(struct func_addr *victim);
static compile_state *make_compile_state(const dbref player, const dbref prog, struct text *text);
static void clear_compile_state(compile_state *state);
static void compile_error(compile_state *state, const char *fmt, ...);
static void compile_warning(compile_state *state, const char *fmt, ...);
static void push_expansion(compile_state *state, const char *exp);
static int expand_if_macro(compile_state *state);
static int next_token(compile_state *state);
static struct func_addr *compile_func(compile_state *state, const char *faname);

struct func_addr *compile_string(const dbref player, const char *string);
void compile_text(const dbref player, const dbref prog, struct text *text);


/*  next_line:  takes care of steping to the next line.
 */
static char next_line(compile_state *state)
{
    if(state->ours)     /* remove the expansion buffer */
        FREE((char *) state->ours);
    state->macrosubs = 0;  /* no expansions done yet on this new line */
    state->needline = 1;   /* emit a line number into the bytecode */
    do {
        state->str = text_line(state->text, ++state->where, &state->remain);
    } while(state->str && !*state->str);
    if(!state->str)
        return '\0';
    else
	return *state->str;
}

/* next_word: gets you the next whitespace-terminated string in the input.
 * expects the word to start right off, so call skip_white first.
 */
static const char *next_word(compile_state *state)
{
    Buffer *buf = &state->buf;
    char c;

    Bufcpy(buf, "");

    c = curr_char(state);
    while(c && !isspace(c)) {
        Bufcat_char(buf, c);
        c = next_char(state);
    }

    return Buftext(buf);
}

/* skip_white: avoids whitespace and comments.
 */
static void skip_white(compile_state *state)
{
    char c;

    c = curr_char(state);
    while(isspace(c) || c == BEGINCOMMENT) {
        while(isspace(c))
	    c = next_char(state);
        if(c == BEGINCOMMENT) {
            while(c && c != ENDCOMMENT)
                c = next_char(state);
            if(c == ENDCOMMENT)
	        c = next_char(state);
	}
    }
}


/*  init_compiler:  should be called by init_game.
 *  have compiler_inited?  call it automatically if it's not set?
 */
void init_compiler(void)
{
    /* should call fn to init primitive hash table */
}

/* make_symbol:  checks to make sure it's legit and then adds it
 */
static void make_symbol(compile_state *state, const char *name, const int type, union inst_un un)
{
    struct symbol_entry *result;

    MALLOC(result, struct symbol_entry, 1);
    result->name = ALLOC_STRING(name);
    result->data.type = type;
    result->data.un = un;
    add_hash_entry(state->symbol_table, result);
}

/* lookup_symbol:  find it in the table
 */
static struct symbol_entry *lookup_symbol(compile_state *state, const char *name)
{
    struct symbol_entry sym;

    sym.name = name;
    return (struct symbol_entry *) find_hash_entry(state->symbol_table, &sym);
}

/* delete_symbol_entry:  hash internal type fn to delete a symbol
 */
static void delete_symbol_entry(void *entry)
{
    struct symbol_entry *sym = entry;
    FREE_STRING(sym->name);
    FREE(sym);
}

/* make_func_addr:  the PC way to make a new func_addr
 */
struct func_addr *make_func_addr(struct code *code, int num_inst, const char *name)
{
    struct func_addr *result;

    MALLOC(result, struct func_addr, 1);
    result->code = code;
    result->num_inst = num_inst;
    result->name = name;
    if(num_inst) {
        MALLOC(result->bytecode, struct inst, num_inst);
    } else {
	result->bytecode = NULL;
    }
    return result;
}

/*  clear_func_addr:  the PC way to get rid of one
 */
void clear_func_addr(struct func_addr *victim)
{
    int x;

    if(!victim)
        return;

    for(x = 0; x < victim->num_inst; ++x) {
	clear_inst_code(&(victim->bytecode[x]));
    }

    FREE(victim->bytecode);

    if(victim->name)
        FREE_STRING(victim->name);
    FREE(victim);
}


/* make_compile_state:  the PC way to allocate a new one
 */
static compile_state *make_compile_state(const dbref player, const dbref prog, struct text *text)
{
    compile_state *result;
    static const char *reserved_vars[] = { "ME", "LOC", "TRIGGER" };
    int x;
    union inst_un temp_un;

    MALLOC(result, compile_state, 1);

    /*  set up the text block and initialize the first line  */
    result->text = text;
    result->str = text_line(text, 1, &result->remain);

    result->where = 1;
    result->apparentline = 1;
    result->needline = 1;

    /*  which program we are and who is compiling us  */
    result->who = player;
    result->which = prog;

    /*  we haven't done an expand yet  */
    result->ours = NULL;
    result->macrosubs = 0;
    result->anondepth = 0;

    result->errors = 0;

    /*  set up our destination code block  */
    result->code = make_code(prog, 0);

    /*  init our symbol table  */
    result->symbol_table = init_hash_table("Temporary Symbol Table",
					   compare_generic_hash,
					   make_key_generic_hash,
					   delete_symbol_entry,
					   SYMTAB_HASH_SIZE);
    /*  copy in default vars me, loc, trigger  */
    for(x = 0; x < (sizeof(reserved_vars) / sizeof(char *)); x++) {
	temp_un.variable = x;
        make_symbol(result, reserved_vars[x], INST_VARIABLE, temp_un);
    }
    result->nextvar = x;

    return result;
}

/* clear_compile_state:  the PC way to get rid of one when you're done
 */
static void clear_compile_state(compile_state *state)
{
    clear_hash_table(state->symbol_table);
    clear_code(state->code);   /* removes our refcount to it */
    if(state->ours)
	FREE((char *) state->ours);
    FREE(state);
}




/*  compile_error:  this should be called to report all compiler errors
 */
static void compile_error(compile_state *state, const char *fmt, ...)
{
    Buffer buf;
    va_list ap;

    state->errors++;

    if(state->who == NOTHING)
        return;

    va_start(ap, fmt);
    Bufvsprint(&buf, fmt, ap);
    va_end(ap);

    if(state->text)
        notify(state->who, "Error in line %d: %S.", state->apparentline, &buf);
    else
        notify(state->who, "Error: %S.", &buf);
}

/*  compile_warning:  this should be called to report all compiler warnings
 */
static void compile_warning(compile_state *state, const char *fmt, ...)
{
    Buffer buf;
    va_list ap;

    if(state->who == NOTHING)
        return;

    va_start(ap, fmt);
    Bufvsprint(&buf, fmt, ap);
    va_end(ap);

    if(state->text)
        notify(state->who, "Warning line %d: %S.", state->apparentline, &buf);
    else
        notify(state->who, "Warning: %S.", &buf);
}




/* push_expansion:  pushes some expansion into the current line.
 * we expand the current line only when necessary.
 * When there is extra data on the line, state->ours points to
 * the string we allocated to hold all this new string.  state->str
 * then points into this new string, so that next_char can access
 * it the same way.
 *
 * next_line chops this off once we're done with it
 */
#define PUSH_BUF_SIZE 2048  /* the smallest push we'll create */
static void push_expansion(compile_state *state, const char *exp)
{
    int len = strlen(exp);

    /* Special case remain of -1 should be treated like 0 here. */
    if (state->remain == -1)
	state->remain = 0;

    if(state->ours) {
        if(state->str - state->ours >= len) {
	    /* if there's room at the front */
	    /* str is a pointer into the same data that ours points to
	       the memcpy below is used rather than strcpy because we
	       don't want a trailing nul in the stream. */
            state->str -= len;
            state->remain += len;
            memcpy((char *) state->str, exp, len);
        } else {                            /* replace old ->ours with a longer one */
            char *ptr, *temp;

            state->remain += len;
            MALLOC(ptr, char, state->remain + PUSH_BUF_SIZE);
            temp = ptr + PUSH_BUF_SIZE - 1;
            strcpy(temp, exp);
            strcpy(temp + len, state->str);
            FREE((char *) state->ours);
            state->ours = ptr;
            state->str = temp;
        }
    } else {                                   /* first expansion */
        char *ptr, *temp;

        state->remain += len;
        MALLOC(ptr, char, state->remain + PUSH_BUF_SIZE);
        temp = ptr + PUSH_BUF_SIZE - 1;
        strcpy(temp, exp);
        strcpy(temp + len, state->str);
        state->ours = ptr;
        state->str = temp;
    }
}

/* expand_if_macro:  looks at the currect compile position and expands
 * a macro if it's the first thing there.
 */
static int expand_if_macro(compile_state *state)
{
    const char *name, *exp;
    int flag = 0;

    while(curr_char(state) == BEGINMACRO) {
        next_char(state);		/* Skip '.' */
        name = next_word(state);
        exp = macro_expansion(name);

        if(!exp) {
            compile_error(state, "Undefined macro %s", name);
            return -1;
        } else {
            if(++state->macrosubs > MAX_MACRO_SUBS) {
                compile_error(state, "%s", "Macro substitution limit exceeded");
                return -1;
            }
            push_expansion(state, exp);
            flag = 1;
        }
    }
    return flag;
}

/*  next_token:  returns the next token from the source.
 *
 *  ...return normal prims
 *  ...turn '.macro into ': .macro ;
 */
static int next_token(compile_state *state)
{
    char c;
    const char *word;

    skip_white(state);

    if(state->needline) {	/* lets get this line number thing right */
        state->needline = 0;
        state->apparentline = state->where;
        return TOK_LINENUMBER;
    }

    switch(expand_if_macro(state)) {                            /* .<macro> */
        case 1:
            return next_token(state);
        case -1:
            return TOK_UNKNOWN;
        /* case 0: fall through */
    }

    c = curr_char(state);  /* look because skip_white already `saw' it */

    if(!c)                                        /* end of source */
        return TOK_END;

    if(isdigit(c) || ((c == '-' || c == '+')
		      && isdigit(look_char(state)))) { /* [-|+]<0-9...> */
        int num;
        char *end;

        word = next_word(state);
        num = strtol(word, &end, 0);
        if(*end) {
            /*  there were trailing nondigit chars, and the old compiler
             *  treats that like a  word.  so we emulate the stupid
             *  piece of, er, ah...
             */
            push_expansion(state, word); /* stick it back into the text */
            /* notice we fall through here */
        } else {
            state->temp.integer = num;
            return INST_INTEGER;
        }
    }

    if(c == BEGINSTRING) {                                      /* "<string>" */
        Buffer buf;
        char c;

        Bufcpy(&buf, "");

        c = next_char(state);
        while(c != ENDSTRING) {
            if(!c || c == '\t') {   /* tab indicates end of line whitespace */
                compile_error(state, "Unterminated string");
                return TOK_UNKNOWN;
            }
            if(c == '\\') {
                c = next_char(state);  /* take next char as literal */
		if(!c || c == '\t') { /* tab is end of line marker */
		    compile_error(state, "Nothing follows \\");
		    return TOK_UNKNOWN;
		}
	    }
            Bufcat_char(&buf, c);
            c = next_char(state);
        }
	c = next_char(state);  /* skip trailing " */
        state->temp.string = make_shared_string(Buftext(&buf), Buflen(&buf));
        return INST_STRING;
    }

    if(c == '#') {                                              /* #<dbref> */
	/* FIX FIX FIX: does any of this number skipping stuff work? 
	 What is it doing???? */
        dbref obj;
        const char *temp;

        temp = (word = next_word(state)) + 1;

        /*  check to make sure it's a number under the old system */
        while(isspace(*temp)) temp++;
        if(*temp == '-') temp++;
        while(isdigit(*temp)) temp++;

        if(*temp) {
            /*  there were trailing nondigit chars, and the old compiler
             *  treats that like a  word.  so we emulate the stupid
             *  piece of, er, ah...
             */
            push_expansion(state, word); /* stick it back into the text */
            /* notice we fall through here */
        } else {
            obj = parse_dbref(word + 1);
            /*
            if(!Valid(obj)) {
                compile_error(state, "Invalid object #%d", obj);
                return TOK_UNKNOWN;
            }
            */
            state->temp.object = obj;
            return INST_OBJECT;
        }
    }

    if(c == '\'') {						/* '<something> */
        int prim;
        struct symbol_entry *sym;

        next_char(state);					/* skip '\'' */
        word = next_word(state);
        if(!*word) {
            compile_error(state, "%s", "Missing identifier after '");
            return TOK_UNKNOWN;
        }

        sym = lookup_symbol(state, word);
        if(sym) {						/** '<symbol> */
            if(sym->data.type == INST_VARIABLE) {
                compile_error(state, "%s", "Illegal to take address of variable");
                return TOK_UNKNOWN;
            } else {
                /* should always be INST_ADDRESS */
                state->temp = sym->data.un;
                return sym->data.type;
            }
        }

        if(*word == BEGINMACRO) {				/** '.<macro> */
            const char *exp;

            exp = macro_expansion(word + 1);
            if(exp) {
                push_expansion(state, " ;");  /* nifty, huh? */
                push_expansion(state, exp);
		/* name is in the buffer */
                return TOK_ANON;
            } else {
                compile_error(state, "Undefined macro %s", word);
                return TOK_UNKNOWN;
            }
        }

        prim = lookup_primitive(word);

        if(prim == PRIM_word_begin) {                           /** ': <body> ; */
	    Bufcpy(&state->buf, "");  /* no name */
            return TOK_ANON;
        }

        switch(prim) {
            case PRIM_if:
            case PRIM_else:
            case PRIM_then:
            case PRIM_var_decl:
            case PRIM_word_end:
#ifdef MUF_REPEAT_UNTIL
            case PRIM_repeat:
            case PRIM_until:
#endif /* MUF_REPEAT_UNTIL */
                compile_error(state, "Illegal quoted primitive %s", word);
                return TOK_UNKNOWN;
        }
        if(prim >= 0) {                                         /** '<prim>   */
	    Buffer temp;

            push_expansion(state, " ;");
            push_expansion(state, word);
	    Bufcpy(&temp, word);  /* because `word' is really Buftext(&state->buf) */
	    Bufsprint(&state->buf, "prim %S", &temp);
            return TOK_ANON;
        }
        compile_error(state, "Unable to take address of %s", word);
        return TOK_UNKNOWN;
    }

    word = next_word(state);
    if(*word) {
        struct symbol_entry *sym;
        int prim;

        sym = lookup_symbol(state, word);
        if(sym) {                                               /* <symbol> */
            state->temp = sym->data.un;
	    if(sym->data.type == INST_ADDRESS)
	        return INST_EXECUTE;
            return sym->data.type;  /* a variable */
        }

        prim = lookup_primitive(word);
        if(prim >= 0) {                                         /* <prim>  */
	    state->temp.primitive = prim;
	    return INST_PRIMITIVE;
        }
    }
    compile_error(state, "Unrecognized word %s", word);
    return TOK_UNKNOWN;
}



/*  compile_func:  this compiles a func (yep, a word).
 *
 *  ...emit all normal prims and constants
 *  ...resolve if/else/then and repeat/until
 */

#define EMIT(FUNC_ADDR, TYPE, DATA) do {\
        int _offset = (FUNC_ADDR)->num_inst++; \
	(FUNC_ADDR)->bytecode[_offset].type = (TYPE); \
	(FUNC_ADDR)->bytecode[_offset].un = (DATA); \
    } while(0)

/* reemit and last_emit_line are used to make two adjacent linenumber
   commands store as only 1 bytecode. */
#define REEMIT(FUNC_ADDR, TYPE, DATA) do {\
        int _offset = (FUNC_ADDR)->num_inst - 1; \
	clear_inst_code(&((FUNC_ADDR)->bytecode[_offset])); \
	(FUNC_ADDR)->bytecode[_offset].type = (TYPE); \
	(FUNC_ADDR)->bytecode[_offset].un = (DATA); \
    } while(0)
#define LAST_EMIT_LINE(fa) \
    ((fa)->num_inst && (fa)->bytecode[(fa)->num_inst - 1].type == INST_LINENO)

static struct func_addr *compile_func(compile_state *state, const char *faname)
{
    struct nest *if_stack = NULL, *temp;  /* where we keep track of if/then */
    int if_depth = 0;
    struct func_addr *result, *fa;
    int token, bcodesize = 2048;
    union inst_un temp_un;

    /* set up our destination func_addr */
    result = make_func_addr(state->code, bcodesize, NULL);
    result->num_inst = 0; /* this is our current position */

    if(faname && *faname) {
	/* add this fn in as a symbol */
	temp_un.address = result;
        make_symbol(state, faname, INST_ADDRESS, temp_un);
	faname = ALLOC_STRING(faname);  /* turn the static into a copy */
        result->name = faname;
    }

    state->needline = 1;  /* force a line number emit */

    for(;;) {
        if(result->num_inst >= bcodesize) {  /* stretch the result buffer */
            bcodesize += 2048;
	    REALLOC(result->bytecode, struct inst, bcodesize);
        }

        token = next_token(state);
        switch(token) {
            case TOK_UNKNOWN:
                goto ERROR;      /* actual error already printed by next_token */

            case TOK_END:
                compile_error(state, "%s", "Unexpected end of file");
                goto ERROR;

            case TOK_LINENUMBER:
		temp_un.lineno = state->apparentline;
		if(LAST_EMIT_LINE(result))
		    REEMIT(result, INST_LINENO, temp_un);
		else
                    EMIT(result, INST_LINENO, temp_un);
                break;
      
            case TOK_ANON:       /* ahh, an anonymous function */
                if(++state->anondepth > MAX_ANON_DEPTH) {
                    compile_error(state, "%s", "Anonymous function depth limit exceeded");
                    goto ERROR;
                }
                fa = compile_func(state, Buftext(&state->buf));
                state->anondepth--;
                if(!fa) {
                    /* compile_func already generated an error for them */
                    goto ERROR;   /* swap with FAIL to tersify deep errors */
		    goto FAIL;
                } else {
		    /* add it to the list, so we free it later */
		    /* (sorry if you were hoping for mem leaks ;-) */
		    state->code->num_funcs++;
		    if(state->code->funcs)
		        REALLOC(state->code->funcs, struct func_addr *, state->code->num_funcs);
		    else
		        MALLOC(state->code->funcs, struct func_addr *, 1);
		    state->code->funcs[state->code->num_funcs - 1] = fa;
		    temp_un.address = fa;
                    EMIT(result, INST_ADDRESS, temp_un);
                    state->needline = 1;  /* force a line number emit */
                }
                break;

	      
            case INST_PRIMITIVE:
                switch(state->temp.primitive) {
                    case PRIM_word_begin:
                        compile_error(state, "%s", "Illegal to nest words");
                        goto ERROR;

                    case PRIM_word_end:
			temp_un.integer = 0;
                        EMIT(result, INST_EXIT, temp_un);
                        goto DONE;

                    case PRIM_var_decl:
                        compile_error(state, "%s", "Local variables not supported");
                        goto ERROR;

		    /*  prog/self (special `constants')  */

	            case PRIM_self:
			temp_un.address = result;
			EMIT(result, INST_ADDRESS, temp_un);
			break;

		    case PRIM_prog:
			temp_un.object = state->which;
			EMIT(result, INST_OBJECT, temp_un);
			break;

		    /* exit */

		    case PRIM_exit:
		        temp_un.integer = 0;
			EMIT(result, INST_EXIT, temp_un);
			break;

                    /*  if/else/then handling  */
		      
                    case PRIM_if:
                        if(if_depth++ == MAX_IF_STMT_NEST) {
                            compile_error(state, "%s", "Nesting depth limit exceeded");
                            goto ERROR;
                        }
                        MALLOC(temp, struct nest, 1);
                        temp->next = if_stack;
                        temp->type = NEST_IF;
                        temp->offset = result->num_inst;
                        if_stack = temp;

			temp_un.offset = 0;
                        EMIT(result, INST_IF, temp_un);
                        break;

                    case PRIM_else:
                        if(!if_stack || if_stack->type == NEST_REPEAT) {
                            compile_error(state, "%s", "ELSE without IF");
                            goto ERROR;
                        }
                        if(if_stack->type == NEST_ELSE) {
                            compile_error(state, "%s", "Too many ELSEs for one IF");
                            goto ERROR;
                        }

                        /* resolve the if's branch */
			result->bytecode[if_stack->offset].un.offset =
			    result->num_inst + 1;
                        if_stack->type = NEST_ELSE;
                        if_stack->offset = result->num_inst;

			temp_un.offset = 0;
                        EMIT(result, INST_OFFSET, temp_un);
                        state->needline = 1;  /* force a line number emit */
                        break;
		      
                    case PRIM_then:
                        if(!if_stack || if_stack->type == NEST_REPEAT) {
                            compile_error(state, "%s", "THEN without IF");
                            goto ERROR;
                        }

                        /* resolve the if's or else's branch */
			result->bytecode[if_stack->offset].un.offset =
			    result->num_inst - (LAST_EMIT_LINE(result) ? 1 : 0);
                        temp = if_stack->next;
                        FREE(if_stack);
                        if_stack = temp;

                        state->needline = 1;  /* force a line number emit */
                        break;

#ifdef MUF_REPEAT_UNTIL
                    case PRIM_repeat:
                        /* just sets a marker.  repeat, like then, has no code */
		      
                        if(if_depth++ == MAX_IF_STMT_NEST) {
                            compile_error(state, "%s", "Nesting depth limit exceeded");
                            goto ERROR;
                        }
                        MALLOC(temp, struct nest, 1);
                        temp->next = if_nest;
                        temp->type = NEST_REPEAT;
                        temp->offset = result->num_inst;
                        if_nest = temp;
                        break;
                        state->needline = 1;  /* force a line number emit */

                    case PRIM_until:
                        if(!if_stack || if_stack->type == NEST_IF || if_stack->type == NEST_ELSE) {
                            compile_error(state, "%s", "UNTIL without REPEAT");
                            goto ERROR;
                        }

                        /* resolve the if's or else's branch */
			temp_un.offset = if_stack->offset;
                        EMIT(result, INST_NOT_IF, temp_un);
                        temp = if_stack->next;
                        FREE(if_stack);
                        if_stack = temp;

                        break;
#endif /* MUF_REPEAT_UNTIL */

		    /* depricated functions: addprop */
		    case PRIM_addprop:
		        compile_warning(state, "addprop is not recommended.  Use setprop instead");
                        EMIT(result, INST_PRIMITIVE, state->temp);
		        break;

		    /* everything else */
   	            default:
                        EMIT(result, INST_PRIMITIVE, state->temp);
                        break;
		}
                break;  /* end of case INST_PRIMITIVE */

/* lots of stuff we just emit
            case INST_STRING:
            case INST_INTEGER:
            case INST_OBJECT:
	    case INST_ADDRESS:
	    case INST_CONNECTION:
            case INST_VARIABLE:
            case INST_EXECUTE:
            case INST_OFFSET:
            case INST_LINENO:
*/
            default:
                EMIT(result, token, state->temp);
                break;
        }
    }
  
DONE:
    while(if_stack) {
        if(!state->errors) {    /* don't spam them, just give first goof */
            switch(if_stack->type) {
                case NEST_IF:
                    compile_error(state, "%s", "IF without THEN");
                    break;
                case NEST_ELSE:
                    compile_error(state, "%s", "IF-ELSE without THEN");
                    break;
                case NEST_REPEAT:
                    compile_error(state, "%s", "REPEAT without UNTIL");
                    break;
            }
        }
        temp = if_stack->next;
        FREE(if_stack);
        if_stack = temp;
    }

    if(result->num_inst < 3) {  /* has at least a line num and EXIT */
        /* just a warning now, no longer an error */
        compile_warning(state, "Empty definition of %s",
	    result->name ? result->name : "anonymous function");
    }
  
    if(state->errors) {
ERROR:
        compile_error(state, "Error in definition of %s",
            result->name ? result->name : "anonymous function");
FAIL:
	/*  oops, I almost released 2.3 with this leak */
	while(if_stack) {
	    temp = if_stack->next;
	    FREE(if_stack);
	    if_stack = temp;
	}
        clear_func_addr(result);
        return NULL;
    } else {
        /* this realloc chops off extra room in the stretchy buffer */
        REALLOC(result->bytecode, struct inst, result->num_inst);
        return result;
    }
}




/*
 *  compile_string and compile_text provide the external interface to
 *  the compiler.  from the editor, compile_text can be called with
 *  the current struct text.  if you want compile_text to load and free
 *  the text itself, pass text as NULL.  To supress compiler error
 *  messages, pass player as NOTHING.
 *
 */

/*  compile_string:  use this to compile non-files (things without
 *  struct text's)
 */
struct func_addr *compile_string(const dbref player, const char *string)
{
    compile_state *state;
    struct func_addr *result;

    if(!string || !*string)
        return NULL;

    state = make_compile_state(player, NOTHING, NULL);
    state->str = string;
    state->remain = strlen(string);

    result = compile_func(state, NULL);
    clear_compile_state(state);
    return result;
}

/*  compile_text:
 */
void compile_text(const dbref player, const dbref prog, struct text *text)
{
    compile_state *state;
    int ourtext;
    union inst_un temp_un;

    if(text) {            /* make sure we have a text block */
        ourtext = 0;
    } else {
        text = read_program(prog);
        ourtext = 1;
    }

    state = make_compile_state(player, prog, text); /* set up our state */

    if(!text) {            /* error if we never got any text */
	compile_error(state, "%s", "Missing program text");
        goto ERROR;
    }

    for(;;) {
        int token;

        token = next_token(state);

        switch(token) {
            case TOK_END:
                goto DONE;

            case TOK_LINENUMBER:
		continue;

            case INST_PRIMITIVE:
                switch(state->temp.primitive) {
                    struct func_addr *fa;
                    struct symbol_entry *sym;
                    const char *word;
                    int prim, x;

                    case PRIM_var_decl:
                        do {
                            skip_white(state);
                        } while((x = expand_if_macro(state)) > 0);
                        if(x == -1) { /* macro sub bombed */
                            goto ERROR;
                        }
                        word = next_word(state);

                        if(!*word) {
                            compile_error(state, "%s", "Unexpected end of source");
                            goto ERROR;
                        }
                        sym = lookup_symbol(state, word);
                        if(sym) {
			    /* mimic that old compiler */
			    compile_warning(state, "Duplicate identifier %s", word);
                        }
                        if(state->nextvar > MAX_VAR) {
                            compile_error(state, "%s", "Variable limit exceeded");
                            goto ERROR;
                        }
                        prim = lookup_primitive(word);
                        if(prim >= 0) {
                            compile_warning(state, "Variable declaration masks primitive %s",
                                prim2name(prim));
                        }
		    
		        temp_un.variable = state->nextvar++;
                        make_symbol(state, word, INST_VARIABLE, temp_un);
                        break;

                    case PRIM_word_begin:
                        do {
                            skip_white(state);
                        } while((x = expand_if_macro(state)) > 0);
                        if(x == -1) { /* macro sub bombed */
                            goto ERROR;
                        }
                        word = next_word(state);

                        if(!*word) {
                            compile_error(state, "%s", "Unexpected end of source");
                            goto ERROR;
                        }
                        sym = lookup_symbol(state, word);
                        if(sym) {
                            compile_error(state, "Duplicate identifier %s", word);
                            goto ERROR;
                        }
/*
                        do we want to limit funcs?

                        if(state->nextvar > MAX_FUNC) {
                            compile_error(state, "%s", "Function limit exceeded");
                            goto ERROR;
                        }
*/
                        prim = lookup_primitive(word);
                        if(prim >= 0) {
                            compile_warning(state, "Function declaration masks primitive %s",
                                prim2name(prim));
                        }

                        fa = compile_func(state, word); /* makes its own symbol */
                        if(!fa) {
                            /* it's already given an appropriate error */
                            goto ERROR;
                        }

                        /* add it to our list */
                        x = state->code->num_funcs++;
			if (x) {
			    REALLOC(state->code->funcs, struct func_addr *, state->code->num_funcs);
			} else {
			    MALLOC(state->code->funcs, struct func_addr *, 1);
			}
		        state->code->funcs[state->code->num_funcs - 1] = fa;
		        state->code->main_func = fa; /* always the last one! */
                        break;

                    default:
                        compile_error(state, "%s",
                            "Expected variable or word declaration");
                        goto ERROR;
                }
                break;
	      
            default:
                compile_error(state, "%s",
                    "Expected variable or word declaration");
		goto ERROR;
        }
    }
  
DONE:
    if(!state->code->num_funcs)
        compile_error(state, "%s", "No word definitions");

ERROR:
    if(ourtext && text)
        free_text(state->text);  /* we weren't passed it */

    if(!state->errors) {
        optimize_code(player, state->code);
	add_code(prog, state->code);
    }

    clear_compile_state(state);
}
