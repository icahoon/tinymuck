/* Copyright (c) 1992 by David Moore.  All rights reserved. */
/* db_prims.c,v 2.11 1997/08/30 06:44:02 dmoore Exp */
#include "config.h"

#include <string.h>

#include "db.h"
#include "code.h"
#include "prim_offsets.h"
#include "buffer.h"
#include "externs.h"

#define SSText(x)	((x)->un.string ? (x)->un.string->data : "")

#define GET_STR_PRIM(NAME, OP) \
    MUF_PRIM(prim_##NAME) \
    { inst obj, result; \
      const char *str; \
      pop_stack(curr_data_st, &obj); \
      str = Get##OP(obj.un.object); \
      result.type = INST_STRING; \
      if (str) result.un.string = make_shared_string(str, strlen(str)); \
      else result.un.string = NULL; \
      push_stack(curr_data_st, &result); }

GET_STR_PRIM(name, Name)
GET_STR_PRIM(desc, Desc)
GET_STR_PRIM(succ, Succ)	
GET_STR_PRIM(fail, Fail)	
GET_STR_PRIM(drop, Drop)
GET_STR_PRIM(osucc, OSucc)	
GET_STR_PRIM(ofail, OFail)	
GET_STR_PRIM(odrop, ODrop)	


#define SET_STR_PRIM(NAME, OP) \
    MUF_PRIM(prim_set##NAME) \
    { inst obj, value; \
      pop_stack(curr_data_st, &value); pop_stack(curr_data_st, &obj); \
      Set##OP(obj.un.object, SSText(&value)); }

SET_STR_PRIM(desc, Desc)
SET_STR_PRIM(succ, Succ)
SET_STR_PRIM(fail, Fail)
SET_STR_PRIM(drop, Drop)
SET_STR_PRIM(osucc, OSucc)
SET_STR_PRIM(ofail, OFail)
SET_STR_PRIM(odrop, ODrop)

MUF_PRIM(prim_setname)
{
    inst obj, value;

    pop_stack(curr_data_st, &value);
    pop_stack(curr_data_st, &obj);

    if (!ok_name(SSText(&value))) {
	interp_error(curr_frame, PRIM_setname, "Invalid name");
	return;
    }

    if (Typeof(obj.un.object) == TYPE_PLAYER) {
	interp_error(curr_frame, PRIM_setname, "Can't set a player's name");
	return;
    }

    SetName(obj.un.object, SSText(&value));
}


#define GET_DBREF_PRIM(NAME, OP) \
    MUF_PRIM(prim_##NAME) \
    { inst obj, result; \
      pop_stack(curr_data_st, &obj); \
      result.type = INST_OBJECT; result.un.object = Get##OP(obj.un.object); \
      push_stack(curr_data_st, &result); }

GET_DBREF_PRIM(contents, Contents)
GET_DBREF_PRIM(exits, Actions)
GET_DBREF_PRIM(getlink, Link)
GET_DBREF_PRIM(loc, Loc)
GET_DBREF_PRIM(next, Next)
GET_DBREF_PRIM(owner, Owner)


#define GET_INT_PRIM(NAME, OP) \
    MUF_PRIM(prim_##NAME) \
    { inst obj, result; \
      pop_stack(curr_data_st, &obj); \
      result.type = INST_INTEGER; result.un.integer = (OP(obj.un.object)); \
      push_stack(curr_data_st, &result); }

GET_INT_PRIM(pennies, GetPennies)
GET_INT_PRIM(exitp, TYPE_EXIT == Typeof)
GET_INT_PRIM(playerp, TYPE_PLAYER == Typeof)
GET_INT_PRIM(progp, TYPE_PROGRAM == Typeof)
GET_INT_PRIM(roomp, TYPE_ROOM == Typeof)
GET_INT_PRIM(thingp, TYPE_THING == Typeof)


MUF_PRIM(prim_okp)
{
    inst arg;
    inst result;

    pop_stack(curr_data_st, &arg);
    
    result.type = INST_INTEGER;
    result.un.integer = ((arg.type == INST_OBJECT) && !Garbage(arg.un.object));

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_dbcmp)
{
    inst obj1, obj2;
    inst result;

    pop_stack(curr_data_st, &obj2);
    pop_stack(curr_data_st, &obj1);

    result.type = INST_INTEGER;
    result.un.integer = (obj1.un.object == obj2.un.object);

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_dbtop)
{
    inst result;

    result.type = INST_OBJECT;
    result.un.object = db_top;

    push_stack(curr_data_st, &result);
}


static const char *check_moveto(const dbref thing, const dbref dest, const dbref player, const int wizzed)
{
    /* Get rid of this once you can moveto exits around. */
    if (Typeof(thing) == TYPE_EXIT)
	return "Object is an exit";

    if (!moveto_type_ok(thing, dest))
	return "Bad destination";

    if (!moveto_source_ok(thing, dest, player, wizzed))
	return "Permission denied on source";

    if (!moveto_thing_ok(thing, dest, player, wizzed))
	return "Permission denied on object";

    if (!moveto_dest_ok(thing, dest, player, wizzed))
	return "Permission denied on destination";

    if (!moveto_loop_ok(thing, dest))
	return "Destination would cause loop";

    return NULL;
}


MUF_PRIM(prim_check_moveto)
{
    inst arg1, arg2;
    inst result;
    dbref thing, dest;
    const char *error;

    pop_stack(curr_data_st, &arg2);
    pop_stack(curr_data_st, &arg1);

    thing = arg1.un.object;
    dest = arg2.un.object;

    if (dest == HOME) dest = GetHome(thing);
    
    error = check_moveto(thing, dest,
			 frame_euid(curr_frame), frame_wizzed(curr_frame));
    result.type = INST_STRING;
    result.un.string = make_shared_string(error, error ? strlen(error) : 0);

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_moveto)
{
    inst arg1, arg2;
    dbref thing, dest;
    const char *error;

    pop_stack(curr_data_st, &arg2);
    pop_stack(curr_data_st, &arg1);

    thing = arg1.un.object;
    dest = arg2.un.object;

    if (dest == HOME) dest = GetHome(thing);
    
    error = check_moveto(thing, dest,
			 frame_euid(curr_frame), frame_wizzed(curr_frame));
    if (error) {
	interp_error(curr_frame, PRIM_moveto, error);
	return;
    }

    move_object(thing, dest, NOTHING);
}


MUF_PRIM(prim_addpennies)
{
    inst what, amount;
    int result, wizzed;

    pop_stack(curr_data_st, &amount);
    pop_stack(curr_data_st, &what);
    
    result = GetPennies(what.un.object) + amount.un.integer;

    wizzed = frame_wizzed(curr_frame);
    
    switch (Typeof(what.un.object)) {
    case TYPE_PLAYER:
	if ((result > MAX_PENNIES)
	    && (amount.un.integer > 0)
	    && !wizzed) {
	    interp_error(curr_frame, PRIM_addpennies,
			 "%i exceeds MAX_PENNIES", result);
	}
	if ((result < 0) && !wizzed) {
	    interp_error(curr_frame, PRIM_addpennies,
			 "%i is negative", result);
	}
	break;
    case TYPE_THING:
	if (!wizzed) {
	    interp_error(curr_frame, PRIM_addpennies,
			 "Only wizards can give pennies to objects");
	}
	break;
    default:
	interp_error(curr_frame, PRIM_addpennies, "Invalid object type");
	break;
    }
	    
    if (!curr_frame->error)
	SetPennies(what.un.object, result);
}


static dbref copyobj(const dbref player, const dbref old)
{
    dbref new;

    new = make_object(TYPE_THING, GetName(old), GetOwner(old),
		      player, GetLink(old));

    SetOnFlag(new, GetFlags(old));
    SetKey(new, copy_bool(GetKey(old)));
    copy_props(old, new);
    SetActions(new, NOTHING);
    move_object(new, player, NOTHING);

    return new;
}


MUF_PRIM(prim_copyobj)
{
    inst arg;
    inst result;

    pop_stack(curr_data_st, &arg);

    if (curr_frame->max_create == 0) {
	interp_error(curr_frame, PRIM_copyobj, "Can't create anymore objects");
	return;
    }

    if (Typeof(arg.un.object) != TYPE_THING) {
	interp_error(curr_frame, PRIM_copyobj, "Can't copy that object type");
	return;
    }

    curr_frame->max_create--;

    result.type = INST_OBJECT;
    result.un.object = copyobj(curr_frame->player, arg.un.object);

    push_stack(curr_data_st, &result);
}


static void check_prop_perms(frame *fr, const dbref object, const char *name, const int check_write, const int prim)
{
    if ((*name == PROP_PRIVATE) || (check_write && *name == PROP_RDONLY))
	check_perms(fr, object, prim);
    if (strchr(name, ':'))
	interp_error(fr, prim, "Can't use ':' in property name");
}


#define TRY_INTERN_PROPS(NAME) \
    MUF_PRIM(prim_##NAME##_i) { prim_##NAME(); }

TRY_INTERN_PROPS(getpropv)
TRY_INTERN_PROPS(getprops)
TRY_INTERN_PROPS(getpropd)
TRY_INTERN_PROPS(rmvprop)
TRY_INTERN_PROPS(addprop)
TRY_INTERN_PROPS(setprop)
TRY_INTERN_PROPS(flagp)


MUF_PRIM(prim_addprop)
{
    inst obj, name, str, val;

    pop_stack(curr_data_st, &val);
    pop_stack(curr_data_st, &str);
    pop_stack(curr_data_st, &name);
    pop_stack(curr_data_st, &obj);

    check_prop_perms(curr_frame, obj.un.object, SSText(&name), 1,
		     PRIM_addprop);
    if (curr_frame->error) return;

    add_property(obj.un.object, SSText(&name), SSText(&str), val.un.integer);
}


MUF_PRIM(prim_setprop)
{
    inst obj, name, what;

    pop_stack(curr_data_st, &what);
    pop_stack(curr_data_st, &name);
    pop_stack(curr_data_st, &obj);

    check_prop_perms(curr_frame, obj.un.object, SSText(&name), 1,
		     PRIM_setprop);
    if (curr_frame->error) return;

    switch (what.type) {
    case INST_STRING:
	add_string_prop(obj.un.object, SSText(&name),
			SSText(&what), NORMAL_PROP, PERM_PROP);
	break;
    case INST_INTEGER:
	add_int_prop(obj.un.object, SSText(&name),
		     what.un.integer, NORMAL_PROP, PERM_PROP);
	break;
    case INST_OBJECT:
	add_dbref_prop(obj.un.object, SSText(&name),
		       what.un.object, NORMAL_PROP, PERM_PROP);
	break;
    default:
	interp_error(curr_frame, PRIM_setprop,
		     "Illegal type to put in property");
	break;
    }
}


MUF_PRIM(prim_getprops)
{
    inst obj, name;
    inst result;
    const char *str;

    pop_stack(curr_data_st, &name);
    pop_stack(curr_data_st, &obj);

    check_prop_perms(curr_frame, obj.un.object, SSText(&name), 0,
		     PRIM_getprops);
    if (curr_frame->error) return;

    str = get_string_prop(obj.un.object, SSText(&name), NORMAL_PROP);

    result.type = INST_STRING;
    if (str) {
        result.un.string = make_shared_string(str, strlen(str));
    } else {
        result.un.string = NULL;
    }

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_getpropv)
{
    inst obj, name;
    inst result;

    pop_stack(curr_data_st, &name);
    pop_stack(curr_data_st, &obj);

    check_prop_perms(curr_frame, obj.un.object, SSText(&name), 0,
		     PRIM_getpropv);
    if (curr_frame->error) return;

    result.type = INST_INTEGER;
    result.un.integer = get_int_prop(obj.un.object, SSText(&name),
				     NORMAL_PROP);

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_getpropd)
{
    inst obj, name;
    inst result;

    pop_stack(curr_data_st, &name);
    pop_stack(curr_data_st, &obj);

    check_prop_perms(curr_frame, obj.un.object, SSText(&name), 0,
		     PRIM_getpropd);
    if (curr_frame->error) return;

    result.type = INST_OBJECT;
    result.un.object = get_dbref_prop(obj.un.object, SSText(&name),
				      NORMAL_PROP);

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_rmvprop)
{
    inst obj, name;

    pop_stack(curr_data_st, &name);
    pop_stack(curr_data_st, &obj);

    check_prop_perms(curr_frame, obj.un.object, SSText(&name), 1,
		     PRIM_rmvprop);
    if (curr_frame->error) return;

    remove_prop(obj.un.object, SSText(&name), NORMAL_PROP);
}


MUF_PRIM(prim_match)
{
    inst str;
    inst result;
    struct match_data md;

    pop_stack(curr_data_st, &str);

    init_match(curr_frame->player, SSText(&str), NOTYPE, &md);
    match_home(&md);
    match_everything(&md,
		     (Wizard(curr_frame->player) || frame_wizzed(curr_frame)));

    result.type = INST_OBJECT;
    result.un.object = match_result(&md);

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_rmatch)
{
    inst obj, str;
    inst result;
    struct match_data md;

    pop_stack(curr_data_st, &str);
    pop_stack(curr_data_st, &obj);

    if ((Typeof(obj.un.object) == TYPE_PROGRAM)
	|| (Typeof(obj.un.object) == TYPE_EXIT)) {
	interp_error(curr_frame, PRIM_rmatch,
		     "Requires ROOM, PLAYER, or THING");
	return;
    }

    init_match(curr_frame->player, SSText(&str), TYPE_THING, &md);
    match_rmatch(obj.un.object, &md);

    result.type = INST_OBJECT;
    result.un.object = match_result(&md);

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_flagp)
{
    inst obj, flagname;
    inst result;
    object_flag_type flag;

    pop_stack(curr_data_st, &flagname);
    pop_stack(curr_data_st, &obj);
    
    flag = which_flag(Typeof(obj.un.object), SSText(&flagname));

    result.type = INST_INTEGER;
    if (flag != BAD_FLAG) {
	result.un.integer = HasFlag(obj.un.object, flag);
    } else {
	result.un.integer = 0;
    }

    if (flag == WIZARD)
	result.un.integer = Wizard(obj.un.object); /* Quell hides in muf. */

#ifdef GOD_FLAGS
    if (flag == GOD)
	result.un.integer = God(obj.un.object); /* Quell hides in muf. */
#endif

    push_stack(curr_data_st, &result);
}


MUF_PRIM(prim_set)
{
    inst obj, which;
    const char *flagname;
    int bang = 0;
    object_flag_type flag;

    pop_stack(curr_data_st, &which);
    pop_stack(curr_data_st, &obj);

    flagname = SSText(&which);

    if (*flagname == '!') {
	bang = 1;
	flagname++;
    }
    
    flag = which_flag(Typeof(obj.un.object), flagname);

    if (flag == BAD_FLAG) {
	interp_error(curr_frame, PRIM_set, "Unrecognized flag (%s)", flagname);
	return;
    }

    if (restricted(frame_euid(curr_frame), obj.un.object, flag,
		   frame_wizzed(curr_frame), 0)) {
	interp_error(curr_frame, PRIM_set, "Permission denied");
	return;
    }

    if (restricted(frame_euid(curr_frame), obj.un.object, flag, 0, 0)) {
	log_status("FLAG SET: %s %s on %u by %u (prog #%d called from #%d).",
		   unparse_flag(obj.un.object, flag, 1),
		   (bang ? "reset" : "set"),
		   GetOwner(obj.un.object), obj.un.object,
		   curr_frame->player, curr_frame->player,
		   curr_frame->program, curr_frame->orig_program);
    }
    
    if (bang) {
	SetOffFlag(obj.un.object, flag);
    } else {
	SetOnFlag(obj.un.object, flag);
    }

    /* In case they set debugs/setuid, etc. */
    curr_frame->cache_dirty = 1;
}

