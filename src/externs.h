#ifndef MUCK_EXTERNS_H
#define MUCK_EXTERNS_H
#ifdef USE_EXTERNS
#include "config.h"
#include "db.h"
#include <stdio.h>
#include <time.h>
#include "ansify.h"
#include "buffer.h"
#include "match.h"
#include "hostname.h"
#include "hashtab.h"
#include "code.h"
#include "muf_con.h"
#define MATH_PRIM(a, b) MUF_PRIM(prim_##a)
#define MATH0_PRIM(a, b) MUF_PRIM(prim_##a)
#define GET_STR_PRIM(a, b) MUF_PRIM(prim_##a)
#define SET_STR_PRIM(a, b) MUF_PRIM(prim_set##a)
#define GET_DBREF_PRIM(a, b) MUF_PRIM(prim_##a)
#define GET_INT_PRIM(a, b) MUF_PRIM(prim_##a)
#define TRY_INTERN_PROPS(a) MUF_PRIM(prim_##a##_i)

/* From: builtins/action.c */
extern void do_action(const dbref player, const char *action_name, const char *source_name, const char *ignore);

/* From: builtins/attach.c */
extern void do_attach(const dbref player, const char *action_name, const char *source_name, const char *ignore);
extern dbref parse_source(const dbref player, const char *source_name);

/* From: builtins/boot.c */
extern void do_boot(const dbref player, const char *name, const char *ignore1, const char *ignore2);

/* From: builtins/builtins.c */
extern void process_command(const dbref player, const char *comm, int comm_len);

/* From: builtins/chown.c */
extern void do_chown(const dbref player, const char *name, const char *newowner, const char *ignore);

/* From: builtins/create.c */
extern void do_create(const dbref player, const char *name, const char *scost, const char *ignore);

/* From: builtins/desc.c */
extern void do_describe(const dbref player, const char *name, const char *description, const char *ignore);

/* From: builtins/dig.c */
extern void do_dig(const dbref player, const char *name, const char *pname, const char *ignore);

/* From: builtins/drop.c */
extern void do_drop_message(const dbref player, const char *name, const char *message, const char *ignore);

/* From: builtins/dropobj.c */
extern void do_drop(const dbref player, const char *name, const char *ignore1, const char *ignore2);

/* From: builtins/dump.c */
extern void do_dump(const dbref player, const char *ignore1, const char *ignore2, const char *newfile);

/* From: builtins/edit.c */
extern void do_edit(const dbref player, const char *name, const char *ignore1, const char *ignore2);

/* From: builtins/entrances.c */
extern void do_entrances(const dbref player, const char *arg, const char *ignore1, const char *ignore2);

/* From: builtins/examine.c */
extern void show_examine(const dbref player, const dbref thing, const int show_muf);
extern void do_examine(const dbref player, const char *name, const char *ignore1, const char *ignore2);

/* From: builtins/extract.c */
extern void do_extract(const dbref player, const char *name, const char *ignore1, const char *ignore2);

/* From: builtins/fail.c */
extern void do_fail(const dbref player, const char *name, const char *message, const char *ignore);

/* From: builtins/find.c */
extern void do_find(const dbref player, const char *name, const char *ignore1, const char *ignore2);

/* From: builtins/force.c */
extern void do_force(const dbref player, const char *name, const char *command, const char *ignore);

/* From: builtins/get.c */
extern void do_get(const dbref player, const char *what, const char *ignore1, const char *ignore2);

/* From: builtins/give.c */
extern void do_give(const dbref player, const char *recipient, const char *samount, const char *ignore);

/* From: builtins/gripe.c */
extern void do_gripe(const dbref player, const char *ignore1, const char *ignore2, const char *message);

/* From: builtins/inv.c */
extern void do_inventory(const dbref player, const char *ignore1, const char *ignore2, const char *ignore3);

/* From: builtins/kill.c */
extern void do_kill(const dbref player, const char *what, const char *scost, const char *ignore);

/* From: builtins/link.c */
extern void do_link(const dbref player, const char *thing_name, const char *dest_name, const char *ignore);
extern int link_exit(const dbref player, const dbref exit, const char *dest_name, dbref *dest_list);

/* From: builtins/list.c */
extern void do_list(const dbref player, const char *name, const char *lines, const char *ignore);

/* From: builtins/lock.c */
extern void do_lock(const dbref player, const char *name, const char *keyname, const char *ignore);

/* From: builtins/look.c */
extern void do_look(const dbref player, const char *name, const char *ignore1, const char *ignore2);

/* From: builtins/move.c */
extern void do_move(const dbref player, const char *ign1, const char *ign2, const char *direction);
extern int try_move(const dbref player, const char *direction);

/* From: builtins/name.c */
extern void do_name(const dbref player, const char *name, const char *newname, const char *ignore);

/* From: builtins/newpass.c */
extern void do_newpassword(const dbref player, const char *name, const char *password, const char *ignore);

/* From: builtins/odrop.c */
extern void do_odrop(const dbref player,const char *name, const char *message, const char *ignore);

/* From: builtins/ofail.c */
extern void do_ofail(const dbref player, const char *name, const char *message, const char *ignore);

/* From: builtins/open.c */
extern void do_open(const dbref player, const char *direction, const char *linkto, const char *ignore);

/* From: builtins/osucc.c */
extern void do_osuccess(const dbref player, const char *name, const char *message, const char *ignore);

/* From: builtins/owned.c */
extern void do_owned(const dbref player, const char *ignore1, const char *ignore2, const char *name);

/* From: builtins/page.c */
extern void do_page(const dbref player, const char *who, const char *mesg, const char *ignore);

/* From: builtins/pass.c */
extern void do_password(const dbref player, const char *old, const char *new, const char *ignore);

/* From: builtins/pcreate.c */
extern void do_pcreate(const dbref player, const char *user, const char *password, const char *ignore);

/* From: builtins/pose.c */
extern void do_pose(const dbref player, const char *ignore1, const char *ignore2, const char *message);

/* From: builtins/prog.c */
extern void do_prog(const dbref player, const char *name, const char *ignore1, const char *ignore2);

/* From: builtins/purge.c */
extern void do_purge(const dbref player, const char *name, const char *ignore1, const char *ignore2);

/* From: builtins/recycle.c */
extern void do_recycle(const dbref player, const char *name, const char *ignore1, const char *ignore2);

/* From: builtins/rob.c */
extern void do_rob(const dbref player, const char *what, const char *ignore1, const char *ignore2);

/* From: builtins/say.c */
extern void do_say(const dbref player, const char *ignore1, const char *ignore2, const char *message);

/* From: builtins/score.c */
extern void do_score(const dbref player, const char *ignore1, const char *ignore2, const char *ignore3);

/* From: builtins/set.c */
extern void do_set(const dbref player, const char *objname, const char *text, const char *ignore);

/* From: builtins/shutdown.c */
extern void do_shutdown(const dbref player, const char *ignore1, const char *ignore2, const char *ignore3);

/* From: builtins/stats.c */
extern void do_stats(const dbref player, const char *name, const char *ignore1, const char *ignore2);

/* From: builtins/succ.c */
extern void do_success(const dbref player, const char *name, const char *message, const char *ignore);

/* From: builtins/tele.c */
extern void do_teleport(const dbref player, const char *what, const char *whereto, const char *ignore);

/* From: builtins/toad.c */
extern void do_toad(const dbref player, const char *name, const char *recip, const char *ignore);

/* From: builtins/trace.c */
extern void do_trace(const dbref player, const char *name, const char *sdepth, const char *ignore);

/* From: builtins/unlink.c */
extern void do_unlink(const dbref player, const char *name, const char *ignore1, const char *ignore2);

/* From: builtins/unlock.c */
extern void do_unlock(const dbref player, const char *name, const char *ignore1, const char *ignore2);

/* From: builtins/version.c */
extern void do_version(const dbref player, const char *ignore1, const char *ignore2, const char *ign3);

/* From: builtins/wall.c */
extern void do_wall(const dbref player, const char *ignore1, const char *ignore2, const char *message);

/* From: builtins/whisper.c */
extern void do_whisper(const dbref player, const char *arg1, const char *arg2, const char *ignore);

/* From: compile/compile.c */
extern void init_compiler(void);
extern struct func_addr *make_func_addr(struct code *code, int num_inst, const char *name);
extern void clear_func_addr(struct func_addr *victim);
extern struct func_addr *compile_string(const dbref player, const char *string);
extern void compile_text(const dbref player, const dbref prog, struct text *text);

/* From: compile/macro.c */
extern void clear_macros(void);
extern void delete_macro_entry(void *entry);
extern int new_macro(const char *name, const char *definition, const dbref player);
extern int kill_macro(const dbref player, const char *name);
extern void chown_macros(const dbref from, const dbref to);
extern const char *macro_expansion(const char *name);
extern void macroload(FILE *f);
extern int macrodump(const char *name);
extern void show_macros(const char *first, const char *last, const int full, const dbref player);
extern void init_macros(void);

/* From: compile/optimize.c */
extern void optimize_code(const dbref who, struct code *code);

/* From: db/boolexp.c */
extern int eval_boolexp(const dbref player, const struct boolexp *b, const dbref thing);
extern struct boolexp *copy_bool(const struct boolexp *old);
extern struct boolexp *parse_boolexp(const dbref player, const char *buf, const int dump_format);
extern const char *unparse_boolexp(const dbref player, const struct boolexp *b, const int dump_format);
extern void free_boolexp(struct boolexp *b);
extern long total_boolexp_nodes(void);
extern void init_boolexp_nodes(const long count);

/* From: db/db.c */
extern void db_grow(const dbref newtop);
extern void init_db(int old_dbtop, int old_growth);
extern void make_object_internal(const dbref newobj, const object_flag_type type);
extern dbref make_object(const object_flag_type type, const char *name, const dbref owner, const dbref location, const dbref link);
extern void db_FREE(void);
extern void recycle(const dbref player, const dbref obj);
extern void purge(const dbref player, const dbref victim);
extern void chown_object(const dbref thing, const dbref newowner);
extern void db_move(const dbref what, dbref where);

/* From: db/debug_malloc.c */
extern void *debug_realloc(void *cp, size_t size, const char *file, const int line);
extern void *debug_malloc(size_t size, const char *file, const int line);
extern void debug_free(void *cp, const char *file, const int line);
extern void *debug_calloc(size_t size, const char *file, const int line);
extern void debug_mstats(const char *s);

/* From: db/dump_db.c */
extern dbref parse_dbref(register const char *s);
extern void putref(FILE *f, const dbref ref);
extern void putstring(FILE *f, const char *s);
extern const char *getstring(FILE *f);
extern void putbackstring(void);
extern dbref getref(FILE *f);
extern int db_write(const char *name);
extern dbref db_read_firiss(FILE *f);
extern dbref db_read(FILE *f);

/* From: db/flags.c */
extern const char *unparse_flag(const dbref thing, object_flag_type flag, const int long_name);
extern const char *unparse_flags(const dbref thing);
extern const char *unparse_long_flags(const dbref thing, const int dump_format);
extern object_flag_type parse_flags(const char *flagstr);
extern int which_flag(object_flag_type type, const char *flagstr);

/* From: db/match.c */
extern void init_match(dbref player, const char *name, int type, struct match_data *md);
extern void init_match_check_keys(dbref player, const char *name, int type, struct match_data *md);
extern void match_player(struct match_data *md);
extern void match_absolute(struct match_data *md);
extern void match_me(struct match_data *md);
extern void match_here(struct match_data *md);
extern void match_home(struct match_data *md);
extern void match_possession(struct match_data *md);
extern void match_neighbor(struct match_data *md);
extern void match_exits(dbref first, struct match_data *md);
extern void match_room_exits(dbref loc, struct match_data *md);
extern void match_invobj_actions(struct match_data *md);
extern void match_roomobj_actions(struct match_data *md);
extern void match_player_actions(struct match_data *md);
extern void match_all_exits(struct match_data *md);
extern void match_everything(struct match_data *md, int wizzed);
extern dbref match_result(struct match_data *md);
extern dbref last_match_result(struct match_data *md);
extern dbref noisy_match_result(struct match_data *md);
extern void match_rmatch(dbref arg1, struct match_data *md);
extern dbref do_match_boolexp(const dbref player, const char *what);
extern dbref match_controlled(const dbref player, const char *name);

/* From: db/muck_malloc.c */
extern void *muck_realloc(void *cp, size_t size, const char *file, const int line);
extern void *muck_malloc(size_t size, const char *file, const int line);
extern void muck_free(void *cp, const char *file, const int line);
extern void muck_free_cnt_string(const char *s, unsigned int len, const char *file, const int line);
extern void muck_free_string(const char *s, const char *file, const int line);
extern const char *muck_alloc_cnt_string(const char *s, const unsigned int len, const char *file, const int line);
extern const char *muck_alloc_cnt2_string(const char *s1, const unsigned int len1, const char *s2, const unsigned int len2, const char *file, const int line);
extern const char *muck_alloc_string(const char *s, const char *file, const int line);

/* From: db/property.c */
extern void clear_prop_list(struct prop_list *plist);
extern void add_int_prop(const dbref object, const char *name, const long value, const int internal, const int temporary);
extern void add_dbref_prop(const dbref object, const char *name, const dbref value, const int internal, const int temporary);
extern void add_string_prop(const dbref object, const char *name, const char *string, const int internal, const int temporary);
extern void add_boolexp_prop(const dbref object, const char *name, struct boolexp *bool, const int internal, const int temporary);
extern void add_property(const dbref object, const char *name, const char *string, const int value);
extern void remove_prop(const dbref object, const char *name, const int internal);
extern int has_prop_contents(const dbref player, const char *name, const char *string);
extern int has_prop(const dbref object, const char *name, const char *string);
extern const char *get_string_prop(const dbref object, const char *name, const int internal);
extern int get_int_prop(const dbref object, const char *name, const int internal);
extern dbref get_dbref_prop(const dbref object, const char *name, const int internal);
extern struct boolexp *get_boolexp_prop(const dbref object, const char *name, const int internal);
extern void clear_nonhidden_props(const dbref object);
extern void free_all_props(const dbref object);
extern void show_properties(const dbref player, const dbref thing);
extern void copy_props(const dbref old, const dbref new);
extern void putproperties(FILE *f, const struct prop_list *plist);
extern void getproperties(const dbref objno, FILE *f);
extern void map_string_props(const dbref object, void (*func)(const dbref obj, const char *name, const char *string));
extern long total_properties(void);
extern void init_properties(const long count);

/* From: db/sane.c */
extern int sanity_check(void);

/* From: editor/edit.c */
extern void init_editor(void);
extern void clear_editor(void);
extern struct text *read_program(const dbref program);
extern void edit_quit_external(const dbref player);
extern void edit_quit_recycle(const dbref program);
extern void begin_editing(const dbref player, const dbref program, const int list);
extern void editor(const dbref player, const char *command, int len);
extern void do_edit_list(const dbref player, const dbref program, const char **args);

/* From: editor/text.c */
extern void free_text(struct text *text);
extern void write_text(const char *fname, struct text *text);
extern struct text *make_new_text(void);
extern struct text *read_text(const char *fname);
extern void insert_text(struct text *text, const char *data, const int len, int where);
extern void insert2_text(struct text *text, const char *data1, const int len1, const char *data2, const int len2, int where);
extern void flush_text(struct text *text, int where, int amount);
extern void delete_text(struct text *text, int start_pos, int end_pos);
extern const char *text_line(struct text *text, int where, int *length);
extern int text_total_lines(struct text *text);
extern int text_total_bytes(struct text *text);

/* From: game/connect.c */
extern void do_connect(const dbref player);

/* From: game/disconnect.c */
extern void do_disconnect(const dbref player);

/* From: game/move.c */
extern void send_contents(const dbref loc, const dbref dest);
extern int moveto_type_ok(const dbref thing, const dbref dest);
extern int moveto_source_ok(const dbref thing, const dbref dest, const dbref player, const int wizard);
extern int moveto_thing_ok(const dbref thing, const dbref dest, const dbref player, const int wizard);
extern int moveto_dest_ok(const dbref thing, const dbref dest, const dbref player, const int wizard);
extern int moveto_loop_ok(const dbref thing, dbref dest);
extern void player_enter_room(const dbref player, const dbref old, const dbref new, const dbref exit);
extern void move_object(const dbref thing, dbref dest, const dbref exit);
extern void send_home(const dbref thing);

/* From: game/predicates.c */
extern int can_link_to(const dbref who, const object_flag_type what_type, const dbref where);
extern int can_see_flags(const dbref who, const dbref what);
extern int can_link(const dbref who, const dbref what);
extern int could_doit(const dbref player, const dbref thing);
extern int can_doit(const dbref player, const dbref thing, const char *default_fail_msg);
extern int can_see(const dbref player, const dbref thing, const int wizard, const int can_see_loc);
extern int controls(const dbref who, const dbref what, const int wizard);
extern int restricted(const dbref player, const dbref thing, const object_flag_type flag, const int wizplayer, const int godplayer);
extern int payfor(const dbref who, const int cost);
extern int word_start (const char *str, const char let);
extern int ok_name(const char *name);
extern int ok_player_name(const char *name);
extern int ok_password(const char *password);

/* From: interface/clilib.c */
extern int rwhocli_setup(const char *server, const char *serverpw, const char *myname, const char *comment);
extern int rwhocli_shutdown(void);
extern int rwhocli_pingalive(void);
extern int rwhocli_userlogin(const char *uid, const char *name, const time_t tim);
extern int rwhocli_userlogout(const char *uid);

/* From: interface/hostname.c */
extern const char *lookup_host(ip_addr_t addr, int try_nameservice);
extern void init_hostnames(void);
extern void clear_hostnames(void);

/* From: interface/interface.c */
extern int check_awake(const dbref player);
extern dbref *connected_players(int *count);
extern int *connected_cons(int *count);
extern int con_info(int descr, struct muf_con_info *dest);
extern int boot_off_d(int descr);
extern int send_player(const dbref player, const char *msg, const int len);
extern void send_except(const dbref where, const dbref except, const char *msg, const int len);
extern void send_all(const char *msg, const int len, const int important);
extern int boot_off(const dbref player, int which);
extern void emergency_shutdown(void);
extern void normal_shutdown(void);
extern void dump_status(int signum);
extern int set_up_socket(const char *port);
extern void main_loop(const int listen_sock);

/* From: interface/lockout.c */
extern void reload_lockout(const int signum);
extern void free_lockout(void);
extern int ok_conn_site(const char *hostname);
extern int ok_create_site(const char *hostname);
extern int ok_player_site(const char *hostname, const dbref who_dbref);

/* From: interface/main.c */
extern void panic(const char *message);
extern void fork_and_dump(void);
extern void close_all_files(void);

/* From: interp/code.c */
extern void init_code_table(void);
extern void clear_code_table(void);
extern struct code *get_code(const dbref program);
extern void add_code(const dbref program, struct code *code);
extern void delete_code(const dbref program);
extern struct code *make_code(dbref obj, int num_funcs);
extern struct code *dup_code(struct code *code);
extern void clear_code(struct code *code);
extern void get_code_size(struct code *code, int *num_func, int *num_inst);

/* From: interp/con_prims.c */
extern MUF_PRIM(prim_concount);
extern MUF_PRIM(prim_connections);
extern MUF_PRIM(prim_condbref);
extern MUF_PRIM(prim_contime);
extern MUF_PRIM(prim_conidle);
extern MUF_PRIM(prim_conhost);
extern MUF_PRIM(prim_conboot);
extern MUF_PRIM(prim_online);
extern MUF_PRIM(prim_awakep);

/* From: interp/db_prims.c */
extern GET_STR_PRIM(name, Name);
extern GET_STR_PRIM(desc, Desc);
extern GET_STR_PRIM(succ, Succ)	;
extern GET_STR_PRIM(fail, Fail)	;
extern GET_STR_PRIM(drop, Drop);
extern GET_STR_PRIM(osucc, OSucc)	;
extern GET_STR_PRIM(ofail, OFail)	;
extern GET_STR_PRIM(odrop, ODrop)	;
extern SET_STR_PRIM(desc, Desc);
extern SET_STR_PRIM(succ, Succ);
extern SET_STR_PRIM(fail, Fail);
extern SET_STR_PRIM(drop, Drop);
extern SET_STR_PRIM(osucc, OSucc);
extern SET_STR_PRIM(ofail, OFail);
extern SET_STR_PRIM(odrop, ODrop);
extern MUF_PRIM(prim_setname);
extern GET_DBREF_PRIM(contents, Contents);
extern GET_DBREF_PRIM(exits, Actions);
extern GET_DBREF_PRIM(getlink, Link);
extern GET_DBREF_PRIM(loc, Loc);
extern GET_DBREF_PRIM(next, Next);
extern GET_DBREF_PRIM(owner, Owner);
extern GET_INT_PRIM(pennies, GetPennies);
extern GET_INT_PRIM(exitp, TYPE_EXIT == Typeof);
extern GET_INT_PRIM(playerp, TYPE_PLAYER == Typeof);
extern GET_INT_PRIM(progp, TYPE_PROGRAM == Typeof);
extern GET_INT_PRIM(roomp, TYPE_ROOM == Typeof);
extern GET_INT_PRIM(thingp, TYPE_THING == Typeof);
extern MUF_PRIM(prim_okp);
extern MUF_PRIM(prim_dbcmp);
extern MUF_PRIM(prim_dbtop);
extern MUF_PRIM(prim_check_moveto);
extern MUF_PRIM(prim_moveto);
extern MUF_PRIM(prim_addpennies);
extern MUF_PRIM(prim_copyobj);
extern TRY_INTERN_PROPS(getpropv);
extern TRY_INTERN_PROPS(getprops);
extern TRY_INTERN_PROPS(getpropd);
extern TRY_INTERN_PROPS(rmvprop);
extern TRY_INTERN_PROPS(addprop);
extern TRY_INTERN_PROPS(setprop);
extern TRY_INTERN_PROPS(flagp);
extern MUF_PRIM(prim_addprop);
extern MUF_PRIM(prim_setprop);
extern MUF_PRIM(prim_getprops);
extern MUF_PRIM(prim_getpropv);
extern MUF_PRIM(prim_getpropd);
extern MUF_PRIM(prim_rmvprop);
extern MUF_PRIM(prim_match);
extern MUF_PRIM(prim_rmatch);
extern MUF_PRIM(prim_flagp);
extern MUF_PRIM(prim_set);

/* From: interp/disassem.c */
extern void disassemble(const dbref player, const dbref program);

/* From: interp/flow_prims.c */
extern MUF_PRIM(prim_execute);
extern MUF_PRIM(prim_call);
extern MUF_PRIM(prim_read);

/* From: interp/frames.c */
extern void init_frame_table(void);
extern void clear_frame_table(void);
extern struct frame *get_frame(const dbref player);
extern int running_program(const dbref player);
extern void add_frame(const dbref player, struct frame *frame);
extern void delete_frame(const dbref player);

/* From: interp/inst.c */
extern const char *prim2name(const int primitive);
extern void check_perms(frame *fr, const dbref object, const int prim);
extern void check_prim_space(const int primitive, frame *fr, data_stack *st);
extern void check_prim_types(const int primitive, frame *fr, data_stack *st);
extern prim_func prim_check(const int primitive, frame *fr, data_stack *st, data_stack *pop_stack);
extern int lookup_primitive(const char *name);
extern int num_args_primitive(const int primitive);
extern int num_rtn_primitive(const int primitive);
extern int list_rtn_primitive(const int primitive);
extern int wiz_primitive(const int primitive);
extern void inst_bufcat(Buffer *buf, inst *what);
extern void debug_inst(frame *fr, data_stack *data_st, addr_stack *addr_st, inst *curr_inst);

/* From: interp/interp.c */
extern void safe_push_data_st(frame *fr, data_stack *st, inst *what, const int prim);
extern void clear_data_stack_(data_stack *st);
extern void clear_frame(frame *fr);
extern void up_inst_limit(frame *fr);
extern void pop_context(frame *fr);
extern void push_context_muf(frame *fr, func_addr *func);
extern void push_context_dbref(frame *fr, const dbref prog);
extern void push_context_cont(frame *fr, continue_func func, void *data);
extern int interp(const dbref player, const dbref program, const dbref trigger, const int writeonly, const char *arg_text);
extern int interp_loop(frame *fr);
extern void interactive(const dbref player, const char *command);
extern void interp_quit_external(const dbref player);
extern void interp_error(frame *fr, int primitive, const char *format, ...);
extern void init_interp(void);
extern void clear_interp(void);

/* From: interp/list_prims.c */
extern void prim_sort_n_lists(frame *fr, data_stack *st);

/* From: interp/math_prims.c */
extern MATH_PRIM(add, +);
extern MATH_PRIM(sub, -);
extern MATH_PRIM(mul, *);
extern MATH_PRIM(lessthan, <);
extern MATH_PRIM(gtrthan, >);
extern MATH_PRIM(lesseq, <=);
extern MATH_PRIM(gtreq, >=);
extern MATH_PRIM(equal, ==);
extern MATH0_PRIM(div, /);
extern MATH0_PRIM(mod, %);
extern MUF_PRIM(prim_random);
extern MUF_PRIM(prim_systime);
extern MUF_PRIM(prim_time);

/* From: interp/olist_prims.c */
extern void prim_map_n_lists(frame *fr, data_stack *st);
extern void prim_sort_n_lists(frame *fr, data_stack *st);

/* From: interp/stack_prims.c */
extern MUF_PRIM(prim_depth);
extern MUF_PRIM(prim_pop);
extern MUF_PRIM(prim_not);
extern MUF_PRIM(prim_dup);
extern MUF_PRIM(prim_over);
extern MUF_PRIM(prim_swap);
extern MUF_PRIM(prim_rot);
extern MUF_PRIM(prim_pick);
extern MUF_PRIM(prim_put);
extern MUF_PRIM(prim_rotate);
extern MUF_PRIM(prim_at);
extern MUF_PRIM(prim_bang);
extern MUF_PRIM(prim_int);
extern MUF_PRIM(prim_dbref);
extern MUF_PRIM(prim_variable);
extern MUF_PRIM(prim_and);
extern MUF_PRIM(prim_or);

/* From: interp/string_prims.c */
extern MUF_PRIM(prim_pronoun);
extern MUF_PRIM(prim_explode);
extern MUF_PRIM(prim_subst);
extern MUF_PRIM(prim_instr);
extern MUF_PRIM(prim_rinstr);
extern MUF_PRIM(prim_numberp);
extern MUF_PRIM(prim_stringcmp);
extern MUF_PRIM(prim_strcmp);
extern MUF_PRIM(prim_strncmp);
extern MUF_PRIM(prim_stringncmp);
extern MUF_PRIM(prim_strcut);
extern MUF_PRIM(prim_strlen);
extern MUF_PRIM(prim_strcat);
extern MUF_PRIM(prim_atoi);
extern MUF_PRIM(prim_intostr);
extern MUF_PRIM(prim_strftime);
extern MUF_PRIM(prim_notify);
extern MUF_PRIM(prim_notify_except);

/* From: interp/typechk.c */
extern int typechk_stack(frame *fr, data_stack *st, const int prim, const int total_args, const int *types);

/* From: utils/hashtab.c */
extern int compare_generic_hash(const void *entry1, const void *entry2);
extern unsigned long make_key_generic_hash(const void *entry);
extern void delete_generic_hash(void *entry);
extern const void *find_hash_entry(hash_tab *table, const void *match);
extern void add_hash_entry(hash_tab *table, const void *data);
extern void clear_hash_entry(hash_tab *table, const void *data);
extern void clear_hash_table(hash_tab *table);
extern hash_tab *init_hash_table(const char *id, compare_function cmpfn, key_function keyfn, delete_function delfn, unsigned long size);
extern void init_hash_walk(hash_tab *table);
extern const void *next_hash_walk(hash_tab *table);
extern void dump_hashtab_stats(hash_tab *table);
extern long total_hash_entries(void);
extern void init_hash_entries(const long count);

/* From: utils/help.c */
extern void help(dbref player, const char *arg1, const char *file);
extern void do_help(const dbref player, const char *arg1, const char *ign2, const char *ign3);
extern void do_man(const dbref player, const char *arg1, const char *ign2, const char *ign3);
extern void do_news(const dbref player, const char *arg1, const char *ign2, const char *ign3);

/* From: utils/intern.c */
extern struct interned_string *find_interned_string(const char *what);
extern struct interned_string *make_interned_string(const char *what);
extern struct interned_string *dup_interned_string(struct interned_string *what);
extern void clear_interned_string(struct interned_string *what);
extern long total_intern_strings(void);
extern void init_intern_strings(const long count);

/* From: utils/log.c */
extern void log_status(const char *fmt, ...);
extern void log_muf(const char *fmt, ...);
extern void log_gripe(const char *fmt, ...);
extern void log_command(const char *fmt, ...);
extern void log_info(const char *fmt, ...);
extern void log_last_command(const char *fmt, ...);
extern void log_sanity(const int type, const dbref obj, const char *fmt, va_list ap);

/* From: utils/muck_strftime.c */
extern size_t muck_strftime(char *s, size_t maxsize, const char *format, long tim, const int use_local_time);

/* From: utils/player.c */
extern int check_crypt_password(const char *password, const char *guess);
extern dbref connect_player(const char *name, const char *password);
extern dbref create_player(const char *name, const char *password);
extern void clear_players(void);
extern void delete_player(const dbref who);
extern void add_player(const dbref who);
extern dbref lookup_player(const char *name);

/* From: utils/stringutil.c */
extern unsigned long default_hash(register const char *s);
extern int muck_stricmp(register const char *str1, register const char *str2);
extern int muck_strnicmp(register const char *str1, register const char *str2, register int n);
extern int muck_instr(const char *str, const int lenstr, const char *pat, const int lenpat);
extern int muck_rinstr(register const char *str, int lenstr, register const char *pat, const int lenpat);
extern int muck_strprefix(register const char *string, register const char *prefix);
extern const char *muck_strmatch(register const char *src, register const char *sub);
extern char *percent_sub(const dbref player, const char sub);
extern char *pronoun_substitute(const dbref player, const char *str_in);
extern struct shared_string *make_shared_string(const char *str, const unsigned int len);
extern struct shared_string *make2_shared_string(const char *str1, const unsigned int len1, const char *str2, const unsigned int len2);
extern struct shared_string *dup_shared_string(struct shared_string *s);
extern void clear_shared_string(struct shared_string *s);
extern const char *muck_subst(const char *str, const char *orig, const int origlen, const char *repl, int *resultlen);
extern const char *make_progname(const dbref prog);
extern int notify(const dbref player, const char *message, ...);
extern void notify_all(const char *message, ...);
extern void notify_except(const dbref where, const dbref except, const char *msg, ...);
extern int number(const char *s);

/* From: utils/utils.c */
extern dbref remove_first(const dbref first, const dbref what);
extern int member(const dbref thing, dbref list);
extern dbref reverse(dbref list);
extern void exec_or_notify(const dbref player, const dbref thing, const char *msg, const int quiet);
#undef MATH_PRIM
#undef MATH0_PRIM
#undef GET_STR_PRIM
#undef SET_STR_PRIM
#undef GET_DBREF_PRIM
#undef GET_INT_PRIM
#undef TRY_INTERN_PROPS
#endif /* USE_EXTERNS */
#endif /* MUCK_EXTERNS_H */
