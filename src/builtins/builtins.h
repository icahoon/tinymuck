#define COMMAND_FUNC(func) extern void func(const dbref, const char *, const char *, const char *)
COMMAND_FUNC(do_action);
COMMAND_FUNC(do_attach);
COMMAND_FUNC(do_boot);
COMMAND_FUNC(do_chown);
COMMAND_FUNC(do_create);
COMMAND_FUNC(do_describe);
COMMAND_FUNC(do_dig);
COMMAND_FUNC(do_drop_message);
COMMAND_FUNC(do_dump);
COMMAND_FUNC(do_edit);
COMMAND_FUNC(do_entrances);
COMMAND_FUNC(do_fail);
COMMAND_FUNC(do_find);
COMMAND_FUNC(do_force);
COMMAND_FUNC(do_link);
COMMAND_FUNC(do_list);
COMMAND_FUNC(do_lock);
COMMAND_FUNC(do_name);
COMMAND_FUNC(do_newpassword);
COMMAND_FUNC(do_odrop);
COMMAND_FUNC(do_ofail);
COMMAND_FUNC(do_open);
COMMAND_FUNC(do_osuccess);
COMMAND_FUNC(do_owned);
COMMAND_FUNC(do_password);
COMMAND_FUNC(do_pcreate);
COMMAND_FUNC(do_prog);
COMMAND_FUNC(do_purge);
COMMAND_FUNC(do_recycle);
COMMAND_FUNC(do_set);
COMMAND_FUNC(do_shutdown);
COMMAND_FUNC(do_stats);
COMMAND_FUNC(do_success);
COMMAND_FUNC(do_teleport);
COMMAND_FUNC(do_toad);
COMMAND_FUNC(do_trace);
COMMAND_FUNC(do_unlink);
COMMAND_FUNC(do_unlock);
COMMAND_FUNC(do_version);
COMMAND_FUNC(do_wall);
COMMAND_FUNC(do_drop);
COMMAND_FUNC(do_examine);
COMMAND_FUNC(do_get);
COMMAND_FUNC(do_give);
COMMAND_FUNC(do_move);
COMMAND_FUNC(do_gripe);
COMMAND_FUNC(do_help);
COMMAND_FUNC(do_inventory);
COMMAND_FUNC(do_kill);
COMMAND_FUNC(do_look);
COMMAND_FUNC(do_man);
COMMAND_FUNC(do_move);
COMMAND_FUNC(do_news);
COMMAND_FUNC(do_page);
COMMAND_FUNC(do_pose);
COMMAND_FUNC(do_drop);
COMMAND_FUNC(do_look);
COMMAND_FUNC(do_rob);
COMMAND_FUNC(do_say);
COMMAND_FUNC(do_score);
COMMAND_FUNC(do_get);
COMMAND_FUNC(do_drop);
COMMAND_FUNC(do_whisper);
#undef COMMAND_FUNC
